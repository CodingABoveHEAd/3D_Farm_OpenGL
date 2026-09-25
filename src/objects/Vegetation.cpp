#include "objects/Vegetation.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

namespace Vegetation {

void drawTree(float x, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);

    glColor3f(0.30f, 0.14f, 0.05f);
    Primitives::drawCylinder(0.42f, 3.2f, 8);

    glColor3f(0.12f, 0.42f, 0.10f);
    glPushMatrix();
    glTranslatef(0.0f, 3.65f, 0.0f);
    glScalef(1.35f, 1.0f, 1.35f);
    Primitives::drawCube(2.2f, 1.8f, 2.2f);
    glPopMatrix();

    glColor3f(0.16f, 0.50f, 0.12f);
    glPushMatrix();
    glTranslatef(-0.85f, 3.45f, 0.0f);
    glScalef(0.85f, 0.82f, 0.85f);
    Primitives::drawCube(1.8f, 1.6f, 1.8f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.85f, 3.55f, 0.0f);
    glScalef(0.82f, 0.90f, 0.82f);
    Primitives::drawCube(1.8f, 1.7f, 1.8f);
    glPopMatrix();

    glPopMatrix();
}

void drawCrop(float x, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, 0.08f, z);
    glScalef(scale, scale, scale);

    glColor3f(0.14f, 0.48f, 0.10f);
    glPushMatrix();
    glTranslatef(0.0f, 0.55f, 0.0f);
    glScalef(0.16f, 1.1f, 0.16f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glColor3f(0.24f, 0.64f, 0.12f);
    glPushMatrix();
    glTranslatef(-0.22f, 0.66f, 0.0f);
    glRotatef(-28.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.55f, 0.10f, 0.16f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.22f, 0.84f, 0.0f);
    glRotatef(28.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.55f, 0.10f, 0.16f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPopMatrix();
}

}
