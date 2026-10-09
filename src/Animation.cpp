#include "Animation.h"

#include "VillageSimulationSettings.h"

#include <cmath>

void Animation::update(float dt)
{
    const float targetSpeed = powerOn_ ? requestedWindmillSpeed_ : 0.0f;
    const float speedBlend = 1.0f - std::exp(
        -VillageSimulationSettings::WindmillPowerResponse * dt);
    windmillSpeed_ += (targetSpeed - windmillSpeed_) * speedBlend;

    if (paused_) return;

    windmillAngle_ += windmillSpeed_ * dt;
    if (windmillAngle_ >= 360.0f) windmillAngle_ -= 360.0f;
    if (windmillAngle_ <  0.0f)   windmillAngle_ += 360.0f;

    cloudOffset_ += 0.8f * dt;
    if (cloudOffset_ > 28.0f)
    {
        cloudOffset_ = -28.0f;
    }

    waterTime_ += dt;
    if (waterTime_ > 1000.0f)
    {
        waterTime_ -= 1000.0f;
    }
}

void Animation::changeWindmillSpeed(float delta)
{
    requestedWindmillSpeed_ += delta;
    if (requestedWindmillSpeed_ >  360.0f) requestedWindmillSpeed_ =  360.0f;
    if (requestedWindmillSpeed_ < -360.0f) requestedWindmillSpeed_ = -360.0f;
}
