#include "graphics/Shadow.h"

#include <GLFW/glfw3.h>
#include <cmath>

// Planar shadows done with the stencil buffer.
//
// Why the old version looked wrong:
//  * The objects drawn between begin() and end() call glColor3f() themselves,
//    which silently replaced the translucent shadow colour with the object's
//    own opaque colour (so you got flat coloured copies of the house/tractor).
//  * Overlapping triangles were blended several times, giving dark blotches.
//
// New approach: pass 1 draws the flattened geometry into the stencil buffer
// only (colour writes off, so the objects' glColor calls can't matter), and
// end() then blends ONE dark quad where the stencil is set. Each pixel is
// darkened exactly once.

namespace Shadow {
namespace {

float g_lx = 0.45f;
float g_ly = 0.85f;
float g_lz = 0.35f;
float g_groundY = 0.03f;

constexpr float QuadHalfSize = 90.0f; // covers one whole farm (farm-local units)

// Column-major matrix that flattens geometry onto y = groundY along the light
void computeShadowMatrix(float mat[16], float lx, float ly, float lz, float groundY)
{
    const float dot = ly;

    mat[0]  = dot;  mat[1]  = 0.0f; mat[2]  = 0.0f; mat[3]  = 0.0f;
    mat[4]  = -lx;  mat[5]  = 0.0f; mat[6]  = -lz;  mat[7]  = 0.0f;
    mat[8]  = 0.0f; mat[9]  = 0.0f; mat[10] = dot;  mat[11] = 0.0f;
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
    g_groundY = groundY;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT |
                 GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_POLYGON_BIT |
                 GL_LIGHTING_BIT | GL_TEXTURE_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // Pass 1: stencil only. No colour, no depth writes.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_EQUAL, 0, 0xFF);                 // first fragment per pixel only
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);          // ...then mark it

    float mat[16];
    computeShadowMatrix(mat, g_lx, g_ly, g_lz, groundY);

    glPushMatrix();
    glMultMatrixf(mat);
}

void end()
{
    glPopMatrix(); // shadow projection matrix

    // Pass 2: one translucent dark quad, only where the stencil was marked.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
    glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);          // clear the stencil as we go

    glColor4f(0.03f, 0.06f, 0.03f, 0.38f);
    glBegin(GL_QUADS);
    glVertex3f(-QuadHalfSize, g_groundY, -QuadHalfSize);
    glVertex3f( QuadHalfSize, g_groundY, -QuadHalfSize);
    glVertex3f( QuadHalfSize, g_groundY,  QuadHalfSize);
    glVertex3f(-QuadHalfSize, g_groundY,  QuadHalfSize);
    glEnd();

    glPopAttrib();
}

} // namespace Shadow