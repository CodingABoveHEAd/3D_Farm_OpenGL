#pragma once

struct GLFWwindow;

enum class CameraMode {
    Free = 0,     // F1: Manual WASD + Mouse exploration
    Overview,     // F2: High-altitude world orbit
    FarmFocus,    // F3: Cinematic orbit of nearest farm
    TractorFollow // F4: 3rd-person chase camera behind tractor
};

class Camera {
public:
    void reset();
    void update(GLFWwindow* window, float deltaTime);
    void onMouseMove(double xPosition, double yPosition);
    void applyProjection(int width, int height) const;
    void applyView() const;

    float posX() const;
    float posY() const;
    float posZ() const;
    float yawDegrees() const;
    float pitchDegrees() const;
    void setPose(float x, float y, float z, float yaw, float pitch);

    void setMode(CameraMode mode);
    CameraMode mode() const;
    void setTractorPose(float x, float y, float z, float heading);
    void setFocusFarm(float x, float z);

private:
    float position_[3]{0.0f, 4.0f, 10.0f};
    float yaw_ = -90.0f;
    float pitch_ = -15.0f;
    float fieldOfView_ = 45.0f;
    bool firstMouse_ = true;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;

    CameraMode mode_ = CameraMode::Free;
    float orbitAngle_ = 0.0f;
    float tractorPos_[3]{-4.0f, 0.0f, 13.0f};
    float tractorHeading_ = 0.0f;
    float farmFocusPos_[2]{0.0f, 0.0f};
};
