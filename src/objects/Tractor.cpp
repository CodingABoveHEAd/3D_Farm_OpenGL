#include "objects/Tractor.h"

#include "Input.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

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

// ------------------------------------------------------------
// Helper: cylinder standing upright (axis = Y)
// ------------------------------------------------------------
void drawCylY(
    float r, float g, float b,
    float x, float y, float z,
    float radius, float height, int segments)
{
    glColor3f(r, g, b);

    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCylinder(radius, height, segments);
    glPopMatrix();
}

// ------------------------------------------------------------
// Helper: cylinder lying along Z (front-to-back)
// ------------------------------------------------------------
void drawCylZ(
    float r, float g, float b,
    float x, float y, float z,
    float radius, float length, int segments)
{
    glColor3f(r, g, b);

    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    Primitives::drawCylinder(radius, length, segments);
    glPopMatrix();
}

// ------------------------------------------------------------
// Helper: translucent glass pane (drawn last so blending works)
// ------------------------------------------------------------
void drawGlassPane(
    float x, float y, float z,
    float sx, float sy, float sz)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glColor4f(0.35f, 0.72f, 0.85f, 0.32f);

    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawCabinGlass()
{
    // Windshield (front of the cab)
    drawGlassPane(0.0f, 3.55f, 0.95f, 2.30f, 1.40f, 0.04f);

    // Rear window
    drawGlassPane(0.0f, 3.55f, 1.80f, 2.30f, 1.40f, 0.04f);

    // Side windows
    drawGlassPane(-1.25f, 3.55f, 1.375f, 0.04f, 1.40f, 0.66f);
    drawGlassPane( 1.25f, 3.55f, 1.375f, 0.04f, 1.40f, 0.66f);
}
}

// ============================================================
// UPDATE
// ============================================================

void Tractor::toggleHeadlights()
{
    headlightsOn_ = !headlightsOn_;
}

bool Tractor::isHeadlightsOn() const
{
    return headlightsOn_;
}

void Tractor::update(float deltaTime)
{
    // Toggle headlights with L
    if (Input::wasPressed(GLFW_KEY_L))
    {
        toggleHeadlights();
    }

    // Steering input (Left / Right arrows, or A / D, or J / L)
    float steerTarget = 0.0f;
    if (Input::isDown(GLFW_KEY_LEFT) || Input::isDown(GLFW_KEY_A) || Input::isDown(GLFW_KEY_J))
    {
        steerTarget += 32.0f;
    }
    if (Input::isDown(GLFW_KEY_RIGHT) || Input::isDown(GLFW_KEY_D))
    {
        steerTarget -= 32.0f;
    }

    // Smooth steering response
    const float steerBlend = 1.0f - std::exp(-8.0f * deltaTime);
    steeringAngle_ += (steerTarget - steeringAngle_) * steerBlend;

    // Throttle / Reverse input (Up / Down arrows, or W / S, or I / K)
    float targetSpeed = 0.0f;
    if (Input::isDown(GLFW_KEY_UP) || Input::isDown(GLFW_KEY_I))
    {
        targetSpeed += TractorSpeed;
    }
    if (Input::isDown(GLFW_KEY_DOWN) || Input::isDown(GLFW_KEY_K))
    {
        targetSpeed -= TractorSpeed * 0.65f;
    }

    // Smooth speed response
    const float accelBlend = 1.0f - std::exp(-6.0f * deltaTime);
    speed_ += (targetSpeed - speed_) * accelBlend;

    if (std::abs(speed_) > 0.01f)
    {
        // When moving, heading rotates based on steering angle
        const float turnRate = (steeringAngle_ / 32.0f) * (speed_ / TractorSpeed) * 45.0f;
        heading_ += turnRate * deltaTime;

        // Keep heading in [0, 360)
        while (heading_ >= 360.0f) heading_ -= 360.0f;
        while (heading_ < 0.0f) heading_ += 360.0f;

        // Move forward along heading vector (forward is -Z in local tractor space)
        const float rad = heading_ * Pi / 180.0f;
        const float dx = -std::sin(rad) * speed_ * deltaTime;
        const float dz = -std::cos(rad) * speed_ * deltaTime;

        position_[0] += dx;
        position_[2] += dz;

        // Soft bounds clamp allowing yard and road driving
        position_[0] = std::fmax(-35.0f, std::fmin(35.0f, position_[0]));
        position_[2] = std::fmax(-30.0f, std::fmin(30.0f, position_[2]));

        // Wheel rotation proportional to distance traveled
        const float distance = speed_ * deltaTime;
        const float wheelRadius = 0.85f;
        wheelRotation_ += (distance / wheelRadius) * (180.0f / Pi);

        while (wheelRotation_ >= 360.0f) wheelRotation_ -= 360.0f;
        while (wheelRotation_ < 0.0f) wheelRotation_ += 360.0f;
    }
}

