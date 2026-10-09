#include "Camera.h"

#include "Input.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265358979323846f;

constexpr float MoveSpeed = 20.0f;          // units per second
constexpr float SprintMultiplier = 3.0f;    // hold Left Shift
constexpr float SlowMultiplier = 0.3f;      // hold Left Ctrl
constexpr float MouseSensitivity = 0.08f;   // degrees per pixel
constexpr float KeyTurnSpeed = 90.0f;       // degrees per second
constexpr float MaxDeltaTime = 0.25f;       // bound long focus/load-time spikes

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

void loadView(float yaw, float pitch, const float position[3], bool translate)
{
    const float yawRadians = yaw * Pi / 180.0f;
    const float pitchRadians = pitch * Pi / 180.0f;
    const float cosPitch = std::cos(pitchRadians);

    // This is the same forward vector used for movement (with pitch added for
    // looking). Building the view basis from it removes the old hidden 90
    // degree correction and prevents rendering and controls from diverging.
    const float forward[3] = {
        std::cos(yawRadians) * cosPitch,
        std::sin(pitchRadians),
        std::sin(yawRadians) * cosPitch
    };
    const float right[3] = {
        -std::sin(yawRadians), 0.0f, std::cos(yawRadians)
    };
    const float up[3] = {
        -forward[1] * right[2],
        right[2] * forward[0] - right[0] * forward[2],
        forward[1] * right[0]
    };

    // Column-major camera matrix: rows are right, up, and -forward.
    const GLfloat view[16] = {
        right[0], up[0], -forward[0], 0.0f,
        right[1], up[1], -forward[1], 0.0f,
        right[2], up[2], -forward[2], 0.0f,
        0.0f,     0.0f,  0.0f,       1.0f
    };
    glMultMatrixf(view);
    if (translate)
    {
        glTranslatef(-position[0], -position[1], -position[2]);
    }
}

}

void Camera::setMouseLook(GLFWwindow* window, bool enabled)
{
    if (!window)
    {
        return;
    }

    cursorCaptured_ = enabled;
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(
            window, GLFW_RAW_MOUSE_MOTION, enabled ? GLFW_TRUE : GLFW_FALSE);
    }
    glfwSetInputMode(
        window, GLFW_CURSOR, enabled ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    firstMouse_ = true;
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
}

void Camera::update(GLFWwindow* window, float deltaTime)
{
    deltaTime = std::min(deltaTime, MaxDeltaTime);

    if (window != nullptr)
    {
        if (!initialized_)
        {
            setMouseLook(window, false);
            initialized_ = true;
        }

        // Lost focus (alt-tab etc.): release the mouse and stop moving
        if (!glfwGetWindowAttrib(window, GLFW_FOCUSED))
        {
            if (cursorCaptured_)
            {
                setMouseLook(window, false);
            }
            Input::clear();
            previousLeftMouse_ = false;
            return;
        }

        // Click the window to capture the mouse (like most games)
        const bool leftMouse =
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (leftMouse && !previousLeftMouse_ && !cursorCaptured_)
        {
            setMouseLook(window, true);
        }
        previousLeftMouse_ = leftMouse;

        // Escape is handled by Application so one press releases the mouse
        // and a second press closes the window.
        if (Input::wasPressed(GLFW_KEY_TAB))
        {
            setMouseLook(window, !cursorCaptured_);
        }
    }

    // ---- Keyboard heading (always available) ----
    // A/D deliberately rotate in place. Summing the two signed key states
    // makes opposing inputs cancel and lets W/S continue moving while yaw is
    // updated each frame. Arrow keys remain equivalent alternate controls.
    float turn = 0.0f;
    if (Input::isDown(GLFW_KEY_A))     turn -= 1.0f;
    if (Input::isDown(GLFW_KEY_D))     turn += 1.0f;
    if (Input::isDown(GLFW_KEY_LEFT))  turn -= 1.0f;
    if (Input::isDown(GLFW_KEY_RIGHT)) turn += 1.0f;
    turn = std::max(-1.0f, std::min(1.0f, turn));
    yaw_ = wrapAngle(yaw_ + turn * KeyTurnSpeed * deltaTime);

    float pitchInput = 0.0f;
    if (Input::isDown(GLFW_KEY_UP))   pitchInput += 1.0f;
    if (Input::isDown(GLFW_KEY_DOWN)) pitchInput -= 1.0f;
    pitch_ = clampPitch(pitch_ + pitchInput * KeyTurnSpeed * deltaTime);

    // ---- Movement ----
    const float yawRad = yaw_ * Pi / 180.0f;
    // W/S use the horizontal projection of the exact heading used by the
    // view matrix. Looking up or down therefore never changes camera height.
    const float fwdX = std::cos(yawRad);
    const float fwdZ = std::sin(yawRad);

    float forward = 0.0f;
    float lift = 0.0f;

    if (Input::isDown(GLFW_KEY_W)) forward += 1.0f;
    if (Input::isDown(GLFW_KEY_S)) forward -= 1.0f;
    if (Input::isDown(GLFW_KEY_Q)) lift += 1.0f;   // up
    if (Input::isDown(GLFW_KEY_E)) lift -= 1.0f;   // down

    float dirX = fwdX * forward;
    float dirY = lift;
    float dirZ = fwdZ * forward;

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

    // Direct integration makes release immediate, while delta time keeps both
    // movement and turning independent of frame rate.
    position_[0] += dirX * speed * deltaTime;
    position_[1] += dirY * speed * deltaTime;
    position_[2] += dirZ * speed * deltaTime;

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
    if (!cursorCaptured_)
    {
        firstMouse_ = true;
        return;
    }

    if (firstMouse_)
    {
        firstMouse_ = false;
        return;
    }

    // Mouse and keyboard modify the same orientation, so switching between
    // them cannot snap or reset the heading.
    yaw_ = wrapAngle(yaw_ + static_cast<float>(dx) * MouseSensitivity);
    pitch_ = clampPitch(pitch_ + static_cast<float>(dy) * MouseSensitivity);
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
    loadView(yaw_, pitch_, position_, true);
}

void Camera::setPose(float x, float y, float z, float yaw, float pitch)
{
    position_[0] = x;
    position_[1] = y;
    position_[2] = z;
    yaw_ = wrapAngle(yaw);
    pitch_ = clampPitch(pitch);
}

void Camera::applySkyView() const
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    loadView(yaw_, pitch_, position_, false);
}
