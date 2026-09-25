#include "objects/Tractor.h"

#include "Input.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace
{
constexpr float Pi = 3.14159265358979323846f;
constexpr float TractorSpeed = 5.0f;

// ------------------------------------------------------------
// Helper: draw a small colored cube
// ------------------------------------------------------------
void drawBox(
    float r, float g, float b,
    float x, float y, float z,
    float sx, float sy, float sz)
{
    glColor3f(r, g, b);

    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}
}

// ============================================================
// UPDATE
// ============================================================

void Tractor::update(float deltaTime)
{
    float moveX = 0.0f;
    float moveZ = 0.0f;

    // J = left
    if (Input::isDown(GLFW_KEY_J))
    {
        moveX -= 1.0f;
    }

    // L = right
    if (Input::isDown(GLFW_KEY_L))
    {
        moveX += 1.0f;
    }

    // I = forward
    if (Input::isDown(GLFW_KEY_I))
    {
        moveZ -= 1.0f;
    }

    // K = backward
    if (Input::isDown(GLFW_KEY_K))
    {
        moveZ += 1.0f;
    }

    const float moveLength =
        std::sqrt(moveX * moveX + moveZ * moveZ);

    if (moveLength <= 0.0f)
    {
        return;
    }

    const float distance = TractorSpeed * deltaTime;

    moveX = moveX / moveLength * distance;
    moveZ = moveZ / moveLength * distance;

    position_[0] += moveX;
    position_[2] += moveZ;

    // Keep tractor inside the farm
    position_[0] =
        std::fmax(-19.0f, std::fmin(19.0f, position_[0]));

    position_[2] =
        std::fmax(-15.5f, std::fmin(15.5f, position_[2]));

    // Wheel rotation
    const float wheelRadius = 0.85f;

    wheelRotation_ +=
        distance / wheelRadius * 180.0f / Pi;

    if (wheelRotation_ >= 360.0f)
    {
        wheelRotation_ -= 360.0f;
    }
}

// ============================================================
// MAIN TRACTOR
// ============================================================

void Tractor::drawTractor() const
{
    glPushMatrix();

    // Move entire tractor
    glTranslatef(
        position_[0],
        position_[1],
        position_[2]
    );

    // Main components
    drawBody();
    drawEngine();
    drawCabin();
    drawRoof();

    // Axles
    drawAxle(-1.45f);
    drawAxle(1.35f);

    // Wheels
    drawWheel(
        -2.15f, -1.45f,
        0.72f, 0.48f, false);

    drawWheel(
         2.15f, -1.45f,
        0.72f, 0.48f, true);

    drawWheel(
        -2.20f, 1.35f,
        0.92f, 0.55f, false);

    drawWheel(
         2.20f, 1.35f,
        0.92f, 0.55f, true);

    // Fender arches / guards
    drawFender(-2.20f, 1.35f, 0.92f, false);
    drawFender( 2.20f, 1.35f, 0.92f, true);

    drawFender(-2.15f, -1.45f, 0.72f, false);
    drawFender( 2.15f, -1.45f, 0.72f, true);

    // Front details
    drawHeadlight(-0.95f, -2.27f);
    drawHeadlight( 0.95f, -2.27f);

    drawBumper();

    // Driver area
    drawSeat();
    drawSteeringWheel();

    // Side steps
    drawStep(-1.65f);
    drawStep( 1.65f);

    // Exhaust
    drawExhaust();

    // Rear attachment
    drawRearHitch();

    glPopMatrix();
}

// ============================================================
// MAIN BODY
// ============================================================

