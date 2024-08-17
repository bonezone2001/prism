#include "prism/prism.h"
#include <iostream>
#include <ranges>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

Prism::Application* Prism::Application::instance = nullptr;

namespace Prism {

Application::Application(std::string name)
{
    // Always assume that any given instance of an application is the main instance.
    // As there should only ever be one instance of an application.
    instance = this;

    // Initialize the application
    init();
}

Application::~Application()
{
    stop();                 // Stop the application
    renderer.reset();       // Destroy the renderer
    appWindows.clear();     // Destroy the windows
}

void Application::run()
{
    if (!running) return;
    while (running) {
        // Poll and handle events & cull closed windows
        glfwPollEvents();
        cullClosedWindowsExitOnMainDeath();

        // Render the windows
        for (auto& window : appWindows)
            window->render();
    }
}

void Application::stop()
{
    running = false;
}

void Application::shutdown()
{
    // Delete self to trigger the destructors
    // We'll come back to this later, this'll do for now.
    delete this;
}

void Application::cullClosedWindowsExitOnMainDeath()
{
    for (size_t i = 0; i < appWindows.size(); i++) {
        if (appWindows[i]->shouldClose()) {
            // If the main window is closing, stop the application
            if (i == 0) {
                stop();
                break;
            }

            // Remove the window
            appWindows.erase(appWindows.begin() + i);
        }
    }
}

void Application::init()
{
    // Initialize GLFW
    glfwSetErrorCallback(Window::GlfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "GLFW: Could not initalize!\n";
        return;
    }

    // Make sure Vulkan is supported
    if (!glfwVulkanSupported()) {
        std::cerr << "GLFW: Vulkan unsupported!\n";
        return;
    }

    // Create the renderer
    renderer = std::make_shared<Renderer>();
}

Application& Application::Get()
{
    return *instance;
}

} // namespace Prism