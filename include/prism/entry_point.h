/**
 * @file entry_point.h
 * @author Kyle Pelham (bonezone2001@gmail.com)
 * @brief Just a simple entry point for the Prism application framework.
 * 
 * You can completely ignore this file and implement your own entry point if you wish.
 * The Prism::AppCreate function doesn't do anything by itself, it's just there for the
 * entry point in this file to call. So use the provided entry point as a reference if
 * you want to implement your own.
 * 
 * @copyright Copyright (c) 2024
*/

#pragma once
#include <Windows.h>
#include "prism/prism.h"

namespace Prism
{
    std::unique_ptr<class Application> AppCreate(int argc, char** argv);
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    // Create the application
    auto app = Prism::AppCreate(__argc, __argv);

    // Run the application
    app->run();

    // Shutdown the application
    app.reset();

    return 0;
}