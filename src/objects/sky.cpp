#include "objects/sky.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

namespace Sky {

// ============================================================
//  Sun — layered discs give a soft, glowing look
// ============================================================
void drawSun()
{
    // Sun shouldn't be lit by our scene light — it IS the light.
    GLboolean lightingWasOn = glIsEnabled(GL_LIGHTING);
    glDisable(GL_LIGHTING);

    glPushMatrix();
    glTranslatef(-10.0f, 14.0f, -18.0f);

    // --- Outer halo (large, pale yellow) ---
    glColor3f(1.00f, 0.90f, 0.55f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, 0.35f);
    Primitives::drawSphere(3.4f, 20, 12);
    glPopMatrix();

    // --- Mid glow ---
    glColor3f(1.00f, 0.86f, 0.35f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, 0.40f);
    Primitives::drawSphere(2.6f, 20, 12);
    glPopMatrix();

    // --- Bright core ---
    glColor3f(1.00f, 0.95f, 0.60f);
    glPushMatrix();
    glScalef(1.0f, 1.0f, 0.55f);
    Primitives::drawSphere(1.9f, 22, 14);
    glPopMatrix();

    glPopMatrix();

    if (lightingWasOn)
    {
        glEnable(GL_LIGHTING);
    }
}


// ============================================================
//  Sky clear color
// ============================================================
void setClearColor(bool isNight)
{
    if (isNight) {
        // Deep midnight blue
        glClearColor(0.03f, 0.04f, 0.12f, 1.0f);
    } else {
        // Fresh daytime blue
        glClearColor(0.53f, 0.75f, 0.95f, 1.0f);
    }
}

} // namespace Sky