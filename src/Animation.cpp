#include "Animation.h"

#include <algorithm>

void Animation::update(float deltaTime)
{
    if (paused_)
    {
        return;
    }

    windmillAngle_ += windmillSpeed_ * deltaTime;
    cloudOffset_ += 0.6f * deltaTime;

    if (windmillAngle_ >= 360.0f)
    {
        windmillAngle_ -= 360.0f;
    }
    if (cloudOffset_ >= 100.0f)
    {
        cloudOffset_ -= 100.0f;
    }
}

void Animation::togglePaused()
{
    paused_ = !paused_;
}

void Animation::changeWindmillSpeed(float amount)
{
    windmillSpeed_ = std::clamp(windmillSpeed_ + amount, 0.0f, 360.0f);
}

float Animation::windmillAngle() const
{
    return windmillAngle_;
}

float Animation::cloudOffset() const
{
    return cloudOffset_;
}

float Animation::windmillSpeed() const
{
    return windmillSpeed_;
}

bool Animation::isPaused() const
{
    return paused_;
}
