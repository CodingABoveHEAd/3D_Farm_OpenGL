#pragma once

#include "Camera.h"
#include "Scene.h"

struct GLFWwindow;

class Application {
public:
    bool initialize(int width, int height, const char* title);
    void run();
    void shutdown();

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    void renderFrame(float deltaTime);

    GLFWwindow* window_ = nullptr;
    Camera camera_;
    Scene scene_;
    double previousTime_ = 0.0;
};
