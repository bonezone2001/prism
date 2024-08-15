/**
 * @file window.h
 * @author Kyle Pelham
 * @brief Header file for the Prism window class and its associated structures.
 * 
 * This file defines the Window class, which handles the lifecycle of a window in Prism.
 * Each window can be added to the application's window stack and manages its own
 * Vulkan and ImGui resources.
 * 
 * @note The current implementation mixes renderer and window code due to ImGui integration.
 *       Future refactoring should separate these concerns into a dedicated renderer-ImGui file.
 * 
 * @copyright Copyright (c) 2024
*/

#pragma once
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "prism/prism_export.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

struct GLFWwindow;
struct ImGuiContext;
struct ImGui_ImplVulkanH_Window;
struct ImDrawData;
struct ImFont;

namespace Prism
{

/**
 * @struct WindowSettings
 * Defines the settings for a Prism window.
*/
struct PRISM_EXPORT WindowSettings
{
    int width = 800;                        ///< The width of the window in pixels.
    int height = 600;                       ///< The height of the window in pixels.
    std::string title = "Prism Window";     ///< The title of the window.
    bool resizable = false;                 ///< Specifies if the window is resizable.
    bool fullscreen = false;                ///< Specifies if the window is fullscreen.
    bool useCustomTitlebar = false;         ///< Specifies if a custom titlebar should be used.
    bool showOnCreate = true;               ///< Specifies if the window should be shown on creation.
    class Window* parent = nullptr;         ///< Pointer to the parent window, if any.
};

/**
 * @class Window
 * Manages a single window in Prism.
 * 
 * The Window class is responsible for managing the lifecycle of a window, including its
 * Vulkan and ImGui resources. It handles rendering, window state, and input processing.
*/
class PRISM_EXPORT Window
{
protected:
    WindowSettings settings;                                       ///< The settings for the window.
    uint32_t minImageCount = 2;                                    ///< The minimum number of images in the swapchain.
    bool swapchainNeedRebuild = false;                             ///< Indicates if the swapchain needs to be rebuilt.
    std::vector<std::vector<VkCommandBuffer>> allocatedCommandBuffers;  ///< Command buffers allocated for the window.
    std::vector<std::vector<std::function<void()>>> resourceFreeQueue;  ///< Queue of resources to free.

    ImGui_ImplVulkanH_Window* imguiWindow = nullptr;               ///< Vulkan information specific to ImGui.
    ImGuiContext* imguiContext = nullptr;                          ///< The ImGui context associated with this window.
    std::unordered_map<std::string, ImFont*> loadedFonts;          ///< Map of loaded ImGui fonts.

private:
    GLFWwindow* windowHandle = nullptr;                            ///< Handle to the GLFW window. (NOT NATIVE HANDLE)
    float lastFrameTime = 0.0f;                                    ///< Time of the last frame.
    float deltaTime = 0.0f;                                        ///< Delta between frame end and start.

public:
    /// Construct a new Window object.
    Window(WindowSettings settings);

    /// Destroy the Window object.
    virtual ~Window();

    /**
     * Renders the content of the window.
     * 
     * This method is called each frame to render the content of the window using Vulkan.
    */
    void render();

    /**
     * Checks if the window is currently shown.
     * @return true if the window is shown; otherwise, false.
    */
    bool isShown() const;

    /**
     * Checks if the window is currently focused.
     * @return true if the window is focused; otherwise, false.
    */
    bool isFocused() const;

    /**
     * Checks if the window is minimized.
     * @return true if the window is minimized; otherwise, false.
    */
    bool isMinimized() const;

    /**
     * Checks if the window is maximized.
     * @return true if the window is maximized; otherwise, false.
    */
    bool isMaximized() const;

    /**
     * Checks if the window should close.
     * @return true if the window wants to close; otherwise, false.
    */
    bool shouldClose() const;

    /**
     * Closes the window.
     * 
     * This method signals that the window should be closed and cleans up resources accordingly.
     * @note This method does not immediately close the window. It simply sets a flag to close the window.
     *      The window will be closed during the next frame. Where glfwPollEvents() is called.
    */
    void close();

    // Getters
    GLFWwindow* getHandle() const;                  ///< @return The GLFW window handle.
    ImGui_ImplVulkanH_Window* getImGuiWindow();     ///< @return The ImGui Vulkan window information.
    uint32_t getMinImageCount();                    ///< @return The minimum number of images in the swapchain.
    ImGuiContext* getImGuiContext();                ///< @return The ImGui context associated with this window.

protected:
    /**
     * Applies a custom ImGui theme to the window.
     * 
     * This method sets a non-standard theme for ImGui, adding a unique visual style.
    */
    void setDefaultTheme();

    /**
     * Renders ImGui's content to command buffers.
     * @param drawData The ImGui draw data to render.
    */
    void frameRender(ImDrawData* drawData);

    /**
     * Presents the frame to the window.
     * Displaying the rendered content from frameRender() to the window.
    */
    void framePresent();

    /**
     * Renders and presents the frame.
     * @param drawData The ImGui draw data to render and present.
    */
    void renderAndPresent(ImDrawData* drawData);

    /**
     * Called when the window is about to render.
     * 
     * Override this method to implement custom update logic before rendering.
    */
    virtual void onUpdate() {}


    /**
     * Called when the window is rendering.
     * 
     * Override this method to implement custom rendering logic.
    */
    virtual void onRender() {}

#ifdef _WIN32
    /**
     * Map of windows to their original window procedures.
     * 
     * This static member is used to store the original window procedures for windows
     * that use a custom titlebar. This allows the custom window procedure to call the
     * original procedure (glfw's window procedure) for handling standard messages.
     */
    static std::unordered_map<HWND, WNDPROC> wndProcMap;

    /**
     * Custom window procedure for handling Windows-specific messages.
     * @param hWnd Handle to the window.
     * @param uMsg The message code.
     * @param wParam Additional message information.
     * @param lParam Additional message information.
     * @return LRESULT The result of message processing.
     */
    static LRESULT CALLBACK CustomWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

    /**
     * Sets up the native window for use with a custom titlebar.
     * 
     * It will set the window style to remove the standard titlebar and border.
     * Then it will set the window procedure to a custom procedure that handles
     * all the now removed window functionality due to lack of a native titlebar.
    */
    void setupForCustomTitlebar();

public:
    /**
     * GLFW callback for errors.
     * @param error The error code.
     * @param description A description of the error.
    */
    static void GlfwErrorCallback(int error, const char* description);
};

} // namespace Prism