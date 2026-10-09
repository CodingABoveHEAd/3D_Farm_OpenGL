#pragma once

class Animation {
public:
    void update(float dt);

    float windmillAngle() const { return windmillAngle_; }
    float cloudOffset() const { return cloudOffset_; }
    float waterTime() const { return waterTime_; }

    void changeWindmillSpeed(float delta);
    void togglePaused() { paused_ = !paused_; }
    bool isPaused() const { return paused_; }

private:
    float windmillAngle_ = 0.0f;     // degrees, accumulated
    float windmillSpeed_ = 28.0f;    // natural, frame-rate-independent speed
    float cloudOffset_ = -28.0f;     // world X position offset
    float waterTime_ = 0.0f;         // seconds used by animated pond surfaces
    bool  paused_        = false;
};
