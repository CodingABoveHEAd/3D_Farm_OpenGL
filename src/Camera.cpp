#include "Camera.h"

#include "Input.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265358979323846f;

constexpr float MoveSpeed = 90.0f;          // units per second
constexpr float SprintMultiplier = 3.0f;    // hold Left Shift
constexpr float SlowMultiplier = 0.3f;      // hold Left Ctrl
constexpr float MoveResponsiveness = 20.0f; // smooth acceleration and braking
constexpr float LookResponsiveness = 24.0f; // smooth mouse/keyboard look
constexpr float MouseSensitivity = 0.08f;   // degrees per pixel
constexpr float KeyLookSpeed = 90.0f;       // degrees per second
constexpr float MaxDeltaTime = 0.05f;       // ignore frame-time spikes

bool initialized = false;
bool cursorCaptured = false;
bool prevLeftMouse = false;

int windowedX = 100;
int windowedY = 100;
int windowedWidth = 1280;
int windowedHeight = 720;

float clampPitch(float pitch)
{
    return std::max(-89.0f, std::min(89.0f, pitch));
}

float wrapAngle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

void toggleFullscreen(GLFWwindow* window)
{
    if (glfwGetWindowMonitor(window) == nullptr)
    {
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(
            window, monitor, 0, 0,
            mode->width, mode->height, mode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(
            window, nullptr,
            windowedX, windowedY,
            windowedWidth, windowedHeight, 0);
    }

    glfwSwapInterval(0);
}

void setMouseLook(GLFWwindow* window, bool enabled)
{
    cursorCaptured = enabled;

    if (enabled)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported())
        {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
    }
    else
    {
        if (glfwRawMouseMotionSupported())
        {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        }
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}
}

void Camera::reset()
{
    position_[0] = 0.0f;
    position_[1] = 4.0f;
    position_[2] = 10.0f;
    yaw_ = -90.0f;
    pitch_ = -15.0f;
    targetYaw_ = yaw_;
    targetPitch_ = pitch_;
    fieldOfView_ = 45.0f;
    firstMouse_ = true;

    velocity_[0] = velocity_[1] = velocity_[2] = 0.0f;
}

