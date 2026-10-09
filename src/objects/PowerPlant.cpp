#include "objects/PowerPlant.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

namespace PowerPlant {
namespace {

float g_subX = 185.0f;
float g_subY = 4.5f;
float g_subZ = 85.0f;

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

void getLightPosition(float& x, float& y, float& z)
{
    x = g_subX;
    y = g_subY;
    z = g_subZ;
}

void drawSubstation(float x, float z, float yaw, float time)
{
    (void)time;
    g_subX = x;
    g_subZ = z;

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);

    // 1. Concrete Ground Pad
    drawBox(0.48f, 0.49f, 0.46f, 0.0f, 0.10f, 0.0f, 22.0f, 0.20f, 18.0f);
    // Gravel border
    drawBox(0.38f, 0.38f, 0.36f, 0.0f, 0.05f, 0.0f, 23.5f, 0.10f, 19.5f);

    // 2. Control & Switchgear Building (Brick / Industrial Plaster)
    // Main brick walls
    drawBox(0.62f, 0.32f, 0.22f, -5.0f, 2.4f, -3.0f, 8.5f, 4.6f, 6.5f);
    // Flat roof with dark gravel / asphalt trim
    drawBox(0.24f, 0.25f, 0.26f, -5.0f, 4.8f, -3.0f, 9.2f, 0.4f, 7.2f);

    // Industrial steel door (olive/gray)
    drawBox(0.28f, 0.32f, 0.30f, -0.7f, 1.4f, -3.0f, 0.15f, 2.6f, 1.4f);
    // Door handle
    drawBox(0.85f, 0.85f, 0.80f, -0.6f, 1.4f, -2.5f, 0.08f, 0.08f, 0.15f);

    // Barred industrial windows
    drawBox(0.18f, 0.20f, 0.24f, -5.0f, 2.8f, 0.3f, 2.4f, 1.4f, 0.15f);
    drawBox(0.10f, 0.10f, 0.10f, -5.0f, 2.8f, 0.35f, 0.08f, 1.4f, 0.05f);
    drawBox(0.10f, 0.10f, 0.10f, -4.5f, 2.8f, 0.35f, 0.08f, 1.4f, 0.05f);
    drawBox(0.10f, 0.10f, 0.10f, -5.5f, 2.8f, 0.35f, 0.08f, 1.4f, 0.05f);

    // Yellow Hazard Warning Sign on building face
    drawBox(0.95f, 0.85f, 0.10f, -0.7f, 3.4f, -3.0f, 0.12f, 0.8f, 0.8f);
    drawBox(0.10f, 0.10f, 0.10f, -0.62f, 3.4f, -3.0f, 0.06f, 0.5f, 0.5f); // black lightning emblem

    // Building exhaust stack
    drawCylinderY(0.35f, 0.36f, 0.38f, -8.0f, 4.8f, -5.0f, 0.35f, 3.2f, 12);
    // Exhaust rain cap
    drawBox(0.25f, 0.26f, 0.27f, -8.0f, 8.1f, -5.0f, 1.0f, 0.1f, 1.0f);

    // 3. Heavy Step-Down Transformers (Green Industrial Metal)
    for (float tx : {3.0f, 7.5f})
    {
        // Transformer body
        drawBox(0.22f, 0.35f, 0.26f, tx, 1.8f, -2.5f, 3.2f, 3.2f, 3.0f);
        // Radiator cooling fins on sides
        for (float fz = -3.8f; fz <= -1.2f; fz += 0.45f)
        {
            drawBox(0.18f, 0.28f, 0.22f, tx + 1.75f, 1.8f, fz, 0.4f, 2.6f, 0.08f);
            drawBox(0.18f, 0.28f, 0.22f, tx - 1.75f, 1.8f, fz, 0.4f, 2.6f, 0.08f);
        }
        // Top oil conservator tank (horizontal cylinder)
        drawBox(0.25f, 0.38f, 0.29f, tx, 3.7f, -2.5f, 2.4f, 0.8f, 0.9f);

        // High voltage ceramic insulator bushings (brown ribbed stacks)
        for (float bx : {-0.8f, 0.0f, 0.8f})
        {
            drawCylinderY(0.55f, 0.35f, 0.20f, tx + bx, 4.1f, -2.5f, 0.14f, 1.1f, 8);
            // Bushing ribs
            drawBox(0.58f, 0.38f, 0.22f, tx + bx, 4.4f, -2.5f, 0.38f, 0.08f, 0.38f);
            drawBox(0.58f, 0.38f, 0.22f, tx + bx, 4.8f, -2.5f, 0.34f, 0.08f, 0.34f);
            // Spark terminal on top
            drawBox(0.85f, 0.85f, 0.85f, tx + bx, 5.25f, -2.5f, 0.08f, 0.15f, 0.08f);
        }
    }

