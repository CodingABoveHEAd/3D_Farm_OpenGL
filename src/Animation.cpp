#include "Animation.h"

void Animation::update(float dt)
{
    if (paused_) return;

    windmillAngle_ += windmillSpeed_ * dt;
    if (windmillAngle_ >= 360.0f) windmillAngle_ -= 360.0f;
    if (windmillAngle_ <  0.0f)   windmillAngle_ += 360.0f;

    cloudOffset_ += 0.8f * dt;
    if (cloudOffset_ > 28.0f)
    {
        cloudOffset_ = -28.0f;
    }
}

void Animation::changeWindmillSpeed(float delta)
{
    windmillSpeed_ += delta;
    if (windmillSpeed_ >  360.0f) windmillSpeed_ =  360.0f;
    if (windmillSpeed_ < -360.0f) windmillSpeed_ = -360.0f;
}