#pragma once

struct GLFWwindow;

class Camera {
public:
    void reset();
    void update(GLFWwindow* window, float deltaTime);
    void onMouseMove(double xPosition, double yPosition);
    void applyProjection(int width, int height) const;
    void applyView() const;
    void applySkyView() const;
    void setMouseLook(GLFWwindow* window, bool enabled);
    bool isMouseCaptured() const { return cursorCaptured_; }
    float posX() const { return position_[0]; }
    float posY() const { return position_[1]; }
    float posZ() const { return position_[2]; }
    float yawDegrees() const { return yaw_; }
    float pitchDegrees() const { return pitch_; }
    void setPose(float x, float y, float z, float yaw, float pitch);

private:
    float position_[3]{0.0f, 4.0f, 10.0f};
    float yaw_ = -90.0f;
    float pitch_ = -15.0f;
    float fieldOfView_ = 45.0f;
    bool initialized_ = false;
    bool cursorCaptured_ = false;
    bool previousLeftMouse_ = false;
    bool firstMouse_ = true;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
};