void Tractor::drawBody() const
{
    // Main red chassis
    glColor3f(0.72f, 0.06f, 0.025f);

    glPushMatrix();
    glTranslatef(0.0f, 1.35f, 0.0f);
    glScalef(1.0f, 1.0f, 1.0f);

    Primitives::drawCube(
        4.2f,
        1.4f,
        4.0f
    );

    glPopMatrix();

    // Upper body
    glColor3f(0.92f, 0.20f, 0.025f);

    glPushMatrix();
    glTranslatef(
        0.0f,
        2.05f,
        0.35f
    );

    glScalef(
        0.95f,
        0.30f,
        0.82f
    );

    Primitives::drawCube(
        4.2f,
        1.0f,
        4.0f
    );

    glPopMatrix();

    // Lower black chassis strip
    drawBox(
        0.08f, 0.08f, 0.07f,
        0.0f, 0.65f, 0.0f,
        4.4f, 0.22f, 3.5f
    );

    // Side red panels
    drawBox(
        0.82f, 0.09f, 0.025f,
        -2.08f, 1.55f, 0.0f,
        0.12f, 1.0f, 3.1f
    );

    drawBox(
        0.82f, 0.09f, 0.025f,
         2.08f, 1.55f, 0.0f,
        0.12f, 1.0f, 3.1f
    );

    // Decorative side stripe
    drawBox(
        1.0f, 0.68f, 0.03f,
        -2.16f, 1.45f, 0.0f,
        0.05f, 0.18f, 2.6f
    );

    drawBox(
        1.0f, 0.68f, 0.03f,
         2.16f, 1.45f, 0.0f,
        0.05f, 0.18f, 2.6f
    );
}

// ============================================================
// ENGINE / HOOD
// ============================================================

void Tractor::drawEngine() const
{
    // Main hood
    glColor3f(
        0.88f,
        0.12f,
        0.025f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        2.45f,
        -1.35f
    );

    glScalef(
        0.82f,
        0.75f,
        0.85f
    );

    Primitives::drawCube(
        3.3f,
        1.5f,
        1.7f
    );

    glPopMatrix();

    // Hood top
    glColor3f(
        0.95f,
        0.18f,
        0.025f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        3.05f,
        -1.35f
    );

    glScalef(
        0.72f,
        0.12f,
        0.82f
    );

    Primitives::drawCube(
        3.4f,
        1.0f,
        1.7f
    );

    glPopMatrix();

    // Front grille
    glColor3f(
        0.08f,
        0.08f,
        0.07f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        2.42f,
        -2.22f
    );

    glScalef(
        0.78f,
        0.52f,
        0.10f
    );

    Primitives::drawCube(
        2.8f,
        1.0f,
        1.0f
    );

    glPopMatrix();

    // Grille bars
    for (int i = -2; i <= 2; ++i)
    {
        drawBox(
            0.45f,
            0.45f,
            0.42f,
            i * 0.35f,
            2.42f,
            -2.30f,
            0.08f,
            0.55f,
            0.04f
        );
    }
}

// ============================================================
// CABIN
// ============================================================

void Tractor::drawCabin() const
{
    // Dark cabin frame
    glColor3f(
        0.08f,
        0.13f,
        0.14f
    );

    // Front-left pillar
    drawBox(
        0.08f, 0.13f, 0.14f,
        -1.25f, 3.55f, 0.95f,
        0.18f, 1.5f, 0.18f
    );

    // Front-right pillar
    drawBox(
        0.08f, 0.13f, 0.14f,
         1.25f, 3.55f, 0.95f,
        0.18f, 1.5f, 0.18f
    );

    // Rear pillars
    drawBox(
        0.08f, 0.13f, 0.14f,
        -1.25f, 3.55f, 1.80f,
        0.18f, 1.5f, 0.18f
    );

    drawBox(
        0.08f, 0.13f, 0.14f,
         1.25f, 3.55f, 1.80f,
        0.18f, 1.5f, 0.18f
    );

    // Front windshield
    glColor3f(
        0.18f,
        0.55f,
        0.68f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        3.55f,
        1.50f
    );

    glScalef(
        1.0f,
        1.4f,
        0.08f
    );

    Primitives::drawCube(
        2.4f,
        2.0f,
        1.0f
    );

    glPopMatrix();

    // Rear window
    glColor3f(
        0.12f,
        0.40f,
        0.52f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        3.55f,
        2.02f
    );

    glScalef(
        1.0f,
        1.25f,
        0.06f
    );

    Primitives::drawCube(
        2.2f,
        2.0f,
        1.0f
    );

    glPopMatrix();

    // Lower cabin dashboard
    drawBox(
        0.10f,
        0.16f,
        0.16f,
        0.0f,
        3.25f,
        0.35f,
        2.5f,
        0.12f,
        0.12f
    );
}

