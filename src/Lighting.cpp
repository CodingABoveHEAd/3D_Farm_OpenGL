#include "Lighting.h"

#include <GLFW/glfw3.h>

namespace
{
// Direction TOWARD the sun, in world space. This is Sky.cpp's sun position
// (SUN_X, SUN_Y, SUN_Z) = (-10, 14, -18) normalised (length = sqrt(620)),
// so the lit side of every object faces the sun you see in the sky and the
// opposite side gets only the softer sky fill. If you move the sun in
// Sky.cpp, update these three numbers to match.
constexpr GLfloat kSunDirection[3] = {
    -10.0f / 24.8998f,
     14.0f / 24.8998f,
    -18.0f / 24.8998f};
}

void Lighting::toggleNight()
{
    night_ = !night_;
}

void Lighting::apply() const
{
    // Shadow side: a cool, fairly bright sky fill so it is only a little
    // darker than the sun side, not black.
    const GLfloat ambient[] = {
        night_ ? 0.08f : 0.36f,
        night_ ? 0.10f : 0.39f,
        night_ ? 0.18f : 0.46f,
        1.0f
    };
    // Sun side: warm, strong sunlight.
    const GLfloat diffuse[] = {
        night_ ? 0.25f : 0.85f,
        night_ ? 0.30f : 0.80f,
        night_ ? 0.55f : 0.66f,
        1.0f
    };
    const GLfloat lightSpecular[] = {
        night_ ? 0.10f : 0.20f,
        night_ ? 0.12f : 0.20f,
        night_ ? 0.18f : 0.18f,
        1.0f
    };
    const GLfloat materialSpecular[] = {0.06f, 0.06f, 0.06f, 1.0f};
    const GLfloat globalAmbient[] = {
        night_ ? 0.03f : 0.10f,
        night_ ? 0.03f : 0.10f,
        night_ ? 0.05f : 0.12f,
        1.0f
    };

    // w = 0 makes this a directional (sun) light: every object in the huge
    // world is lit from the same direction, instead of from one nearby point.
    const GLfloat position[] = {
        kSunDirection[0], kSunDirection[1], kSunDirection[2], 0.0f};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // The scene scales geometry with glScalef, so normals must be renormalised.
    glEnable(GL_NORMALIZE);

    glShadeModel(GL_SMOOTH);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    // The direction is transformed by the current modelview matrix, so
    // apply() must be called AFTER the camera/view matrix is set each frame.
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, night_ ? 12.0f : 16.0f);
}

bool Lighting::isNight() const
{
    return night_;
}