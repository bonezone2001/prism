#include "prism/window.h"
#include "prism/colors.h"
#include "prism/prism.h"
#include <fmt/core.h>
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "prism/embeds/roboto_regular.embed"
#include "prism/embeds/roboto_italic.embed"
#include "prism/embeds/roboto_bold.embed"

#ifndef PRISM_EXCLUDE_FA
#include "prism/embeds/font_awesome.embed"
#include "prism/fa_embedings.h"
#endif

// I will hopefully someday remove this stuff
#include "prism/imgui_rip.h"
static void UpdateKeyModifiers(GLFWwindow* glfwWindow);

std::unordered_map<HWND, WNDPROC> Prism::Window::wndProcMap;

namespace Prism {

Window::Window(WindowSettings settings) :
    settings(settings)
{
    VkResult err;

    // Create ImGui window instance
    imguiWindow = new ImGui_ImplVulkanH_Window();

    // We want a contextless window since we're using Vulkan.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Enforce it being hidden or shown on creation.
    glfwWindowHint(GLFW_VISIBLE, settings.showOnCreate);

    // Get the monitor the parent window is on. If there is no parent, use the primary monitor.
    GLFWmonitor* monitor = nullptr;
    if (settings.parent == nullptr)
        monitor = glfwGetPrimaryMonitor();
    else
        monitor = glfwGetWindowMonitor(settings.parent->getHandle());

    // Get the monitor's video mode and position.
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int monitorX, monitorY;
    glfwGetMonitorPos(monitor, &monitorX, &monitorY);

    // Create the window
    windowHandle = glfwCreateWindow(settings.width, settings.height, settings.title.c_str(), settings.fullscreen ? monitor : nullptr, nullptr);
    glfwSetWindowUserPointer(windowHandle, this);

    // Set the window position
    // I should probably add a method of falling back to default os window positioning.
    // For now just spawn the window in the center of the screen.
    int windowX = monitorX + (mode->width - settings.width) / 2;
    int windowY = monitorY + (mode->height - settings.height) / 2;
    glfwSetWindowPos(windowHandle, windowX, windowY);

    // Implement the window flags & os specific custom window settings.
    glfwSetWindowAttrib(windowHandle, GLFW_RESIZABLE, settings.resizable);
    if (settings.useCustomTitlebar)
        setupForCustomTitlebar();

    // Set the window user pointer to this window class.
    glfwSetWindowUserPointer(windowHandle, this);

    // Setup the vulkan context for the window.
    std::shared_ptr<Renderer> renderer = Application::Get().getRenderer();
    VkSurfaceKHR surface;
    err = glfwCreateWindowSurface(renderer->getInstance(), windowHandle, renderer->getAllocator(), &surface);
    Renderer::CheckVkResult(err);

    // Create frame buffers
    int w, h;
    glfwGetFramebufferSize(windowHandle, &w, &h);
    renderer->setupWindow(this, surface, w, h);

    // Allocate the command buffers
    allocatedCommandBuffers.resize(minImageCount);
    resourceFreeQueue.resize(minImageCount);

    // Debug check version 
    IMGUI_CHECKVERSION();

    // Backup the current context, if one exists (since we're creating a new one and it will override the current one)
    ImGuiContext* backupImGuiContext = ImGui::GetCurrentContext();

    // Setup a new ImGui context
    imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(imguiContext);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard navigation Controls
    io.IniFilename = nullptr;                               // Disable the ImGui .ini file by default. TODO: Add support for this later.

    // Setup our custom ImGui style
    setDefaultTheme();

    // Setup renderer backends
    ImGui_ImplGlfw_InitForVulkan(windowHandle, false);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = renderer->getInstance();
    initInfo.PhysicalDevice = renderer->getPhysicalDevice();
    initInfo.Device = renderer->getDevice();
    initInfo.QueueFamily = renderer->getQueueFamilyIndex();
    initInfo.Queue = renderer->getQueue();
    initInfo.PipelineCache = renderer->getPipelineCache();
    initInfo.DescriptorPool = renderer->getDescriptorPool();
    initInfo.Subpass = 0;
    initInfo.MinImageCount = minImageCount;
    initInfo.ImageCount = imguiWindow->ImageCount;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = renderer->getAllocator();
    initInfo.CheckVkResultFn = Renderer::CheckVkResult;
    ImGui_ImplVulkan_Init(&initInfo, imguiWindow->RenderPass);

    // Setup the window callbacks
    // TODO: This really needs some work
    installGlfwCallbacks();

    // Load default font merged with font awesome
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;

    // Roboto font
    ImFont* robotoFont = io.Fonts->AddFontFromMemoryTTF((void*)Fonts::robotoRegular, sizeof(Fonts::robotoRegular), 20.f, &fontConfig);
    loadedFonts["default"] = robotoFont;
    loadedFonts["bold"] = io.Fonts->AddFontFromMemoryTTF((void*)Fonts::robotoBold, sizeof(Fonts::robotoBold), 20.0f, &fontConfig);
    loadedFonts["italic"] = io.Fonts->AddFontFromMemoryTTF((void*)Fonts::robotoItalic, sizeof(Fonts::robotoItalic), 20.0f, &fontConfig);

    // Font awesome
#ifndef PRISM_EXCLUDE_FA
    static const ImWchar faRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    fontConfig.MergeMode = true;         // Merge icon font with the default font
    fontConfig.GlyphMinAdvanceX = 20.0f; // Ensure icons are rendered with a similar size as the font
    fontConfig.PixelSnapH = true;        // Align icons on pixel boundaries
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)Fonts::fontAwesome, sizeof(Fonts::fontAwesome), 20.0f, &fontConfig, faRanges);
#endif
    io.FontDefault = robotoFont;
    io.Fonts->Build();

    // Create the font texture
    ImGui_ImplVulkan_CreateFontsTexture();

    // Restore the previous contexts
    // TODO: Is this really doing what I'm expecting? I'll have to come back to this.
    if (backupImGuiContext) ImGui::SetCurrentContext(backupImGuiContext);
}

