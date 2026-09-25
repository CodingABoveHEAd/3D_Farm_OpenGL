#include "objects/Farmhouse.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <initializer_list>

namespace {
constexpr float HouseX = 10.0f;
constexpr float HouseZ = 7.0f;
}

void Farmhouse::render() const
{
    glPushMatrix();
    glTranslatef(HouseX, 0.0f, HouseZ);

    renderBody();
    renderRoof();
    renderDoor();
    renderWindows();
    renderChimney();
    renderPorch();

    glPopMatrix();
}

void Farmhouse::renderBody() const
{
    glColor3f(0.78f, 0.52f, 0.25f);

    glPushMatrix();
    glTranslatef(0.0f, 2.5f, 0.0f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(8.0f, 5.0f, 6.0f);
    glPopMatrix();
}

void Farmhouse::renderRoof() const
{
    glColor3f(0.35f, 0.08f, 0.05f);

    glPushMatrix();
    glTranslatef(0.0f, 5.15f, -1.25f);
    glRotatef(-30.0f, 1.0f, 0.0f, 0.0f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(8.8f, 0.35f, 3.8f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 5.15f, 1.25f);
    glRotatef(30.0f, 1.0f, 0.0f, 0.0f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(8.8f, 0.35f, 3.8f);
    glPopMatrix();
}

void Farmhouse::renderDoor() const
{
    glColor3f(0.22f, 0.09f, 0.04f);

    glPushMatrix();
    glTranslatef(0.0f, 1.25f, 3.08f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(1.6f, 2.5f, 0.20f);
    glPopMatrix();

    glColor3f(0.85f, 0.65f, 0.20f);
    glPushMatrix();
    glTranslatef(0.48f, 1.25f, 3.21f);
    Primitives::drawCube(0.12f, 0.12f, 0.08f);
    glPopMatrix();
}

void Farmhouse::renderWindows() const
{
    glColor3f(0.22f, 0.62f, 0.78f);
    drawWindow(-2.35f);
    drawWindow(2.35f);
}

void Farmhouse::drawWindow(float x) const
{
    glPushMatrix();
    glTranslatef(x, 2.85f, 3.08f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(1.65f, 1.45f, 0.20f);
    glPopMatrix();

    glColor3f(0.93f, 0.82f, 0.43f);
    drawWindowCross(x, 2.85f, 3.21f);
    glColor3f(0.22f, 0.62f, 0.78f);
}

void Farmhouse::drawWindowCross(float x, float y, float z) const
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(0.08f, 1.0f, 1.0f);
    Primitives::drawCube(1.0f, 1.45f, 0.06f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(x, y, z + 0.01f);
    glScalef(1.0f, 0.08f, 1.0f);
    Primitives::drawCube(1.65f, 1.0f, 0.06f);
    glPopMatrix();
}

void Farmhouse::renderChimney() const
{
    glColor3f(0.42f, 0.18f, 0.10f);

    glPushMatrix();
    glTranslatef(2.5f, 6.0f, -0.8f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(0.85f, 1.8f, 0.85f);
    glPopMatrix();
}

void Farmhouse::renderPorch() const
{
    glColor3f(0.50f, 0.27f, 0.10f);

    glPushMatrix();
    glTranslatef(0.0f, 0.25f, 3.75f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(3.6f, 0.5f, 1.5f);
    glPopMatrix();

    glColor3f(0.28f, 0.12f, 0.04f);
    for (float x : {-1.45f, 1.45f})
    {
        glPushMatrix();
        glTranslatef(x, 1.65f, 4.25f);
        glScalef(0.22f, 1.0f, 0.22f);
        Primitives::drawCube(1.0f, 3.0f, 1.0f);
        glPopMatrix();
    }

    glColor3f(0.40f, 0.18f, 0.07f);
    glPushMatrix();
    glTranslatef(0.0f, 3.15f, 4.25f);
    glRotatef(8.0f, 1.0f, 0.0f, 0.0f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawCube(3.8f, 0.25f, 1.7f);
    glPopMatrix();
}
