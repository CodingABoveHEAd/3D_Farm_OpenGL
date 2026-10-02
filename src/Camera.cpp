#include "Camera.h"

#include "Input.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265358979323846f;

constexpr float MoveSpeed = 5.0f;          // units per second
constexpr float MoveResponsiveness = 12.0f; // higher = snappier acceleration
constexpr float LookResponsiveness = 30.0f; // higher = less mouse smoothing
constexpr float MouseSensitivity = 0.10f;
constexpr float KeyLookSpeed = 60.0f;       // degrees per second
constexpr float MaxDeltaTime = 0.05f;       // ignore frame-time spikes

// Smoothing state (single camera, so file-level state is fine and
// keeps Camera.h unchanged)
float velocity[3] = {0.0f, 0.0f, 0.0f};
float pendingYaw = 0.0f;
float pendingPitch = 0.0f;

bool initialized = false;
bool cursorCaptured = false;

int windowedX = 100;
int windowedY = 100;
int windowedWidth = 1280;
int windowedHeight = 720;

float clampPitch(float pitch)
{
    return std::max(-89.0f, std::min(89.0f, pitch));
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

    // Keep presentation synced to the monitor after the mode change
    glfwSwapInterval(1);
}

void setMouseLook(GLFWwindow* window, bool enabled)
{
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
    fieldOfView_ = 45.0f;
    firstMouse_ = true;

    velocity[0] = velocity[1] = velocity[2] = 0.0f;
    pendingYaw = 0.0f;
    pendingPitch = 0.0f;
}

void Camera::update(GLFWwindow* window, float deltaTime)
{
    // Avoid huge jumps after a hitch (window drag, resize, etc.)
    deltaTime = std::min(deltaTime, MaxDeltaTime);

    if (window != nullptr)
    {
        // One-time setup: visible cursor and vsync
        if (!initialized)
        {
            glfwSwapInterval(1);
            setMouseLook(window, false);
            initialized = true;
        }

        // F11 = toggle fullscreen
        if (Input::wasPressed(GLFW_KEY_F11))
        {
            toggleFullscreen(window);
        }

        // Tab = toggle between a visible cursor and a captured (hidden)
        // cursor. Captured mode lets you keep turning without ever
        // hitting the edge of the screen.
        if (Input::wasPressed(GLFW_KEY_TAB))
        {
            cursorCaptured = !cursorCaptured;
            setMouseLook(window, cursorCaptured);
            firstMouse_ = true;
            pendingYaw = 0.0f;
            pendingPitch = 0.0f;
        }
    }

    // Camera Mode Switching
    if (Input::wasPressed(GLFW_KEY_F1)) mode_ = CameraMode::Free;
    if (Input::wasPressed(GLFW_KEY_F2)) mode_ = CameraMode::Overview;
    if (Input::wasPressed(GLFW_KEY_F3)) mode_ = CameraMode::FarmFocus;
    if (Input::wasPressed(GLFW_KEY_F4)) mode_ = CameraMode::TractorFollow;

    // Reset camera with Home or F5 (R is now for windmill control)
    if (Input::wasPressed(GLFW_KEY_HOME) || Input::wasPressed(GLFW_KEY_F5))
    {
        reset();
    }

    if (mode_ == CameraMode::Overview)
    {
        // High-altitude circular orbit around entire countryside
        orbitAngle_ += 6.5f * deltaTime;
        const float rad = orbitAngle_ * Pi / 180.0f;
        position_[0] = std::sin(rad) * 260.0f;
        position_[1] = 115.0f;
        position_[2] = std::cos(rad) * 260.0f;
        yaw_ = -orbitAngle_ - 90.0f;
        pitch_ = -22.0f;
        velocity[0] = velocity[1] = velocity[2] = 0.0f;
        return;
    }
    else if (mode_ == CameraMode::FarmFocus)
    {
        // Low cinematic orbit around nearest farm
        orbitAngle_ += 14.0f * deltaTime;
        const float rad = orbitAngle_ * Pi / 180.0f;
        position_[0] = farmFocusPos_[0] + std::sin(rad) * 54.0f;
        position_[1] = 18.0f;
        position_[2] = farmFocusPos_[1] + std::cos(rad) * 54.0f;
        yaw_ = -orbitAngle_ - 90.0f;
        pitch_ = -16.0f;
        velocity[0] = velocity[1] = velocity[2] = 0.0f;
        return;
    }
    else if (mode_ == CameraMode::TractorFollow)
    {
        // 3rd person chase camera behind tractor
        const float rad = tractorHeading_ * Pi / 180.0f;
        const float chaseDist = 11.5f;
        const float chaseHeight = 5.2f;

        // Position behind tractor (tractor faces -Z in local coords, so behind is +Z rotated by heading)
        const float targetCamX = tractorPos_[0] + std::sin(rad) * chaseDist;
        const float targetCamZ = tractorPos_[2] + std::cos(rad) * chaseDist;
        const float targetCamY = tractorPos_[1] + chaseHeight;

        const float followBlend = 1.0f - std::exp(-9.0f * deltaTime);
        position_[0] += (targetCamX - position_[0]) * followBlend;
        position_[1] += (targetCamY - position_[1]) * followBlend;
        position_[2] += (targetCamZ - position_[2]) * followBlend;

        yaw_ = tractorHeading_ - 90.0f;
        pitch_ = -18.0f;
        velocity[0] = velocity[1] = velocity[2] = 0.0f;
        return;
    }

    // ---- Free Mode: Smoothed mouse look ----
    const float lookBlend = 1.0f - std::exp(-LookResponsiveness * deltaTime);
    const float appliedYaw = pendingYaw * lookBlend;
    const float appliedPitch = pendingPitch * lookBlend;
    yaw_ += appliedYaw;
    pitch_ += appliedPitch;
    pendingYaw -= appliedYaw;
    pendingPitch -= appliedPitch;

    pitch_ = clampPitch(pitch_);

    if (yaw_ > 360.0f)
    {
        yaw_ -= 360.0f;
    }
    else if (yaw_ < -360.0f)
    {
        yaw_ += 360.0f;
    }

    // ---- Movement (WASD + QE) ----
    const float yawRadians = yaw_ * Pi / 180.0f;
    const float forwardX = std::cos(yawRadians);
    const float forwardZ = std::sin(yawRadians);

    float forward = 0.0f;
    float strafe = 0.0f;
    float lift = 0.0f;

    if (Input::isDown(GLFW_KEY_W)) forward += 1.0f;
    if (Input::isDown(GLFW_KEY_S)) forward -= 1.0f;
    if (Input::isDown(GLFW_KEY_D)) strafe += 1.0f;
    if (Input::isDown(GLFW_KEY_A)) strafe -= 1.0f;
    if (Input::isDown(GLFW_KEY_Q)) lift += 1.0f;
    if (Input::isDown(GLFW_KEY_E)) lift -= 1.0f;

    const float planarLength = std::sqrt(forward * forward + strafe * strafe);
    if (planarLength > 1.0f)
    {
        forward /= planarLength;
        strafe /= planarLength;
    }

    const float targetX = (forwardX * forward - forwardZ * strafe) * MoveSpeed;
    const float targetZ = (forwardZ * forward + forwardX * strafe) * MoveSpeed;
    const float targetY = lift * MoveSpeed;

    const float moveBlend = 1.0f - std::exp(-MoveResponsiveness * deltaTime);
    velocity[0] += (targetX - velocity[0]) * moveBlend;
    velocity[1] += (targetY - velocity[1]) * moveBlend;
    velocity[2] += (targetZ - velocity[2]) * moveBlend;

    for (float& v : velocity)
    {
        if (std::fabs(v) < 0.001f)
        {
            v = 0.0f;
        }
    }

    position_[0] += velocity[0] * deltaTime;
    position_[1] += velocity[1] * deltaTime;
    position_[2] += velocity[2] * deltaTime;
}

