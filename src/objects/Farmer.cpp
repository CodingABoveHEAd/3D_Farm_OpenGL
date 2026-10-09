#include "objects/Farmer.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace
{
constexpr float Pi = 3.14159265358979323846f;

struct RGB { float r, g, b; };

// Palette
constexpr RGB kSkin      {0.82f, 0.58f, 0.40f};
constexpr RGB kSkinDark  {0.72f, 0.48f, 0.32f};
constexpr RGB kDenim     {0.18f, 0.28f, 0.50f};
constexpr RGB kDenimDark {0.13f, 0.21f, 0.40f};
constexpr RGB kBoot      {0.20f, 0.12f, 0.07f};
constexpr RGB kSole      {0.08f, 0.06f, 0.05f};
constexpr RGB kStraw     {0.90f, 0.78f, 0.42f};
constexpr RGB kStrawDark {0.78f, 0.64f, 0.30f};
constexpr RGB kHatBand   {0.45f, 0.14f, 0.10f};
constexpr RGB kGold      {0.92f, 0.76f, 0.20f};
constexpr RGB kWhite     {0.96f, 0.95f, 0.92f};
constexpr RGB kBlack     {0.04f, 0.03f, 0.03f};
constexpr RGB kMouth     {0.45f, 0.20f, 0.18f};

float hash01(float v)
{
    const float s = std::sin(v * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

// Each farmer gets a different shirt, chosen from the phase value.
RGB shirtColor(float phase)
{
    static const RGB shirts[4] = {
        {0.72f, 0.20f, 0.17f},   // red
        {0.24f, 0.50f, 0.28f},   // green
        {0.85f, 0.65f, 0.20f},   // mustard
        {0.55f, 0.72f, 0.88f},   // light blue
    };
    return shirts[static_cast<int>(std::fabs(phase) * 10.0f) % 4];
}

RGB hairColor(float phase)
{
    static const RGB hair[3] = {
        {0.15f, 0.09f, 0.05f},   // dark brown
        {0.42f, 0.27f, 0.12f},   // brown
        {0.62f, 0.60f, 0.58f},   // grey
    };
    return hair[static_cast<int>(std::fabs(phase) * 7.0f) % 3];
}

RGB shade(const RGB& c, float f)
{
    auto cl = [](float v) { return v > 1.0f ? 1.0f : v; };
    return { cl(c.r * f), cl(c.g * f), cl(c.b * f) };
}

void drawBox(const RGB& c,
             float x, float y, float z,
             float sx, float sy, float sz)
{
    glColor3f(c.r, c.g, c.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void drawEllipsoid(const RGB& c,
                   float x, float y, float z,
                   float sx, float sy, float sz,
                   int slices = 14, int stacks = 10)
{
    glColor3f(c.r, c.g, c.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawSphere(1.0f, slices, stacks);
    glPopMatrix();
}

void drawHead(float phase)
{
    const RGB hair = hairColor(phase);
    const bool beard = hash01(phase + 3.0f) > 0.5f;

    // Neck
    drawEllipsoid(kSkinDark, 0.0f, 2.36f, 0.0f, 0.11f, 0.13f, 0.11f, 10, 8);

    // Head and ears
    drawEllipsoid(kSkin, 0.0f, 2.68f, 0.0f, 0.27f, 0.32f, 0.28f);
    drawEllipsoid(kSkinDark, -0.27f, 2.66f, 0.0f, 0.04f, 0.08f, 0.05f, 8, 6);
    drawEllipsoid(kSkinDark,  0.27f, 2.66f, 0.0f, 0.04f, 0.08f, 0.05f, 8, 6);

    // Hair at the back and sides, under the hat
    drawEllipsoid(hair, 0.0f, 2.76f, -0.07f, 0.285f, 0.27f, 0.25f);

    // Nose
    drawEllipsoid(kSkinDark, 0.0f, 2.64f, 0.28f, 0.045f, 0.06f, 0.06f, 8, 6);

    // Eyes and brows (face points toward +Z)
    for (float s : {-1.0f, 1.0f})
    {
        drawEllipsoid(kWhite, 0.10f * s, 2.72f, 0.245f, 0.05f, 0.04f, 0.03f, 8, 6);
        drawEllipsoid(kBlack, 0.10f * s, 2.72f, 0.272f, 0.022f, 0.025f, 0.012f, 6, 4);
        drawBox(shade(hair, 0.9f), 0.10f * s, 2.79f, 0.25f, 0.10f, 0.022f, 0.03f);
    }

    // Mouth, or a beard covering the lower face
    if (beard)
    {
        drawEllipsoid(hair, 0.0f, 2.50f, 0.15f, 0.22f, 0.15f, 0.17f);
        drawBox(kMouth, 0.0f, 2.55f, 0.30f, 0.09f, 0.018f, 0.02f);
    }
    else
    {
        drawBox(kMouth, 0.0f, 2.53f, 0.265f, 0.10f, 0.022f, 0.02f);
    }

    // Straw hat: wide brim, crown, band
    drawEllipsoid(kStrawDark, 0.0f, 2.94f, 0.0f, 0.64f, 0.045f, 0.64f, 20, 6);
    drawEllipsoid(kStraw,     0.0f, 2.97f, 0.0f, 0.62f, 0.040f, 0.62f, 20, 6);
    drawEllipsoid(kStraw,     0.0f, 3.08f, 0.0f, 0.31f, 0.22f, 0.31f, 16, 10);
    drawEllipsoid(kHatBand,   0.0f, 3.00f, 0.0f, 0.325f, 0.055f, 0.325f, 16, 4);
}
}

Farmer::Farmer(float x, float z, FarmerRoute route, float speed, float phase)
    : x_(x),
      z_(z),
      heading_(route == FarmerRoute::Road ? 0.0f : 90.0f),
      speed_(speed),
      phase_(phase),
      targetX_(x),
      targetZ_(z),
      route_(route)
{
    if (route_ == FarmerRoute::Wander)
    {
        chooseNextWanderTarget();
    }
    else if (route_ == FarmerRoute::FarmVisit)
    {
        targetX_ = x < 0.0f ? -23.0f : 23.0f;
    }
    else if (route_ == FarmerRoute::CropWork)
    {
        targetX_ = x;
        targetZ_ = z + 1.6f;
        heading_ = 0.0f;
    }
}

void Farmer::update(float deltaTime)
{
    animationTime_ += deltaTime;

    if (route_ == FarmerRoute::Road)
    {
        z_ += speed_ * deltaTime;
        if (z_ > 108.0f)
        {
            z_ = -108.0f;
        }

    }
    else if (route_ == FarmerRoute::CropWork)
    {
        const float dx = targetX_ - x_;
        const float dz = targetZ_ - z_;
        const float distance = std::sqrt(dx * dx + dz * dz);

        if (distance < 0.12f)
        {
            workMovingForward_ = !workMovingForward_;
            targetX_ = x_ + (workMovingForward_ ? 1.8f : -1.8f);
            targetZ_ = z_ + (workMovingForward_ ? 1.4f : -1.4f);
        }
        else
        {
            heading_ = std::atan2(dx, dz) * 180.0f / Pi;
            const float step = std::min(speed_ * deltaTime, distance);
            x_ += dx / distance * step;
            z_ += dz / distance * step;
        }
    }
    else
    {
        const float dx = targetX_ - x_;
        const float dz = targetZ_ - z_;
        const float distance = std::sqrt(dx * dx + dz * dz);

        if (distance < 0.2f)
        {
            if (route_ == FarmerRoute::Wander)
            {
                chooseNextWanderTarget();
            }
            else
            {
                // Farm visitors alternate between the road and a farm entrance.
                const bool atFarmSide = std::fabs(x_) > 12.0f;
                targetX_ = atFarmSide
                    ? (x_ < 0.0f ? -4.0f : 4.0f)
                    : (x_ < 0.0f ? -23.0f : 23.0f);
                targetZ_ = z_;
            }
        }
        else
        {
            heading_ = std::atan2(dx, dz) * 180.0f / Pi;
            const float step = std::min(speed_ * deltaTime, distance);
            x_ += dx / distance * step;
            z_ += dz / distance * step;
        }
    }
}

void Farmer::setVisible(bool visible)
{
    visible_ = visible;
}

void Farmer::setRoadPosition(float z)
{
    // Pedestrians use the grass shoulders, leaving the vehicle lane clear.
    x_ = x_ < 0.0f ? -8.5f : 8.5f;
    z_ = z;
    heading_ = 0.0f;
}

void Farmer::chooseNextWanderTarget()
{
    // Offset targets keep wanderers in the open grass and out of structures.
    const float direction = std::sin(animationTime_ * 1.7f + phase_);
    const float distance = 5.0f + 3.0f * std::cos(animationTime_ * 0.9f + phase_);
    targetX_ = x_ + direction * distance;
    targetZ_ = z_ + std::cos(animationTime_ * 1.3f + phase_) * distance;

    if (targetX_ < -68.0f) targetX_ = -68.0f;
    if (targetX_ > 68.0f) targetX_ = 68.0f;
    if (targetZ_ < -70.0f) targetZ_ = -70.0f;
    if (targetZ_ > 70.0f) targetZ_ = 70.0f;
}

// Drop-in replacement for Farmer::render() in Farmer.cpp.
// Everything else in the file stays exactly as it is.
// Only the CropWork (`working`) behaviour changes; other routes render as before.

void Farmer::render() const
{
    if (!visible_)
    {
        return;
    }

    const bool working = route_ == FarmerRoute::CropWork;
    const float cycle = animationTime_ * (working ? 3.2f : 7.0f) + phase_;
    const float swing = std::sin(cycle) * 28.0f;       // arms (degrees)
    const float legSwing = std::sin(cycle) * 28.0f;    // legs (degrees)
    const float bob = std::fabs(std::sin(cycle)) * 0.04f;
    const float sway = std::sin(cycle) * (working ? 5.0f : 2.5f);

    // Positive X rotation tips the body toward +Z (the direction the farmer
    // faces), so a positive angle is a forward bend from the hips.
    const float workBend = working
        ? 48.0f + std::sin(cycle * 0.5f) * 6.0f
        : 0.0f;
    const float workArmMotion = std::sin(cycle) * 18.0f;        // picking / pulling
    const float handSweep = std::sin(cycle * 0.5f) * 10.0f;     // gathering sideways

    // Head: counter-rotate against the bend so the face looks down at the
    // crop row instead of straight at the ground, plus a nod and a look-around.
    const float headPitch = working
        ? -workBend * 0.6f + std::sin(cycle * 0.5f + 1.2f) * 8.0f
        : 0.0f;
    const float headYaw = working ? std::sin(cycle * 0.35f) * 20.0f : 0.0f;

    const RGB shirt = shirtColor(phase_);

    glPushMatrix();
    glTranslatef(x_, 0.0f, z_);
    glRotatef(heading_, 0.0f, 1.0f, 0.0f);
    glScalef(0.90f, 0.90f, 0.90f);

    // Legs: normal stride when walking, a small shuffle while working the rows.
    renderLeg(-0.20f, 0.0f, working ? legSwing * 0.3f : legSwing);
    renderLeg( 0.20f, 0.0f, working ? -legSwing * 0.3f : -legSwing);

    glPushMatrix();
    glTranslatef(0.0f, bob, 0.0f);
    glRotatef(sway, 0.0f, 1.0f, 0.0f);
    if (working)
    {
        // Bend the upper body forward from the hips so the farmer reaches
        // down into the crop rows.
        glTranslatef(0.0f, 1.32f, 0.0f);
        glRotatef(workBend, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, -1.32f, 0.0f);
    }

    // Pelvis and belt
    drawEllipsoid(kDenim, 0.0f, 1.32f, 0.0f, 0.40f, 0.22f, 0.26f);
    drawBox(kBoot, 0.0f, 1.50f, 0.0f, 0.84f, 0.07f, 0.52f);
    drawBox(kGold, 0.0f, 1.50f, 0.265f, 0.10f, 0.08f, 0.02f);

    // Shirt torso (broader at the chest)
    drawEllipsoid(shirt, 0.0f, 1.90f, 0.0f, 0.44f, 0.52f, 0.27f);
    drawEllipsoid(shirt, 0.0f, 2.15f, 0.0f, 0.50f, 0.26f, 0.27f);
    drawEllipsoid(shade(shirt, 0.85f), 0.0f, 2.30f, 0.0f, 0.24f, 0.10f, 0.20f, 10, 6);   // collar

    // Overalls: bib, straps, buckles, pocket
    drawBox(kDenim, 0.0f, 1.80f, 0.255f, 0.52f, 0.50f, 0.05f);
    drawBox(kDenimDark, 0.0f, 1.74f, 0.285f, 0.22f, 0.16f, 0.02f);     // bib pocket
    for (float s : {-1.0f, 1.0f})
    {
        drawBox(kDenim, 0.19f * s, 2.14f, 0.235f, 0.08f, 0.52f, 0.05f);   // front strap
        drawBox(kDenim, 0.19f * s, 2.34f, 0.00f,  0.08f, 0.05f, 0.46f);   // over shoulder
        drawBox(kDenim, 0.19f * s, 2.10f, -0.245f, 0.08f, 0.60f, 0.05f);  // back strap
        drawBox(kGold,  0.19f * s, 2.04f, 0.272f, 0.06f, 0.06f, 0.02f);   // buckle
    }
    drawBox(kDenim, 0.0f, 1.80f, -0.255f, 0.52f, 0.50f, 0.05f);        // back panel

    // Head pivots at the neck.
    glPushMatrix();
    glTranslatef(0.0f, 2.36f, 0.0f);
    glRotatef(headYaw, 0.0f, 1.0f, 0.0f);
    glRotatef(headPitch, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -2.36f, 0.0f);
    drawHead(phase_);
    glPopMatrix();

    if (working)
    {
        // The upper body is already bent, so arm angles are relative to the
        // torso. Subtracting the bend keeps the arms hanging down into the
        // crops; the alternating motion is the picking/pulling action and the
        // shoulder yaw sweeps the hands inward as if gathering plants.
        const float armBase = -workBend - 10.0f;

        glPushMatrix();
        glTranslatef(-0.54f, 2.18f, 0.0f);
        glRotatef(handSweep, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.54f, -2.18f, 0.0f);
        renderArm(-0.54f, 2.18f, armBase - workArmMotion);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.54f, 2.18f, 0.0f);
        glRotatef(-handSweep, 0.0f, 1.0f, 0.0f);
        glTranslatef(-0.54f, -2.18f, 0.0f);
        renderArm( 0.54f, 2.18f, armBase + workArmMotion);
        glPopMatrix();
    }
    else
    {
        renderArm(-0.54f, 2.18f, -swing);
        renderArm( 0.54f, 2.18f,  swing);
    }

    glPopMatrix();
    glPopMatrix();
}

// Leg: hip pivot, thigh, knee bend, shin, boot.
void Farmer::renderLeg(float x, float z, float swing) const
{
    const float knee = swing > 0.0f ? swing * 0.9f : 0.0f;

    glPushMatrix();
    glTranslatef(x, 1.30f, z);
    glRotatef(swing, 1.0f, 0.0f, 0.0f);

    // Thigh
    drawEllipsoid(kDenim, 0.0f, -0.30f, 0.0f, 0.17f, 0.37f, 0.19f);

    // Shin, boot and sole hang from the knee
    glTranslatef(0.0f, -0.62f, 0.0f);
    glRotatef(knee, 1.0f, 0.0f, 0.0f);
    drawEllipsoid(kDenimDark, 0.0f, -0.28f, 0.0f, 0.145f, 0.34f, 0.155f);
    drawBox(kBoot, 0.0f, -0.58f, 0.07f, 0.26f, 0.17f, 0.42f);
    drawBox(kSole, 0.0f, -0.665f, 0.08f, 0.28f, 0.035f, 0.44f);

    glPopMatrix();
}

// Arm: shoulder pivot (x, y), upper arm, elbow bend, forearm, hand.
// Note: the second parameter is the shoulder height.
void Farmer::renderArm(float x, float y, float swing) const
{
    const RGB shirt = shirtColor(phase_);
    const float elbow = -(15.0f + 0.6f * (swing > 0.0f ? swing : 0.0f));

    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glRotatef(swing, 1.0f, 0.0f, 0.0f);

    // Shoulder cap and upper arm
    drawEllipsoid(shirt, 0.0f, -0.02f, 0.0f, 0.15f, 0.14f, 0.15f, 10, 8);
    drawEllipsoid(shirt, 0.0f, -0.26f, 0.0f, 0.115f, 0.30f, 0.115f);

    // Forearm hangs from the elbow
    glTranslatef(0.0f, -0.52f, 0.0f);
    glRotatef(elbow, 1.0f, 0.0f, 0.0f);
    drawEllipsoid(shade(shirt, 0.9f), 0.0f, -0.04f, 0.0f, 0.125f, 0.08f, 0.125f, 10, 6);   // rolled cuff
    drawEllipsoid(kSkin, 0.0f, -0.24f, 0.0f, 0.085f, 0.24f, 0.085f, 10, 8);
    drawEllipsoid(kSkin, 0.0f, -0.50f, 0.0f, 0.095f, 0.10f, 0.075f, 10, 8);               // hand

    glPopMatrix();
}