Window::~Window()
{
    // Shutdown ImGui
    if (imguiContext) {
        // Set current context
        ImGuiContext* backupContext = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(imguiContext);
        imguiContext = nullptr;

        // Wait for the device to be idle
        auto renderer = Application::Get().getRenderer();
        vkDeviceWaitIdle(renderer->getDevice());

        // Shutdown context
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // Restore the previous context if it exists
        if (backupContext) ImGui::SetCurrentContext(backupContext);
    }

    // Destroy the glfw window
    if (windowHandle) {
        glfwDestroyWindow(windowHandle);
        windowHandle = nullptr;
    }

    // Destroy the imgui window context
    if (imguiWindow) {
        delete imguiWindow;
        imguiWindow = nullptr;
    }
}

void Window::render()
{
    // Swap contexts
    ImGuiContext* backupContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(imguiContext);

    // TODO: Resize swap chain

    // Start ImGui Frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Run update logic, then render ImGui
    onUpdate();
    onRender();

    // Render
    ImGui::Render();
    ImDrawData* mainDrawData = ImGui::GetDrawData();
    
    ImVec4 clearColor = ImVec4(0.f, 0.f, 0.f, 0.f);
    const bool windowShouldRender = mainDrawData->DisplaySize.x > 0.f && mainDrawData->DisplaySize.y > 0.f;
    imguiWindow->ClearValue.color.float32[0] = clearColor.x * clearColor.w; // Red
    imguiWindow->ClearValue.color.float32[1] = clearColor.y * clearColor.w; // Green
    imguiWindow->ClearValue.color.float32[2] = clearColor.z * clearColor.w; // Blue
    imguiWindow->ClearValue.color.float32[3] = clearColor.w;                // Alpha
    
    // Frame render
    if (windowShouldRender)
        renderAndPresent(mainDrawData);
    else
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // TODO: Do delta time here

    // Restore the previous context
    if (backupContext) ImGui::SetCurrentContext(backupContext);
}

void Window::renderAndPresent(ImDrawData* drawData)
{
    frameRender(drawData);
    framePresent();
}

bool Window::isShown() const
{
    return glfwGetWindowAttrib(windowHandle, GLFW_VISIBLE);
}

void Window::focus()
{
    glfwFocusWindow(windowHandle);
}

void Window::minimize()
{
    glfwIconifyWindow(windowHandle);
}

bool Window::isFocused() const
{
    return glfwGetWindowAttrib(windowHandle, GLFW_FOCUSED);
}

bool Window::isMinimized() const
{
    return glfwGetWindowAttrib(windowHandle, GLFW_ICONIFIED);
}

