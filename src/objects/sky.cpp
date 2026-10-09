#include "objects/sky.h"

#include "DayNightSettings.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace Sky {
namespace {

const float PI = 3.14159265f;

// Set before the frame clear so the sky and celestial bodies share one blend.
float gNightAmount = 0.0f;

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

// Rotate a local XY disc so its +Z normal points back toward the origin. The
// sky is rendered without camera translation, so the origin is always the
// viewer and celestial bodies remain circular from every look direction.
void faceViewer(float x, float y, float z)
{
    const float length = sqrtf(x * x + y * y + z * z);
    if (length <= 0.0001f) return;
    const float nx = -x / length;
    const float ny = -y / length;
    const float nz = -z / length;
    const float clampedZ = fmaxf(-1.0f, fminf(1.0f, nz));
    const float angle = acosf(clampedZ) * 180.0f / PI;
    const float axisX = -ny;
    const float axisY = nx;
    const float axisLength = sqrtf(axisX * axisX + axisY * axisY);
    if (axisLength > 0.0001f)
        glRotatef(angle, axisX / axisLength, axisY / axisLength, 0.0f);
    else if (nz < 0.0f)
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
}

// Sky colour for a direction. h = height (0 horizon .. 1 zenith),
// s = how much the direction points at the sun (0..1).
RGB skyColor(float h, float s)
{
    if (h < 0.0f) h = 0.0f;

    const RGB dayHorizon = {DayNightSettings::DaySkyHorizon[0],
        DayNightSettings::DaySkyHorizon[1], DayNightSettings::DaySkyHorizon[2]};
    const RGB dayMid = {DayNightSettings::DaySkyMid[0],
        DayNightSettings::DaySkyMid[1], DayNightSettings::DaySkyMid[2]};
    const RGB dayZenith = {DayNightSettings::DaySkyZenith[0],
        DayNightSettings::DaySkyZenith[1], DayNightSettings::DaySkyZenith[2]};
    RGB day = (h < 0.30f) ? mix(dayHorizon, dayMid, h / 0.30f)
                          : mix(dayMid, dayZenith, (h - 0.30f) / 0.70f);
    const float dayGlow = powf(s, 6.0f) * 0.45f;
    day.r = fminf(1.0f, day.r + dayGlow * 0.55f);
    day.g = fminf(1.0f, day.g + dayGlow * 0.38f);
    day.b = fminf(1.0f, day.b + dayGlow * 0.05f);

    const RGB nightHorizon = {DayNightSettings::NightSkyHorizon[0],
        DayNightSettings::NightSkyHorizon[1], DayNightSettings::NightSkyHorizon[2]};
    const RGB nightMid = {DayNightSettings::NightSkyMid[0],
        DayNightSettings::NightSkyMid[1], DayNightSettings::NightSkyMid[2]};
    const RGB nightZenith = {DayNightSettings::NightSkyZenith[0],
        DayNightSettings::NightSkyZenith[1], DayNightSettings::NightSkyZenith[2]};
    RGB night = (h < 0.35f) ? mix(nightHorizon, nightMid, h / 0.35f)
                            : mix(nightMid, nightZenith, (h - 0.35f) / 0.65f);
    const float moonGlow = powf(s, 8.0f) * 0.12f;
    night.r += moonGlow * 0.55f;
    night.g += moonGlow * 0.68f;
    night.b += moonGlow;
    return mix(day, night, gNightAmount);
}

// Gradient hemisphere, drawn with depth test ON so it only shows where
// nothing else was drawn (it sits behind everything).
void drawDome()
{
    const int RINGS = 16;
    const int SEGS  = 40;
    const float elMin = -0.14f;
    const float elMax = PI * 0.5f;

    // Follow the celestial body during the transition so its glow remains
    // visually attached to the sun or moon.
    const float bodyX = DayNightSettings::SunPosition[0]
        + (DayNightSettings::MoonPosition[0] - DayNightSettings::SunPosition[0])
            * gNightAmount;
    const float bodyY = DayNightSettings::SunPosition[1]
        + (DayNightSettings::MoonPosition[1] - DayNightSettings::SunPosition[1])
            * gNightAmount;
    const float bodyZ = DayNightSettings::SunPosition[2]
        + (DayNightSettings::MoonPosition[2] - DayNightSettings::SunPosition[2])
            * gNightAmount;
    const float sl = sqrtf(bodyX * bodyX + bodyY * bodyY + bodyZ * bodyZ);
    const float sx = bodyX / sl, sy = bodyY / sl, sz = bodyZ / sl;

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

void drawStars(float animationTime)
{
    const float t = animationTime;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 260; ++i) {
        float az = hashf(i * 3 + 1) * 2.0f * PI;
        float el = 0.10f + hashf(i * 3 + 2) * (PI * 0.5f - 0.10f);
        float x, y, z;
        dirOf(az, el, x, y, z);

        float twinkle = 0.65f + 0.35f * sinf(t * (1.0f + hashf(i) * 2.5f) + i);
        float b = (0.55f + 0.45f * hashf(i * 7)) * twinkle;
        glColor4f(b, b, b * 1.05f > 1.0f ? 1.0f : b * 1.05f,
                  gNightAmount);
        float r = DOME_R * 0.96f;
        glVertex3f(x * r, y * r, z * r);
    }
    glEnd();
    glDisable(GL_BLEND);
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
void solidDisc(float radius, const RGB& centre, const RGB& rim,
               float opacity = 1.0f, int segs = 48)
{
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(centre.r, centre.g, centre.b, opacity);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor4f(rim.r, rim.g, rim.b, opacity);
    for (int i = 0; i <= segs; ++i) {
        float a = 2.0f * PI * i / segs;
        glVertex3f(radius * cosf(a), radius * sinf(a), 0.0f);
    }
    glEnd();
}

void drawSunBody(float animationTime, float opacity)
{
    (void)animationTime;

    // --- glow layers (additive, no depth writes) ---
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glowDisc(5.0f, {1.00f, 0.82f, 0.44f}, 0.12f * opacity);
    glowDisc(3.0f, {1.00f, 0.90f, 0.58f}, 0.18f * opacity);
    glowDisc(1.8f, {1.00f, 0.96f, 0.76f}, 0.25f * opacity);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // The sky is rendered before the world with depth writes disabled. World
    // geometry therefore occludes the distant disc naturally and the sun can
    // never leave a near-depth stamp that hides scene objects.
    solidDisc(1.15f, {1.00f, 0.98f, 0.82f},
              {1.00f, 0.86f, 0.42f}, opacity, 64);
    glDisable(GL_BLEND);
}

void drawMoonBody(float opacity)
{
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glowDisc(8.0f, {0.45f, 0.55f, 0.90f}, 0.22f * opacity);
    glowDisc(4.5f, {0.70f, 0.78f, 1.00f}, 0.40f * opacity);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);

    solidDisc(1.8f, {0.96f, 0.97f, 1.00f}, {0.82f, 0.85f, 0.94f}, opacity);

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
        solidDisc(c.r, {0.78f, 0.81f, 0.90f}, {0.86f, 0.89f, 0.96f}, opacity, 16);
        glPopMatrix();
    }
    glDisable(GL_BLEND);
}

} // namespace