void Camera::onMouseMove(double xPosition, double yPosition)
{
    if (firstMouse_)
    {
        lastMouseX_ = xPosition;
        lastMouseY_ = yPosition;
        firstMouse_ = false;
        return;
    }

    const float xOffset = static_cast<float>(xPosition - lastMouseX_) * MouseSensitivity;
    const float yOffset = static_cast<float>(lastMouseY_ - yPosition) * MouseSensitivity;
    lastMouseX_ = xPosition;
    lastMouseY_ = yPosition;

    // Queue the movement; update() applies it smoothly
    pendingYaw += xOffset;
    pendingPitch += yOffset;
}

void Camera::applyProjection(int width, int height) const
{
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    // FIX: far plane was 200 but fog ends at 420 and the ground grid / farms are
    // drawn out to ~380-480 units, so everything past 200 was clipped away.
    const float nearPlane = 0.3f;
    const float farPlane = 800.0f;
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

float Camera::posX() const
{
    return position_[0];
}

float Camera::posY() const
{
    return position_[1];
}

float Camera::posZ() const
{
    return position_[2];
}

float Camera::yawDegrees() const
{
    return yaw_;
}

float Camera::pitchDegrees() const
{
    return pitch_;
}

void Camera::setPose(float x, float y, float z, float yaw, float pitch)
{
    position_[0] = x;
    position_[1] = y;
    position_[2] = z;
    yaw_ = yaw;
    pitch_ = clampPitch(pitch);
}

void Camera::setMode(CameraMode mode)
{
    mode_ = mode;
}

CameraMode Camera::mode() const
{
    return mode_;
}

void Camera::setTractorPose(float x, float y, float z, float heading)
{
    tractorPos_[0] = x;
    tractorPos_[1] = y;
    tractorPos_[2] = z;
    tractorHeading_ = heading;
}

void Camera::setFocusFarm(float x, float z)
{
    farmFocusPos_[0] = x;
    farmFocusPos_[1] = z;
}