// ============================================================
// HEADLIGHT BEAMS & DUST
// ============================================================

void Tractor::drawHeadlightBeams() const
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending for volumetric glow
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    for (float hx : {-0.95f, 0.95f})
    {
        glPushMatrix();
        glTranslatef(hx, 1.25f, -2.35f);

        // Warm bright yellow-white light cone
        glColor4f(1.0f, 0.95f, 0.65f, 0.14f);

        constexpr int segs = 14;
        constexpr float length = 14.0f;
        constexpr float endRadius = 3.6f;

        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(0.0f, 0.0f, 0.0f); // Apex at headlight lens
        for (int i = 0; i <= segs; ++i)
        {
            const float angle = static_cast<float>(i) * 2.0f * Pi / segs;
            const float cx = std::cos(angle) * endRadius;
            const float cy = std::sin(angle) * endRadius;
            glVertex3f(cx, cy - 0.5f, -length);
        }
        glEnd();

        glPopMatrix();
    }

    glPopAttrib();
}

void Tractor::drawDust(float time) const
{
    if (std::abs(speed_) < 0.2f) return;

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    // Dust puffs behind rear wheels
    for (int i = 0; i < 6; ++i)
    {
        const float phase = std::fmod(time * 3.5f + static_cast<float>(i) * 0.45f, 1.0f);
        const float size = 0.25f + phase * 0.85f;
        const float alpha = (1.0f - phase) * 0.28f;
        const float pz = 1.9f + phase * 2.8f;
        const float py = 0.25f + phase * 0.6f;

        for (float px : {-1.9f, 1.9f})
        {
            glColor4f(0.68f, 0.58f, 0.40f, alpha);
            glPushMatrix();
            glTranslatef(px + (i % 2 == 0 ? 0.2f : -0.2f), py, pz);
            glScalef(size, size * 0.75f, size);
            Primitives::drawCube(1.0f, 1.0f, 1.0f);
            glPopMatrix();
        }
    }

    glPopAttrib();
}

// ============================================================
// MAIN TRACTOR
// ============================================================

