#pragma once

class Animation {
public:
    void update(float dt);

    float windmillAngle() const { return windmillAngle_; }
    float cloudOffset() const { return cloudOffset_; }
    float waterTime() const { return waterTime_; }

    void changeWindmillSpeed(float delta);
    void setPowerOn(bool powerOn) { powerOn_ = powerOn; }
    void togglePaused() { paused_ = !paused_; }
    bool isPaused() const { return paused_; }

private:
    float windmillAngle_ = 0.0f;     // degrees, accumulated
    float requestedWindmillSpeed_ = 28.0f;
    float windmillSpeed_ = 28.0f;    // smoothly approaches requested or zero
    float cloudOffset_ = -28.0f;     // world X position offset
    float waterTime_ = 0.0f;         // seconds used by animated pond surfaces
    bool  paused_        = false;
    bool  powerOn_       = true;
};
