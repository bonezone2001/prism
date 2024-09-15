#include "prism/renderer.h"
#include "prism/window.h"
#include <GLFW/glfw3.h>
#include <assert.h>
#include <vector>

// Enable debug reporting in debug builds
#ifdef _DEBUG
#define VULKAN_DEBUG_REPORT
#endif

#include "imgui_impl_vulkan.h"

namespace Prism {

Renderer::Renderer()
{
    createInstance();
    selectPhysicalDevice();
    chooseQueueFamilyIndex();
    createDevice();
    createDescriptorPool();
}

Renderer::~Renderer()
{
    shutdown();
}

void Renderer::shutdown()
{
    // Wait for the device to finish all operations
    if (device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(device);
        
    // Destroy the descriptor pool
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, allocator);
        descriptorPool = VK_NULL_HANDLE;
    }

    // Destroy the pipeline cache
    if (pipelineCache != VK_NULL_HANDLE) {
        vkDestroyPipelineCache(device, pipelineCache, allocator);
        pipelineCache = VK_NULL_HANDLE;
    }

    // Destroy the logical device
    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, allocator);
        device = VK_NULL_HANDLE;
    }

#ifdef VULKAN_DEBUG_REPORT
    // Remove the debug report callback
    if (debugReport != VK_NULL_HANDLE) {
        auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        vkDestroyDebugReportCallbackEXT(instance, debugReport, allocator);
        debugReport = VK_NULL_HANDLE;
    }
#endif

    // Destroy the Vulkan instance
    if (instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, allocator);
        instance = VK_NULL_HANDLE;
    }
    
    // Reset other members
    physicalDevice = VK_NULL_HANDLE;
    queueFamilyIndex = UINT32_MAX;
    queue = VK_NULL_HANDLE;
}

void Renderer::createInstance()
{
    VkResult err;

    // Get the required instance extensions from GLFW
    uint32_t extensionsCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);

    // Prepare instance creation info
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.enabledExtensionCount = extensionsCount;
    createInfo.ppEnabledExtensionNames = extensions;

#ifdef VULKAN_DEBUG_REPORT
    // Enable validation layers for debug builds
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = layers;
    
    // Add debug report extension
    const char** extensionsExt = (const char**)malloc(sizeof(const char*) * (extensionsCount + 1));
    memcpy(extensionsExt, extensions, extensionsCount * sizeof(const char*));
    extensionsExt[extensionsCount] = "VK_EXT_debug_report";
    createInfo.enabledExtensionCount = extensionsCount + 1;
    createInfo.ppEnabledExtensionNames = extensionsExt;
    
    // Create Vulkan Instance
    err = vkCreateInstance(&createInfo, allocator, &instance);
    CheckVkResult(err);
    free(extensionsExt);
    
    // Set up debug reporting
    auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
    assert(vkCreateDebugReportCallbackEXT != NULL);

    VkDebugReportCallbackCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    debugCreateInfo.pfnCallback = debugCallback;
    debugCreateInfo.pUserData = NULL;
    err = vkCreateDebugReportCallbackEXT(instance, &debugCreateInfo, allocator, &debugReport);
    CheckVkResult(err);
#else
    // Create Vulkan Instance without debug features
    err = vkCreateInstance(&createInfo, allocator, &instance);
    CheckVkResult(err);
    (void)debugReport;
#endif
}

void Renderer::selectPhysicalDevice()
{
    VkResult err;

    // Get the number of available physical devices
    uint32_t gpuCount;
    err = vkEnumeratePhysicalDevices(instance, &gpuCount, NULL);
    CheckVkResult(err);
    assert(gpuCount > 0);

    // Enumerate physical devices
    VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpuCount);
    err = vkEnumeratePhysicalDevices(instance, &gpuCount, gpus);
    CheckVkResult(err);

    // Select discrete GPU if available, otherwise use the first available device
    int useGpu = 0;
    for (int i = 0; i < (int)gpuCount; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpus[i], &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            useGpu = i;
            break;
        }
    }

    physicalDevice = gpus[useGpu];
    free(gpus);
}

void Renderer::chooseQueueFamilyIndex()
{
    // Get the number of queue families
    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
    assert(queueCount >= 1);

    // Get queue family properties
    VkQueueFamilyProperties* queueProps = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps);

    // Find a queue family that supports graphics operations
    for (uint32_t i = 0; i < queueCount; i++) {
        if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }

    free(queueProps);
    assert(queueFamilyIndex != UINT32_MAX);
}

void Renderer::createDevice()
{
    VkResult err;

    // Specify queue creation info
    float queuePriorities[] = { 1.0 };
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;

    // Specify device extensions
    const char* deviceExtensions[] = { "VK_KHR_swapchain" };
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;

    // Create logical device
    err = vkCreateDevice(physicalDevice, &deviceInfo, allocator, &device);
    CheckVkResult(err);

    // Get device queue
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
}

void Renderer::createDescriptorPool()
{
    // Define pool sizes for various descriptor types
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    // Create descriptor pool
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * (uint32_t)poolSizes.size();
    poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    VkResult err = vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool);
    CheckVkResult(err);
}

void Renderer::CheckVkResult(VkResult result)
{
    if (result == 0) return;                        // No error
    fmt::print("Vulkan error: {}\n", (int)result);  // Log any errors (above 0 is non-major error)
    if (result < 0) abort();                        // Abort on major errors (below 0)
}

void Renderer::setupWindow(Window* window,
                           VkSurfaceKHR surface,
                           int width,
                           int height)
{
    // Set the window surface
    auto* imguiWindow = window->getImGuiWindow();
    imguiWindow->Surface = surface;

    // Check for WSI support
    VkBool32 res;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, imguiWindow->Surface, &res);
    if (res != VK_TRUE) {
        fmt::print("Error: WSI not supported on selected physical device\n");
        abort();
    }

    // Select surface format
    const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    imguiWindow->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(physicalDevice, imguiWindow->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

    // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
    VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
    VkPresentModeKHR presentModes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
    imguiWindow->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physicalDevice, imguiWindow->Surface, &presentModes[0], IM_ARRAYSIZE(presentModes));

    // Create SwapChain, RenderPass, Framebuffer, etc.
    uint32_t minImageCount = window->getMinImageCount();
    assert(minImageCount >= 2);
    ImGui_ImplVulkanH_CreateOrResizeWindow(instance, physicalDevice, device, imguiWindow, queueFamilyIndex, allocator, width, height, minImageCount);
}

// Debug callback function for Vulkan validation layers
VKAPI_ATTR VkBool32 VKAPI_CALL
Renderer::debugCallback(VkDebugReportFlagsEXT flags,
                        VkDebugReportObjectTypeEXT objType,
                        uint64_t obj,
                        size_t location,
                        int32_t code,
                        const char* layerPrefix,
                        const char* msg,
                        void* userData)
{
    // Unused parameters
    (void)flags;(void)obj;(void)location;(void)code;(void)layerPrefix;(void)userData;
    (void)objType; (void)msg; //Potentially unused

    // Print debug message
    fmt::print("Vulkan Debug: {}: {}\n", (int)objType, msg);
    return VK_FALSE;
}

} // namespace Prism