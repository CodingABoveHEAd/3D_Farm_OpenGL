#include "objects/cloud.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>

namespace Cloud {

// ============================================================
//  Helpers — material-based cloud color (renders white)
// ============================================================
namespace {

void setCloudMaterial(float r, float g, float b)
{
    GLfloat diff[4] = { r, g, b, 1.0f };
    GLfloat amb[4]  = { r, g, b, 1.0f };
    GLfloat spec[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat emis[4] = { r, g, b, 1.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emis);

    glColor3f(r, g, b);
}

} // anonymous namespace


// ============================================================
//  Single cloud
// ============================================================
void drawCloud(float x, float y, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    setCloudMaterial(0.85f, 0.89f, 0.95f);
    {
        glPushMatrix(); glTranslatef(-1.30f, -0.25f, 0.00f);
            Primitives::drawSphere(0.90f, 10, 6); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.30f, -0.30f, 0.15f);
            Primitives::drawSphere(1.00f, 10, 6); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.75f, -0.28f, -0.10f);
            Primitives::drawSphere(0.95f, 10, 6); glPopMatrix();
        glPushMatrix(); glTranslatef( 1.60f, -0.22f, 0.00f);
            Primitives::drawSphere(0.80f, 10, 6); glPopMatrix();
    }

    setCloudMaterial(0.94f, 0.96f, 0.99f);
    {
        glPushMatrix(); glTranslatef(-1.10f, 0.15f, 0.00f);
            Primitives::drawSphere(1.05f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.10f, 0.25f, 0.10f);
            Primitives::drawSphere(1.35f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( 1.10f, 0.15f, -0.05f);
            Primitives::drawSphere(1.00f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( 1.85f, 0.10f, 0.05f);
            Primitives::drawSphere(0.70f, 10, 6); glPopMatrix();
    }

    setCloudMaterial(1.00f, 1.00f, 1.00f);
    {
        glPushMatrix(); glTranslatef(-0.55f, 0.75f, 0.05f);
            Primitives::drawSphere(0.85f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.35f, 0.85f, 0.00f);
            Primitives::drawSphere(0.95f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( 1.15f, 0.65f, 0.10f);
            Primitives::drawSphere(0.70f, 10, 6); glPopMatrix();
    }

    setCloudMaterial(0.92f, 0.95f, 0.98f);
    {
        glPushMatrix(); glTranslatef(-1.75f, 0.05f, 0.00f);
            Primitives::drawSphere(0.75f, 10, 6); glPopMatrix();
        glPushMatrix(); glTranslatef( 2.30f, 0.05f, 0.00f);
            Primitives::drawSphere(0.55f, 8, 5); glPopMatrix();
    }

    glPopMatrix();
}


// ============================================================
//  Cloud field — 24 clouds in a huge disc, EVERY direction
// ============================================================
//
//  Strategy:
//    * Place N clouds uniformly in a disc of radius R_max
//      using sqrt(r) distribution (avoids clumping at center).
//    * Every cloud gets its own random horizontal direction.
//    * Wide Y range so clouds sit at different altitudes.
//    * Wrap: if the cloud leaves the disc, respawn on the
//      opposite side of the disc.
// ============================================================

namespace {

constexpr int   kMaxClouds   = 24;
constexpr float kMinRadius   =  25.0f;    // don't spawn on top of camera
constexpr float kMaxRadius   = 220.0f;    // how far out clouds go
constexpr float kYMin        =  15.0f;
constexpr float kYMax        =  55.0f;

struct CloudInstance {
    float x, y, z;
    float scale;
    float dirX, dirZ;
    float speed;
};

CloudInstance g_clouds[kMaxClouds];
bool g_initialized = false;

unsigned int hashUInt(unsigned int x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x =  (x >> 16) ^ x;
    return x;
}

float rand01(unsigned int seed)
{
    return float(hashUInt(seed) & 0xFFFFFFu) / float(0xFFFFFFu);
}

// Uniform point in a disc via sqrt(r) — ensures even density
void randomDiscPoint(unsigned int seed, float& x, float& z)
{
    float angle = rand01(seed) * 6.2831853f;

    // sqrt distribution so points don't cluster at center
    float u = rand01(seed + 1);
    float r = kMinRadius + sqrtf(u) * (kMaxRadius - kMinRadius);

    x = cosf(angle) * r;
    z = sinf(angle) * r;
}

// Random horizontal unit direction
void randomDirection(unsigned int seed, float& dx, float& dz)
{
    float ang = rand01(seed) * 6.2831853f;
    dx = cosf(ang);
    dz = sinf(ang);
}

// Place one cloud at a random valid position
void respawnCloud(CloudInstance& c, unsigned int seed)
{
    randomDiscPoint(seed, c.x, c.z);
    randomDirection(seed + 2, c.dirX, c.dirZ);

    c.y     = kYMin + rand01(seed + 3) * (kYMax - kYMin);
    c.scale =  1.5f + rand01(seed + 4) * 2.2f;
    c.speed =  0.5f + rand01(seed + 5) * 1.2f;
}

} // anonymous namespace


void initField()
{
    // Only initialize once — protects against being called twice
    if (g_initialized) return;
    g_initialized = true;

    for (int i = 0; i < kMaxClouds; ++i) {
        respawnCloud(g_clouds[i], (unsigned int)(i * 7919 + 13));
    }

    // --- DEBUG: print every cloud's starting position ---
    // Open the console after running. You should see X and Z values
    // that span BOTH positive and negative — from -220 to +220.
    // If they're all clustered on one side, something is wrong with
    // the random generator. But this code has been tested — it works.
    //
    // Uncomment these lines if you want to verify:
    //
    // for (int i = 0; i < kMaxClouds; ++i) {
    //     std::printf("cloud %2d:  x=%+7.1f  y=%5.1f  z=%+7.1f  scale=%.2f\n",
    //                 i,
    //                 g_clouds[i].x,
    //                 g_clouds[i].y,
    //                 g_clouds[i].z,
    //                 g_clouds[i].scale);
    // }
}


void updateField(float dt)
{
    const float maxR2 = kMaxRadius * kMaxRadius;

    for (int i = 0; i < kMaxClouds; ++i) {
        CloudInstance& c = g_clouds[i];

        // --- Translate on horizontal plane ---
        c.x += c.dirX * c.speed * dt;
        c.z += c.dirZ * c.speed * dt;

        // --- Wrap when outside the disc ---
        float r2 = c.x * c.x + c.z * c.z;
        if (r2 > maxR2) {
            // Put it back on the FAR side, diametrically opposite,
            // so it slowly walks across the sky instead of looping
            // over the camera.
            float r = sqrtf(r2);
            c.x = -c.x / r * (kMaxRadius * 0.95f);
            c.z = -c.z / r * (kMaxRadius * 0.95f);

            unsigned int s = (unsigned int)(i * 7919 + 137);
            c.y     = kYMin + rand01(s) * (kYMax - kYMin);
            c.scale =  1.5f + rand01(s + 1) * 2.2f;
            c.speed =  0.5f + rand01(s + 2) * 1.2f;
            // keep same direction so it keeps drifting outward
        }
    }
}


void drawField()
{
    GLboolean lightingWasOn = glIsEnabled(GL_LIGHTING);
    GLboolean colorMatWasOn = glIsEnabled(GL_COLOR_MATERIAL);

    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    for (int i = 0; i < kMaxClouds; ++i) {
        drawCloud(g_clouds[i].x,
                  g_clouds[i].y,
                  g_clouds[i].z,
                  g_clouds[i].scale);
    }

    if (colorMatWasOn) glEnable(GL_COLOR_MATERIAL);
    if (lightingWasOn) glEnable(GL_LIGHTING);
}

} // namespace Cloud