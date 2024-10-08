<a name="readme-top"></a>


<!-- PROJECT LOGO -->
<br />
<div align="center">
  <img src="assets/images/logo.png" alt="Logo" width="110" height="100">

  <h3 align="center">C++ GUI Application Framework</h3>

  <p align="center">
    Removing the headache of creating modern native GUIs
    <br />
    <br />
    <!-- <a href="https://github.com/bonezone2001/prism/blob/master/example.cpp">View Example</a>
    · -->
    <a href="https://github.com/bonezone2001/prism/issues">Report Bug</a>
    ·
    <a href="https://github.com/bonezone2001/prism/issues">Request Feature</a>
  </p>
</div>


<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">About The Project</a></li>
    <li><a href="#usage-and-examples">Usage and Examples</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>

<br>

<!-- ABOUT THE PROJECT -->
## About The Project

Creating native C++ GUI apps is cumbersome when all the mature options seem to look samesy and stifle creativity. Leveraging the power and flexibility of ImGUI, this issue is averted but getting that running adds unneeded friction. That's what this library attempts to solve.

> NOTE: The project is still fairly young so it's missing some vital features but for most it should suffice. If you need a particular feature implemented just create a feature request in the issues section.

<br>

<!-- USAGE EXAMPLES -->
## Usage and Examples

Here is how you create the application entry point, this is optional and you can instead create an application instance yourself. [Check here](https://github.com/bonezone2001/prism/blob/master/include/prism/entry_point.h)
```cpp
#include "windows/main_window.h"
#include "prism/entry_point.h"

Prism::Application* Prism::AppCreate(int argc, char** argv)
{
    Prism::Application* app = new Prism::Application("Prism App");
    app->addWindow<MainWindow>();

    return app;
}
```

For each window you create, simply create a class and override it's methods.
```cpp
#pragma once
#include "prism/prism.h"

class MainWindow : public Prism::Window
{
public:
    MainWindow();
    MainWindow(Prism::WindowSettings settings);
    ~MainWindow();

    void onUpdate() override;
    void onRender() override;
};
```

Here is an example implementation
```cpp
#include "main_window.h"

// Here is just a sample of a window that can be created with Prism.
MainWindow::MainWindow(int test)
    : Prism::Window({
        .width = 1000,
        .height = 1000,
        .title = "Prism Window",
        .resizable = false,
        .fullscreen = false,
        .useCustomTitlebar = false,
        .showOnCreate = true,
        .parent = nullptr
    }),
    test(test)
{}

// 
MainWindow::MainWindow(Prism::WindowSettings settings)
    : Prism::Window(settings)
{}

MainWindow::~MainWindow()
{
    // The deconstructor is called when the window is destroyed.
}

void MainWindow::onUpdate()
{
    // Any code you want to run before rendering goes here.
}

void MainWindow::onRender()
{
    // Any code related to rendering goes here.

    ImGui::Begin("Test Window");
    ImGui::Text("Test: %d", test);
    ImGui::End();
}
```
Note: the constructor here is slightly different (a test number being passed for the sake of example)

<br>

<!-- CONTRIBUTING -->
## Contributing

Any contributions you can make are **super appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Mwah!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<br>


<!-- LICENSE -->
## License

Distributed under the MIT License. See [LICENSE](https://github.com/bonezone2001/prism/blob/master/LICENSE) for more information.
