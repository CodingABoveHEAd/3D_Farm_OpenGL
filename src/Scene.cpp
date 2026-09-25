#include "Scene.h"

#include "Input.h"
#include "graphics/Primitives.h"
#include "objects/Vegetation.h"

#include <GLFW/glfw3.h>
#include <initializer_list>

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
    renderCrops();
    renderTrees();
    renderPath();
    renderBoundary();
    farmhouse_.render();
    renderRocks();
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

void Scene::renderCrops() const
{
    for (int row = 0; row < 6; ++row)
    {
        const float z = -10.0f + static_cast<float>(row) * 1.35f;
        for (int column = 0; column < 7; ++column)
        {
            const float x = -7.5f + static_cast<float>(column) * 2.5f;
            const float size = 0.82f + static_cast<float>((row + column) % 3) * 0.08f;
            Vegetation::drawCrop(x, z, size);
        }
    }
}

void Scene::renderTrees() const
{
    Vegetation::drawTree(-18.0f, -13.0f, 1.45f);
    Vegetation::drawTree(-12.0f, -16.0f, 1.05f);
    Vegetation::drawTree(18.0f, -13.0f, 1.25f);
    Vegetation::drawTree(19.0f, 1.0f, 0.90f);
    Vegetation::drawTree(-18.0f, 9.0f, 1.30f);
    Vegetation::drawTree(18.0f, 14.0f, 1.55f);
}

void Scene::renderBoundary() const
{
    constexpr float halfWidth = 24.0f;
    constexpr float halfDepth = 19.0f;

    glColor3f(0.35f, 0.18f, 0.06f);

    for (int x = -24; x <= 24; x += 8)
    {
        if (x < 8 || x > 12)
        {
            drawFencePost(static_cast<float>(x), -halfDepth);
        }
        drawFencePost(static_cast<float>(x), halfDepth);
    }
    for (int z = -16; z <= 16; z += 8)
    {
        drawFencePost(-halfWidth, static_cast<float>(z));
        drawFencePost(halfWidth, static_cast<float>(z));
    }

    drawFenceRail(-8.0f, -halfDepth, 32.0f, false);
    drawFenceRail(18.0f, -halfDepth, 12.0f, false);
    drawFenceRail(0.0f, halfDepth, 48.0f, false);
    drawFenceRail(-halfWidth, 0.0f, 38.0f, true);
    drawFenceRail(halfWidth, 0.0f, 38.0f, true);
    renderGate();
}

void Scene::renderPath() const
{
    glColor3f(0.58f, 0.42f, 0.23f);

    glPushMatrix();
    glTranslatef(10.0f, 0.035f, -5.5f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawPlane(3.5f, 27.0f);
    glPopMatrix();
}

void Scene::renderGate() const
{
    glColor3f(0.28f, 0.12f, 0.04f);
    drawFencePost(8.0f, -19.0f);
    drawFencePost(12.0f, -19.0f);

    drawGatePanel(9.0f, -5.0f);
    drawGatePanel(11.0f, 5.0f);
}

void Scene::drawGatePanel(float x, float angle) const
{
    for (float y : {0.70f, 1.45f})
    {
        glPushMatrix();
        glTranslatef(x, y, -19.0f);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        glScalef(2.0f, 0.16f, 0.16f);
        Primitives::drawCube(1.0f, 1.0f, 1.0f);
        glPopMatrix();
    }
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

void Scene::renderRocks() const
{
    glColor3f(0.30f, 0.32f, 0.28f);
    drawRock(-20.0f, -6.0f, 0.75f, 18.0f);
    drawRock(20.0f, -5.0f, 0.55f, 35.0f);
    drawRock(-19.0f, 5.0f, 0.45f, 12.0f);
    drawRock(20.0f, 9.0f, 0.70f, 52.0f);
}

void Scene::drawRock(float x, float z, float scale, float rotation) const
{
    glPushMatrix();
    glTranslatef(x, scale * 0.28f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale * 0.55f, scale * 0.75f);
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
