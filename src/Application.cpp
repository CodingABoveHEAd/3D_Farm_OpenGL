#include "Application.h"

#include "Input.h"
#include "objects/sky.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>

bool Application::initialize(int width, int height, const char* title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

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
    glfwSetCursorPosCallback(window_, cursorPositionCallback);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);
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
        const float deltaTime = std::min(static_cast<float>(currentTime - previousTime_), 0.1f);
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

void Application::cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition)
{
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application)
    {
        application->camera_.onMouseMove(xPosition, yPosition);
    }
}

void Application::renderFrame(float)
{
    glDisable(GL_LIGHTING);
    Sky::setClearColor(scene_.isNight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera_.applyView();
    scene_.render();
}
