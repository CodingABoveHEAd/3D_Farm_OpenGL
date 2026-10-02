#include "graphics/Shadow.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace Shadow {
namespace {

float g_lx = 0.45f;
float g_ly = 0.85f;
float g_lz = 0.35f;

// Computes the planar shadow projection matrix onto plane y = groundY
void computeShadowMatrix(float mat[16], float lx, float ly, float lz, float groundY)
{
    // Plane equation: 0*x + 1*y + 0*z - groundY = 0 => N = (0, 1, 0), D = -groundY
    // dot = N . L = 1 * ly = ly
    const float dot = ly;

    mat[0]  = dot;
    mat[1]  = 0.0f;
    mat[2]  = 0.0f;
    mat[3]  = 0.0f;

    mat[4]  = -lx;
    mat[5]  = 0.0f;
    mat[6]  = -lz;
    mat[7]  = 0.0f;

    mat[8]  = 0.0f;
    mat[9]  = 0.0f;
    mat[10] = dot;
    mat[11] = 0.0f;

    mat[12] = lx * groundY;
    mat[13] = dot * groundY;
    mat[14] = lz * groundY;
    mat[15] = dot;
}

} // namespace

void setLightDirection(float lx, float ly, float lz)
{
    const float len = std::sqrt(lx * lx + ly * ly + lz * lz);
    if (len > 0.0001f)
    {
        g_lx = lx / len;
        g_ly = ly / len;
        g_lz = lz / len;
    }
}

void begin(float groundY)
{
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Prevent shadow from writing to depth buffer to avoid z-fighting with itself
    glDepthMask(GL_FALSE);

    // Translucent shadow color
    glColor4f(0.04f, 0.08f, 0.04f, 0.35f);

    float mat[16];
    computeShadowMatrix(mat, g_lx, g_ly, g_lz, groundY);

    glPushMatrix();
    glMultMatrixf(mat);
}

void end()
{
    glPopMatrix();
    glPopAttrib();
}

} // namespace Shadow