void Tractor::drawTractor() const
{
    glPushMatrix();

    // Move entire tractor and rotate by heading
    glTranslatef(position_[0], position_[1], position_[2]);
    glRotatef(heading_, 0.0f, 1.0f, 0.0f);

    // Main components
    drawBody();
    drawEngine();
    drawCabin();
    drawRoof();

    // Axles
    drawAxle(-1.45f);
    drawAxle(1.35f);

    // Wheels: front wheels turn with steeringAngle_
    drawWheel(-2.15f, -1.45f, 0.72f, 0.48f, false, true);
    drawWheel( 2.15f, -1.45f, 0.72f, 0.48f, true,  true);

    // Rear wheels: fixed steering
    drawWheel(-2.20f,  1.35f, 0.92f, 0.55f, false, false);
    drawWheel( 2.20f,  1.35f, 0.92f, 0.55f, true,  false);

    // Fender arches / guards
    drawFender(-2.20f, 1.35f, 0.92f, false);
    drawFender( 2.20f, 1.35f, 0.92f, true);

    drawFender(-2.15f, -1.45f, 0.72f, false);
    drawFender( 2.15f, -1.45f, 0.72f, true);

    // Front details
    drawHeadlight(-0.95f, -2.27f);
    drawHeadlight( 0.95f, -2.27f);

    if (headlightsOn_)
    {
        drawHeadlightBeams();
    }

    drawDust(static_cast<float>(glfwGetTime()));

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

    // Translucent glass goes last so blending looks right
    drawCabinGlass();

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

    // ---- Extra detail ----

    // Chrome accent line above the stripe
    drawBox(
        0.75f, 0.75f, 0.72f,
        -2.19f, 1.68f, 0.0f,
        0.03f, 0.03f, 2.6f
    );

    drawBox(
        0.75f, 0.75f, 0.72f,
         2.19f, 1.68f, 0.0f,
        0.03f, 0.03f, 2.6f
    );

    // Black rubber skirt along the lower edge
    drawBox(
        0.05f, 0.05f, 0.05f,
        -2.12f, 0.98f, 0.0f,
        0.06f, 0.22f, 3.2f
    );

    drawBox(
        0.05f, 0.05f, 0.05f,
         2.12f, 0.98f, 0.0f,
        0.06f, 0.22f, 3.2f
    );

    // Model plates on the side panels
    drawBox(
        0.90f, 0.75f, 0.10f,
        -2.20f, 1.88f, -0.90f,
        0.03f, 0.14f, 0.55f
    );

    drawBox(
        0.90f, 0.75f, 0.10f,
         2.20f, 1.88f, -0.90f,
        0.03f, 0.14f, 0.55f
    );

    // Fuel filler cap
    drawCylY(
        0.35f, 0.35f, 0.33f,
        -1.30f, 2.24f, -0.15f,
        0.16f, 0.06f, 12
    );

    drawCylY(
        0.08f, 0.08f, 0.07f,
        -1.30f, 2.28f, -0.15f,
        0.09f, 0.03f, 12
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

    // ---- Extra detail ----

    // Filler block so the grille connects to the hood
    drawBox(
        0.08f, 0.08f, 0.07f,
        0.0f, 2.42f, -2.12f,
        2.18f, 0.52f, 0.12f
    );

    // Chrome grille frame
    drawBox(
        0.78f, 0.78f, 0.75f,
        0.0f, 2.71f, -2.29f,
        2.34f, 0.05f, 0.07f
    );

    drawBox(
        0.78f, 0.78f, 0.75f,
        0.0f, 2.13f, -2.29f,
        2.34f, 0.05f, 0.07f
    );

    drawBox(
        0.78f, 0.78f, 0.75f,
        -1.14f, 2.42f, -2.29f,
        0.05f, 0.63f, 0.07f
    );

    drawBox(
        0.78f, 0.78f, 0.75f,
         1.14f, 2.42f, -2.29f,
        0.05f, 0.63f, 0.07f
    );

    // Brand badge above the grille
    drawBox(
        0.95f, 0.78f, 0.12f,
        0.0f, 2.86f, -2.30f,
        0.42f, 0.14f, 0.04f
    );

    // Black center stripe running along the hood top
    drawBox(
        0.06f, 0.06f, 0.05f,
        0.0f, 3.12f, -1.35f,
        0.30f, 0.03f, 1.35f
    );

    // Side vent slats on both sides of the hood
    for (int i = 0; i < 5; ++i)
    {
        const float y = 2.30f + i * 0.12f;

        drawBox(
            0.06f, 0.06f, 0.05f,
            -1.37f, y, -1.35f,
            0.03f, 0.05f, 1.0f
        );

        drawBox(
            0.06f, 0.06f, 0.05f,
             1.37f, y, -1.35f,
            0.03f, 0.05f, 1.0f
        );
    }

    // Air pre-cleaner (opposite the exhaust)
    drawCylY(
        0.40f, 0.41f, 0.39f,
        -1.05f, 3.60f, -1.25f,
        0.17f, 0.95f, 12
    );

    drawCylY(
        0.06f, 0.06f, 0.05f,
        -1.05f, 4.10f, -1.25f,
        0.21f, 0.10f, 12
    );

    drawCylY(
        0.75f, 0.75f, 0.72f,
        -1.05f, 3.20f, -1.25f,
        0.20f, 0.06f, 12
    );
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

    // Windshield and rear window are drawn as translucent
    // glass at the end of drawTractor().

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

    // ---- Extra detail ----

    // Top frame rails
    drawBox(
        0.08f, 0.13f, 0.14f,
        0.0f, 4.30f, 0.95f,
        2.68f, 0.14f, 0.14f
    );

    drawBox(
        0.08f, 0.13f, 0.14f,
        0.0f, 4.30f, 1.80f,
        2.68f, 0.14f, 0.14f
    );

    drawBox(
        0.08f, 0.13f, 0.14f,
        -1.25f, 4.30f, 1.375f,
        0.14f, 0.14f, 1.03f
    );

    drawBox(
        0.08f, 0.13f, 0.14f,
         1.25f, 4.30f, 1.375f,
        0.14f, 0.14f, 1.03f
    );

    // Lower side sills
    drawBox(
        0.08f, 0.13f, 0.14f,
        -1.25f, 2.82f, 1.375f,
        0.14f, 0.12f, 1.03f
    );

    drawBox(
        0.08f, 0.13f, 0.14f,
         1.25f, 2.82f, 1.375f,
        0.14f, 0.12f, 1.03f
    );

    // Door split bar in the middle of each side
    drawBox(
        0.08f, 0.13f, 0.14f,
        -1.25f, 3.55f, 1.375f,
        0.10f, 1.40f, 0.06f
    );

    drawBox(
        0.08f, 0.13f, 0.14f,
         1.25f, 3.55f, 1.375f,
        0.10f, 1.40f, 0.06f
    );

    // Door handles
    drawBox(
        0.78f, 0.78f, 0.75f,
        -1.33f, 3.15f, 1.55f,
        0.04f, 0.05f, 0.20f
    );

    drawBox(
        0.78f, 0.78f, 0.75f,
         1.33f, 3.15f, 1.55f,
        0.04f, 0.05f, 0.20f
    );

    // Side mirrors: arm, housing and reflective face
    drawBox(
        0.08f, 0.13f, 0.14f,
        -1.45f, 4.00f, 0.95f,
        0.40f, 0.05f, 0.05f
    );

    drawBox(
        0.08f, 0.13f, 0.14f,
         1.45f, 4.00f, 0.95f,
        0.40f, 0.05f, 0.05f
    );

    drawBox(
        0.10f, 0.10f, 0.10f,
        -1.65f, 3.95f, 0.88f,
        0.06f, 0.42f, 0.26f
    );

    drawBox(
        0.10f, 0.10f, 0.10f,
         1.65f, 3.95f, 0.88f,
        0.06f, 0.42f, 0.26f
    );

    drawBox(
        0.55f, 0.70f, 0.78f,
        -1.65f, 3.95f, 1.02f,
        0.05f, 0.34f, 0.02f
    );

    drawBox(
        0.55f, 0.70f, 0.78f,
         1.65f, 3.95f, 1.02f,
        0.05f, 0.34f, 0.02f
    );

    // Gear lever and handbrake beside the seat
    drawBox(
        0.10f, 0.10f, 0.09f,
        0.75f, 3.22f, 0.80f,
        0.05f, 0.35f, 0.05f
    );

    drawBox(
        0.85f, 0.10f, 0.05f,
        0.75f, 3.42f, 0.80f,
        0.10f, 0.10f, 0.10f
    );

    drawBox(
        0.10f, 0.10f, 0.09f,
        -0.75f, 3.20f, 0.80f,
        0.05f, 0.30f, 0.05f
    );

    drawBox(
        0.85f, 0.75f, 0.10f,
        -0.75f, 3.37f, 0.80f,
        0.08f, 0.08f, 0.08f
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

    // ---- Extra detail ----

    // Front visor
    drawBox(
        0.06f, 0.07f, 0.06f,
        0.0f, 4.50f, -0.12f,
        3.40f, 0.08f, 0.35f
    );

    // Red trim along the rear roof edge
    drawBox(
        0.80f, 0.07f, 0.02f,
        0.0f, 4.48f, 2.78f,
        3.40f, 0.08f, 0.06f
    );

    // Roof vent hatch
    drawBox(
        0.16f, 0.17f, 0.16f,
        0.0f, 4.68f, 1.35f,
        0.95f, 0.06f, 0.75f
    );

    // Front work lights
    drawBox(
        0.10f, 0.10f, 0.09f,
        -1.10f, 4.62f, -0.10f,
        0.36f, 0.16f, 0.16f
    );

    drawBox(
        0.10f, 0.10f, 0.09f,
         1.10f, 4.62f, -0.10f,
        0.36f, 0.16f, 0.16f
    );

    drawBox(
        1.0f, 0.95f, 0.70f,
        -1.10f, 4.62f, -0.19f,
        0.30f, 0.11f, 0.03f
    );

    drawBox(
        1.0f, 0.95f, 0.70f,
         1.10f, 4.62f, -0.19f,
        0.30f, 0.11f, 0.03f
    );

    // Amber beacon on the rear of the roof
    drawBox(
        0.08f, 0.08f, 0.07f,
        0.0f, 4.70f, 2.35f,
        0.30f, 0.06f, 0.30f
    );

    drawCylY(
        1.0f, 0.62f, 0.05f,
        0.0f, 4.83f, 2.35f,
        0.12f, 0.22f, 12
    );

    // Radio antenna
    drawCylY(
        0.05f, 0.05f, 0.05f,
        -1.45f, 5.15f, 2.55f,
        0.025f, 1.15f, 6
    );

    drawBox(
        0.10f, 0.10f, 0.09f,
        -1.45f, 4.68f, 2.55f,
        0.14f, 0.08f, 0.14f
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

    // ---- Extra detail ----

    // Differential housing under the chassis
    drawBox(
        0.16f, 0.16f, 0.15f,
        0.0f, 0.42f, z,
        0.85f, 0.42f, 0.85f
    );

    // Differential cover plate
    drawBox(
        0.45f, 0.45f, 0.42f,
        0.0f, 0.42f, z - 0.44f,
        0.45f, 0.30f, 0.03f
    );

    // Axle end housings near the wheels
    drawBox(
        0.30f, 0.30f, 0.28f,
        -1.55f, 0.72f, z,
        0.28f, 0.34f, 0.34f
    );

    drawBox(
        0.30f, 0.30f, 0.28f,
         1.55f, 0.72f, z,
        0.28f, 0.34f, 0.34f
    );
}

// ============================================================
// WHEELS
// ============================================================

void Tractor::drawWheel(
    float x,
    float z,
    float radius,
    float width,
    bool rightSide,
    bool isFront) const
{
    glPushMatrix();

    // 1. Position wheel at axle end
    glTranslatef(x, radius, z);

    // 2. Steer front wheels around vertical axis Y
    if (isFront)
    {
        glRotatef(steeringAngle_, 0.0f, 1.0f, 0.0f);
    }

    // 3. Roll wheel around axle X
    glRotatef(wheelRotation_, 1.0f, 0.0f, 0.0f);

    const float sideSign = rightSide ? 1.0f : -1.0f;

    // --- Tire ---
    glColor3f(0.025f, 0.025f, 0.022f);
    glPushMatrix();
    glRotatef(rightSide ? -90.0f : 90.0f, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(radius, width, 14);
    glPopMatrix();

    // --- Metal rim ---
    glColor3f(0.38f, 0.39f, 0.36f);
    glPushMatrix();
    glRotatef(rightSide ? -90.0f : 90.0f, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(radius * 0.55f, width + 0.025f, 14);
    glPopMatrix();

    // --- Painted inner rim disc ---
    glColor3f(0.90f, 0.72f, 0.12f);
    glPushMatrix();
    glRotatef(rightSide ? -90.0f : 90.0f, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(radius * 0.46f, width + 0.035f, 14);
    glPopMatrix();

    // --- Central hub ---
    glColor3f(0.12f, 0.13f, 0.12f);
    glPushMatrix();
    glRotatef(rightSide ? -90.0f : 90.0f, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(radius * 0.20f, width + 0.05f, 10);
    glPopMatrix();

    // --- Herringbone tread lugs ---
    const int lugCount = 20;
    for (int i = 0; i < lugCount; ++i)
    {
        const float angle = i * 360.0f / lugCount;
        for (int s = -1; s <= 1; s += 2)
        {
            glPushMatrix();
            glRotatef(angle, 1.0f, 0.0f, 0.0f);
            glTranslatef(s * width * 0.24f, radius * 0.99f, 0.0f);
            glRotatef(-s * 30.0f, 0.0f, 1.0f, 0.0f);
            drawBox(0.05f, 0.05f, 0.045f, 0.0f, 0.0f, 0.0f,
                    width * 0.55f, radius * 0.13f, radius * 0.11f);
            glPopMatrix();
        }
    }

    // --- Hub bolts on the outer face ---
    for (int i = 0; i < 6; ++i)
    {
        glPushMatrix();
        glRotatef(i * 60.0f, 1.0f, 0.0f, 0.0f);
        drawBox(0.72f, 0.72f, 0.68f,
                sideSign * (width * 0.5f + 0.035f), radius * 0.32f, 0.0f,
                0.05f, radius * 0.08f, radius * 0.08f);
        glPopMatrix();
    }

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
    // Curved fender built from short segments that wrap
    // over the top of the tire, with a lip on the outer edge.

    const float sideSign = rightSide ? 1.0f : -1.0f;
    const float fenderWidth = radius * 0.75f;
    const float segmentLength = radius * 0.45f;

    for (int i = 0; i < 9; ++i)
    {
        const float angle = -80.0f + i * 20.0f;

        glPushMatrix();

        glTranslatef(x, radius, z);
        glRotatef(angle, 1.0f, 0.0f, 0.0f);
        glTranslatef(0.0f, radius * 1.16f, 0.0f);

        // Fender surface
        drawBox(
            0.75f, 0.07f, 0.025f,
            0.0f, 0.0f, 0.0f,
            fenderWidth, 0.06f, segmentLength
        );

        // Outer lip
        drawBox(
            0.62f, 0.05f, 0.02f,
            sideSign * fenderWidth * 0.5f,
            -0.04f,
            0.0f,
            0.04f, 0.13f, segmentLength
        );

        glPopMatrix();
    }
}

// ============================================================
// HEADLIGHTS
// ============================================================

void Tractor::drawHeadlight(float x, float z) const
{
    // Housing (chrome-dark ring, facing forward)
    drawCylZ(
        0.14f, 0.14f, 0.12f,
        x, 2.80f, z,
        0.30f, 0.22f, 16
    );

    // Bright lens
    drawCylZ(
        1.0f, 0.90f, 0.45f,
        x, 2.80f, z - 0.09f,
        0.23f, 0.10f, 16
    );

    // Inner reflector highlight
    drawCylZ(
        1.0f, 1.0f, 0.90f,
        x, 2.80f, z - 0.15f,
        0.10f, 0.04f, 12
    );

    // Small visor above the light
    drawBox(
        0.08f, 0.08f, 0.07f,
        x, 3.13f, z - 0.03f,
        0.55f, 0.05f, 0.30f
    );
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

    // ---- Extra detail ----

    // Chrome heat bands
    drawCylY(
        0.78f, 0.78f, 0.75f,
        1.05f, 3.30f, -1.25f,
        0.175f, 0.07f, 10
    );

    drawCylY(
        0.78f, 0.78f, 0.75f,
        1.05f, 3.85f, -1.25f,
        0.175f, 0.07f, 10
    );

    // Rain flap on top
    drawBox(
        0.05f, 0.05f, 0.045f,
        1.05f, 4.40f, -1.25f,
        0.30f, 0.03f, 0.22f
    );

    // Mounting bracket to the hood
    drawBox(
        0.15f, 0.15f, 0.14f,
        1.05f, 3.15f, -1.25f,
        0.36f, 0.06f, 0.36f
    );
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

    // ---- Extra detail ----

    // Front ballast weight block
    drawBox(
        0.30f, 0.31f, 0.29f,
        0.0f, 1.30f, -2.32f,
        1.10f, 0.55f, 0.28f
    );

    // Weight plate lines
    drawBox(
        0.10f, 0.10f, 0.09f,
        0.0f, 1.20f, -2.47f,
        1.10f, 0.03f, 0.02f
    );

    drawBox(
        0.10f, 0.10f, 0.09f,
        0.0f, 1.40f, -2.47f,
        1.10f, 0.03f, 0.02f
    );

    // Tow hook
    drawBox(
        0.70f, 0.70f, 0.66f,
        0.0f, 1.05f, -2.42f,
        0.16f, 0.10f, 0.22f
    );

    // Amber indicators at the bumper ends
    drawBox(
        1.0f, 0.60f, 0.05f,
        -1.35f, 1.65f, -2.49f,
        0.22f, 0.14f, 0.04f
    );

    drawBox(
        1.0f, 0.60f, 0.05f,
         1.35f, 1.65f, -2.49f,
        0.22f, 0.14f, 0.04f
    );
}

// ============================================================
// SIDE STEP
// ============================================================

void Tractor::drawStep(float x) const
{
    // Step is moved outward so it sticks out of the body
    // and is actually visible.
    const float sideSign = x > 0.0f ? 1.0f : -1.0f;
    const float stepX = x * 1.36f;
    const float stepZ = -0.05f;

    glColor3f(
        0.18f,
        0.18f,
        0.16f
    );

    glPushMatrix();

    glTranslatef(
        stepX,
        1.05f,
        stepZ
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

    // ---- Extra detail ----

    // Anti-slip ridges
    for (int i = -1; i <= 1; ++i)
    {
        drawBox(
            0.07f, 0.07f, 0.06f,
            stepX, 1.12f, stepZ + i * 0.22f,
            0.46f, 0.02f, 0.07f
        );
    }

    // Support bracket under the step
    drawBox(
        0.10f, 0.10f, 0.09f,
        sideSign * 2.10f, 0.82f, stepZ,
        0.10f, 0.40f, 0.55f
    );

    // Front lip
    drawBox(
        0.70f, 0.70f, 0.66f,
        stepX, 1.05f, stepZ - 0.38f,
        0.55f, 0.13f, 0.03f
    );
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

    // ---- Extra detail ----

    // Pedestal and suspension spring
    drawBox(
        0.12f, 0.12f, 0.11f,
        0.0f, 2.58f, 1.00f,
        0.35f, 0.77f, 0.35f
    );

    drawCylY(
        0.60f, 0.60f, 0.58f,
        0.0f, 2.85f, 1.00f,
        0.13f, 0.30f, 10
    );

    // Headrest
    drawBox(
        0.05f, 0.05f, 0.045f,
        0.0f, 3.98f, 1.56f,
        0.40f, 0.22f, 0.14f
    );

    // Side bolsters on the cushion
    drawBox(
        0.04f, 0.04f, 0.035f,
        -0.36f, 3.19f, 1.00f,
        0.10f, 0.20f, 0.75f
    );

    drawBox(
        0.04f, 0.04f, 0.035f,
         0.36f, 3.19f, 1.00f,
        0.10f, 0.20f, 0.75f
    );

    // Armrest
    drawBox(
        0.10f, 0.10f, 0.09f,
        0.52f, 3.38f, 1.22f,
        0.10f, 0.06f, 0.50f
    );

    drawBox(
        0.10f, 0.10f, 0.09f,
        0.52f, 3.24f, 1.44f,
        0.06f, 0.24f, 0.06f
    );

    // Seat trim accent
    drawBox(
        0.72f, 0.09f, 0.03f,
        0.0f, 3.55f, 1.48f,
        0.55f, 0.08f, 0.02f
    );
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

    // Rim: ring of small segments instead of a solid disc
    const int rimSegments = 14;

    for (int i = 0; i < rimSegments; ++i)
    {
        glPushMatrix();

        glRotatef(i * 360.0f / rimSegments, 0.0f, 1.0f, 0.0f);

        drawBox(
            0.04f, 0.04f, 0.035f,
            0.34f, 0.0f, 0.0f,
            0.09f, 0.09f, 0.17f
        );

        glPopMatrix();
    }

    // Spokes
    drawBox(
        0.06f, 0.06f, 0.055f,
        0.0f, 0.0f, 0.0f,
        0.68f, 0.04f, 0.06f
    );

    drawBox(
        0.06f, 0.06f, 0.055f,
        0.0f, 0.0f, 0.0f,
        0.06f, 0.04f, 0.68f
    );

    // Center hub with a small chrome cap
    drawCylY(
        0.10f, 0.10f, 0.09f,
        0.0f, 0.0f, 0.0f,
        0.10f, 0.10f, 10
    );

    drawCylY(
        0.75f, 0.75f, 0.72f,
        0.0f, 0.06f, 0.0f,
        0.05f, 0.03f, 10
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

    // Column shroud
    drawBox(
        0.06f, 0.06f, 0.055f,
        0.0f, 3.20f, 0.30f,
        0.24f, 0.22f, 0.30f
    );
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

    // ---- Extra detail ----

    // Lower lift arms
    drawBox(
        0.10f, 0.10f, 0.09f,
        -0.75f, 0.95f, 2.45f,
        0.10f, 0.10f, 0.95f
    );

    drawBox(
        0.10f, 0.10f, 0.09f,
         0.75f, 0.95f, 2.45f,
        0.10f, 0.10f, 0.95f
    );

    // Ball ends on the lift arms
    drawBox(
        0.70f, 0.70f, 0.66f,
        -0.75f, 0.95f, 2.95f,
        0.16f, 0.16f, 0.12f
    );

    drawBox(
        0.70f, 0.70f, 0.66f,
         0.75f, 0.95f, 2.95f,
        0.16f, 0.16f, 0.12f
    );

    // Vertical lift rods up to the body
    drawBox(
        0.65f, 0.65f, 0.62f,
        -1.05f, 1.50f, 2.20f,
        0.07f, 1.10f, 0.07f
    );

    drawBox(
        0.65f, 0.65f, 0.62f,
         1.05f, 1.50f, 2.20f,
        0.07f, 1.10f, 0.07f
    );

    // Top link
    drawBox(
        0.12f, 0.12f, 0.11f,
        0.0f, 1.70f, 2.35f,
        0.08f, 0.08f, 0.75f
    );

    // PTO shaft stub with collar
    drawCylZ(
        0.60f, 0.60f, 0.58f,
        0.0f, 1.45f, 2.20f,
        0.09f, 0.40f, 10
    );

    drawCylZ(
        0.08f, 0.08f, 0.07f,
        0.0f, 1.45f, 2.05f,
        0.15f, 0.10f, 10
    );

    // Tail lights and reflectors on the rear face
    drawBox(
        0.95f, 0.05f, 0.03f,
        -1.70f, 1.75f, 2.03f,
        0.20f, 0.14f, 0.05f
    );

    drawBox(
        0.95f, 0.05f, 0.03f,
         1.70f, 1.75f, 2.03f,
        0.20f, 0.14f, 0.05f
    );

    drawBox(
        1.0f, 0.60f, 0.05f,
        -1.70f, 1.55f, 2.03f,
        0.20f, 0.08f, 0.05f
    );

    drawBox(
        1.0f, 0.60f, 0.05f,
         1.70f, 1.55f, 2.03f,
        0.20f, 0.08f, 0.05f
    );
}