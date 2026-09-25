#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

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

}
