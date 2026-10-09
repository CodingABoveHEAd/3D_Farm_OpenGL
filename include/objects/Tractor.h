#pragma once

class Tractor
{
public:
    enum class Color
    {
        Red,
        Green,
        Blue,
        Brown
    };

    void update(float deltaTime);
    void drawTractor() const;
    void updateRoad(float deltaTime, float& z, float& wheelRotation);
    void drawRoadTractor(float z, float wheelRotation, Color color,
                         float animationTime, bool detailedDriver) const;
    void setNightAmount(float amount) { nightAmount_ = amount; }

private:
    void drawBody() const;
    void drawEngine() const;
    void drawCabin() const;
    void drawRoof() const;
    void drawAxle(float z) const;
    void drawWheel(float x, float z, float radius, float width, bool rightSide) const;
    void drawFender(float x, float z, float radius, bool rightSide) const;
    void drawHeadlight(float x, float z) const;
    void drawExhaust() const;
    void drawBumper() const;
    void drawStep(float x) const;
    void drawSeat() const;
    void drawSteeringWheel() const;
    void drawRearHitch() const;
    void setBodyColor(float red, float green, float blue) const;

    mutable float position_[3]{-4.0f, 0.0f, 13.0f};
    mutable float heading_ = 0.0f;
    mutable float wheelRotation_ = 0.0f;
    mutable Color displayColor_ = Color::Red;
    mutable float roadDriverTime_ = -1.0f;
    mutable bool detailedRoadDriver_ = true;
    float nightAmount_ = 0.0f;
};
