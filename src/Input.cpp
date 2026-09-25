#include "Input.h"

#include <GLFW/glfw3.h>

bool Input::keys_[512]{};
bool Input::previousKeys_[512]{};

void Input::initialize(GLFWwindow* window)
{
    for (int key = 0; key < 512; ++key)
    {
        keys_[key] = glfwGetKey(window, key) == GLFW_PRESS;
        previousKeys_[key] = keys_[key];
    }
}

void Input::update(GLFWwindow* window)
{
    for (int key = 0; key < 512; ++key)
    {
        previousKeys_[key] = keys_[key];
        keys_[key] = glfwGetKey(window, key) == GLFW_PRESS;
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
