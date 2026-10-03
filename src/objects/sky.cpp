#include "objects/sky.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace Sky {
namespace {

const float PI = 3.14159265f;

// Set by setClearColor() so drawSun() knows whether it is day or night.
bool gNight = false;

// Sun / moon position in the world
const float SUN_X = -10.0f, SUN_Y = 14.0f, SUN_Z = -18.0f;

// Dome radius. If your camera far plane is closer than this, lower it.
const float DOME_R = 90.0f;

struct RGB { float r, g, b; };

RGB mix(const RGB& a, const RGB& b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}

float hashf(int n)
{
    float s = sinf(n * 12.9898f) * 43758.5453f;
    return s - floorf(s);
}

// Unit direction from azimuth / elevation
void dirOf(float az, float el, float& x, float& y, float& z)
{
    x = cosf(el) * sinf(az);
    y = sinf(el);
    z = cosf(el) * cosf(az);
}

// Sky colour for a direction. h = height (0 horizon .. 1 zenith),
// s = how much the direction points at the sun (0..1).
RGB skyColor(float h, float s)
{
    if (h < 0.0f) h = 0.0f;

    if (gNight) {
        const RGB horizon = {0.10f, 0.13f, 0.28f};
        const RGB mid     = {0.04f, 0.06f, 0.17f};
        const RGB zenith  = {0.01f, 0.02f, 0.07f};
        RGB c = (h < 0.35f) ? mix(horizon, mid, h / 0.35f)
                            : mix(mid, zenith, (h - 0.35f) / 0.65f);
        // faint moon glow
        float g = powf(s, 8.0f) * 0.10f;
        return { c.r + g * 0.6f, c.g + g * 0.7f, c.b + g };
    }

    const RGB horizon = {0.84f, 0.92f, 0.98f};   // pale haze
    const RGB mid     = {0.50f, 0.74f, 0.96f};
    const RGB zenith  = {0.20f, 0.46f, 0.86f};   // deeper blue overhead
    RGB c = (h < 0.30f) ? mix(horizon, mid, h / 0.30f)
                        : mix(mid, zenith, (h - 0.30f) / 0.70f);

    // warm glow around the sun
    float g = powf(s, 6.0f) * 0.45f;
    c.r += g * 0.55f;
    c.g += g * 0.38f;
    c.b += g * 0.05f;
    if (c.r > 1.0f) c.r = 1.0f;
    if (c.g > 1.0f) c.g = 1.0f;
    if (c.b > 1.0f) c.b = 1.0f;
    return c;
}

// Gradient hemisphere, drawn with depth test ON so it only shows where
// nothing else was drawn (it sits behind everything).
void drawDome()
{
    const int RINGS = 16;
    const int SEGS  = 40;
    const float elMin = -0.14f;
    const float elMax = PI * 0.5f;

    // sun direction (normalised)
    float sl = sqrtf(SUN_X * SUN_X + SUN_Y * SUN_Y + SUN_Z * SUN_Z);
    float sx = SUN_X / sl, sy = SUN_Y / sl, sz = SUN_Z / sl;

    for (int i = 0; i < RINGS; ++i) {
        // ease elevation so more rings are near the horizon
        float t0 = (float)i / RINGS, t1 = (float)(i + 1) / RINGS;
        float el0 = elMin + (elMax - elMin) * t0 * t0;
        float el1 = elMin + (elMax - elMin) * t1 * t1;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= SEGS; ++j) {
            float az = 2.0f * PI * j / SEGS;
            for (int k = 0; k < 2; ++k) {
                float el = (k == 0) ? el0 : el1;
                float x, y, z;
                dirOf(az, el, x, y, z);
                float dot = x * sx + y * sy + z * sz;
                if (dot < 0.0f) dot = 0.0f;
                RGB c = skyColor(y, dot);
                glColor3f(c.r, c.g, c.b);
                glVertex3f(x * DOME_R, y * DOME_R, z * DOME_R);
            }
        }
        glEnd();
    }
}

void drawStars()
{
    const float t = static_cast<float>(glfwGetTime());
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 260; ++i) {
        float az = hashf(i * 3 + 1) * 2.0f * PI;
        float el = 0.10f + hashf(i * 3 + 2) * (PI * 0.5f - 0.10f);
        float x, y, z;
        dirOf(az, el, x, y, z);

        float twinkle = 0.65f + 0.35f * sinf(t * (1.0f + hashf(i) * 2.5f) + i);
        float b = (0.55f + 0.45f * hashf(i * 7)) * twinkle;
        glColor3f(b, b, b * 1.05f > 1.0f ? 1.0f : b * 1.05f);
        float r = DOME_R * 0.96f;
        glVertex3f(x * r, y * r, z * r);
    }
    glEnd();
}

