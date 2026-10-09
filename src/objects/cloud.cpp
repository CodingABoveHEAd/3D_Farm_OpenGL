#include "objects/cloud.h"

#include "DayNightSettings.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace Cloud {

// ============================================================
//  Helpers — material-based cloud color (renders white)
// ============================================================
namespace {

float gNightAmount = 0.0f;

void setCloudColor(float r, float g, float b)
{
    glColor3f(r, g, b);
}

void drawCloudModel(bool lowDetail)
{
    if (lowDetail)
    {
        const float puffs[][4] = {
            {-1.2f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.25f, 0.1f, 1.35f},
            {1.25f, 0.05f, 0.0f, 1.05f}, {-0.35f, 0.85f, 0.0f, 0.9f},
            {0.65f, 0.75f, 0.0f, 0.8f}};
        for (const auto& puff : puffs)
        {
            glPushMatrix();
            glTranslatef(puff[0], puff[1], puff[2]);
            Primitives::drawSphere(puff[3], 7, 4);
            glPopMatrix();
        }
        return;
    }

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

    {
        glPushMatrix(); glTranslatef(-0.55f, 0.75f, 0.05f);
            Primitives::drawSphere(0.85f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( 0.35f, 0.85f, 0.00f);
            Primitives::drawSphere(0.95f, 12, 8); glPopMatrix();
        glPushMatrix(); glTranslatef( 1.15f, 0.65f, 0.10f);
            Primitives::drawSphere(0.70f, 10, 6); glPopMatrix();
    }

    {
        glPushMatrix(); glTranslatef(-1.75f, 0.05f, 0.00f);
            Primitives::drawSphere(0.75f, 10, 6); glPopMatrix();
        glPushMatrix(); glTranslatef( 2.30f, 0.05f, 0.00f);
            Primitives::drawSphere(0.55f, 8, 5); glPopMatrix();
    }

}

GLuint cloudDisplayList(bool lowDetail)
{
    static GLuint detailedList = 0;
    static GLuint simpleList = 0;
    GLuint& list = lowDetail ? simpleList : detailedList;
    if (list == 0)
    {
        list = glGenLists(1);
        if (list != 0)
        {
            glNewList(list, GL_COMPILE);
            drawCloudModel(lowDetail);
            glEndList();
        }
    }
    return list;
}

struct Frustum
{
    float planes[6][4]{};
    float eyeX = 0.0f;
    float eyeZ = 0.0f;
    bool valid = false;
};

Frustum currentFrustum()
{
    GLfloat projection[16];
    GLfloat modelview[16];
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

    float clip[16];
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
        {
            clip[column * 4 + row] = 0.0f;
            for (int k = 0; k < 4; ++k)
                clip[column * 4 + row] +=
                    projection[k * 4 + row] * modelview[column * 4 + k];
        }

    Frustum frustum;
    frustum.eyeX = -(modelview[0] * modelview[12]
        + modelview[1] * modelview[13] + modelview[2] * modelview[14]);
    frustum.eyeZ = -(modelview[8] * modelview[12]
        + modelview[9] * modelview[13] + modelview[10] * modelview[14]);
    const int rows[3] = {0, 1, 2};
    for (int axis = 0; axis < 3; ++axis)
        for (int side = 0; side < 2; ++side)
        {
            const float sign = side == 0 ? 1.0f : -1.0f;
            float* plane = frustum.planes[axis * 2 + side];
            for (int component = 0; component < 4; ++component)
                plane[component] = clip[component * 4 + 3]
                    + sign * clip[component * 4 + rows[axis]];
            const float length = std::sqrt(
                plane[0] * plane[0] + plane[1] * plane[1]
                + plane[2] * plane[2]);
            if (length <= 1e-6f)
                return frustum;
            for (float& component : frustum.planes[axis * 2 + side])
                component /= length;
        }
    frustum.valid = true;
    return frustum;
}

bool visible(const Frustum& frustum, float x, float y, float z, float radius)
{
    if (!frustum.valid)
        return true;
    for (const auto& plane : frustum.planes)
        if (plane[0] * x + plane[1] * y + plane[2] * z + plane[3] < -radius)
            return false;
    return true;
}

} // anonymous namespace


// ============================================================
//  Single cloud
// ============================================================
void drawCloud(float x, float y, float z, float scale)
{
    setCloudColor(
        DayNightSettings::DayCloud[0] +
            (DayNightSettings::NightCloud[0] - DayNightSettings::DayCloud[0]) * gNightAmount,
        DayNightSettings::DayCloud[1] +
            (DayNightSettings::NightCloud[1] - DayNightSettings::DayCloud[1]) * gNightAmount,
        DayNightSettings::DayCloud[2] +
            (DayNightSettings::NightCloud[2] - DayNightSettings::DayCloud[2]) * gNightAmount);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);
    const GLuint list = cloudDisplayList(false);
    if (list != 0) glCallList(list);
    else drawCloudModel(false);
    glPopMatrix();
}


