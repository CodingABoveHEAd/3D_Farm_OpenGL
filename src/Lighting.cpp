#include "Lighting.h"

#include <GLFW/glfw3.h>

void Lighting::toggleNight()
{
    night_ = !night_;
}

void Lighting::apply() const
{
    const GLfloat ambient[] = {
        night_ ? 0.08f : 0.30f,
        night_ ? 0.10f : 0.30f,
        night_ ? 0.18f : 0.30f,
        1.0f
    };
    const GLfloat diffuse[] = {
        night_ ? 0.25f : 1.0f,
        night_ ? 0.30f : 0.95f,
        night_ ? 0.55f : 0.80f,
        1.0f
    };
    const GLfloat position[] = {30.0f, 40.0f, 20.0f, 1.0f};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
}

bool Lighting::isNight() const
{
    return night_;
}