void Window::maximize()
{
    glfwMaximizeWindow(windowHandle);
}

bool Window::isMaximized() const
{
    return glfwGetWindowAttrib(windowHandle, GLFW_MAXIMIZED);
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(windowHandle);
}

void Window::setVisible(bool visible)
{
    if (visible)
        glfwShowWindow(windowHandle);
    else
        glfwHideWindow(windowHandle);
}

void Window::close()
{
    glfwSetWindowShouldClose(windowHandle, true);
}

void Window::setDefaultTheme()
{
    auto& style = ImGui::GetStyle();
    auto& colors = style.Colors;

    // Style
    style.FrameRounding = 2.5f;
    style.FrameBorderSize = 1.0f;
    style.IndentSpacing = 11.0f;
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
    style.ItemSpacing = ImVec2(6.0f, 6.0f);
    style.ChildRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.FrameRounding = 6.0f;

    // Headers
    colors[ImGuiCol_Header] = ImGui::ColorConvertU32ToFloat4(Colors::groupHeader);
    colors[ImGuiCol_HeaderHovered] = ImGui::ColorConvertU32ToFloat4(Colors::groupHeader);
    colors[ImGuiCol_HeaderActive] = ImGui::ColorConvertU32ToFloat4(Colors::groupHeader);

    // Buttons
    colors[ImGuiCol_Button] = ImGui::ColorConvertU32ToFloat4(Colors::button);
    colors[ImGuiCol_ButtonHovered] = ImGui::ColorConvertU32ToFloat4(Colors::buttonDarker);
    colors[ImGuiCol_ButtonActive] = ImGui::ColorConvertU32ToFloat4(Colors::buttonBrighter);

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImGui::ColorConvertU32ToFloat4(Colors::propertyField);
    colors[ImGuiCol_FrameBgHovered] = ImGui::ColorConvertU32ToFloat4(Colors::propertyField);
    colors[ImGuiCol_FrameBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::propertyField);

    // Tabs
    colors[ImGuiCol_Tab] = ImGui::ColorConvertU32ToFloat4(Colors::titlebar);
    colors[ImGuiCol_TabHovered] = ImColor(Colors::titlebarDarker);
    colors[ImGuiCol_TabActive] = ImColor(Colors::titlebarBrighter);
    colors[ImGuiCol_TabUnfocused] = ImGui::ColorConvertU32ToFloat4(Colors::titlebar);
    colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabHovered];

    // Title
    colors[ImGuiCol_TitleBg] = ImGui::ColorConvertU32ToFloat4(Colors::titlebar);
    colors[ImGuiCol_TitleBgActive] = ImGui::ColorConvertU32ToFloat4(Colors::titlebar);
    colors[ImGuiCol_TitleBgCollapsed] = ImGui::ColorConvertU32ToFloat4(Colors::titlebarDarker);

    // Resize Grip
    colors[ImGuiCol_ResizeGrip] = ImVec4(232, 232, 232, 64);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(207, 207, 207, 171);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(117, 117, 117, 242);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(5, 5, 5, 135);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(79, 79, 79, 255);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(105, 105, 105, 255);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(130, 130, 130, 255);

    // Check Mark
    colors[ImGuiCol_CheckMark] = ImColor(200, 200, 200, 255);

    // Slider
    colors[ImGuiCol_SliderGrab] = ImVec4(130, 130, 130, 179);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(168, 168, 168, 255);

    // Text
    colors[ImGuiCol_Text] = ImGui::ColorConvertU32ToFloat4(Colors::text);

    // Checkbox
    colors[ImGuiCol_CheckMark] = ImGui::ColorConvertU32ToFloat4(Colors::text);

    // Separator
    colors[ImGuiCol_Separator] = ImGui::ColorConvertU32ToFloat4(Colors::backgroundDark);
    colors[ImGuiCol_SeparatorActive] = ImGui::ColorConvertU32ToFloat4(Colors::highlight);
    colors[ImGuiCol_SeparatorHovered] = ImColor(39, 185, 242, 150);

    // Window Background
    colors[ImGuiCol_WindowBg] = ImGui::ColorConvertU32ToFloat4(Colors::titlebar);
    colors[ImGuiCol_ChildBg] = ImGui::ColorConvertU32ToFloat4(Colors::background);
    colors[ImGuiCol_PopupBg] = ImGui::ColorConvertU32ToFloat4(Colors::backgroundPopup);
    colors[ImGuiCol_Border] = ImGui::ColorConvertU32ToFloat4(Colors::backgroundDark);

    // Tables
    colors[ImGuiCol_TableHeaderBg] = ImGui::ColorConvertU32ToFloat4(Colors::groupHeader);
    colors[ImGuiCol_TableBorderLight] = ImGui::ColorConvertU32ToFloat4(Colors::backgroundDark);

    // Menubar
    colors[ImGuiCol_MenuBarBg] = ImVec4{ 0, 0, 0, 0 };
}

