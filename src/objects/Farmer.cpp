#include "objects/Farmer.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace Farmer {
namespace {

constexpr float Pi = 3.14159265358979323846f;

void drawBox(float r, float g, float b,
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

void drawCylinderY(float r, float g, float b,
                   float x, float y, float z,
                   float radius, float height, int segments = 12)
{
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCylinder(radius, height, segments);
    glPopMatrix();
}

} // namespace

void drawFarmer(float x, float z, float heading, float time,
                Activity activity, float scale)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(heading, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    // Compute joint angles based on activity
    float legLeft = 0.0f, legRight = 0.0f;
    float armLeft = 0.0f, armRight = 0.0f;
    float torsoTilt = 0.0f;
    float idleSway = 0.0f;

    if (activity == Walking)
    {
        const float walkCycle = std::sin(time * 5.0f);
        legLeft  =  walkCycle * 28.0f;
        legRight = -walkCycle * 28.0f;
        // Opposite phase for arms
        armLeft  = -walkCycle * 25.0f;
        armRight =  walkCycle * 25.0f;
        torsoTilt = 3.0f; // slight forward lean
    }
    else if (activity == Working)
    {
        // Hoeing / bending animation
        const float workCycle = std::sin(time * 3.0f);
        torsoTilt = 22.0f + workCycle * 10.0f;
        armLeft   = 35.0f + workCycle * 20.0f;
        armRight  = 45.0f + workCycle * 20.0f;
        legLeft   =  8.0f;
        legRight  = -8.0f;
    }
    else // Standing / Idle
    {
        idleSway = std::sin(time * 1.5f) * 1.5f;
        armLeft  = std::sin(time * 1.2f) * 3.0f;
        armRight = -std::sin(time * 1.2f) * 3.0f;
    }

    // Torso pivot at pelvis (y = 1.15)
    glPushMatrix();
    glTranslatef(0.0f, 1.15f, 0.0f);
    glRotatef(torsoTilt, 1.0f, 0.0f, 0.0f);
    glRotatef(idleSway, 0.0f, 1.0f, 0.0f);

    // Denim overalls / plaid shirt (Blue/Plum)
    // Shirt / Torso
    drawBox(0.20f, 0.38f, 0.65f, 0.0f, 0.40f, 0.0f, 0.50f, 0.60f, 0.28f);

    // Overalls straps
    drawBox(0.12f, 0.22f, 0.45f, -0.15f, 0.42f, 0.01f, 0.09f, 0.58f, 0.30f);
    drawBox(0.12f, 0.22f, 0.45f,  0.15f, 0.42f, 0.01f, 0.09f, 0.58f, 0.30f);
    // Brass buttons on straps
    drawBox(0.85f, 0.75f, 0.20f, -0.15f, 0.55f, 0.16f, 0.04f, 0.04f, 0.02f);
    drawBox(0.85f, 0.75f, 0.20f,  0.15f, 0.55f, 0.16f, 0.04f, 0.04f, 0.02f);

    // Neck
    drawBox(0.88f, 0.72f, 0.58f, 0.0f, 0.73f, 0.0f, 0.16f, 0.10f, 0.16f);

    // Head
    drawBox(0.88f, 0.72f, 0.58f, 0.0f, 0.92f, 0.0f, 0.28f, 0.30f, 0.26f);

    // Farmer Straw Hat
    // Brim
    drawBox(0.82f, 0.68f, 0.35f, 0.0f, 1.04f, 0.0f, 0.68f, 0.05f, 0.68f);
    // Crown
    drawBox(0.78f, 0.62f, 0.30f, 0.0f, 1.16f, 0.0f, 0.34f, 0.20f, 0.34f);
    // Hat band
    drawBox(0.65f, 0.15f, 0.12f, 0.0f, 1.08f, 0.0f, 0.36f, 0.04f, 0.36f);

    // Left Arm (pivot at shoulder y = 0.62, x = -0.32)
    glPushMatrix();
    glTranslatef(-0.32f, 0.62f, 0.0f);
    glRotatef(armLeft, 1.0f, 0.0f, 0.0f);
    // Upper arm
    drawBox(0.20f, 0.38f, 0.65f, 0.0f, -0.18f, 0.0f, 0.14f, 0.36f, 0.15f);
    // Lower arm / hand
    drawBox(0.88f, 0.72f, 0.58f, 0.0f, -0.42f, 0.04f, 0.12f, 0.25f, 0.12f);
    glPopMatrix();

    // Right Arm (pivot at shoulder y = 0.62, x = 0.32)
    glPushMatrix();
    glTranslatef(0.32f, 0.62f, 0.0f);
    glRotatef(armRight, 1.0f, 0.0f, 0.0f);
    // Upper arm
    drawBox(0.20f, 0.38f, 0.65f, 0.0f, -0.18f, 0.0f, 0.14f, 0.36f, 0.15f);
    // Lower arm / hand
    drawBox(0.88f, 0.72f, 0.58f, 0.0f, -0.42f, 0.04f, 0.12f, 0.25f, 0.12f);

    // If working, draw farm hoe/tool in hand
    if (activity == Working)
    {
        // Hoe handle (wood)
        drawBox(0.55f, 0.35f, 0.18f, 0.0f, -0.20f, 0.55f, 0.04f, 0.04f, 1.40f);
        // Hoe blade (iron)
        drawBox(0.40f, 0.42f, 0.45f, 0.0f, -0.28f, 1.25f, 0.24f, 0.12f, 0.04f);
    }
    glPopMatrix();

    glPopMatrix(); // end torso

    // Left Leg (pivot at hip y = 1.15, x = -0.14)
    glPushMatrix();
    glTranslatef(-0.14f, 1.15f, 0.0f);
    glRotatef(legLeft, 1.0f, 0.0f, 0.0f);
    // Pants / leg
    drawBox(0.12f, 0.22f, 0.45f, 0.0f, -0.45f, 0.0f, 0.18f, 0.65f, 0.19f);
    // Boot (dark brown leather)
    drawBox(0.22f, 0.14f, 0.08f, 0.0f, -0.95f, 0.06f, 0.20f, 0.22f, 0.32f);
    glPopMatrix();

    // Right Leg (pivot at hip y = 1.15, x = 0.14)
    glPushMatrix();
    glTranslatef(0.14f, 1.15f, 0.0f);
    glRotatef(legRight, 1.0f, 0.0f, 0.0f);
    // Pants / leg
    drawBox(0.12f, 0.22f, 0.45f, 0.0f, -0.45f, 0.0f, 0.18f, 0.65f, 0.19f);
    // Boot (dark brown leather)
    drawBox(0.22f, 0.14f, 0.08f, 0.0f, -0.95f, 0.06f, 0.20f, 0.22f, 0.32f);
    glPopMatrix();

    glPopMatrix();
}

} // namespace Farmer
