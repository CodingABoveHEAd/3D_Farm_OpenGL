#pragma once

struct GLFWwindow;

class Input {
public:
    static void initialize(GLFWwindow* window);
    static void update(GLFWwindow* window);
    static bool isDown(int key);
    static bool wasPressed(int key);

private:
    static bool keys_[512];
    static bool previousKeys_[512];
};