void Window::installGlfwCallbacks()
{
    glfwSetWindowFocusCallback(windowHandle, WindowFocusCallback);
    glfwSetCursorEnterCallback(windowHandle, CursorEnterCallback);
    glfwSetCursorPosCallback(windowHandle, CursorPosCallback);
    glfwSetMouseButtonCallback(windowHandle, MouseButtonCallback);
    glfwSetScrollCallback(windowHandle, ScrollCallback);
    glfwSetCharCallback(windowHandle, CharCallback);
    glfwSetKeyCallback(windowHandle, KeyCallback);
    glfwSetMonitorCallback(MonitorCallback);
}

void Window::frameRender(ImDrawData* drawData)
{
    VkResult err;

    auto renderer = Application::Get().getRenderer();
    VkSemaphore imageAcquiredSemaphore = imguiWindow->FrameSemaphores[imguiWindow->SemaphoreIndex].ImageAcquiredSemaphore;
    VkSemaphore renderCompleteSemaphore = imguiWindow->FrameSemaphores[imguiWindow->SemaphoreIndex].RenderCompleteSemaphore;
    err = vkAcquireNextImageKHR(renderer->getDevice(), imguiWindow->Swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &imguiWindow->FrameIndex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        fmt::print("Window::frameRender: Swapchain needs to be rebuilt.\n");
        swapchainNeedRebuild = true;
		return;
	}
    Renderer::CheckVkResult(err);

    // Increase the frame index
    // currentFrameIndex = (currentFrameIndex + 1) % imguiWindow->ImageCount;

    // Wait for the fence to be signaled, which indicates the previous frame using this image is done
    ImGui_ImplVulkanH_Frame* fd = &imguiWindow->Frames[imguiWindow->FrameIndex];
    err = vkWaitForFences(renderer->getDevice(), 1, &fd->Fence, VK_TRUE, UINT64_MAX);
    Renderer::CheckVkResult(err);

    // Reset the fence for use in the next frame
    err = vkResetFences(renderer->getDevice(), 1, &fd->Fence);
    Renderer::CheckVkResult(err);

    // Free resources in queue
    for (auto& func : resourceFreeQueue[imguiWindow->FrameIndex]) {
        func();
    }
    resourceFreeQueue[imguiWindow->FrameIndex].clear();

    auto& cmdBuf = allocatedCommandBuffers[imguiWindow->FrameIndex];
    if (!cmdBuf.empty()) {
        vkFreeCommandBuffers(renderer->getDevice(), fd->CommandPool, static_cast<uint32_t>(cmdBuf.size()), cmdBuf.data());
        cmdBuf.clear();
    }

    err = vkResetCommandPool(renderer->getDevice(), fd->CommandPool, 0);
    Renderer::CheckVkResult(err);

    VkCommandBufferBeginInfo buffBeginInfo = {};
    buffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    buffBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    err = vkBeginCommandBuffer(fd->CommandBuffer, &buffBeginInfo);
    Renderer::CheckVkResult(err);
    
    VkRenderPassBeginInfo renderBeginInfo = {};
    renderBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBeginInfo.renderPass = imguiWindow->RenderPass;
    renderBeginInfo.framebuffer = fd->Framebuffer;
    renderBeginInfo.renderArea.extent.width = imguiWindow->Width;
    renderBeginInfo.renderArea.extent.height = imguiWindow->Height;
    renderBeginInfo.clearValueCount = 1;
    renderBeginInfo.pClearValues = &imguiWindow->ClearValue;
    vkCmdBeginRenderPass(fd->CommandBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Render ImGui
	ImGui_ImplVulkan_RenderDrawData(drawData, fd->CommandBuffer);

	// Submit command buffer
	vkCmdEndRenderPass(fd->CommandBuffer);
    VkSubmitInfo submitInfo = {};
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAcquiredSemaphore;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &fd->CommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderCompleteSemaphore;

    err = vkEndCommandBuffer(fd->CommandBuffer);
    Renderer::CheckVkResult(err);

    err = vkQueueSubmit(renderer->getQueue(), 1, &submitInfo, fd->Fence);
		Renderer::CheckVkResult(err);
}

void Window::framePresent()
{
	if (swapchainNeedRebuild) return;
    auto renderer = Application::Get().getRenderer();
	
    VkSemaphore renderCompleteSemaphore = imguiWindow->FrameSemaphores[imguiWindow->SemaphoreIndex].RenderCompleteSemaphore;
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &renderCompleteSemaphore;
	info.swapchainCount = 1;
	info.pSwapchains = &imguiWindow->Swapchain;
	info.pImageIndices = &imguiWindow->FrameIndex;
	
    VkResult err = vkQueuePresentKHR(renderer->getQueue(), &info);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
        fmt::print("Window::framePresent: Swapchain needs to be rebuilt.\n");
        swapchainNeedRebuild = true;
		return;
	}
	Renderer::CheckVkResult(err);

	imguiWindow->SemaphoreIndex = (imguiWindow->SemaphoreIndex + 1) % imguiWindow->SemaphoreCount;
}