void Camera::update(GLFWwindow* window, float deltaTime)
{
    deltaTime = std::min(deltaTime, MaxDeltaTime);

    if (window != nullptr)
    {
        if (!initialized)
        {
            glfwSwapInterval(0);
            setMouseLook(window, false);
            initialized = true;
        }

        // Lost focus (alt-tab etc.): release the mouse and stop moving
        if (!glfwGetWindowAttrib(window, GLFW_FOCUSED))
        {
            if (cursorCaptured)
            {
                setMouseLook(window, false);
            }
            velocity_[0] = velocity_[1] = velocity_[2] = 0.0f;
            prevLeftMouse = false;
            return;
        }

        if (Input::wasPressed(GLFW_KEY_F11))
        {
            toggleFullscreen(window);
        }

        // Click the window to capture the mouse (like most games)
        const bool leftMouse =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (leftMouse && !prevLeftMouse && !cursorCaptured)
        {
            setMouseLook(window, true);
            firstMouse_ = true;
        }
        prevLeftMouse = leftMouse;

        // Esc releases the mouse; Tab toggles it
        if (cursorCaptured && Input::wasPressed(GLFW_KEY_ESCAPE))
        {
            setMouseLook(window, false);
        }
        if (Input::wasPressed(GLFW_KEY_TAB))
        {
            setMouseLook(window, !cursorCaptured);
            firstMouse_ = true;
        }
    }

    // ---- Keyboard look (always available) ----
    if (Input::isDown(GLFW_KEY_LEFT))  targetYaw_ -= KeyLookSpeed * deltaTime;
    if (Input::isDown(GLFW_KEY_RIGHT)) targetYaw_ += KeyLookSpeed * deltaTime;
    if (Input::isDown(GLFW_KEY_UP))    targetPitch_ += KeyLookSpeed * deltaTime;
    if (Input::isDown(GLFW_KEY_DOWN))  targetPitch_ -= KeyLookSpeed * deltaTime;

    targetPitch_ = clampPitch(targetPitch_);
    const float lookBlend = 1.0f - std::exp(-LookResponsiveness * deltaTime);
    yaw_ += wrapAngle(targetYaw_ - yaw_) * lookBlend;
    pitch_ += (targetPitch_ - pitch_) * lookBlend;

    // ---- Movement ----
    const float yawRad = yaw_ * Pi / 180.0f;
    // W/S use horizontal camera-forward and A/D use horizontal camera-right.
    // Looking up or down never changes the height of WASD movement.
    const float fwdX = std::cos(yawRad);
    const float fwdZ = std::sin(yawRad);
    const float rightX = -std::sin(yawRad);
    const float rightZ = std::cos(yawRad);

    float forward = 0.0f;
    float strafe = 0.0f;
    float lift = 0.0f;

    if (Input::isDown(GLFW_KEY_W)) forward += 1.0f;
    if (Input::isDown(GLFW_KEY_S)) forward -= 1.0f;
    if (Input::isDown(GLFW_KEY_D)) strafe += 1.0f;
    if (Input::isDown(GLFW_KEY_A)) strafe -= 1.0f;
    if (Input::isDown(GLFW_KEY_Q)) lift += 1.0f;   // up
    if (Input::isDown(GLFW_KEY_E)) lift -= 1.0f;   // down

    // Build the wish direction, then normalize so diagonals aren't faster
    float dirX = fwdX * forward + rightX * strafe;
    float dirY = lift;
    float dirZ = fwdZ * forward + rightZ * strafe;

    const float length = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (length > 1.0f)
    {
        dirX /= length;
        dirY /= length;
        dirZ /= length;
    }

    float speed = MoveSpeed;
    if (Input::isDown(GLFW_KEY_LEFT_SHIFT))   speed *= SprintMultiplier;
    if (Input::isDown(GLFW_KEY_LEFT_CONTROL)) speed *= SlowMultiplier;

    // Ease toward target velocity (smooth start and stop)
    const float blend = 1.0f - std::exp(-MoveResponsiveness * deltaTime);
    velocity_[0] += (dirX * speed - velocity_[0]) * blend;
    velocity_[1] += (dirY * speed - velocity_[1]) * blend;
    velocity_[2] += (dirZ * speed - velocity_[2]) * blend;

    for (float& v : velocity_)
    {
        if (std::fabs(v) < 0.001f) v = 0.0f;
    }

    position_[0] += velocity_[0] * deltaTime;
    position_[1] += velocity_[1] * deltaTime;
    position_[2] += velocity_[2] * deltaTime;

    if (Input::wasPressed(GLFW_KEY_R))
    {
        reset();
    }
}

void Camera::onMouseMove(double xPosition, double yPosition)
{
    const double dx = xPosition - lastMouseX_;
    const double dy = lastMouseY_ - yPosition;
    lastMouseX_ = xPosition;
    lastMouseY_ = yPosition;

    // Only look around while the mouse is captured
    if (!cursorCaptured)
    {
        firstMouse_ = true;
        return;
    }

    if (firstMouse_)
    {
        firstMouse_ = false;
        return;
    }

    // Applied immediately: no mouse lag, feels like a real game
    targetYaw_ += static_cast<float>(dx) * MouseSensitivity;
    targetPitch_ = clampPitch(targetPitch_ + static_cast<float>(dy) * MouseSensitivity);
}

void Camera::applyProjection(int width, int height) const
{
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 1000.0f;
    const float top = nearPlane * std::tan(fieldOfView_ * Pi / 360.0f);
    const float right = top * aspect;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
}

void Camera::applyView() const
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(-pitch_, 1.0f, 0.0f, 0.0f);
    glRotatef(-yaw_ - 90.0f, 0.0f, 1.0f, 0.0f);
    glTranslatef(-position_[0], -position_[1], -position_[2]);
}