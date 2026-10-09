#include "Application.h"

#include "DayNightSettings.h"
#include "Input.h"
#include "objects/sky.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
GLFWmonitor* monitorForWindow(GLFWwindow* window)
{
    int windowX = 0;
    int windowY = 0;
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowPos(window, &windowX, &windowY);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    GLFWmonitor* bestMonitor = glfwGetPrimaryMonitor();
    long long bestOverlap = -1;
    for (int index = 0; index < monitorCount; ++index)
    {
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitors[index], &monitorX, &monitorY);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[index]);
        if (!mode)
        {
            continue;
        }

        const int overlapWidth = std::max(
            0, std::min(windowX + windowWidth, monitorX + mode->width)
                - std::max(windowX, monitorX));
        const int overlapHeight = std::max(
            0, std::min(windowY + windowHeight, monitorY + mode->height)
                - std::max(windowY, monitorY));
        const long long overlap =
            static_cast<long long>(overlapWidth) * overlapHeight;
        if (overlap > bestOverlap)
        {
            bestOverlap = overlap;
            bestMonitor = monitors[index];
        }
    }
    return bestMonitor;
}
}

bool Application::initialize(int width, int height, const char* title)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    baseTitle_ = title ? title : "Animated 3D Farm Scene";
    windowedWidth_ = width;
    windowedHeight_ = height;
    window_ = glfwCreateWindow(width, height, baseTitle_.c_str(), nullptr, nullptr);
    if (!window_)
    {
        std::cerr << "Failed to create OpenGL window.\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    // V-sync avoids needless CPU/GPU usage and visible tearing. It can be
    // toggled with V, or disabled for repeatable profiling with FARM_VSYNC=0.
    const char* verticalSync = std::getenv("FARM_VSYNC");
    setVerticalSync(!verticalSync || std::string(verticalSync) != "0");
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);
    glfwSetCursorPosCallback(window_, cursorPositionCallback);
    glfwSetWindowFocusCallback(window_, windowFocusCallback);
    glfwSetWindowSizeLimits(window_, 640, 360, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 150.0f);
    glFogf(GL_FOG_END, 260.0f);
    glHint(GL_FOG_HINT, GL_NICEST);
    glClearColor(0.52f, 0.80f, 0.98f, 1.0f);

    glfwGetWindowPos(window_, &windowedX_, &windowedY_);
    glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);
    glViewport(0, 0, framebufferWidth_, framebufferHeight_);
    camera_.applyProjection(framebufferWidth_, framebufferHeight_);
    Input::initialize(window_);
    previousTime_ = glfwGetTime();
    statsStartTime_ = previousTime_;
    benchmarkStartTime_ = previousTime_;
    windowSmokeStartTime_ = previousTime_;
    windowSmokeTest_ = std::getenv("FARM_WINDOW_SMOKE_TEST") != nullptr;

    std::cout
        << "BARN_CONTROLS G=doors Z/X=wheelbarrow C=chest F=feeding_gate\n";

    if (const char* seconds = std::getenv("FARM_BENCHMARK_SECONDS"))
    {
        benchmarkDuration_ = std::max(0.0, std::strtod(seconds, nullptr));
        benchmarkWarmup_ = 5.0;
        if (const char* warmup = std::getenv("FARM_BENCHMARK_WARMUP"))
        {
            benchmarkWarmup_ = std::max(0.0, std::strtod(warmup, nullptr));
        }
        benchmarkStarted_ = benchmarkWarmup_ <= 0.0;
    }

    return true;
}