// ============================================================
//  Sky (dome) + Sun / Moon
// ============================================================
void drawSun(float animationTime)
{
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_POINT_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_FOG);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // 1. Gradient sky (only visible where nothing else was drawn)
    glPushMatrix();
    drawDome();
    if (gNightAmount > 0.001f) drawStars(animationTime);
    glPopMatrix();

    // 2. Sun or moon
    if (gNightAmount < 0.999f) {
        glPushMatrix();
        glTranslatef(DayNightSettings::SunPosition[0],
                     DayNightSettings::SunPosition[1],
                     DayNightSettings::SunPosition[2]);
        faceViewer(DayNightSettings::SunPosition[0],
                   DayNightSettings::SunPosition[1],
                   DayNightSettings::SunPosition[2]);
        drawSunBody(animationTime, 1.0f - gNightAmount);
        glPopMatrix();
    }
    if (gNightAmount > 0.001f) {
        glPushMatrix();
        glTranslatef(DayNightSettings::MoonPosition[0],
                     DayNightSettings::MoonPosition[1],
                     DayNightSettings::MoonPosition[2]);
        faceViewer(DayNightSettings::MoonPosition[0],
                   DayNightSettings::MoonPosition[1],
                   DayNightSettings::MoonPosition[2]);
        drawMoonBody(gNightAmount);
        glPopMatrix();
    }

    glPopAttrib();
}


// ============================================================
//  Sky clear color (fallback behind the dome)
// ============================================================
void setNightAmount(float nightAmount)
{
    gNightAmount = fmaxf(0.0f, fminf(1.0f, nightAmount));
    glClearColor(
        DayNightSettings::DayClear[0] +
            (DayNightSettings::NightClear[0] - DayNightSettings::DayClear[0]) * gNightAmount,
        DayNightSettings::DayClear[1] +
            (DayNightSettings::NightClear[1] - DayNightSettings::DayClear[1]) * gNightAmount,
        DayNightSettings::DayClear[2] +
            (DayNightSettings::NightClear[2] - DayNightSettings::DayClear[2]) * gNightAmount,
        1.0f);
}

} // namespace Sky
