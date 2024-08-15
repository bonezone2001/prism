/**
 * @file entry_point.h
 * @author Kyle Pelham (bonezone2001@gmail.com)
 * @brief Just a simple entry point for the Prism application framework.
 * 
 * You can completely ignore this file and implement your own entry point if you wish.
 * 
 * @version 0.1
 * @date 2024-08-09
 * 
 * @copyright Copyright (c) 2024
 * 
*/

#pragma once
#include <Windows.h>
#include "prism/prism.h"

namespace Prism
{
    class Application* AppCreate(int argc, char** argv);
};

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    // Create the application
    Prism::Application* app = Prism::AppCreate(__argc, __argv);

    // Run the application
    app->run();

    // Clean up
    delete app;
    return 0;
}