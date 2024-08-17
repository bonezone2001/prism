/**
 * @file renderer.h
 * @author Kyle Pelham (bonezone2001@gmail.com)
 * @brief The vulkan renderer for Prism.
 * 
 * This file contains the Renderer class, which initializes and manages the Vulkan renderer
 * that powers the Prism windows and integrates ImGui. It primarily handles Vulkan initialization
 * and provides access to core Vulkan objects.
 * 
 * @copyright Copyright (c) 2024
*/

#pragma once
#include <functional>
#include <fmt/core.h>
#include "prism/prism_export.hpp"
#include <vulkan/vulkan.h>

#include "imgui.h"

namespace Prism {

/**
 * The Vulkan renderer class for Prism.
 * 
 * This class manages the Vulkan instance, device, and related objects.
 * It handles initialization of Vulkan and provides methods to access
 * core Vulkan objects needed for rendering.
 */
class PRISM_EXPORT Renderer
{
private:
    VkInstance instance = VK_NULL_HANDLE;                   ///< The Vulkan instance.
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;       ///< The Vulkan physical device. This is the GPU.
    VkAllocationCallbacks* allocator = nullptr;             ///< The Vulkan allocator.
    VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;  ///< The Vulkan debug report.
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;       ///< The Vulkan descriptor pool.
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;         ///< The Vulkan pipeline cache.
    uint32_t queueFamilyIndex = UINT32_MAX;                 ///< The Vulkan queue family index.
    VkDevice device = VK_NULL_HANDLE;                       ///< The Vulkan device.
    VkQueue queue = VK_NULL_HANDLE;                         ///< The Vulkan queue.

public:
    /// Construct a new Renderer object
    Renderer();

    /// Destroy the Renderer object
    virtual ~Renderer();

    /**
     * Hook the renderer into a window and initialize ImGui.
     * This allows the renderer to draw to the specified window.
     * @param window The window to hook into.
     * @param surface The Vulkan surface to render to.
     * @param width The width of the window.
     * @param height The height of the window.
    */
    void setupWindow(class Window* window, VkSurfaceKHR surface, int width, int height);

    /**
     * Validates a Vulkan result and handles errors.
     * 
     * This static method checks the result of a Vulkan operation. If the result
     * indicates an error (VkResult < 0), it logs the error and terminates the program.
     * Non-critical errors (VkResult > 0) are logged for debugging purposes.
     * 
     * @param result VkResult code to check.
    */
    static void CheckVkResult(VkResult result);

    // Getters & Setters
    // -------------------------------------------------------------------------
    inline VkInstance getInstance() const { return instance; }                      ///< @return Vulkan instance.
    inline VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }    ///< @return Vulkan physical device.
    inline VkAllocationCallbacks* getAllocator() const { return allocator; }        ///< @return Vulkan memory allocator.
    inline VkDevice getDevice() const { return device; }                            ///< @return Logical Vulkan device.
    inline VkQueue getQueue() const { return queue; }                               ///< @return Vulkan queue.
    inline uint32_t getQueueFamilyIndex() const { return queueFamilyIndex; }        ///< @return Index of the queue family.
    inline VkDescriptorPool getDescriptorPool() const { return descriptorPool; }    ///< @return Vulkan descriptor pool.
    inline VkPipelineCache getPipelineCache() const { return pipelineCache; }       ///< @return Vulkan pipeline cache.


private:
    // Internal Methods
    // -------------------------------------------------------------------------
    /**
     * Cleans up and releases Vulkan resources.
     * 
     * This method is called to properly shut down the renderer by releasing
     * all Vulkan resources that were allocated during its lifetime.
     */
    void shutdown();

    /**
     * Creates the Vulkan instance.
     * 
     * This method initializes the Vulkan instance, which is the foundational object
     * required for all Vulkan operations.
    */
    void createInstance();

    /**
     * Selects the physical device (GPU) for rendering.
     * 
     * This method chooses the most appropriate physical device (GPU) from those
     * available on the system to be used by Vulkan.
    */
    void selectPhysicalDevice();

    /**
     * Chooses the appropriate queue family for rendering.
     * 
     * This method selects the queue family index that will be used for submitting
     * rendering commands to the GPU.
    */
    void chooseQueueFamilyIndex();

    /**
     * Creates the logical Vulkan device.
     * 
     * This method initializes the Vulkan logical device, which is an abstraction
     * over the physical device and is used to perform rendering operations.
    */
    void createDevice();

    /**
     * Creates the Vulkan descriptor pool.
     * 
     * This method sets up a descriptor pool, which is used to manage resources
     * such as textures and buffers during rendering.
    */
    void createDescriptorPool();

    /**
     * Callback for Vulkan debug reports.
     * 
     * This static method serves as a callback for Vulkan's debugging layer,
     * capturing errors and warnings during runtime.
     * 
     * @param flags Debug report flags.
     * @param objType Type of the Vulkan object that caused the error.
     * @param obj Handle to the Vulkan object that caused the error.
     * @param location Location in the code where the error occurred.
     * @param code Error code.
     * @param layerPrefix Prefix of the Vulkan layer where the error occurred.
     * @param msg Error message.
     * @param userData User data associated with the callback.
     * @return Always returns VK_FALSE.
    */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType,
        uint64_t obj,
        size_t location,
        int32_t code,
        const char* layerPrefix,
        const char* msg,
        void* userData
    );
};

} // namespace Prism