void Window::setupForCustomTitlebar()
{
#ifdef _WIN32
        HWND hWnd = glfwGetWin32Window(windowHandle);

        LONG_PTR lStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
        lStyle |= WS_THICKFRAME;
        lStyle &= ~WS_CAPTION;
        SetWindowLongPtr(hWnd, GWL_STYLE, lStyle);

        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);
        int width = windowRect.right - windowRect.left;
        int height = windowRect.bottom - windowRect.top;

        wndProcMap[hWnd] = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CustomWindowProc));
        SetWindowPos(hWnd, NULL, 0, 0, width, height, SWP_FRAMECHANGED | SWP_NOMOVE);
#else
        fmt::print("Custom titlebars are not yet supported on this platform.\n");
#endif
}

void Window::GlfwErrorCallback(int error, const char* description)
{
    fmt::print("GLFW Error {}: {}\n", error, description);
}

#ifdef _WIN32
LRESULT Window::CustomWindowProc(HWND hWnd,
                                 UINT uMsg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    switch (uMsg) {
        case WM_NCCALCSIZE:
        {
            // Remove the window's standard sizing border
            if (wParam == TRUE && lParam != NULL) {
                NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                pParams->rgrc[0].top += 1;
                pParams->rgrc[0].right -= 1;
                pParams->rgrc[0].bottom -= 1;
                pParams->rgrc[0].left += 1;
            }
            return 0;
        }
        // Prevent the non-client area from being painted
        case WM_NCPAINT:
            return 0;
        case WM_NCHITTEST:
        {
            // Expand the hit test area for resizing
            const int borderWidth = 8; // Adjust this value to control the hit test area size

            POINTS mousePos = MAKEPOINTS(lParam);
            POINT clientMousePos = { mousePos.x, mousePos.y };
            ScreenToClient(hWnd, &clientMousePos);

            RECT windowRect;
            GetClientRect(hWnd, &windowRect);

            if (clientMousePos.y >= windowRect.bottom - borderWidth) {
                if (clientMousePos.x <= borderWidth)
                    return HTBOTTOMLEFT;
                else if (clientMousePos.x >= windowRect.right - borderWidth)
                    return HTBOTTOMRIGHT;
                else
                    return HTBOTTOM;
            }
            else if (clientMousePos.y <= borderWidth) {
                if (clientMousePos.x <= borderWidth)
                    return HTTOPLEFT;
                else if (clientMousePos.x >= windowRect.right - borderWidth)
                    return HTTOPRIGHT;
                else
                    return HTTOP;
            }
            else if (clientMousePos.x <= borderWidth)
                return HTLEFT;
            else if (clientMousePos.x >= windowRect.right - borderWidth)
                return HTRIGHT;

            break;
        }
        // Prevent non-client area from being redrawn during window activation
        case WM_NCACTIVATE:
            return TRUE;
    }
    
    return CallWindowProc(wndProcMap[hWnd], hWnd, uMsg, wParam, lParam);
}
#endif

