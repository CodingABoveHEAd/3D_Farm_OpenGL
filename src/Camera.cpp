#include "Camera.h"

#include "Input.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265358979323846f;
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

void Camera::update(GLFWwindow*, float deltaTime)
{
    const float speed = 5.0f * deltaTime;

    const float yawRadians = yaw_ * Pi / 180.0f;
    const float forwardX = std::cos(yawRadians);
    const float forwardZ = std::sin(yawRadians);

    if (Input::isDown(GLFW_KEY_W))
    {
        position_[0] += forwardX * speed;
        position_[2] += forwardZ * speed;
    }
    if (Input::isDown(GLFW_KEY_S))
    {
        position_[0] -= forwardX * speed;
        position_[2] -= forwardZ * speed;
    }
    if (Input::isDown(GLFW_KEY_A))
    {
        position_[0] += forwardZ * speed;
        position_[2] -= forwardX * speed;
    }
    if (Input::isDown(GLFW_KEY_D))
    {
        position_[0] -= forwardZ * speed;
        position_[2] += forwardX * speed;
    }
    if (Input::isDown(GLFW_KEY_Q))
    {
        position_[1] += speed;
    }
    if (Input::isDown(GLFW_KEY_E))
    {
        position_[1] -= speed;
    }

    if (Input::isDown(GLFW_KEY_LEFT))
    {
        yaw_ -= 60.0f * deltaTime;
    }
    if (Input::isDown(GLFW_KEY_RIGHT))
    {
        yaw_ += 60.0f * deltaTime;
    }
    if (Input::isDown(GLFW_KEY_UP))
    {
        pitch_ += 60.0f * deltaTime;
    }
    if (Input::isDown(GLFW_KEY_DOWN))
    {
        pitch_ -= 60.0f * deltaTime;
    }

    if (pitch_ > 89.0f)
    {
        pitch_ = 89.0f;
    }
    if (pitch_ < -89.0f)
    {
        pitch_ = -89.0f;
    }

    if (Input::wasPressed(GLFW_KEY_R))
    {
        reset();
    }
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

    const float sensitivity = 0.10f;
    const float xOffset = static_cast<float>(xPosition - lastMouseX_) * sensitivity;
    const float yOffset = static_cast<float>(lastMouseY_ - yPosition) * sensitivity;
    lastMouseX_ = xPosition;
    lastMouseY_ = yPosition;

    yaw_ += xOffset;
    pitch_ += yOffset;
    if (pitch_ > 89.0f)
    {
        pitch_ = 89.0f;
    }
    if (pitch_ < -89.0f)
    {
        pitch_ = -89.0f;
    }
}

void Camera::applyProjection(int width, int height) const
{
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 200.0f;
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
