#pragma once

class Tractor
{
public:
    void update(float deltaTime);
    void drawTractor() const;

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

    float position_[3]{-4.0f, 0.0f, 13.0f};
    float wheelRotation_ = 0.0f;
};