void Window::WindowFocusCallback(GLFWwindow* glfwWindow, int focused)
{
    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
    window->getImGuiContext()->IO.AddFocusEvent(focused != 0);
}

void Window::CursorEnterCallback(GLFWwindow* glfwWindow, int entered)
{
    if (glfwGetInputMode(glfwWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;
    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO& io = window->getImGuiContext()->IO;
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)io.BackendPlatformUserData;

    if (entered) {
        bd->MouseWindow = glfwWindow;
        io.AddMousePosEvent(bd->LastValidMousePos.x, bd->LastValidMousePos.y);
    }
    else if (!entered && bd->MouseWindow == glfwWindow) {
        bd->LastValidMousePos = io.MousePos;
        bd->MouseWindow = nullptr;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

void Window::CursorPosCallback(GLFWwindow* glfwWindow, double x, double y)
{
    if (glfwGetInputMode(glfwWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO& io = window->getImGuiContext()->IO;
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)io.BackendPlatformUserData;

    io.AddMousePosEvent((float)x, (float)y);
    bd->LastValidMousePos = ImVec2((float)x, (float)y);
}

void Window::MouseButtonCallback(GLFWwindow* glfwWindow,
                                 int button,
                                 int action,
                                 int mods)
{
    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO& io = window->getImGuiContext()->IO;
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)io.BackendPlatformUserData;

    UpdateKeyModifiers(glfwWindow);

    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
}

void Window::ScrollCallback(GLFWwindow* glfwWindow,
                            double xoffset,
                            double yoffset)
{

    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO& io = window->getImGuiContext()->IO;
    io.AddMouseWheelEvent((float)xoffset, (float)yoffset);
}

void Window::KeyCallback(GLFWwindow* glfwWindow,
                         int keycode,
                         int scancode,
                         int action,
                         int mods)
{
    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO& io = window->getImGuiContext()->IO;
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)io.BackendPlatformUserData;

    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    UpdateKeyModifiers(glfwWindow);

    if (keycode >= 0 && keycode < IM_ARRAYSIZE(bd->KeyOwnerWindows))
        bd->KeyOwnerWindows[keycode] = (action == GLFW_PRESS) ? glfwWindow : nullptr;

    keycode = TranslateUntranslatedKey(keycode, scancode);

    ImGuiKey imguiKey = KeyToImGuiKey(keycode);
    io.AddKeyEvent(imguiKey, (action == GLFW_PRESS));
    io.SetKeyEventNativeData(imguiKey, keycode, scancode); // To support legacy indexing (<1.87 user code)
}

void Window::CharCallback(GLFWwindow* glfwWindow, unsigned int c)
{
    Window* window = (Window*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO& io = window->getImGuiContext()->IO;
    io.AddInputCharacter(c);
}

void Window::MonitorCallback(GLFWmonitor* monitor, int event)
{
    std::vector<std::shared_ptr<Window>> windows = Application::Get().getWindows();
    for (auto& window : windows) {
        ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)window->getImGuiContext()->IO.BackendPlatformUserData;
        bd->WantUpdateMonitors = true;
    }
}

} // namespace Prism

// from imgui_impl_glfw.cpp
static void UpdateKeyModifiers(GLFWwindow* glfwWindow)
{
    Prism::Window* window = (Prism::Window*)glfwGetWindowUserPointer(glfwWindow);
    ImGuiIO& io = window->getImGuiContext()->IO;
    ImGui_ImplGlfw_Data* bd = (ImGui_ImplGlfw_Data*)io.BackendPlatformUserData;

    io.AddKeyEvent(ImGuiMod_Ctrl,  (glfwGetKey(bd->Window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(bd->Window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(bd->Window, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS) || (glfwGetKey(bd->Window, GLFW_KEY_RIGHT_SHIFT)   == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Alt,   (glfwGetKey(bd->Window, GLFW_KEY_LEFT_ALT)     == GLFW_PRESS) || (glfwGetKey(bd->Window, GLFW_KEY_RIGHT_ALT)     == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(bd->Window, GLFW_KEY_LEFT_SUPER)   == GLFW_PRESS) || (glfwGetKey(bd->Window, GLFW_KEY_RIGHT_SUPER)   == GLFW_PRESS));
}