// ============================================================
// ROOF
// ============================================================

void Tractor::drawRoof() const
{
    // Main roof
    glColor3f(
        0.08f,
        0.09f,
        0.08f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        4.55f,
        1.35f
    );

    glScalef(
        1.05f,
        0.18f,
        1.0f
    );

    Primitives::drawCube(
        3.2f,
        1.0f,
        2.8f
    );

    glPopMatrix();

    // Red roof edge
    drawBox(
        0.80f,
        0.07f,
        0.02f,
        0.0f,
        4.35f,
        0.15f,
        3.25f,
        0.10f,
        0.12f
    );
}

// ============================================================
// AXLES
// ============================================================

void Tractor::drawAxle(float z) const
{
    glColor3f(
        0.12f,
        0.12f,
        0.10f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        0.72f,
        z
    );

    glScalef(
        1.0f,
        0.20f,
        0.20f
    );

    Primitives::drawCube(
        4.8f,
        1.0f,
        1.0f
    );

    glPopMatrix();
}

// ============================================================
// WHEELS
// ============================================================

void Tractor::drawWheel(
    float x,
    float z,
    float radius,
    float width,
    bool rightSide) const
{
    // Tire
    glColor3f(
        0.025f,
        0.025f,
        0.022f
    );

    glPushMatrix();

    glTranslatef(
        x,
        radius,
        z
    );

    // Rotate wheel while tractor moves
    glRotatef(
        wheelRotation_,
        1.0f,
        0.0f,
        0.0f
    );

    // Put cylinder axis along X
    glRotatef(
        rightSide ? -90.0f : 90.0f,
        0.0f,
        0.0f,
        1.0f
    );

    Primitives::drawCylinder(
        radius,
        width,
        14
    );

    glPopMatrix();

    // Metal rim
    glColor3f(
        0.38f,
        0.39f,
        0.36f
    );

    glPushMatrix();

    glTranslatef(
        x,
        radius,
        z
    );

    glRotatef(
        rightSide ? -90.0f : 90.0f,
        0.0f,
        0.0f,
        1.0f
    );

    Primitives::drawCylinder(
        radius * 0.55f,
        width + 0.025f,
        14
    );

    glPopMatrix();

    // Central hub
    glColor3f(
        0.12f,
        0.13f,
        0.12f
    );

    glPushMatrix();

    glTranslatef(
        x,
        radius,
        z
    );

    glRotatef(
        rightSide ? -90.0f : 90.0f,
        0.0f,
        0.0f,
        1.0f
    );

    Primitives::drawCylinder(
        radius * 0.20f,
        width + 0.05f,
        10
    );

    glPopMatrix();
}

// ============================================================
// FENDERS
// ============================================================

void Tractor::drawFender(
    float x,
    float z,
    float radius,
    bool rightSide) const
{
    // Small fender positioned above the tire.
    // It visually connects the tire with the tractor body.

    glColor3f(
        0.75f,
        0.07f,
        0.025f
    );

    glPushMatrix();

    glTranslatef(
        x,
        radius * 2.0f + 0.15f,
        z
    );

    glScalef(
        0.18f,
        0.12f,
        radius * 1.75f
    );

    Primitives::drawCube(
        1.0f,
        1.0f,
        1.0f
    );

    glPopMatrix();

    (void)rightSide;
}

// ============================================================
// HEADLIGHTS
// ============================================================

void Tractor::drawHeadlight(float x, float z) const
{
    // Headlight housing
    glColor3f(
        0.12f,
        0.12f,
        0.10f
    );

    glPushMatrix();

    glTranslatef(
        x,
        2.80f,
        z
    );

    glScalef(
        0.38f,
        0.38f,
        0.12f
    );

    Primitives::drawCylinder(
        0.45f,
        0.20f,
        12
    );

    glPopMatrix();

    // Bright lamp
    glColor3f(
        1.0f,
        0.88f,
        0.35f
    );

    glPushMatrix();

    glTranslatef(
        x,
        2.80f,
        z - 0.12f
    );

    glScalef(
        0.75f,
        0.75f,
        0.20f
    );

    Primitives::drawCylinder(
        0.25f,
        0.10f,
        12
    );

    glPopMatrix();
}

