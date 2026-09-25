#pragma once

class Animation {
public:
    void update(float deltaTime);
    void togglePaused();
    void changeWindmillSpeed(float amount);

    float windmillAngle() const;
    float cloudOffset() const;
    float windmillSpeed() const;
    bool isPaused() const;

private:
    float windmillAngle_ = 0.0f;
    float cloudOffset_ = 0.0f;
    float windmillSpeed_ = 45.0f;
    bool paused_ = false;
};
