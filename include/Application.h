#pragma once

#include "Camera.h"
#include "Scene.h"

#include <string>

struct GLFWwindow;

class Application {
public:
    bool initialize(int width, int height, const char* title);
    void run();
    void shutdown();

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void cursorPositionCallback(GLFWwindow* window, double xPosition, double yPosition);
    static void windowFocusCallback(GLFWwindow* window, int focused);
    void toggleFullscreen();
    void setVerticalSync(bool enabled);
    void updatePerformanceStats(double now);
    void updateWindowSmokeTest(double now);
    void renderFrame(float deltaTime);

    GLFWwindow* window_ = nullptr;
    Camera camera_;
    Scene scene_;
    std::string baseTitle_;
    double previousTime_ = 0.0;
    double statsStartTime_ = 0.0;
    double benchmarkStartTime_ = 0.0;
    double benchmarkDuration_ = 0.0;
    double benchmarkWarmup_ = 0.0;
    unsigned long long statsFrameCount_ = 0;
    unsigned long long benchmarkFrameCount_ = 0;
    int framebufferWidth_ = 1;
    int framebufferHeight_ = 1;
    int windowedX_ = 100;
    int windowedY_ = 100;
    int windowedWidth_ = 1280;
    int windowedHeight_ = 720;
    bool fullscreen_ = false;
    bool verticalSync_ = true;
    bool benchmarkStarted_ = false;
    bool windowSmokeTest_ = false;
    int windowSmokeStage_ = 0;
    double windowSmokeStartTime_ = 0.0;
};
