#pragma once

class Animation {
public:
    void update(float dt);

    float windmillAngle() const { return windmillAngle_; }
    float cloudOffset() const { return cloudOffset_; }

    void changeWindmillSpeed(float delta);
    void togglePaused() { paused_ = !paused_; }
    bool isPaused() const { return paused_; }

private:
    float windmillAngle_ = 0.0f;     // degrees, accumulated
    float windmillSpeed_ = 45.0f;    // degrees per second
    float cloudOffset_ = -28.0f;     // world X position offset
    bool  paused_        = false;
};