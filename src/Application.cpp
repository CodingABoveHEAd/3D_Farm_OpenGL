#include "Application.h"

#include "Input.h"

#include <GLFW/glfw3.h>
#include <iostream>

bool Application::initialize(int width, int height, const char* title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return false;
    }

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_)
    {
        std::cerr << "Failed to create OpenGL window.\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glClearColor(0.52f, 0.80f, 0.98f, 1.0f);

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    camera_.applyProjection(framebufferWidth, framebufferHeight);
    Input::initialize(window_);
    previousTime_ = glfwGetTime();

    return true;
}

void Application::run()
{
    while (!glfwWindowShouldClose(window_))
    {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - previousTime_);
        previousTime_ = currentTime;

        Input::update(window_);
        if (Input::wasPressed(GLFW_KEY_ESCAPE))
        {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }

        scene_.handleInput();
        camera_.update(window_, deltaTime);
        scene_.update(deltaTime);
        renderFrame(deltaTime);

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }
}

void Application::shutdown()
{
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    glViewport(0, 0, width, height);
    if (application)
    {
        application->camera_.applyProjection(width, height);
    }
}

void Application::renderFrame(float)
{
    if (scene_.isNight())
    {
        glClearColor(0.03f, 0.06f, 0.16f, 1.0f);
    }
    else
    {
        glClearColor(0.52f, 0.80f, 0.98f, 1.0f);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera_.applyView();
    scene_.render();
}
