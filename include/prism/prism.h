/**
 * @file prism.h
 * @author Kyle Pelham (bonezone2001@gmail.com)
 * @brief The main header file for the Prism application framework.
 * 
 * @copyright Copyright (c) 2024
*/

#pragma once
#include <string>
#include <fmt/core.h>
#include "prism/prism_export.hpp"
#include "prism/renderer.h"
#include "prism/window.h"

namespace Prism
{

class PRISM_EXPORT Application
{
protected:
    bool running = true;                                ///< Is the application running?
    std::string name;                                   ///< The name of the application.
    static Application* instance;                       ///< The global instance of the application.
    std::vector<std::shared_ptr<Window>> appWindows;    ///< The windows in the application.
    std::shared_ptr<Renderer> renderer;                 ///< The Vulkan renderer for the application.

public:
    /// Construct a new Application object
    Application(std::string name = "Prism App");

    /// Destroy the Application object
    virtual ~Application();

    /**
     * Getter for the Application singleton object.
     * @return Application& The global Application object.
    */
    static Application& Get();

    /**
     * Initialize the application.
     * This should be called in the constructor.
    */
    virtual void init();

    /**
     * Run the application.
     * This will spawn the main loop for the application and show any windows.
    */
    virtual void run();

    /**
     * Stop the application.
     * This will set the running flag to false, causing the application to exit on the next iteration.
    */
    virtual void stop();

    /**
     * Add a window to the application's window stack.
     * 
     * Note: The first window added will be the main window, terminating the application when closed.
     * If this is not desired, add an empty window first.
     * 
     * @tparam T The type of window to add.
     * @param args The arguments to pass to the window's constructor.
    */
    template<typename T, typename... Args>
    std::shared_ptr<T> addWindow(Args&&... args)
    {
        static_assert(std::is_base_of<Prism::Window, T>::value, "Added type is not subclass of a Prism::Window!");
        auto window = std::make_shared<T>(std::forward<Args>(args)...);
        appWindows.emplace_back(window);
        return window;
    }

    // Getters
    // -------------------------------------------------------------------------
    bool isRunning() const { return running; }                                      ///< @return bool Is the application running?
    std::shared_ptr<Renderer> getRenderer() const { return renderer; }              ///< @return std::shared_ptr<Renderer> The renderer for the application.
    std::string getName() const { return name; }                                    ///< @return std::string The name of the application.
    std::vector<std::shared_ptr<Window>> getWindows() const { return appWindows; }  ///< @return std::vector<std::shared_ptr<Window>> The windows in the application.

private:
    /**
     * Shutdown the application.
     * This will essentially clean up any resources used by the application.
    */
    virtual void shutdown();

    /**
     * Remove windows that have been closed.
    */
    void cullClosedWindowsExitOnMainDeath();
};

} // namespace Prism