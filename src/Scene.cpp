#include "Scene.h"

#include "Input.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

void Scene::handleInput()
{
    if (Input::wasPressed(GLFW_KEY_N))
    {
        lighting_.toggleNight();
    }
    if (Input::wasPressed(GLFW_KEY_P))
    {
        animation_.togglePaused();
    }
    if (Input::wasPressed(GLFW_KEY_EQUAL) || Input::wasPressed(GLFW_KEY_KP_ADD))
    {
        animation_.changeWindmillSpeed(15.0f);
    }
    if (Input::wasPressed(GLFW_KEY_MINUS) || Input::wasPressed(GLFW_KEY_KP_SUBTRACT))
    {
        animation_.changeWindmillSpeed(-15.0f);
    }
}

void Scene::update(float deltaTime)
{
    animation_.update(deltaTime);
}

void Scene::render() const
{
    renderGround();
    renderCropField();
    renderBoundary();
    farmhouse_.render();
    renderTransformationMarker();
}

void Scene::renderGround() const
{
    glDisable(GL_LIGHTING);
    glColor3f(0.20f, 0.50f, 0.20f);

    glPushMatrix();
    glTranslatef(0.0f, -0.02f, 0.0f);
    glScalef(1.0f, 1.0f, 0.85f);
    Primitives::drawPlane(60.0f, 60.0f);
    glPopMatrix();

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int coordinate = -25; coordinate <= 25; ++coordinate)
    {
        const float value = static_cast<float>(coordinate);
        glColor3f(0.24f, 0.56f, 0.24f);
        glVertex3f(value, 0.01f, -25.0f);
        glVertex3f(value, 0.01f, 25.0f);
        glVertex3f(-25.0f, 0.01f, value);
        glVertex3f(25.0f, 0.01f, value);
    }
    glEnd();
}

void Scene::renderCropField() const
{
    glColor3f(0.48f, 0.28f, 0.10f);

    glPushMatrix();
    glTranslatef(0.0f, 0.04f, -7.0f);
    glRotatef(-3.0f, 0.0f, 1.0f, 0.0f);
    glScalef(1.5f, 1.0f, 1.25f);
    Primitives::drawPlane(12.0f, 8.0f);
    glPopMatrix();

    glColor3f(0.34f, 0.18f, 0.06f);
    for (int row = -4; row <= 4; row += 2)
    {
        glPushMatrix();
        glTranslatef(static_cast<float>(row), 0.07f, -7.0f);
        glRotatef(-3.0f, 0.0f, 1.0f, 0.0f);
        glScalef(0.08f, 1.0f, 1.0f);
        Primitives::drawCube(1.0f, 0.02f, 7.0f);
        glPopMatrix();
    }
}

void Scene::renderBoundary() const
{
    constexpr float halfWidth = 24.0f;
    constexpr float halfDepth = 19.0f;

    glColor3f(0.35f, 0.18f, 0.06f);

    for (int x = -24; x <= 24; x += 8)
    {
        drawFencePost(static_cast<float>(x), -halfDepth);
        drawFencePost(static_cast<float>(x), halfDepth);
    }
    for (int z = -16; z <= 16; z += 8)
    {
        drawFencePost(-halfWidth, static_cast<float>(z));
        drawFencePost(halfWidth, static_cast<float>(z));
    }

    drawFenceRail(0.0f, -halfDepth, 48.0f, false);
    drawFenceRail(0.0f, halfDepth, 48.0f, false);
    drawFenceRail(-halfWidth, 0.0f, 38.0f, true);
    drawFenceRail(halfWidth, 0.0f, 38.0f, true);
}

void Scene::drawFencePost(float x, float z) const
{
    glPushMatrix();
    glTranslatef(x, 0.9f, z);
    glScalef(0.35f, 1.8f, 0.35f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void Scene::drawFenceRail(float x, float z, float length, bool rotate) const
{
    glPushMatrix();
    glTranslatef(x, 1.15f, z);
    if (rotate)
    {
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    }
    glScalef(length, 0.16f, 0.16f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void Scene::renderTransformationMarker() const
{
    glColor3f(0.88f, 0.22f, 0.10f);

    glPushMatrix();
    glTranslatef(-14.0f, 1.0f, 8.0f);
    glRotatef(25.0f, 0.0f, 1.0f, 0.0f);
    glScalef(2.0f, 1.0f, 0.75f);
    Primitives::drawCube(2.0f, 2.0f, 2.0f);
    glPopMatrix();
}

bool Scene::isNight() const
{
    return lighting_.isNight();
}