void Application::run()
{
    while (!glfwWindowShouldClose(window_))
    {
        const double currentTime = glfwGetTime();
        const float deltaTime = std::min(static_cast<float>(currentTime - previousTime_), 0.25f);
        previousTime_ = currentTime;

        // Process mouse and keyboard events before sampling input so movement
        // responds in the same frame instead of one frame late.
        glfwPollEvents();
        Input::update(window_);
        if (Input::wasPressed(GLFW_KEY_ESCAPE))
        {
            if (camera_.isMouseCaptured())
            {
                camera_.setMouseLook(window_, false);
            }
            else
            {
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            }
        }
        if (Input::wasPressed(GLFW_KEY_F11))
        {
            toggleFullscreen();
        }
        if (Input::wasPressed(GLFW_KEY_V))
        {
            setVerticalSync(!verticalSync_);
        }

        // A zero-sized framebuffer is normal while minimized. Avoid a busy
        // render loop and avoid replacing the last valid aspect ratio.
        if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0)
        {
            glfwWaitEventsTimeout(0.05);
            previousTime_ = glfwGetTime();
            continue;
        }

        scene_.handleInput();
        const float previousCameraX = camera_.posX();
        const float previousCameraZ = camera_.posZ();
        camera_.update(window_, deltaTime);
        float resolvedCameraX = camera_.posX();
        float resolvedCameraZ = camera_.posZ();
        scene_.resolveCameraCollision(
            previousCameraX, previousCameraZ, camera_.posY(),
            resolvedCameraX, resolvedCameraZ);
        camera_.setPosition(resolvedCameraX, camera_.posY(), resolvedCameraZ);
        scene_.update(deltaTime);
        renderFrame(deltaTime);

        glfwSwapBuffers(window_);
        ++statsFrameCount_;
        if (benchmarkStarted_)
        {
            ++benchmarkFrameCount_;
        }
        updatePerformanceStats(glfwGetTime());
        updateWindowSmokeTest(glfwGetTime());
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
    if (application)
    {
        application->framebufferWidth_ = width;
        application->framebufferHeight_ = height;
        if (width > 0 && height > 0)
        {
            glViewport(0, 0, width, height);
            application->camera_.applyProjection(width, height);
        }
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

void Application::windowFocusCallback(GLFWwindow* window, int focused)
{
    if (focused == GLFW_TRUE)
    {
        return;
    }

    // Clear held actions at the focus event rather than waiting for GLFW's
    // synthetic releases. Also release capture so returning to the window
    // cannot create a large mouse-look jump.
    Input::clear();
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
    if (application && application->camera_.isMouseCaptured())
    {
        application->camera_.setMouseLook(window, false);
    }
}

void Application::toggleFullscreen()
{
    if (!window_)
    {
        return;
    }

    if (!fullscreen_)
    {
        glfwGetWindowPos(window_, &windowedX_, &windowedY_);
        glfwGetWindowSize(window_, &windowedWidth_, &windowedHeight_);
        GLFWmonitor* monitor = monitorForWindow(window_);
        const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
        if (!monitor || !mode)
        {
            return;
        }

        glfwSetWindowMonitor(
            window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        fullscreen_ = true;
    }
    else
    {
        glfwSetWindowMonitor(
            window_, nullptr, windowedX_, windowedY_,
            std::max(640, windowedWidth_), std::max(360, windowedHeight_),
            GLFW_DONT_CARE);
        fullscreen_ = false;
    }

    // GLFW may recreate/switch the swap chain during a monitor transition.
    setVerticalSync(verticalSync_);
    glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);
    if (framebufferWidth_ > 0 && framebufferHeight_ > 0)
    {
        glViewport(0, 0, framebufferWidth_, framebufferHeight_);
        camera_.applyProjection(framebufferWidth_, framebufferHeight_);
    }
}

void Application::setVerticalSync(bool enabled)
{
    verticalSync_ = enabled;
    glfwSwapInterval(enabled ? 1 : 0);
}

void Application::updatePerformanceStats(double now)
{
    const double statsElapsed = now - statsStartTime_;
    if (statsElapsed >= 1.0)
    {
        const double framesPerSecond =
            static_cast<double>(statsFrameCount_) / statsElapsed;
        std::ostringstream title;
        title << baseTitle_ << " | " << std::fixed << std::setprecision(1)
              << framesPerSecond << " FPS | "
              << (1000.0 / std::max(0.001, framesPerSecond)) << " ms | "
              << (verticalSync_ ? "VSync" : "Uncapped")
              << (fullscreen_ ? " | Fullscreen" : " | Windowed")
              << (scene_.isNight() ? " | Night" : " | Day")
              << (scene_.isPowerOn() ? " | Power ON" : " | POWER OUT")
              << (scene_.isTrafficRunning() ? " | Traffic ON" : " | Traffic PAUSED")
              << (scene_.areBonfiresEnabled() ? " | Fires ON" : " | Fires OFF")
              << (scene_.areWorkersPaused() ? " | Workers PAUSED" : " | Workers ON")
              << (scene_.areBarnDoorsOpen() ? " | Barn OPEN" : " | Barn CLOSED")
              << " x" << std::setprecision(2) << scene_.workerSpeed();
        glfwSetWindowTitle(window_, title.str().c_str());
        statsFrameCount_ = 0;
        statsStartTime_ = now;
    }

    if (benchmarkDuration_ > 0.0 && !benchmarkStarted_)
    {
        if (now - benchmarkStartTime_ < benchmarkWarmup_)
        {
            return;
        }
        benchmarkStarted_ = true;
        benchmarkStartTime_ = now;
        benchmarkFrameCount_ = 0;
        std::cout << "BENCHMARK warmup_complete seconds="
                  << std::fixed << std::setprecision(3) << benchmarkWarmup_ << '\n';
        return;
    }

    const double benchmarkElapsed = now - benchmarkStartTime_;
    if (benchmarkDuration_ > 0.0 && benchmarkElapsed >= benchmarkDuration_)
    {
        const double fps =
            static_cast<double>(benchmarkFrameCount_) / benchmarkElapsed;
        std::cout << "BENCHMARK frames=" << benchmarkFrameCount_
                  << " seconds=" << std::fixed << std::setprecision(3)
                  << benchmarkElapsed << " fps=" << std::setprecision(2) << fps
                  << " ms=" << std::setprecision(3) << (1000.0 / fps)
                  << " camera=" << std::setprecision(2)
                  << camera_.posX() << ',' << camera_.posY() << ','
                  << camera_.posZ()
                  << " yaw=" << camera_.yawDegrees()
                  << " pitch=" << camera_.pitchDegrees()
                  << " night=" << std::setprecision(3)
                  << scene_.nightAmount()
                  << " power=" << (scene_.isPowerOn() ? 1 : 0)
                  << " traffic=" << (scene_.isTrafficRunning() ? 1 : 0)
                  << " bonfires=" << (scene_.areBonfiresEnabled() ? 1 : 0)
                  << " traffic_gap=" << std::setprecision(2)
                  << scene_.minimumTrafficGap()
                  << " crop_workers=" << scene_.cropWorkerCount()
                  << " workers_paused=" << (scene_.areWorkersPaused() ? 1 : 0)
                  << " work_speed=" << scene_.workerSpeed() << '\n';
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
        benchmarkDuration_ = 0.0;
    }
}

void Application::updateWindowSmokeTest(double now)
{
    if (!windowSmokeTest_)
    {
        return;
    }

    const double elapsed = now - windowSmokeStartTime_;
    if (windowSmokeStage_ == 0 && elapsed >= 1.0)
    {
        std::cout << "WINDOW_TEST initial=" << windowedWidth_ << 'x'
                  << windowedHeight_ << '\n';
        toggleFullscreen();
        windowSmokeStage_ = 1;
    }
    else if (windowSmokeStage_ == 1 && elapsed >= 2.5)
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window_, &width, &height);
        std::cout << "WINDOW_TEST fullscreen=" << width << 'x' << height
                  << " monitor=" << (glfwGetWindowMonitor(window_) ? 1 : 0)
                  << " framebuffer=" << framebufferWidth_ << 'x'
                  << framebufferHeight_ << '\n';
        toggleFullscreen();
        windowSmokeStage_ = 2;
    }
    else if (windowSmokeStage_ == 2 && elapsed >= 4.0)
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window_, &width, &height);
        std::cout << "WINDOW_TEST restored=" << width << 'x' << height
                  << " monitor=" << (glfwGetWindowMonitor(window_) ? 1 : 0)
                  << '\n';
        glfwSetWindowSize(window_, 1000, 600);
        windowSmokeStage_ = 3;
    }
    else if (windowSmokeStage_ == 3 && elapsed >= 5.5)
    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window_, &width, &height);
        std::cout << "WINDOW_TEST resized=" << width << 'x' << height
                  << " framebuffer=" << framebufferWidth_ << 'x'
                  << framebufferHeight_ << '\n';
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
        windowSmokeTest_ = false;
    }
}

void Application::renderFrame(float)
{
    const float night = scene_.nightAmount();
    Sky::setNightAmount(night);
    GLfloat fog[4];
    for (int component = 0; component < 4; ++component)
    {
        fog[component] = DayNightSettings::DayFog[component]
            + (DayNightSettings::NightFog[component]
               - DayNightSettings::DayFog[component]) * night;
    }
    glFogfv(GL_FOG_COLOR, fog);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the sky without camera translation so it remains infinitely far
    // away and cannot be left behind as the player moves around the world.
    camera_.applySkyView();
    scene_.renderSky();

    camera_.applyView();
    scene_.render();
}
