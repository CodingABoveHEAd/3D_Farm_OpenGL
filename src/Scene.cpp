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

    glPushMatrix();
    glTranslatef(0.0f, 1.0f, 0.0f);
    glColor3f(0.90f, 0.25f, 0.12f);
    Primitives::drawCube(2.0f, 2.0f, 2.0f);
    glPopMatrix();
}

void Scene::renderGround() const
{
    glDisable(GL_LIGHTING);
    glColor3f(0.18f, 0.42f, 0.18f);
    Primitives::drawPlane(60.0f, 60.0f);

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int coordinate = -30; coordinate <= 30; ++coordinate)
    {
        const float value = static_cast<float>(coordinate);
        glColor3f(coordinate == 0 ? 0.80f : 0.28f, coordinate == 0 ? 0.20f : 0.50f, 0.20f);
        glVertex3f(value, 0.01f, -30.0f);
        glVertex3f(value, 0.01f, 30.0f);

        glColor3f(0.28f, coordinate == 0 ? 0.80f : 0.50f, coordinate == 0 ? 0.20f : 0.28f);
        glVertex3f(-30.0f, 0.01f, value);
        glVertex3f(30.0f, 0.01f, value);
    }
    glEnd();
}

bool Scene::isNight() const
{
    return lighting_.isNight();
}