// ============================================================
//  Cloud field — a dense set of large clouds throughout the sky
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

constexpr int   kMaxClouds   = 96;
constexpr float kMinRadius   =  25.0f;    // don't spawn on top of camera
constexpr float kMaxRadius   =  300.0f;    // cover the complete visible horizon
constexpr float kYMin        =  18.0f;
constexpr float kYMax        =  72.0f;

struct CloudInstance {
    float x, y, z;
    float scale;
    float dirX, dirZ;
    float speed;
    bool lowDetail;
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
    const float u = rand01(seed + 1);
    const float r = sqrtf(
        kMinRadius * kMinRadius
        + u * (kMaxRadius * kMaxRadius - kMinRadius * kMinRadius));

    x = cosf(angle) * r;
    z = sinf(angle) * r;
}

// Place one cloud at a random valid position
void respawnCloud(CloudInstance& c, unsigned int seed)
{
    randomDiscPoint(seed, c.x, c.z);
    // A shared prevailing wind looks like a weather system; the small angle
    // variation prevents the field from moving as one rigid layer.
    const float direction = -0.10f + rand01(seed + 2) * 0.20f;
    c.dirX = std::cos(direction);
    c.dirZ = std::sin(direction);

    c.y     = kYMin + rand01(seed + 3) * (kYMax - kYMin);
    c.scale =  1.7f + rand01(seed + 4) * 3.3f;
    c.speed =  0.35f + rand01(seed + 5) * 0.85f;
    c.lowDetail = c.scale < 2.8f || rand01(seed + 6) < 0.45f;
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
    // that span BOTH positive and negative — from -300 to +300.
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
            c.scale = 1.7f + rand01(s + 1) * 3.3f;
            c.speed = 0.35f + rand01(s + 2) * 0.85f;
            c.lowDetail = c.scale < 2.8f || rand01(s + 3) < 0.45f;
            // Keep the prevailing direction so motion stays coherent.
        }
    }
}

void setNightAmount(float nightAmount)
{
    gNightAmount = std::fmax(0.0f, std::fmin(1.0f, nightAmount));
}


void drawField()
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
    setCloudColor(
        DayNightSettings::DayCloud[0] +
            (DayNightSettings::NightCloud[0] - DayNightSettings::DayCloud[0]) * gNightAmount,
        DayNightSettings::DayCloud[1] +
            (DayNightSettings::NightCloud[1] - DayNightSettings::DayCloud[1]) * gNightAmount,
        DayNightSettings::DayCloud[2] +
            (DayNightSettings::NightCloud[2] - DayNightSettings::DayCloud[2]) * gNightAmount);

    const Frustum frustum = currentFrustum();
    const GLuint detailedList = cloudDisplayList(false);
    const GLuint simpleList = cloudDisplayList(true);

    for (int i = 0; i < kMaxClouds; ++i) {
        const CloudInstance& cloud = g_clouds[i];
        constexpr float repeat = kMaxRadius * 2.0f;
        const float drawX = cloud.x
            + std::round((frustum.eyeX - cloud.x) / repeat) * repeat;
        const float drawZ = cloud.z
            + std::round((frustum.eyeZ - cloud.z) / repeat) * repeat;
        if (!visible(frustum, drawX, cloud.y, drawZ, cloud.scale * 3.1f))
            continue;

        glPushMatrix();
        glTranslatef(drawX, cloud.y, drawZ);
        glScalef(cloud.scale, cloud.scale, cloud.scale);
        const GLuint list = cloud.lowDetail ? simpleList : detailedList;
        if (list != 0) glCallList(list);
        else drawCloudModel(cloud.lowDetail);
        glPopMatrix();
    }
    glPopAttrib();
}

} // namespace Cloud
