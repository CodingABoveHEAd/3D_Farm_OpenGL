#pragma once

struct GLFWwindow;

class Camera {
public:
    void reset();
    void update(GLFWwindow* window, float deltaTime);
    void onMouseMove(double xPosition, double yPosition);
    void applyProjection(int width, int height) const;
    void applyView() const;

private:
    float position_[3]{0.0f, 4.0f, 10.0f};
    float yaw_ = -90.0f;
    float pitch_ = -15.0f;
    float fieldOfView_ = 45.0f;
    bool firstMouse_ = true;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
};
