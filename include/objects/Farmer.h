#pragma once

enum class FarmerRoute
{
    Road,
    Wander,
    FarmVisit,
    CropWork
};

class Farmer
{
public:
    Farmer(float x, float z, FarmerRoute route, float speed, float phase);

    void update(float deltaTime);
    void render() const;
    void setVisible(bool visible);
    void setRoadPosition(float z);

private:
    void chooseNextWanderTarget();
    void renderLeg(float x, float z, float swing) const;
    void renderArm(float x, float z, float swing) const;

    float x_;
    float z_;
    float heading_;
    float speed_;
    float phase_;
    float animationTime_ = 0.0f;
    float targetX_;
    float targetZ_;
    FarmerRoute route_;
    bool visible_ = true;
    bool workMovingForward_ = true;
};
