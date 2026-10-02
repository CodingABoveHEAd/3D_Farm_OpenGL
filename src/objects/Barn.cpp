#include "objects/Barn.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <initializer_list>

namespace
{
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

void drawWindow(float x, float y, float z, bool side)
{
    glColor3f(0.20f, 0.48f, 0.58f);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(side ? 0.08f : 1.10f, 0.90f, side ? 1.10f : 0.08f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    const float frame = 0.10f;
    if (side)
    {
        drawBox(0.88f, 0.70f, 0.43f, x - 0.06f, y, z, frame, 1.10f, 0.10f);
        drawBox(0.88f, 0.70f, 0.43f, x + 0.06f, y, z, frame, 1.10f, 0.10f);
    }
    else
    {
        drawBox(0.88f, 0.70f, 0.43f, x, y, z - 0.06f, 1.10f, 0.10f, frame);
        drawBox(0.88f, 0.70f, 0.43f, x, y, z + 0.06f, 1.10f, 0.10f, frame);
    }
}

void drawFence(float width, float depth)
{
    glColor3f(0.38f, 0.20f, 0.07f);
    for (float x : {-width, 0.0f, width})
    {
        drawBox(0.38f, 1.15f, 0.38f, x, 0.58f, depth, 0.35f, 1.15f, 0.35f);
        drawBox(0.38f, 1.15f, 0.38f, x, 0.58f, -depth, 0.35f, 1.15f, 0.35f);
    }
    for (float z : {-depth, depth})
    {
        drawBox(0.38f, 1.15f, 0.38f, 0.0f, 0.82f, z, width * 2.0f, 0.14f, 0.14f);
    }
    drawBox(0.38f, 1.15f, 0.38f, -width, 0.82f, 0.0f, 0.14f, 0.14f, depth * 2.0f);
}

void drawEquipment()
{
    // A small hand cart and a barrel near the barn entrance.
    drawBox(0.24f, 0.12f, 0.04f, 7.0f, 0.65f, -3.4f, 1.8f, 0.18f, 1.0f);
    drawBox(0.30f, 0.16f, 0.06f, 7.0f, 1.05f, -3.4f, 1.2f, 0.55f, 0.80f);
    glColor3f(0.06f, 0.06f, 0.05f);
    for (float x : {6.45f, 7.55f})
    {
        glPushMatrix();
        glTranslatef(x, 0.35f, -3.4f);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        Primitives::drawCylinder(0.28f, 0.16f, 12);
        glPopMatrix();
    }
    drawBox(0.46f, 0.25f, 0.08f, -7.0f, 0.55f, -3.4f, 1.0f, 1.0f, 1.0f);
}
}

void Barn::draw(float x, float z, float scale, float rotation, bool fenced)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    // Main barn body.
    drawBox(0.54f, 0.12f, 0.06f, 0.0f, 2.75f, 0.0f, 10.0f, 5.5f, 8.0f);
    drawBox(0.68f, 0.19f, 0.07f, 0.0f, 5.25f, -0.05f, 10.25f, 0.22f, 8.15f);

    // Sloped roof made from two hierarchical roof planes.
    for (float side : {-1.0f, 1.0f})
    {
        glPushMatrix();
        glTranslatef(side * 2.35f, 6.05f, 0.0f);
        glRotatef(side * 28.0f, 0.0f, 0.0f, 1.0f);
        drawBox(0.24f, 0.08f, 0.035f, 0.0f, 0.0f, 0.0f, 5.8f, 0.26f, 8.5f);
        glPopMatrix();
    }
    drawBox(0.16f, 0.05f, 0.03f, 0.0f, 6.85f, 0.0f, 0.30f, 0.30f, 8.7f);

    // Large front barn doors and a smaller side door.
    drawBox(0.30f, 0.08f, 0.035f, -2.25f, 2.25f, -4.08f, 3.55f, 4.0f, 0.14f);
    drawBox(0.30f, 0.08f, 0.035f, 2.25f, 2.25f, -4.08f, 3.55f, 4.0f, 0.14f);
    for (float xDoor : {-2.25f, 2.25f})
    {
        drawBox(0.18f, 0.05f, 0.025f, xDoor, 2.25f, -4.17f, 0.10f, 4.15f, 0.10f);
        drawBox(0.18f, 0.05f, 0.025f, xDoor, 2.25f, -4.18f, 3.45f, 0.10f, 0.10f);
    }
    drawBox(0.34f, 0.12f, 0.05f, 4.25f, 1.35f, -0.20f, 0.18f, 2.20f, 1.60f);

    // Beam frame, braces, and windows.
    for (float xBeam : {-4.45f, 0.0f, 4.45f})
    {
        drawBox(0.28f, 0.13f, 0.05f, xBeam, 2.9f, -4.18f, 0.22f, 5.7f, 0.18f);
        drawBox(0.28f, 0.13f, 0.05f, xBeam, 2.9f, 4.18f, 0.22f, 5.7f, 0.18f);
    }
    drawBox(0.28f, 0.13f, 0.05f, 0.0f, 5.25f, -4.18f, 9.4f, 0.22f, 0.18f);
    drawBox(0.28f, 0.13f, 0.05f, 0.0f, 5.25f, 4.18f, 9.4f, 0.22f, 0.18f);
    drawWindow(-3.2f, 3.8f, -4.20f, false);
    drawWindow(3.2f, 3.8f, -4.20f, false);
    drawWindow(-5.08f, 3.7f, 0.8f, true);

    // Hay storage stacked under the rear half of the roof.
    for (int row = 0; row < 2; ++row)
    {
        for (int bale = 0; bale < 3; ++bale)
        {
            drawBox(0.70f, 0.48f, 0.20f,
                    -2.2f + bale * 1.55f, 1.0f + row * 0.95f,
                    2.65f, 1.25f, 0.72f, 1.05f);
        }
    }

    drawEquipment();
    if (fenced)
    {
        drawFence(7.0f, 6.0f);
    }

    glPopMatrix();
}
