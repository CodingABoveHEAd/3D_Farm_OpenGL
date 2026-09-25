#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace Primitives {

void drawCube(float width, float height, float depth)
{
    const float x = width * 0.5f;
    const float y = height * 0.5f;
    const float z = depth * 0.5f;

    glBegin(GL_QUADS);

    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-x, -y, z); glVertex3f(x, -y, z); glVertex3f(x, y, z); glVertex3f(-x, y, z);
    glNormal3f(0.0f, 0.0f, -1.0f);
    glVertex3f(x, -y, -z); glVertex3f(-x, -y, -z); glVertex3f(-x, y, -z); glVertex3f(x, y, -z);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(x, -y, z); glVertex3f(x, -y, -z); glVertex3f(x, y, -z); glVertex3f(x, y, z);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-x, -y, -z); glVertex3f(-x, -y, z); glVertex3f(-x, y, z); glVertex3f(-x, y, -z);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-x, y, z); glVertex3f(x, y, z); glVertex3f(x, y, -z); glVertex3f(-x, y, -z);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(-x, -y, -z); glVertex3f(x, -y, -z); glVertex3f(x, -y, z); glVertex3f(-x, -y, z);

    glEnd();
}

void drawPlane(float width, float depth)
{
    const float x = width * 0.5f;
    const float z = depth * 0.5f;

    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(-x, 0.0f, -z);
    glVertex3f(x, 0.0f, -z);
    glVertex3f(x, 0.0f, z);
    glVertex3f(-x, 0.0f, z);
    glEnd();
}

void drawCylinder(float radius, float height, int slices)
{
    constexpr float Pi = 3.14159265358979323846f;
    const int safeSlices = slices < 3 ? 3 : slices;

    glBegin(GL_QUAD_STRIP);
    for (int slice = 0; slice <= safeSlices; ++slice)
    {
        const float angle = 2.0f * Pi * static_cast<float>(slice) / static_cast<float>(safeSlices);
        const float x = radius * std::cos(angle);
        const float z = radius * std::sin(angle);
        glNormal3f(std::cos(angle), 0.0f, std::sin(angle));
        glVertex3f(x, 0.0f, z);
        glVertex3f(x, height, z);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int slice = safeSlices; slice >= 0; --slice)
    {
        const float angle = 2.0f * Pi * static_cast<float>(slice) / static_cast<float>(safeSlices);
        glVertex3f(radius * std::cos(angle), 0.0f, radius * std::sin(angle));
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, height, 0.0f);
    for (int slice = 0; slice <= safeSlices; ++slice)
    {
        const float angle = 2.0f * Pi * static_cast<float>(slice) / static_cast<float>(safeSlices);
        glVertex3f(radius * std::cos(angle), height, radius * std::sin(angle));
    }
    glEnd();
}

void drawSphere(float radius, int slices, int stacks)
{
    constexpr float Pi = 3.14159265358979323846f;
    const int safeSlices = slices < 3 ? 3 : slices;
    const int safeStacks = stacks < 2 ? 2 : stacks;

    for (int stack = 0; stack < safeStacks; ++stack)
    {
        const float phi0 = Pi * static_cast<float>(stack) / static_cast<float>(safeStacks);
        const float phi1 = Pi * static_cast<float>(stack + 1) / static_cast<float>(safeStacks);

        glBegin(GL_QUAD_STRIP);
        for (int slice = 0; slice <= safeSlices; ++slice)
        {
            const float theta = 2.0f * Pi * static_cast<float>(slice) / static_cast<float>(safeSlices);
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            const float sinPhi0 = std::sin(phi0);
            const float cosPhi0 = std::cos(phi0);
            glNormal3f(sinPhi0 * cosTheta, cosPhi0, sinPhi0 * sinTheta);
            glVertex3f(radius * sinPhi0 * cosTheta,
                       radius * cosPhi0,
                       radius * sinPhi0 * sinTheta);

            const float sinPhi1 = std::sin(phi1);
            const float cosPhi1 = std::cos(phi1);
            glNormal3f(sinPhi1 * cosTheta, cosPhi1, sinPhi1 * sinTheta);
            glVertex3f(radius * sinPhi1 * cosTheta,
                       radius * cosPhi1,
                       radius * sinPhi1 * sinTheta);
        }
        glEnd();
    }
}

}
