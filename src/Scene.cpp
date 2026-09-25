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
    lighting_.apply();
    renderGround();
}

void Scene::renderGround() const
{
    glColor3f(0.22f, 0.58f, 0.20f);
    Primitives::drawPlane(60.0f, 60.0f);
}

bool Scene::isNight() const
{
    return lighting_.isNight();
}
