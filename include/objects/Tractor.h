#pragma once

class Tractor
{
public:
    void update(float deltaTime);
    void drawTractor() const;

    void toggleHeadlights();
    bool isHeadlightsOn() const;

    float posX() const { return position_[0]; }
    float posY() const { return position_[1]; }
    float posZ() const { return position_[2]; }
    float heading() const { return heading_; }
    float steeringAngle() const { return steeringAngle_; }
    float speed() const { return speed_; }

private:
    void drawBody() const;
    void drawEngine() const;
    void drawCabin() const;
    void drawRoof() const;
    void drawAxle(float z) const;
    void drawWheel(float x, float z, float radius, float width, bool rightSide, bool isFront) const;
    void drawFender(float x, float z, float radius, bool rightSide) const;
    void drawHeadlight(float x, float z) const;
    void drawHeadlightBeams() const;
    void drawDust(float time) const;
    void drawExhaust() const;
    void drawBumper() const;
    void drawStep(float x) const;
    void drawSeat() const;
    void drawSteeringWheel() const;
    void drawRearHitch() const;

    float position_[3]{-4.0f, 0.0f, 13.0f};
    float heading_ = 0.0f;          // degrees around Y
    float steeringAngle_ = 0.0f;    // degrees for front wheel turning
    float wheelRotation_ = 0.0f;    // degrees around wheel axle
    float speed_ = 0.0f;            // current forward speed
    bool  headlightsOn_ = false;
};