    // 4. Lattice Transmission Pylons / Gantries inside yard
    // Steel cross gantry
    drawBox(0.40f, 0.42f, 0.44f, 5.25f, 6.2f, 2.5f, 8.5f, 0.3f, 0.3f);
    // Supporting steel legs
    drawCylinderY(0.38f, 0.40f, 0.42f, 1.2f, 0.2f, 2.5f, 0.18f, 6.0f, 8);
    drawCylinderY(0.38f, 0.40f, 0.42f, 9.3f, 0.2f, 2.5f, 0.18f, 6.0f, 8);
    // Diagonal brace struts
    drawBox(0.38f, 0.40f, 0.42f, 3.2f, 3.2f, 2.5f, 4.2f, 0.12f, 0.12f);
    drawBox(0.38f, 0.40f, 0.42f, 7.3f, 3.2f, 2.5f, 4.2f, 0.12f, 0.12f);

    // Hanging insulators from gantry
    for (float hx = 2.0f; hx <= 8.5f; hx += 2.2f)
    {
        drawCylinderY(0.60f, 0.42f, 0.25f, hx, 5.1f, 2.5f, 0.12f, 1.0f, 8);
        drawBox(0.80f, 0.80f, 0.80f, hx, 5.0f, 2.5f, 0.06f, 0.06f, 2.0f); // busbar line
    }

    // 5. Perimeter Security Fence with Steel Posts
    constexpr float halfW = 10.5f;
    constexpr float halfD = 8.5f;

    // Posts
    for (float fx = -halfW; fx <= halfW; fx += 3.5f)
    {
        drawBox(0.35f, 0.37f, 0.40f, fx, 1.1f, -halfD, 0.14f, 2.2f, 0.14f);
        drawBox(0.35f, 0.37f, 0.40f, fx, 1.1f,  halfD, 0.14f, 2.2f, 0.14f);
    }
    for (float fz = -halfD; fz <= halfD; fz += 3.4f)
    {
        drawBox(0.35f, 0.37f, 0.40f, -halfW, 1.1f, fz, 0.14f, 2.2f, 0.14f);
        drawBox(0.35f, 0.37f, 0.40f,  halfW, 1.1f, fz, 0.14f, 2.2f, 0.14f);
    }

    // Horizontal rails (top and bottom)
    for (float ry : {0.5f, 2.0f})
    {
        drawBox(0.35f, 0.37f, 0.40f, 0.0f, ry, -halfD, 21.0f, 0.08f, 0.08f);
        drawBox(0.35f, 0.37f, 0.40f, 0.0f, ry,  halfD, 21.0f, 0.08f, 0.08f);
        drawBox(0.35f, 0.37f, 0.40f, -halfW, ry, 0.0f, 0.08f, 0.08f, 17.0f);
        drawBox(0.35f, 0.37f, 0.40f,  halfW, ry, 0.0f, 0.08f, 0.08f, 17.0f);
    }

    // 6. Yard Floodlight on Building Corner (GL_LIGHT2 position)
    drawBox(0.30f, 0.32f, 0.35f, -0.6f, 4.3f, 0.1f, 0.4f, 0.15f, 0.15f); // arm
    drawBox(0.95f, 0.95f, 0.85f, -0.3f, 4.3f, 0.1f, 0.3f, 0.25f, 0.25f); // fixture

    glPopMatrix();
}

} // namespace PowerPlant
