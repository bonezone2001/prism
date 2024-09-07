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
struct GLFWmonitor;
struct ImGuiContext;
struct ImGui_ImplVulkanH_Window;
struct ImDrawData;
struct ImFont;

namespace Prism {

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
    std::unordered_map<std::string, ImFont*> loadedFonts;          ///< Map of loaded ImGui fonts.
    ImGuiContext* imguiContext = nullptr;                          ///< The ImGui context associated with this window.

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
     * GLFW callback for errors.
     * @param error The error code.
     * @param description A description of the error.
    */
    static void GlfwErrorCallback(int error, const char* description);
    
    // Getters, Setters & GLFW Method Wrappers
    // -------------------------------------------------------------------------
    /**
     * Closes the window.
     * 
     * This method signals that the window should be closed and cleans up resources accordingly.
     * @note This method does not immediately close the window. It simply sets a flag to close the window.
     *      The window will be closed during the next frame. Where glfwPollEvents() is called.
    */
    void close();

    /**
     * Checks if the window should close.
     * @return true if the window wants to close; otherwise, false.
    */
    bool shouldClose() const;

    /**
     * Sets the window visibility.
     * @param visible true to show the window; false to hide it.
    */
    void setVisible(bool visible);

    /**
     * Checks if the window is currently shown.
     * @return true if the window is shown; otherwise, false.
    */
    bool isShown() const;

    /**
     * Focuses the window.
     * This method brings the window to the front and focuses it.
    */
    void focus();

    /**
     * Checks if the window is currently focused.
     * @return true if the window is focused; otherwise, false.
    */
    bool isFocused() const;

    /**
     * Minimizes the window.
     * This method minimizes the window to the taskbar.
    */
    void minimize();

    /**
     * Checks if the window is minimized.
     * @return true if the window is minimized; otherwise, false.
    */
    bool isMinimized() const;

    /**
     * Maximizes the window.
     * This method maximizes the window to fill the screen.
    */
    void maximize();

    /**
     * Checks if the window is maximized.
     * @return true if the window is maximized; otherwise, false.
    */
    bool isMaximized() const;

    GLFWwindow* getHandle() const { return windowHandle; }                      ///< @return The GLFW window handle.
    ImGui_ImplVulkanH_Window* getImGuiWindow() const { return imguiWindow; }    ///< @return The ImGui Vulkan window information.
    uint32_t getMinImageCount() const { return minImageCount; }                 ///< @return The minimum number of images in the swapchain.
    ImGuiContext* getImGuiContext() const { return imguiContext; }              ///< @return The ImGui context associated with this window.

private:
    // Internal Methods
    // -------------------------------------------------------------------------
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
     * Sets up the native window for use with a custom titlebar.
     * 
     * It will set the window style to remove the standard titlebar and border.
     * Then it will set the window procedure to a custom procedure that handles
     * all the now removed window functionality due to lack of a native titlebar.
    */
    void setupForCustomTitlebar();

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

    // Custom glfw callbacks
    // -------------------------------------------------------------------------
    /**
     * Install custom glfw callbacks.
     * We use custom ones since the default ImGui ones are not compatible with the multi-context.
    */
    void installGlfwCallbacks();

    /**
     * Custom glfw callback for window focus.
     * @param glfwWindow The glfw window handle that received the event.
     * @param focused Indicates if the window is focused.
    */
    static void WindowFocusCallback(GLFWwindow* glfwWindow, int focused);

    /**
     * Custom glfw callback for cursor enter.
     * @param glfwWindow The glfw window handle that received the event.
     * @param entered Indicates if the cursor entered the window.
    */
    static void CursorEnterCallback(GLFWwindow* glfwWindow, int entered);

    /**
     * Custom glfw callback for cursor position.
     * @param glfwWindow The glfw window handle that received the event.
     * @param x The x-coordinate of the cursor.
     * @param y The y-coordinate of the cursor.
    */
    static void CursorPosCallback(GLFWwindow* glfwWindow, double x, double y);

    /**
     * Custom glfw callback for mouse button.
     * @param glfwWindow The glfw window handle that received the event.
     * @param button The mouse button that was pressed or released.
     * @param action The action that was taken.
     * @param mods The modifier keys that were pressed.
    */
    static void MouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);

    /**
     * Custom glfw callback for mouse scroll.
     * @param glfwWindow The glfw window handle that received the event.
     * @param xoffset The horizontal scroll offset.
     * @param yoffset The vertical scroll offset.
    */
    static void ScrollCallback(GLFWwindow* glfwWindow, double xoffset, double yoffset);

    /**
     * Custom glfw callback for key input.
     * @param glfwWindow The glfw window handle that received the event.
     * @param keycode The keyboard key that was pressed or released.
     * @param scancode The system-specific scancode of the key.
     * @param action The action that was taken.
     * @param mods The modifier keys that were pressed.
    */
    static void KeyCallback(GLFWwindow* glfwWindow, int keycode, int scancode, int action, int mods);

    /**
     * Custom glfw callback for character input.
     * @param glfwWindow The glfw window handle that received the event.
     * @param c The Unicode code point of the character.
    */
    static void CharCallback(GLFWwindow* glfwWindow, unsigned int c);

    /**
     * Custom glfw callback for monitor changes.
     * @param monitor The monitor that was connected or disconnected.
     * @param event The event that occurred.
    */
    static void MonitorCallback(GLFWmonitor* monitor, int event);

protected:
    // Virtual User Callbacks
    // -------------------------------------------------------------------------
    /**
     * Called when the window is about to render.
     * Override this method to implement custom update logic before rendering.
     * @note The ImGui context will be CORRECT during this callback.
    */
    virtual void onUpdate() {}

    /**
     * Called when the window is rendering.
     * Override this method to implement custom rendering logic.
     * @note The ImGui context will be CORRECT during this callback.
    */
    virtual void onRender() {}
};

} // namespace Prism