// Radial disc facing +Z: centre colour/alpha -> transparent edge
void glowDisc(float radius, const RGB& c, float centerAlpha, int segs = 48)
{
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(c.r, c.g, c.b, centerAlpha);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor4f(c.r, c.g, c.b, 0.0f);
    for (int i = 0; i <= segs; ++i) {
        float a = 2.0f * PI * i / segs;
        glVertex3f(radius * cosf(a), radius * sinf(a), 0.0f);
    }
    glEnd();
}

// Solid disc with a slightly darker rim
void solidDisc(float radius, const RGB& centre, const RGB& rim, int segs = 48)
{
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(centre.r, centre.g, centre.b);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(rim.r, rim.g, rim.b);
    for (int i = 0; i <= segs; ++i) {
        float a = 2.0f * PI * i / segs;
        glVertex3f(radius * cosf(a), radius * sinf(a), 0.0f);
    }
    glEnd();
}

void drawSunBody()
{
    const float t = static_cast<float>(glfwGetTime());

    // --- glow layers (additive, no depth writes) ---
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glowDisc(10.0f, {1.00f, 0.80f, 0.40f}, 0.28f);
    glowDisc( 6.0f, {1.00f, 0.88f, 0.50f}, 0.45f);
    glowDisc( 3.6f, {1.00f, 0.95f, 0.70f}, 0.70f);

    // --- slowly rotating rays ---
    glPushMatrix();
    glRotatef(t * 4.0f, 0.0f, 0.0f, 1.0f);
    const int RAYS = 14;
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < RAYS; ++i) {
        float a  = 2.0f * PI * i / RAYS;
        float w  = 0.10f;
        float len = (i % 2 == 0) ? 7.5f : 5.5f;
        glColor4f(1.0f, 0.92f, 0.55f, 0.35f);
        glVertex3f(2.0f * cosf(a - w), 2.0f * sinf(a - w), 0.0f);
        glVertex3f(2.0f * cosf(a + w), 2.0f * sinf(a + w), 0.0f);
        glColor4f(1.0f, 0.92f, 0.55f, 0.0f);
        glVertex3f(len * cosf(a), len * sinf(a), 0.0f);
    }
    glEnd();
    glPopMatrix();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    // --- bright core ---
    // Keep depth testing active so nearby scene geometry can occlude the sun.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    solidDisc(2.0f, {1.00f, 0.99f, 0.85f}, {1.00f, 0.88f, 0.40f});
}

void drawMoonBody()
{
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glowDisc(8.0f, {0.45f, 0.55f, 0.90f}, 0.22f);
    glowDisc(4.5f, {0.70f, 0.78f, 1.00f}, 0.40f);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    solidDisc(1.8f, {0.96f, 0.97f, 1.00f}, {0.82f, 0.85f, 0.94f});

    // craters
    struct Cr { float x, y, r; };
    const Cr craters[] = {
        {-0.5f,  0.4f, 0.30f}, { 0.6f,  0.1f, 0.22f},
        {-0.1f, -0.6f, 0.26f}, { 0.5f, -0.7f, 0.14f},
        {-0.9f, -0.3f, 0.15f},
    };
    for (const Cr& c : craters) {
        glPushMatrix();
        glTranslatef(c.x, c.y, 0.02f);
        solidDisc(c.r, {0.78f, 0.81f, 0.90f}, {0.86f, 0.89f, 0.96f}, 16);
        glPopMatrix();
    }
}

} // namespace


// ============================================================
//  Sky (dome) + Sun / Moon
// ============================================================
void drawSun()
{
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_POINT_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    // 1. Gradient sky (only visible where nothing else was drawn)
    glPushMatrix();
    drawDome();
    if (gNight) drawStars();
    glPopMatrix();

    // 2. Sun or moon
    glPushMatrix();
    glTranslatef(SUN_X, SUN_Y, SUN_Z);
    if (gNight) drawMoonBody();
    else        drawSunBody();
    glPopMatrix();

    glPopAttrib();
}


// ============================================================
//  Sky clear color (fallback behind the dome)
// ============================================================
void setClearColor(bool isNight)
{
    gNight = isNight;

    if (isNight) {
        glClearColor(0.05f, 0.07f, 0.17f, 1.0f);
    } else {
        glClearColor(0.62f, 0.80f, 0.96f, 1.0f);
    }
}

} // namespace Sky