// ============================================================
// EXHAUST
// ============================================================

void Tractor::drawExhaust() const
{
    // Exhaust pipe
    glColor3f(
        0.06f,
        0.06f,
        0.055f
    );

    glPushMatrix();

    glTranslatef(
        1.05f,
        3.45f,
        -1.25f
    );

    Primitives::drawCylinder(
        0.14f,
        1.65f,
        10
    );

    glPopMatrix();

    // Exhaust cap
    glColor3f(
        0.02f,
        0.02f,
        0.018f
    );

    glPushMatrix();

    glTranslatef(
        1.05f,
        4.28f,
        -1.25f
    );

    glScalef(
        1.2f,
        0.35f,
        1.2f
    );

    Primitives::drawCylinder(
        0.18f,
        0.15f,
        10
    );

    glPopMatrix();
}

// ============================================================
// FRONT BUMPER
// ============================================================

void Tractor::drawBumper() const
{
    glColor3f(
        0.12f,
        0.12f,
        0.10f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        1.65f,
        -2.38f
    );

    glScalef(
        1.15f,
        0.18f,
        0.18f
    );

    Primitives::drawCube(
        2.5f,
        1.0f,
        1.0f
    );

    glPopMatrix();
}

// ============================================================
// SIDE STEP
// ============================================================

void Tractor::drawStep(float x) const
{
    glColor3f(
        0.18f,
        0.18f,
        0.16f
    );

    glPushMatrix();

    glTranslatef(
        x,
        1.05f,
        0.25f
    );

    glScalef(
        0.55f,
        0.12f,
        0.75f
    );

    Primitives::drawCube(
        1.0f,
        1.0f,
        1.0f
    );

    glPopMatrix();
}

// ============================================================
// DRIVER SEAT
// ============================================================

void Tractor::drawSeat() const
{
    // Seat cushion
    glColor3f(
        0.05f,
        0.05f,
        0.045f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        3.05f,
        1.00f
    );

    glScalef(
        0.80f,
        0.16f,
        0.80f
    );

    Primitives::drawCube(
        1.0f,
        1.0f,
        1.0f
    );

    glPopMatrix();

    // Seat back
    glPushMatrix();

    glTranslatef(
        0.0f,
        3.55f,
        1.55f
    );

    glScalef(
        0.75f,
        0.65f,
        0.12f
    );

    Primitives::drawCube(
        1.0f,
        1.0f,
        1.0f
    );

    glPopMatrix();
}

// ============================================================
// STEERING WHEEL
// ============================================================

void Tractor::drawSteeringWheel() const
{
    glColor3f(
        0.04f,
        0.04f,
        0.035f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        3.65f,
        0.15f
    );

    // Tilt steering wheel toward driver
    glRotatef(
        65.0f,
        1.0f,
        0.0f,
        0.0f
    );

    glRotatef(
        90.0f,
        0.0f,
        1.0f,
        0.0f
    );

    Primitives::drawCylinder(
        0.38f,
        0.08f,
        16
    );

    glPopMatrix();

    // Steering column
    glColor3f(
        0.12f,
        0.12f,
        0.10f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        3.38f,
        0.25f
    );

    glRotatef(
        -20.0f,
        1.0f,
        0.0f,
        0.0f
    );

    Primitives::drawCylinder(
        0.07f,
        0.55f,
        8
    );

    glPopMatrix();
}

// ============================================================
// REAR HITCH
// ============================================================

void Tractor::drawRearHitch() const
{
    glColor3f(
        0.08f,
        0.08f,
        0.07f
    );

    // Horizontal hitch
    glPushMatrix();

    glTranslatef(
        0.0f,
        1.00f,
        2.35f
    );

    glScalef(
        0.35f,
        0.25f,
        0.75f
    );

    Primitives::drawCube(
        1.0f,
        1.0f,
        1.0f
    );

    glPopMatrix();

    // Hitch connector
    glPushMatrix();

    glTranslatef(
        0.0f,
        0.85f,
        2.85f
    );

    glScalef(
        0.18f,
        0.18f,
        0.45f
    );

    Primitives::drawCube(
        1.0f,
        1.0f,
        1.0f
    );

    glPopMatrix();
}