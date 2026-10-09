#include "Input.h"

#include <GLFW/glfw3.h>

bool Input::keys_[512]{};
bool Input::previousKeys_[512]{};

void Input::initialize(GLFWwindow* window)
{
    clear();
    if (!window)
    {
        return;
    }

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
    {
        keys_[key] = glfwGetKey(window, key) == GLFW_PRESS;
        previousKeys_[key] = keys_[key];
    }
}

void Input::update(GLFWwindow* window)
{
    // GLFW normally synthesizes releases when focus is lost, but clearing the
    // snapshot ourselves guarantees that no movement key can remain latched
    // after alt-tab, minimization, or a fullscreen monitor switch.
    if (!window || !glfwGetWindowAttrib(window, GLFW_FOCUSED))
    {
        clear();
        return;
    }

    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
    {
        previousKeys_[key] = keys_[key];
        keys_[key] = glfwGetKey(window, key) == GLFW_PRESS;
    }
}

void Input::clear()
{
    for (int key = 0; key < 512; ++key)
    {
        keys_[key] = false;
        previousKeys_[key] = false;
    }
}

bool Input::isDown(int key)
{
    return key >= 0 && key < 512 && keys_[key];
}

bool Input::wasPressed(int key)
{
    return key >= 0 && key < 512 && keys_[key] && !previousKeys_[key];
}
