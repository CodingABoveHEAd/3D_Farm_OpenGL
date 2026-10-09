#include "objects/Village.h"

#include "DayNightSettings.h"
#include "VillageSimulationSettings.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace Village {
namespace {
constexpr float Pi = 3.14159265358979323846f;
// Every reusable seat is authored to this surface height. The seated pelvis
// starts exactly at the surface, avoiding per-location offsets and keeping
// benches, tea-shop seats and bonfire logs on the same pose contract.
constexpr float SeatSurfaceY = 0.96f;
constexpr float SeatedHipY = SeatSurfaceY + 0.16f;
// Static shop architecture is compiled into one display list per variant.
// Set to false if Primitives creates its own lists lazily.
constexpr bool UseDisplayLists = true;

struct Color { float r, g, b; };
constexpr Color Wood{0.43f, 0.24f, 0.09f};
constexpr Color WoodDark{0.24f, 0.12f, 0.04f};
constexpr Color WoodLight{0.63f, 0.39f, 0.16f};
constexpr Color Skin{0.76f, 0.51f, 0.32f};
constexpr Color Metal{0.28f, 0.30f, 0.31f};
constexpr Color Aluminium{0.72f, 0.73f, 0.72f};
constexpr Color RustTin{0.47f, 0.30f, 0.20f};
constexpr Color Tin{0.55f, 0.57f, 0.56f};
constexpr Color Brick{0.58f, 0.25f, 0.17f};
constexpr Color Mortar{0.72f, 0.66f, 0.55f};
constexpr Color Plaster{0.84f, 0.78f, 0.64f};
constexpr Color Concrete{0.60f, 0.58f, 0.54f};
constexpr Color BambooC{0.74f, 0.64f, 0.30f};
constexpr Color BambooDark{0.52f, 0.42f, 0.18f};
constexpr Color BambooMat{0.68f, 0.57f, 0.31f};
constexpr Color Thatch{0.62f, 0.50f, 0.25f};
constexpr Color Clay{0.60f, 0.30f, 0.18f};
constexpr Color Mud{0.45f, 0.33f, 0.22f};
constexpr Color Soot{0.09f, 0.08f, 0.07f};
constexpr Color Teal{0.12f, 0.40f, 0.38f};
constexpr Color Cream{0.90f, 0.84f, 0.66f};

float hash01(int value)
{
    const float s = std::sin(value * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}
float fract01(float v) { return v - std::floor(v); }
Color tint(const Color& c, float k) { return {c.r * k, c.g * k, c.b * k}; }
// Subtle per-element material variation (weathering / hand-made look).
Color vary(const Color& c, int seed, float amount = 0.07f)
{
    return tint(c, 1.0f + (hash01(seed) - 0.5f) * 2.0f * amount);
}

// ---------------------------------------------------------------- primitives
void box(const Color& color, float x, float y, float z,
         float width, float height, float depth)
{
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCube(width, height, depth);
    glPopMatrix();
}

void sphere(const Color& color, float x, float y, float z,
            float sx, float sy, float sz, int slices = 10, int stacks = 7)
{
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawSphere(1.0f, slices, stacks);
    glPopMatrix();
}

// Vertical cylinder: base at (x,y,z), extends UP by h.
void column(const Color& c, float x, float y, float z, float r, float h, int slices = 10)
{
    glColor3f(c.r, c.g, c.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i)
    {
        const float a = 2.0f * Pi * i / slices;
        const float cs = std::cos(a), sn = std::sin(a);
        glNormal3f(cs, 0.0f, sn);
        glVertex3f(cs * r, 0.0f, sn * r);
        glVertex3f(cs * r, h, sn * r);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, h, 0.0f);
    for (int i = slices; i >= 0; --i)
    {
        const float a = 2.0f * Pi * i / slices;
        glVertex3f(std::cos(a) * r, h, std::sin(a) * r);
    }
    glEnd();
    glPopMatrix();
}

// Cylinder between two points (rails, braces, rafters, bicycle tubes...).
void strut(const Color& c, float x0, float y0, float z0,
           float x1, float y1, float z1, float r, int slices = 8)
{
    const float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f) return;
    glPushMatrix();
    glTranslatef(x0, y0, z0);
    const float ax = dz, az = -dx; // (0,1,0) x d
    if (std::sqrt(ax * ax + az * az) > 1e-4f)
        glRotatef(std::acos(std::max(-1.0f, std::min(1.0f, dy / len))) * 180.0f / Pi,
                  ax, 0.0f, az);
    else if (dy < 0.0f)
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    column(c, 0.0f, 0.0f, 0.0f, r, len, slices);
    glPopMatrix();
}

// Flat ring in the local XY plane (wheel tyres).
void ring(const Color& c, float rOut, float rIn, float thick, int seg = 20)
{
    glColor3f(c.r, c.g, c.b);
    for (int face = 0; face < 2; ++face)
    {
        const float z = face ? -thick * 0.5f : thick * 0.5f;
        glBegin(GL_QUAD_STRIP);
        glNormal3f(0.0f, 0.0f, face ? -1.0f : 1.0f);
        for (int i = 0; i <= seg; ++i)
        {
            const float a = 2.0f * Pi * i / seg;
            const float cs = std::cos(a), sn = std::sin(a);
            if (face == 0) { glVertex3f(cs * rIn, sn * rIn, z); glVertex3f(cs * rOut, sn * rOut, z); }
            else           { glVertex3f(cs * rOut, sn * rOut, z); glVertex3f(cs * rIn, sn * rIn, z); }
        }
        glEnd();
    }
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= seg; ++i)
    {
        const float a = 2.0f * Pi * i / seg;
        const float cs = std::cos(a), sn = std::sin(a);
        glNormal3f(cs, sn, 0.0f);
        glVertex3f(cs * rOut, sn * rOut, -thick * 0.5f);
        glVertex3f(cs * rOut, sn * rOut, thick * 0.5f);
    }
    glEnd();
}

void groundPatch(const Color& c, float x, float z, float w, float d, float y = 0.025f)
{
    glColor3f(c.r, c.g, c.b);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(x - w * 0.5f, y, z - d * 0.5f);
    glVertex3f(x - w * 0.5f, y, z + d * 0.5f);
    glVertex3f(x + w * 0.5f, y, z + d * 0.5f);
    glVertex3f(x + w * 0.5f, y, z - d * 0.5f);
    glEnd();
}

// ------------------------------------------------- lighting / atmosphere
// Soft warm light pool (additive, depth-write off). horizontal = floor/counter
// decal, otherwise a wall decal facing +Z.
void glow(float x, float y, float z, float radius, float strength, bool horizontal)
{
    if (strength < 0.02f) return;
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glPushMatrix();
    glTranslatef(x, y, z);
    if (!horizontal) glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    const float r = DayNightSettings::WarmLamp[0] * 0.9f;
    const float g = DayNightSettings::WarmLamp[1] * 0.8f;
    const float b = DayNightSettings::WarmLamp[2] * 0.6f;
    const int seg = 20;
    const float a0 = 0.42f * strength;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, a0);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= seg; ++i)
    {
        const float a = 2.0f * Pi * i / seg;
        glColor4f(r, g, b, a0 * 0.30f);
        glVertex3f(std::cos(a) * radius * 0.5f, 0.0f, std::sin(a) * radius * 0.5f);
    }
    glEnd();
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= seg; ++i)
    {
        const float a = 2.0f * Pi * i / seg;
        const float cs = std::cos(a), sn = std::sin(a);
        glColor4f(r, g, b, a0 * 0.30f);
        glVertex3f(cs * radius * 0.5f, 0.0f, sn * radius * 0.5f);
        glColor4f(r, g, b, 0.0f);
        glVertex3f(cs * radius, 0.0f, sn * radius);
    }
    glEnd();
    glPopMatrix();
    glPopAttrib();
}

void emissiveSphere(const Color& base, float x, float y, float z, float rad, float night)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT);
    const GLfloat emission[] = {DayNightSettings::WarmLamp[0] * night,
                                DayNightSettings::WarmLamp[1] * night,
                                DayNightSettings::WarmLamp[2] * night, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    sphere(base, x, y, z, rad, rad, rad, 10, 7);
    glPopAttrib();
}

void hangingLamp(float x, float yBulb, float z, float cordTop, float night)
{
    strut(Soot, x, yBulb + 0.15f, z, x, cordTop, z, 0.015f, 5);
    sphere(WoodDark, x, yBulb + 0.15f, z, 0.20f, 0.09f, 0.20f, 10, 5); // shade
    emissiveSphere({1.0f, 0.93f, 0.72f}, x, yBulb, z, 0.09f, night);
}

void fireGlow(float x, float y, float z, float time)
{
    const float f = 0.70f + 0.20f * std::sin(time * 9.0f + x) + 0.10f * std::sin(time * 17.0f);
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT);
    const GLfloat emission[] = {0.95f * f, 0.38f * f, 0.05f * f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    box({0.95f, 0.45f, 0.10f}, x, y, z, 0.22f, 0.16f, 0.03f);
    glPopAttrib();
}

void steam(float x, float y, float z, float time, float phase)
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < 5; ++i)
    {
        const float t = fract01(time * 0.22f + phase + i * 0.2f);
        const float alpha = 0.26f * (1.0f - t) * std::min(1.0f, t * 6.0f);
        glColor4f(0.96f, 0.96f, 0.94f, alpha);
        glPushMatrix();
        glTranslatef(x + std::sin(time * 0.9f + i * 1.7f + phase * 6.0f) * 0.07f * (0.3f + t * 2.5f),
                     y + t * 1.1f, z + t * 0.12f);
        const float s = 0.05f + 0.14f * t;
        glScalef(s, s * 0.9f, s);
        Primitives::drawSphere(1.0f, 8, 5);
        glPopMatrix();
    }
    glPopAttrib();
}

// ------------------------------------------------------------- roofs
// Corrugated sheet roof centred at (x,y,z); positive pitch drops the +Z edge.
void corrugatedRoof(const Color& c, float x, float y, float z,
                    float width, float depth, float pitch, int seed)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    box(tint(c, 0.75f), 0.0f, 0.0f, 0.0f, width, 0.10f, depth);
    const int ribs = static_cast<int>(width / 0.30f);
    for (int i = 0; i < ribs; ++i)
    {
        const float rx = -width * 0.5f + (i + 0.5f) * width / ribs;
        box(vary(c, seed * 31 + i, 0.10f), rx, 0.06f, 0.0f, 0.15f, 0.05f, depth + 0.02f);
    }
    box(tint(c, 0.60f), 0.0f, -0.03f, depth * 0.5f, width + 0.04f, 0.15f, 0.05f); // drip edge
    glPopMatrix();
}

void thatchRoof(float x, float y, float z, float width, float depth, float pitch, int seed)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    box(tint(Thatch, 0.7f), 0.0f, 0.0f, 0.0f, width, 0.28f, depth);
    const int bands = 6;
    for (int b = 0; b < bands; ++b)  // overlapping layers read as bundled thatch
    {
        const float bz = -depth * 0.5f + (b + 0.5f) * depth / bands;
        box(vary(Thatch, seed * 7 + b, 0.12f), 0.0f, 0.14f + 0.012f * b, bz,
            width, 0.07f, depth / bands + 0.12f);
    }
    for (int i = 0; i < 36; ++i)    // straw fringe along the front edge
    {
        const float fx = -width * 0.5f + 0.12f + i * (width - 0.24f) / 35.0f;
        const float fl = 0.30f + 0.18f * hash01(seed * 53 + i);
        box(vary(Thatch, seed * 11 + i, 0.15f), fx, -0.14f - fl * 0.5f, depth * 0.5f + 0.01f,
            0.09f, fl, 0.07f);
    }
    glPopMatrix();
}

// ------------------------------------------------------- tea-shop props
void drawCup(float x, float y, float z)
{
    column(Cream, x, y, z, 0.075f, 0.13f, 10);
    column({0.30f, 0.15f, 0.06f}, x, y + 0.122f, z, 0.066f, 0.012f, 10); // tea surface
    box(Cream, x + 0.09f, y + 0.07f, z, 0.035f, 0.07f, 0.03f);          // handle
}

void drawBenchStyled(float length, const Color& seat, const Color& leg, const Color& back)
{
    for (float x : {-length * 0.38f, length * 0.38f})
    {
        box(leg, x, 0.42f, 0.0f, 0.22f, 0.82f, 0.72f);
        box(leg, x, 1.26f, -0.36f, 0.18f, 1.25f, 0.18f);
    }
    box(seat, 0.0f, SeatSurfaceY - 0.10f, 0.02f,
        length, 0.20f, 0.78f);
    for (float y : {1.22f, 1.58f})
        box(back, 0.0f, y, -0.43f, length, 0.23f, 0.16f);
    box(Metal, 0.0f, 0.72f, 0.02f, length + 0.12f, 0.08f, 0.08f);
}

void drawBench(float length = 4.4f)
{
    drawBenchStyled(length, Wood, WoodDark, WoodLight);
}

void drawStool(const Color& c, float x, float z)
{
    column(tint(c, 1.1f), x, 0.87f, z, 0.40f, 0.09f, 12);
    for (int i = 0; i < 4; ++i)
    {
        const float a = Pi * 0.25f + i * Pi * 0.5f;
        strut(tint(c, 0.8f), x + std::cos(a) * 0.24f, 0.90f, z + std::sin(a) * 0.24f,
              x + std::cos(a) * 0.34f, 0.0f, z + std::sin(a) * 0.34f, 0.045f, 6);
    }
    column(tint(c, 0.7f), x, 0.38f, z, 0.30f, 0.04f, 10); // foot ring
}

void drawTable(float w, float d, const Color& top, const Color& leg)
{
    box(top, 0.0f, 1.28f, 0.0f, w, 0.08f, d);
    box(tint(top, 0.65f), 0.0f, 1.20f, 0.0f, w - 0.14f, 0.08f, d - 0.14f);
    for (float sx : {-1.0f, 1.0f})
        for (float sz : {-1.0f, 1.0f})
            column(leg, sx * (w * 0.5f - 0.1f), 0.0f, sz * (d * 0.5f - 0.1f), 0.055f, 1.24f, 8);
}

void drawTableware(int seed)
{
    column(Cream, -0.45f, 1.32f, 0.0f, 0.20f, 0.02f, 12);                 // biscuit plate
    for (int i = 0; i < 4; ++i)
        sphere(vary({0.70f, 0.48f, 0.22f}, seed + i, 0.15f),
               -0.45f + (i % 2 - 0.5f) * 0.16f, 1.37f + (i / 2) * 0.04f, (i / 2 - 0.5f) * 0.10f,
               0.07f, 0.025f, 0.07f, 8, 4);
    drawCup(0.05f, 1.32f, 0.20f);
    drawCup(0.30f, 1.32f, -0.18f);
    column({0.18f, 0.40f, 0.52f}, 0.58f, 1.32f, 0.10f, 0.07f, 0.34f, 8);   // flask
    column(Metal, 0.58f, 1.66f, 0.10f, 0.045f, 0.04f, 8);
}

void drawBiscuitJar(int style)
{
    static const Color contents[] = {{0.78f, 0.55f, 0.24f}, {0.86f, 0.80f, 0.62f}, {0.62f, 0.34f, 0.16f}};
    static const Color lids[] = {{0.75f, 0.12f, 0.10f}, {0.20f, 0.40f, 0.62f}, {0.85f, 0.70f, 0.15f}};
    const int s = ((style % 3) + 3) % 3;
    column(contents[s], 0.0f, 0.0f, 0.0f, 0.19f, 0.36f, 12);
    column(tint(contents[s], 0.65f), 0.0f, 0.31f, 0.0f, 0.205f, 0.04f, 12);
    box({0.92f, 0.88f, 0.78f}, 0.0f, 0.17f, 0.19f, 0.20f, 0.14f, 0.02f); // label
    column(lids[s], 0.0f, 0.36f, 0.0f, 0.215f, 0.08f, 12);
    sphere(lids[s], 0.0f, 0.46f, 0.0f, 0.05f, 0.05f, 0.05f, 6, 4);
}

void drawShelfUnit(float width, int variant)
{
    box(WoodDark, -width * 0.5f, 0.70f, 0.0f, 0.08f, 1.55f, 0.50f);
    box(WoodDark,  width * 0.5f, 0.70f, 0.0f, 0.08f, 1.55f, 0.50f);
    for (float y : {0.0f, 0.75f})
    {
        box(Wood, 0.0f, y, 0.0f, width, 0.08f, 0.50f);
        const int count = std::max(2, static_cast<int>(width / 0.62f));
        for (int i = 0; i < count; ++i)
        {
            const float ix = -width * 0.5f + 0.45f + i * (width - 0.9f) / (count - 1);
            const int kind = (i + variant + (y > 0.0f ? 1 : 0)) % 4;
            glPushMatrix();
            glTranslatef(ix, y + 0.04f, 0.0f);
            if (kind < 2) drawBiscuitJar(i + variant);
            else if (kind == 2)
            {
                box(vary({0.72f, 0.60f, 0.20f}, i + variant * 5, 0.2f), 0.0f, 0.22f, 0.0f, 0.34f, 0.44f, 0.30f);
                box({0.85f, 0.82f, 0.70f}, 0.0f, 0.22f, 0.155f, 0.24f, 0.18f, 0.01f);
            }
            else
            {
                column({0.30f, 0.52f, 0.34f}, -0.10f, 0.0f, 0.0f, 0.085f, 0.36f, 8);
                column({0.30f, 0.52f, 0.34f}, -0.10f, 0.36f, 0.0f, 0.035f, 0.10f, 8);
                column({0.55f, 0.35f, 0.15f}, 0.12f, 0.0f, 0.0f, 0.085f, 0.36f, 8);
                column({0.55f, 0.35f, 0.15f}, 0.12f, 0.36f, 0.0f, 0.035f, 0.10f, 8);
            }
            glPopMatrix();
        }
    }
}

// Kettle at the origin, spout towards +X, base on y=0 (about 0.75 tall).
void drawKettle()
{
    sphere(Aluminium, 0.0f, 0.30f, 0.0f, 0.30f, 0.26f, 0.30f, 14, 9);
    column(tint(Aluminium, 0.85f), 0.0f, 0.0f, 0.0f, 0.26f, 0.05f, 12);
    column(tint(Aluminium, 0.9f), 0.0f, 0.52f, 0.0f, 0.15f, 0.05f, 12);
    sphere(Soot, 0.0f, 0.62f, 0.0f, 0.045f, 0.045f, 0.045f, 6, 4);
    strut(Aluminium, 0.22f, 0.26f, 0.0f, 0.46f, 0.50f, 0.0f, 0.05f, 8);   // spout
    strut(WoodDark, -0.22f, 0.48f, 0.0f, 0.0f, 0.78f, 0.0f, 0.03f, 6);    // bail handle
    strut(WoodDark, 0.22f, 0.48f, 0.0f, 0.0f, 0.78f, 0.0f, 0.03f, 6);
}

// Clay stove at the origin: 0.70 tall, firing mouth towards +Z.
void drawClayStove()
{
    column(Clay, 0.0f, 0.0f, 0.0f, 0.52f, 0.38f, 12);
    column(tint(Clay, 0.92f), 0.0f, 0.38f, 0.0f, 0.46f, 0.32f, 12);
    column(Soot, 0.0f, 0.66f, 0.0f, 0.38f, 0.05f, 12);
    box(Soot, 0.0f, 0.24f, 0.46f, 0.34f, 0.28f, 0.10f);
}

void drawCupTray(float x, float y, float z)
{
    box(Metal, x, y + 0.015f, z, 1.0f, 0.03f, 0.50f);
    for (int i = 0; i < 6; ++i)
        drawCup(x - 0.36f + (i % 3) * 0.36f, y + 0.03f, z + (i / 3 - 0.5f) * 0.24f);
}

void drawSnackPackets(float x, float y, float z)
{
    static const Color cols[] = {{0.85f, 0.20f, 0.15f}, {0.15f, 0.45f, 0.75f}, {0.90f, 0.75f, 0.15f},
                                 {0.20f, 0.60f, 0.30f}, {0.80f, 0.35f, 0.65f}};
    box(WoodDark, x, y + 0.02f, z, 1.5f, 0.04f, 0.32f);
    for (int i = 0; i < 5; ++i)
    {
        glPushMatrix();
        glTranslatef(x - 0.60f + i * 0.30f, y + 0.22f, z - 0.06f);
        glRotatef(-12.0f, 1.0f, 0.0f, 0.0f);
        box(cols[i], 0.0f, 0.0f, 0.0f, 0.24f, 0.36f, 0.03f);
        box({0.95f, 0.92f, 0.85f}, 0.0f, 0.02f, 0.02f, 0.14f, 0.10f, 0.01f);
        glPopMatrix();
    }
}

void drawSign(const Color& board, const Color& fg, float x, float y, float z, float w, float h)
{
    box(board, x, y, z, w, h, 0.07f);
    box(fg, x, y + h * 0.5f - 0.04f, z + 0.01f, w, 0.05f, 0.08f);
    box(fg, x, y - h * 0.5f + 0.04f, z + 0.01f, w, 0.05f, 0.08f);
    box(fg, x - w * 0.5f + 0.04f, y, z + 0.01f, 0.05f, h, 0.08f);
    box(fg, x + w * 0.5f - 0.04f, y, z + 0.01f, 0.05f, h, 0.08f);
    const float gx = x - w * 0.32f; // painted tea-cup emblem
    column(fg, gx, y - h * 0.26f, z + 0.04f, 0.09f, 0.15f, 10);
    box(fg, gx, y - h * 0.28f, z + 0.04f, 0.30f, 0.025f, 0.20f);
    box(fg, gx + 0.12f, y - h * 0.18f, z + 0.04f, 0.05f, 0.08f, 0.03f);
    for (int i = 0; i < 3; ++i)
        box(fg, gx - 0.04f + i * 0.04f, y + h * 0.12f, z + 0.045f, 0.015f, h * 0.22f, 0.02f);
    const float widths[] = {0.34f, 0.22f, 0.42f, 0.28f};
    for (int row = 0; row < 2; ++row)
    {
        float cx = x - w * 0.10f;
        for (int i = 0; i < 4; ++i)  // lettering suggested by dashes
        {
            const float ww = widths[(i + row) % 4] * w / 3.2f;
            box(fg, cx + ww * 0.5f, y + h * 0.14f - row * h * 0.30f, z + 0.045f, ww, 0.07f, 0.02f);
            cx += ww + 0.07f;
        }
    }
}

void drawPlant(float x, float y, float z, int seed)
{
    column(Clay, x, y, z, 0.24f, 0.40f, 10);
    column(tint(Clay, 1.1f), x, y + 0.38f, z, 0.29f, 0.08f, 10);
    for (int i = 0; i < 5; ++i)
    {
        const float a = i * 1.26f + seed;
        sphere(vary({0.14f, 0.42f, 0.12f}, seed * 9 + i, 0.2f),
               x + std::cos(a) * 0.12f, y + 0.62f + 0.11f * (i % 3), z + std::sin(a) * 0.12f,
               0.17f, 0.22f, 0.17f, 8, 5);
    }
}

void drawBin(float x, float z)
{
    column({0.16f, 0.30f, 0.18f}, x, 0.0f, z, 0.30f, 0.85f, 12);
    column({0.10f, 0.20f, 0.12f}, x, 0.85f, z, 0.33f, 0.06f, 12);
}

// Bicycle in the local XY plane, wheels on y=0 (origin between the wheels).
void drawBicycle()
{
    const Color frame{0.12f, 0.22f, 0.32f};
    for (float wx : {-0.95f, 0.95f})
    {
        glPushMatrix();
        glTranslatef(wx, 0.58f, 0.0f);
        ring(Soot, 0.58f, 0.52f, 0.08f, 20);
        ring(Metal, 0.52f, 0.49f, 0.05f, 20);
        for (int i = 0; i < 4; ++i)
        {
            const float a = i * Pi * 0.25f;
            strut(Metal, std::cos(a) * 0.5f, std::sin(a) * 0.5f, 0.0f,
                  -std::cos(a) * 0.5f, -std::sin(a) * 0.5f, 0.0f, 0.012f, 4);
        }
        strut(Metal, 0.0f, 0.0f, -0.06f, 0.0f, 0.0f, 0.06f, 0.04f, 6);
        glPopMatrix();
    }
    strut(frame, -0.95f, 0.58f, 0.0f, -0.10f, 0.45f, 0.0f, 0.03f);
    strut(frame, -0.10f, 0.45f, 0.0f, -0.25f, 1.20f, 0.0f, 0.03f);
    strut(frame, -0.95f, 0.58f, 0.0f, -0.25f, 1.20f, 0.0f, 0.025f);
    strut(frame, -0.25f, 1.20f, 0.0f, 0.72f, 1.15f, 0.0f, 0.03f);
    strut(frame, -0.10f, 0.45f, 0.0f, 0.80f, 0.95f, 0.0f, 0.032f);
    strut(Metal, 0.72f, 1.15f, 0.0f, 0.95f, 0.58f, 0.0f, 0.025f);
    strut(Metal, 0.72f, 1.15f, 0.0f, 0.70f, 1.32f, 0.0f, 0.03f);
    strut(Metal, 0.70f, 1.32f, -0.36f, 0.70f, 1.32f, 0.36f, 0.025f);
    strut(Metal, -0.25f, 1.20f, 0.0f, -0.27f, 1.30f, 0.0f, 0.03f);
    box(Soot, -0.30f, 1.33f, 0.0f, 0.45f, 0.07f, 0.20f);
    box(Metal, -0.95f, 0.92f, 0.0f, 0.55f, 0.04f, 0.26f);
    box(WoodDark, 1.05f, 1.05f, 0.0f, 0.28f, 0.20f, 0.30f);   // front basket
}

// ------------------------------------------------------------- villagers
// ===================================================================
// Villager (replaces the old limb() + drawVillager())
// ===================================================================
struct V3 { float x, y, z; };

float smooth01(float t)
{
    t = std::max(0.0f, std::min(1.0f, t));
    return t * t * (3.0f - 2.0f * t);
}

// Cone frustum, base at origin, extends up by h.
void frustum(const Color& c, float r0, float r1, float h, int slices)
{
    glColor3f(c.r, c.g, c.b);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i)
    {
        const float a = 2.0f * Pi * i / slices;
        const float cs = std::cos(a), sn = std::sin(a);
        glNormal3f(cs, (r0 - r1) / h, sn);
        glVertex3f(cs * r0, 0.0f, sn * r0);
        glVertex3f(cs * r1, h, sn * r1);
    }
    glEnd();
}

// Tapered limb segment between two points.
void taper(const Color& c, const V3& a, const V3& b, float r0, float r1, int slices = 9)
{
    const float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f) return;
    glPushMatrix();
    glTranslatef(a.x, a.y, a.z);
    if (std::sqrt(dz * dz + dx * dx) > 1e-4f)
        glRotatef(std::acos(std::max(-1.0f, std::min(1.0f, dy / len))) * 180.0f / Pi, dz, 0.0f, -dx);
    else if (dy < 0.0f)
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    frustum(c, r0, r1, len, slices);
    glPopMatrix();
}

struct Ring { float y, rx, rz; };

// Elliptical loft through a list of rings (torso / pelvis shaping).
void loft(const Color& c, float baseY, const Ring* rings, int n, int slices = 14)
{
    glColor3f(c.r, c.g, c.b);
    glPushMatrix();
    glTranslatef(0.0f, baseY, 0.0f);
    for (int k = 0; k < n - 1; ++k)
    {
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= slices; ++i)
        {
            const float a = 2.0f * Pi * i / slices;
            const float cs = std::cos(a), sn = std::sin(a);
            float nx = cs / rings[k].rx, nz = sn / rings[k].rz;
            const float nl = std::sqrt(nx * nx + nz * nz);
            glNormal3f(nx / nl, 0.1f, nz / nl);
            glVertex3f(cs * rings[k].rx, rings[k].y, sn * rings[k].rz);
            glVertex3f(cs * rings[k + 1].rx, rings[k + 1].y, sn * rings[k + 1].rz);
        }
        glEnd();
    }
    glPopMatrix();
}

// Two-bone arm. The hand is clamped to reach; returns the final hand position.
V3 drawArm(const V3& s, V3 target, float side, const Color& sleeve,
           const Color& skin, bool rolled)
{
    const float l1 = 0.56f, l2 = 0.52f;
    V3 d{target.x - s.x, target.y - s.y, target.z - s.z};
    float dist = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    dist = std::max(dist, 0.05f);
    const float reach = std::min(dist, l1 + l2 - 0.01f);
    const V3 u{d.x / dist, d.y / dist, d.z / dist};
    const V3 hand{s.x + u.x * reach, s.y + u.y * reach, s.z + u.z * reach};

    // Elbow: circle-intersection, bent outward / down / back.
    const float a = (l1 * l1 - l2 * l2 + reach * reach) / (2.0f * reach);
    const float h = std::sqrt(std::max(0.0f, l1 * l1 - a * a));
    V3 pole{side * 0.7f, -0.7f, -0.5f};
    const float pd = pole.x * u.x + pole.y * u.y + pole.z * u.z;
    pole = {pole.x - u.x * pd, pole.y - u.y * pd, pole.z - u.z * pd};
    const float pl = std::sqrt(pole.x * pole.x + pole.y * pole.y + pole.z * pole.z);
    if (pl > 1e-4f) { pole.x /= pl; pole.y /= pl; pole.z /= pl; }
    const V3 elbow{s.x + u.x * a + pole.x * h, s.y + u.y * a + pole.y * h, s.z + u.z * a + pole.z * h};

    sphere(sleeve, s.x, s.y, s.z, 0.115f, 0.115f, 0.115f, 9, 7);            // shoulder
    taper(sleeve, s, elbow, 0.088f, 0.070f);
    sphere(rolled ? skin : sleeve, elbow.x, elbow.y, elbow.z, 0.068f, 0.068f, 0.068f, 8, 6);
    taper(rolled ? skin : sleeve, elbow, hand, 0.064f, 0.046f);
    sphere(skin, hand.x, hand.y, hand.z, 0.055f, 0.068f, 0.050f, 8, 6);      // hand
    return hand;
}

void drawVillager(bool seated, float phase, float time, bool holdingCup,
                  bool driving = false)
{
    const Color shirts[] = {
        {0.68f, 0.18f, 0.14f}, {0.16f, 0.42f, 0.26f},
        {0.20f, 0.38f, 0.68f}, {0.78f, 0.56f, 0.15f}};
    const Color shirt = shirts[static_cast<int>(phase * 3.0f) & 3];

    // Per-person variety, derived from the phase so every villager differs.
    const int id = static_cast<int>(phase * 10.0f);
    const Color skin = vary(Skin, id * 17 + 3, 0.09f);
    const Color trouserSet[] = {{0.16f, 0.22f, 0.34f}, {0.24f, 0.21f, 0.17f}, {0.62f, 0.58f, 0.48f}};
    const Color pants = trouserSet[id % 3];
    const bool elder = hash01(id * 5 + 1) > 0.72f;
    const Color hair = elder ? Color{0.62f, 0.61f, 0.58f} : Color{0.07f, 0.05f, 0.04f};
    const bool beard  = hash01(id * 7 + 2) > 0.55f;
    const bool cap    = hash01(id * 3 + 4) > 0.68f;
    const bool scarf  = hash01(id * 11 + 6) > 0.60f;
    const bool rolled = hash01(id * 13 + 8) > 0.50f;
    const Color shoe{0.12f, 0.07f, 0.035f};

    // Animation (same rates as before).
    const float g = std::sin(time * (0.75f + phase * 0.08f) + phase);
    const float sipWave = std::sin(time * 0.48f + phase * 2.3f);
    const float sip = holdingCup ? smooth01((sipWave - 0.30f) / 0.55f) : 0.0f;
    const float headTurn = std::sin(time * 0.31f + phase)
        * (driving ? 3.0f : 14.0f);
    const float hipY = seated ? SeatedHipY : 1.45f;

    // ---- Legs and shoes ----------------------------------------------------
    for (float side : {-1.0f, 1.0f})
    {
        V3 hip, knee, ankle;
        if (seated)
        {   // thigh slopes down from the seat so the feet reach the ground
            hip   = {side * 0.18f, 1.07f, 0.00f};
            knee  = {side * 0.19f, 0.84f, 0.52f};
            ankle = {side * 0.19f, 0.13f, 0.58f};
        }
        else
        {
            hip   = {side * 0.16f, hipY - 0.05f, 0.00f};
            knee  = {side * 0.17f, 0.78f, 0.03f};
            ankle = {side * 0.17f, 0.14f, 0.00f};
        }
        taper(pants, hip, knee, 0.125f, 0.095f);
        sphere(pants, knee.x, knee.y, knee.z, 0.097f, 0.097f, 0.097f, 8, 6);
        taper(pants, knee, ankle, 0.092f, 0.062f);

        glPushMatrix();
        glTranslatef(ankle.x, 0.0f, ankle.z);
        glRotatef(side * 6.0f, 0.0f, 1.0f, 0.0f);
        sphere(shoe, 0.0f, 0.065f, 0.07f, 0.095f, 0.065f, 0.14f, 9, 6);
        sphere(shoe, 0.0f, 0.050f, 0.15f, 0.085f, 0.045f, 0.07f, 8, 5);
        glPopMatrix();
    }

    // ---- Pelvis (trousers) and torso (shirt) ------------------------------
    const Ring pelvis[] = {{-0.16f, 0.30f, 0.20f}, {-0.02f, 0.345f, 0.225f}, {0.12f, 0.325f, 0.215f}};
    loft(pants, hipY, pelvis, 3);
    const Ring torso[] = {{0.00f, 0.350f, 0.235f}, {0.22f, 0.300f, 0.205f}, {0.55f, 0.335f, 0.225f},
                          {0.80f, 0.385f, 0.225f}, {0.94f, 0.340f, 0.190f}, {1.00f, 0.150f, 0.120f}};
    loft(shirt, hipY, torso, 6);
    if (scarf)
    {
        const Ring wrap[] = {{0.92f, 0.370f, 0.235f}, {1.04f, 0.200f, 0.150f}};
        loft({0.75f, 0.15f, 0.12f}, hipY, wrap, 2);
    }

    // Neck.
    taper(skin, {0.0f, hipY + 0.98f, 0.0f}, {0.0f, hipY + 1.10f, 0.01f}, 0.085f, 0.075f);

    // ---- Arms ---------------------------------------------------------------
    for (float side : {-1.0f, 1.0f})
    {
        const V3 shoulder{side * 0.41f, hipY + 0.86f, 0.0f};
        const V3 rest = seated ? V3{side * 0.27f, 1.13f, 0.32f}
                               : V3{side * 0.47f, hipY - 0.12f, 0.04f};
        const bool cupArm = holdingCup && side > 0.0f;

        V3 target;
        if (driving)
        {
            // Both hands reach the tractor wheel.  The target is deliberately
            // forward of the ordinary seated pose and is clamped by the
            // two-bone arm solver, keeping elbows inside the cabin.
            target = {side * 0.30f, hipY + 0.66f, 1.18f};
        }
        else if (cupArm)
        {   // hold the cup at chest height, lift it to the lips when sipping
            const V3 hold{0.30f, hipY + 0.50f, 0.34f};
            const V3 mouth{0.06f, hipY + 1.04f, 0.30f};
            target = {hold.x + (mouth.x - hold.x) * sip,
                      hold.y + (mouth.y - hold.y) * sip,
                      hold.z + (mouth.z - hold.z) * sip};
        }
        else
        {   // conversational gesture: hand lifts forward now and then
            const float gp = smooth01(((side > 0.0f ? g : -g) - 0.10f) / 0.80f);
            target = {rest.x - side * 0.05f * gp,
                      rest.y + (seated ? 0.35f : 0.45f) * gp,
                      rest.z + (seated ? 0.15f : 0.30f) * gp};
        }
        const V3 hand = drawArm(shoulder, target, side, shirt, skin, rolled);
        if (cupArm)
        {
            glPushMatrix();
            glTranslatef(hand.x, hand.y - 0.05f, hand.z);
            glRotatef(-38.0f * sip, 1.0f, 0.0f, 0.0f);
            drawCup(0.0f, 0.0f, 0.0f);
            glPopMatrix();
        }
    }

    // ---- Head ----------------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, hipY + 1.08f, 0.0f);
    glRotatef(headTurn, 0.0f, 1.0f, 0.0f);
    glRotatef(-10.0f * sip, 1.0f, 0.0f, 0.0f);   // tilt back while drinking
    glTranslatef(0.0f, 0.22f, 0.0f);

    sphere(skin, 0.0f, 0.0f, 0.0f, 0.200f, 0.260f, 0.220f, 14, 10);        // skull
    sphere(skin, 0.0f, -0.14f, 0.045f, 0.145f, 0.115f, 0.155f, 10, 7);     // jaw / chin
    for (float side : {-1.0f, 1.0f})
    {
        sphere(skin, side * 0.198f, -0.01f, -0.01f, 0.040f, 0.060f, 0.030f, 8, 5);   // ears
        sphere({0.93f, 0.92f, 0.88f}, side * 0.08f, 0.04f, 0.192f, 0.030f, 0.020f, 0.020f, 8, 5);
        sphere({0.05f, 0.03f, 0.02f}, side * 0.08f, 0.04f, 0.206f, 0.014f, 0.014f, 0.010f, 6, 4);
        sphere(hair, side * 0.08f, 0.092f, 0.185f, 0.045f, 0.010f, 0.020f, 6, 4);    // brows
    }
    sphere(tint(skin, 0.96f), 0.0f, -0.04f, 0.215f, 0.030f, 0.045f, 0.035f, 8, 6);   // nose

    if (cap)
    {
        sphere(hair, 0.0f, -0.02f, -0.07f, 0.215f, 0.200f, 0.210f, 12, 8);
        sphere({0.92f, 0.90f, 0.84f}, 0.0f, 0.12f, -0.01f, 0.215f, 0.140f, 0.225f, 12, 7);
    }
    else
        sphere(hair, 0.0f, 0.07f, -0.05f, 0.225f, 0.235f, 0.235f, 12, 9);

    if (beard)
    {
        sphere(hair, 0.0f, -0.13f, 0.075f, 0.165f, 0.125f, 0.150f, 10, 7);
        sphere(hair, 0.0f, -0.075f, 0.205f, 0.060f, 0.015f, 0.020f, 6, 4);          // moustache
    }
    else
        sphere({0.45f, 0.22f, 0.18f}, 0.0f, -0.11f, 0.200f, 0.045f, 0.010f, 0.015f, 6, 4);
    glPopMatrix();
}

// Bench + table group. In its local frame villagers face +Z, the backrest is
// at -Z and the table sits in front, so the group's yaw decides what it faces.
struct Seating { float x, floorY, z, yaw, length; };

void drawSeatingStatic(const Seating& s, const Color& seat, const Color& leg,
                       const Color& table, int seed)
{
    glPushMatrix();
    glTranslatef(s.x, s.floorY, s.z);
    glRotatef(s.yaw, 0.0f, 1.0f, 0.0f);
    drawBenchStyled(s.length, seat, leg, tint(seat, 1.12f));
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 1.25f);
    drawTable(1.7f, 0.9f, table, leg);
    drawTableware(seed);
    glPopMatrix();
    glPopMatrix();
}

void drawSeatingDynamic(const Seating& s, float time, int seed, bool cupLeft, bool cupRight)
{
    glPushMatrix();
    glTranslatef(s.x, s.floorY, s.z);
    glRotatef(s.yaw, 0.0f, 1.0f, 0.0f);
    glPushMatrix(); glTranslatef(-s.length * 0.25f, 0.0f, 0.0f);
    drawVillager(true, seed + 0.3f, time, cupLeft); glPopMatrix();
    glPushMatrix(); glTranslatef(s.length * 0.25f, 0.0f, 0.0f);
    drawVillager(true, seed + 1.9f, time, cupRight); glPopMatrix();
    glPopMatrix();
}

void placeVillager(float x, float y, float z, float yaw, bool seated,
                   float phase, float time, bool cup)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    drawVillager(seated, phase, time, cup);
    glPopMatrix();
}

void placeKettle(float x, float y, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);   // spout towards +Z (the customers)
    glScalef(scale, scale, scale);
    drawKettle();
    glPopMatrix();
}

// ------------------------------------------------- brick / mat panels
void drawBrickPanel(float len, float y0, float y1, float thick, int seed)
{
    const float h = y1 - y0;
    box(vary(Brick, seed, 0.05f), 0.0f, (y0 + y1) * 0.5f, 0.0f, len, h, thick);
    const float baseH = std::min(0.5f, h);
    box(tint(Brick, 0.55f), 0.0f, y0 + baseH * 0.5f, 0.0f, len, baseH, thick + 0.04f); // damp base
    for (float y = y0 + 0.55f; y < y1 - 0.1f; y += 0.30f)
        box(Mortar, 0.0f, y, 0.0f, len, 0.04f, thick + 0.03f);
}

void wallSegment(float x0, float x1, float y0, float y1, float z, int seed)
{
    glPushMatrix();
    glTranslatef((x0 + x1) * 0.5f, 0.0f, z);
    drawBrickPanel(x1 - x0, y0, y1, 0.30f, seed);
    glPopMatrix();
}

void drawMatPanel(float w, float h, int seed)
{
    box(BambooMat, 0.0f, h * 0.5f, 0.0f, w, h, 0.06f);
    const int rows = static_cast<int>(h / 0.22f), cols = static_cast<int>(w / 0.30f);
    for (int i = 0; i < rows; ++i)
        box(vary(BambooC, seed * 13 + i, 0.10f), 0.0f, 0.11f + i * 0.22f, 0.035f, w, 0.10f, 0.02f);
    for (int j = 0; j < cols; ++j)
        if (j % 2)
            box(vary(BambooDark, seed * 7 + j, 0.10f), -w * 0.5f + 0.15f + j * 0.30f, h * 0.5f, 0.052f,
                0.10f, h, 0.02f);
}

void drawBamboo(float x, float y, float z, float h, float r, int seed)
{
    column(vary(BambooC, seed, 0.08f), x, y, z, r, h, 8);
    for (float t = 0.55f; t < h - 0.1f; t += 0.8f)
        column(BambooDark, x, y + t, z, r * 1.2f, 0.07f, 8);
}

// ===================================================================== SHOP 0
// Traditional timber stall with a corrugated tin roof. Front faces +Z.
float timberRoofY(float z) { return 4.24f - 0.80f * (z + 2.8f) / 5.7f; } // roof underside

constexpr Seating TimberSeating{-2.2f, 0.0f, 4.15f, 180.0f, 2.6f};

void drawTimberStatic()
{
    const float fy = 0.41f;
    const Color stoneGrey{0.50f, 0.49f, 0.46f};

    // Raised deck on stilts.
    for (float x : {-3.0f, 0.0f, 3.0f})
        for (float z : {-2.3f, 1.6f})
            column(WoodDark, x, 0.0f, z, 0.13f, 0.40f, 8);
    box(Wood, 0.0f, 0.36f, -0.35f, 6.7f, 0.10f, 4.5f);
    for (int i = 0; i < 11; ++i)
        box(WoodDark, -3.0f + i * 0.6f, 0.415f, -0.35f, 0.02f, 0.01f, 4.5f);

    // Frame: posts, beams, knee braces, rafters.
    for (float x : {-3.0f, 3.0f})
    {
        const float yf = timberRoofY(1.75f), yr = timberRoofY(-2.45f);
        column(vary(Wood, 1), x, fy, 1.75f, 0.12f, yf - fy, 10);
        column(vary(Wood, 2), x, fy, -2.45f, 0.12f, yr - fy, 10);
        strut(WoodDark, x, yf - 0.8f, 1.75f, x - (x > 0.0f ? 0.8f : -0.8f), yf - 0.02f, 1.75f, 0.06f);
        strut(Wood, x, yr - 0.11f, -2.45f, x, yf - 0.11f, 1.75f, 0.09f);
    }
    box(Wood, 0.0f, timberRoofY(1.75f) - 0.11f, 1.75f, 6.5f, 0.22f, 0.22f);
    box(Wood, 0.0f, timberRoofY(-2.45f) - 0.11f, -2.45f, 6.5f, 0.22f, 0.22f);
    for (int i = 0; i < 6; ++i)
    {
        const float x = -2.75f + i * 1.1f;
        strut(WoodDark, x, timberRoofY(-2.7f) - 0.02f, -2.7f, x, timberRoofY(2.7f) - 0.02f, 2.7f, 0.05f);
    }

    // Plank walls.
    const float wallH = timberRoofY(-2.5f) - fy;
    for (int i = 0; i < 13; ++i)
        box(vary(Wood, 10 + i, 0.12f), -3.0f + i * 0.5f, fy + wallH * 0.5f, -2.5f, 0.47f, wallH, 0.08f);
    box(WoodDark, 0.0f, 1.45f, -2.43f, 6.5f, 0.14f, 0.06f);
    box(WoodDark, 0.0f, 3.00f, -2.43f, 6.5f, 0.14f, 0.06f);
    for (float x : {-3.05f, 3.05f})
    {
        for (int i = 0; i < 5; ++i)
            box(vary(Wood, 30 + i + (x > 0.0f ? 5 : 0), 0.12f), x, fy + 0.7f, -2.2f + i * 0.5f, 0.08f, 1.4f, 0.47f);
        box(WoodDark, x, fy + 1.45f, -1.2f, 0.14f, 0.10f, 2.6f);
    }

    // Tin roof + ridge cap + painted sign on the front edge.
    corrugatedRoof(RustTin, 0.0f, 3.9f, 0.05f, 7.5f, 5.78f, 8.0f, 1);
    box(tint(RustTin, 0.7f), 0.0f, 4.36f, -2.78f, 7.5f, 0.12f, 0.35f);
    drawSign(Teal, Cream, 0.0f, 3.55f, 2.95f, 2.8f, 0.58f);

    // Serving counter.
    const float cBody = 1.70f - fy;
    box(tint(Wood, 0.9f), 0.0f, fy + cBody * 0.5f, 1.45f, 4.8f, cBody, 0.7f);
    for (int i = 0; i < 8; ++i)
        box(WoodDark, -2.1f + i * 0.6f, fy + cBody * 0.5f, 1.81f, 0.04f, cBody - 0.2f, 0.03f);
    box(WoodLight, 0.0f, 1.75f, 1.5f, 5.0f, 0.10f, 0.92f);
    box(WoodDark, 0.0f, fy + 0.15f, 1.80f, 4.8f, 0.18f, 0.05f);

    // Counter-top: tray of cups, biscuit jars, spare kettle, packets.
    drawCupTray(-0.55f, 1.80f, 1.45f);
    for (int j = 0; j < 3; ++j)
    {
        glPushMatrix(); glTranslatef(0.75f + j * 0.50f, 1.80f, 1.35f);
        drawBiscuitJar(j); glPopMatrix();
    }
    placeKettle(-1.9f, 1.80f, 1.35f, 0.8f);
    drawSnackPackets(2.0f, 1.80f, 1.50f);

    // Preparation area: brick-based clay stove with kettle, prep table behind.
    box(tint(Brick, 0.9f), 1.6f, fy + 0.17f, 0.3f, 1.3f, 0.34f, 1.1f);
    glPushMatrix(); glTranslatef(1.6f, fy + 0.34f, 0.3f); drawClayStove(); glPopMatrix();
    placeKettle(1.6f, fy + 0.34f + 0.71f, 0.3f, 1.15f);
    box(WoodLight, -1.2f, 1.65f, 0.3f, 2.4f, 0.08f, 0.9f);
    for (float lx : {-2.3f, -0.1f})
        for (float lz : {0.0f, 0.6f})
            column(WoodDark, lx, fy, lz, 0.05f, 1.65f - fy, 6);
    glPushMatrix(); glTranslatef(-1.95f, 1.69f, 0.45f); drawBiscuitJar(1); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.45f, 1.69f, 0.45f); drawBiscuitJar(2); glPopMatrix();
    box({0.72f, 0.60f, 0.20f}, -0.75f, 1.90f, 0.40f, 0.36f, 0.44f, 0.30f);
    column(Cream, -0.30f, 1.69f, 0.45f, 0.17f, 0.20f, 10);
    for (int i = 0; i < 3; ++i) drawCup(-1.0f + i * 0.16f, 1.69f, 0.05f);

    // Back shelves.
    glPushMatrix(); glTranslatef(0.5f, 2.0f, -2.21f); drawShelfUnit(4.6f, 0); glPopMatrix();

    // Seating, approach and props.
    groundPatch({0.50f, 0.37f, 0.21f}, 0.2f, 3.7f, 3.6f, 2.4f);
    groundPatch({0.46f, 0.33f, 0.19f}, -2.2f, 3.6f, 3.0f, 2.4f);
    groundPatch({0.56f, 0.42f, 0.24f}, 0.2f, 3.9f, 1.4f, 1.6f, 0.03f);
    for (int i = 0; i < 7; ++i)
        box(vary(stoneGrey, i, 0.15f), -1.5f + i * 0.5f, 0.05f, 2.55f + 0.12f * hash01(i * 3), 0.30f, 0.10f, 0.22f);
    drawSeatingStatic(TimberSeating, Wood, WoodDark, WoodLight, 1);
    drawStool(WoodDark, 3.1f, 2.95f);
    drawPlant(-3.3f, fy, 1.55f, 3);
    drawBin(3.7f, 2.0f);
    glPushMatrix(); glTranslatef(4.4f, 0.0f, 3.4f); glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(9.0f, 1.0f, 0.0f, 0.0f); drawBicycle(); glPopMatrix();
}

void drawTimberDynamic(float time, float night)
{
    drawSeatingDynamic(TimberSeating, time, 1, true, false);
    placeVillager(1.6f, 0.0f, 2.75f, 95.0f, false, 2.8f, time, true);
    placeVillager(3.1f, 0.0f, 2.95f, -85.0f, true, 3.6f, time, false);

    hangingLamp(-1.3f, 3.25f, 1.7f, timberRoofY(1.7f), night);
    hangingLamp(1.4f, 3.25f, 1.7f, timberRoofY(1.7f), night);
    hangingLamp(0.0f, 3.20f, -1.2f, timberRoofY(-1.2f), night);
    fireGlow(1.6f, 0.41f + 0.34f + 0.24f, 0.3f + 0.50f, time);

    glow(0.0f, 0.03f, 2.8f, 3.2f, night, true);        // ground under the roof edge
    glow(0.0f, 1.83f, 1.45f, 1.7f, night, true);       // counter top
    glow(0.0f, 0.43f, 0.2f, 2.2f, night * 0.7f, true); // deck
    glow(0.0f, 2.6f, -2.40f, 2.3f, night * 0.8f, false); // back wall / shelves

    steam(1.6f + 0.55f, 0.41f + 0.34f + 0.71f + 0.60f, 0.3f, time, 0.00f);
    steam(-1.9f, 1.80f + 0.44f, 1.35f + 0.40f, time, 0.37f);
}

// ===================================================================== SHOP 1
// Small brick shop with a veranda. Front faces +Z.
constexpr Seating BrickSeating{-2.15f, 0.5f, 0.55f, 0.0f, 2.8f};

void drawBrickStatic()
{
    const float fy = 0.5f, wallTop = 4.6f, W = 3.7f, winSill = 2.25f, openTop = 3.85f;

    // Plinth, step and paved approach.
    box(Concrete, 0.0f, 0.25f, -0.2f, 7.8f, 0.5f, 5.2f);
    box(tint(Concrete, 0.8f), 0.0f, 0.06f, -0.2f, 7.9f, 0.12f, 5.3f);
    box(Concrete, 0.8f, 0.15f, 2.7f, 2.8f, 0.30f, 0.6f);
    groundPatch({0.52f, 0.38f, 0.21f}, 0.8f, 4.0f, 3.6f, 3.0f);
    groundPatch(Concrete, 0.8f, 4.0f, 2.6f, 2.2f, 0.03f);
    for (int i = 1; i < 4; ++i) box(tint(Concrete, 0.6f), 0.8f, 0.035f, 2.95f + i * 0.55f, 2.6f, 0.01f, 0.03f);
    box(tint(Concrete, 0.6f), 0.8f, 0.035f, 4.0f, 0.03f, 0.01f, 2.2f);

    // Walls: back, sides, and a front wall with window and door openings.
    glPushMatrix(); glTranslatef(0.0f, 0.0f, -2.7f); drawBrickPanel(W * 2.0f + 0.3f, fy, wallTop, 0.30f, 11); glPopMatrix();
    for (float sx : {-W, W})
    {
        glPushMatrix(); glTranslatef(sx, 0.0f, -1.4f); glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        drawBrickPanel(2.9f, fy, wallTop, 0.30f, sx > 0.0f ? 12 : 13); glPopMatrix();
    }
    const float fz = -0.1f;
    wallSegment(-W, -0.5f, fy, wallTop, fz, 21);
    wallSegment(-0.5f, 2.1f, fy, winSill, fz, 22);
    wallSegment(-0.5f, 2.1f, openTop, wallTop, fz, 23);
    wallSegment(2.1f, 2.4f, fy, wallTop, fz, 24);
    wallSegment(2.4f, 3.6f, openTop, wallTop, fz, 25);
    wallSegment(3.6f, W, fy, wallTop, fz, 26);
    for (float sx : {-W - 0.05f, W + 0.05f})           // plaster corner quoins
        box(Plaster, sx, (fy + wallTop) * 0.5f, -0.1f, 0.20f, wallTop - fy, 0.38f);
    box(Plaster, 0.0f, wallTop - 0.10f, -0.1f, W * 2.0f + 0.3f, 0.16f, 0.40f);   // cornice

    // Window: frame, ledge on brackets, open shutters, shelves visible inside.
    box(WoodDark, -0.5f, (winSill + openTop) * 0.5f, 0.0f, 0.12f, openTop - winSill, 0.36f);
    box(WoodDark, 2.1f, (winSill + openTop) * 0.5f, 0.0f, 0.12f, openTop - winSill, 0.36f);
    box(WoodDark, 0.8f, openTop + 0.04f, 0.0f, 2.8f, 0.12f, 0.36f);
    box(WoodLight, 0.8f, 2.26f, 0.35f, 3.0f, 0.10f, 0.60f);
    for (float bx : {-0.3f, 0.8f, 1.9f})
        strut(WoodDark, bx, 1.85f, 0.04f, bx, 2.20f, 0.58f, 0.05f);
    for (int s = 0; s < 2; ++s)
    {
        const float hx = s ? 2.1f : -0.5f;
        glPushMatrix(); glTranslatef(hx, 3.05f, 0.05f); glRotatef(s ? 78.0f : -78.0f, 0.0f, 1.0f, 0.0f);
        box(Teal, s ? -0.45f : 0.45f, 0.0f, 0.0f, 0.88f, 1.50f, 0.06f);
        box(tint(Teal, 0.7f), s ? -0.45f : 0.45f, 0.0f, 0.04f, 0.70f, 0.04f, 0.03f);
        glPopMatrix();
    }
    glPushMatrix(); glTranslatef(0.6f, 2.9f, -2.54f); drawShelfUnit(4.8f, 1); glPopMatrix();
    box(Soot, 0.0f, (fy + openTop) * 0.5f, -2.3f, 0.01f, 0.01f, 0.01f);

    // Door: frame, dark interior, ajar leaf.
    box(WoodDark, 2.4f, (fy + openTop) * 0.5f, 0.0f, 0.12f, openTop - fy, 0.36f);
    box(WoodDark, 3.6f, (fy + openTop) * 0.5f, 0.0f, 0.12f, openTop - fy, 0.36f);
    box(WoodDark, 3.0f, openTop + 0.04f, 0.0f, 1.3f, 0.12f, 0.36f);
    box(Soot, 3.0f, (fy + openTop) * 0.5f, -0.20f, 1.2f, openTop - fy, 0.05f);
    glPushMatrix(); glTranslatef(3.58f, (fy + openTop) * 0.5f, 0.06f); glRotatef(68.0f, 0.0f, 1.0f, 0.0f);
    box(Teal, -0.55f, 0.0f, 0.0f, 1.10f, openTop - fy - 0.05f, 0.07f);
    box(tint(Teal, 0.7f), -0.55f, 0.35f, 0.04f, 0.80f, 0.90f, 0.02f);
    box(Metal, -0.95f, 0.0f, 0.06f, 0.05f, 0.18f, 0.04f);
    glPopMatrix();

    // Gable roof (tin), whitewashed gable ends, barge boards.
    corrugatedRoof(RustTin, 0.0f, 5.10f, -0.55f, 8.3f, 1.97f, 30.5f, 4);
    corrugatedRoof(tint(RustTin, 1.05f), 0.0f, 5.10f, -2.2f, 8.3f, 1.89f, -32.0f, 5);
    box(tint(RustTin, 0.6f), 0.0f, 5.62f, -1.4f, 8.4f, 0.14f, 0.34f);
    for (float sx : {-W - 0.02f, W + 0.02f})
    {
        const float nx = sx > 0.0f ? 1.0f : -1.0f;
        glColor3f(Plaster.r, Plaster.g, Plaster.b);
        glBegin(GL_TRIANGLES);
        glNormal3f(nx, 0.0f, 0.0f);
        if (nx > 0.0f) { glVertex3f(sx, wallTop, -2.85f); glVertex3f(sx, 5.60f, -1.4f); glVertex3f(sx, wallTop, 0.05f); }
        else           { glVertex3f(sx, wallTop, -2.85f); glVertex3f(sx, wallTop, 0.05f); glVertex3f(sx, 5.60f, -1.4f); }
        glEnd();
        box(WoodDark, sx + nx * 0.02f, 4.95f, -1.4f, 0.04f, 0.40f, 0.60f); // vent
        strut(WoodDark, nx * 4.12f, 4.58f, 0.32f, nx * 4.12f, 5.62f, -1.4f, 0.06f);
        strut(WoodDark, nx * 4.12f, 4.58f, -3.02f, nx * 4.12f, 5.62f, -1.4f, 0.06f);
    }

    // Veranda: plaster posts, beam, rafters, lean-to roof, sign.
    for (float x : {-3.55f, -0.9f, 2.25f, 3.6f})
    {
        box(Plaster, x, 0.70f, 2.1f, 0.42f, 0.40f, 0.42f);
        box(Plaster, x, (fy + 3.83f) * 0.5f, 2.1f, 0.28f, 3.83f - fy, 0.28f);
        box(tint(Plaster, 0.9f), x, 3.78f, 2.1f, 0.40f, 0.10f, 0.40f);
    }
    box(Wood, 0.02f, 3.95f, 2.1f, 7.4f, 0.25f, 0.28f);
    for (int i = 0; i < 8; ++i)
        strut(WoodDark, -3.5f + i * 1.0f, 4.38f, 0.05f, -3.5f + i * 1.0f, 4.12f, 2.50f, 0.05f);
    corrugatedRoof(Tin, 0.0f, 4.30f, 1.30f, 8.2f, 2.52f, 6.8f, 6);
    drawSign({0.45f, 0.10f, 0.08f}, Cream, 0.8f, 4.00f, 2.26f, 3.0f, 0.52f);

    // Ledge: tea station (small stove + kettle), cups, jars.
    glPushMatrix(); glTranslatef(1.55f, 2.31f, 0.35f); glScalef(0.6f, 0.6f, 0.6f); drawClayStove(); glPopMatrix();
    placeKettle(1.55f, 2.31f + 0.43f, 0.35f, 0.8f);
    drawCupTray(0.05f, 2.31f, 0.38f);
    glPushMatrix(); glTranslatef(0.75f, 2.31f, 0.40f); drawBiscuitJar(0); glPopMatrix();
    glPushMatrix(); glTranslatef(1.05f, 2.31f, 0.50f); drawBiscuitJar(2); glPopMatrix();
    drawSnackPackets(-0.15f, 2.31f, 0.55f);

    // Veranda furniture and props.
    drawSeatingStatic(BrickSeating, Wood, WoodDark, WoodLight, 2);
    drawPlant(3.2f, fy, 1.9f, 5);
    drawPlant(-4.2f, 0.0f, 2.6f, 8);
    drawBin(4.4f, 2.0f);
    glPushMatrix(); glTranslatef(-4.5f, 0.0f, -0.6f); glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(8.0f, 1.0f, 0.0f, 0.0f); drawBicycle(); glPopMatrix();
}

void drawBrickDynamic(float time, float night)
{
    drawSeatingDynamic(BrickSeating, time, 4, false, true);
    placeVillager(0.45f, 0.5f, 1.45f, 172.0f, false, 1.2f, time, true);
    placeVillager(1.95f, 0.5f, 1.55f, 196.0f, false, 4.4f, time, false);

    hangingLamp(-2.1f, 3.75f, 1.2f, 4.30f, night);
    hangingLamp(1.0f, 3.75f, 1.35f, 4.26f, night);
    hangingLamp(0.8f, 3.60f, -1.2f, 4.58f, night);
    fireGlow(1.55f, 2.31f + 0.15f, 0.35f + 0.28f, time);

    // Warm window spill and veranda pools.
    glow(0.0f, 0.53f, 1.3f, 3.2f, night, true);
    glow(0.8f, 2.34f, 0.35f, 1.5f, night, true);
    glow(0.6f, 3.2f, -2.52f, 2.6f, night * 0.9f, false);
    glow(0.8f, 0.03f, 3.5f, 3.0f, night * 0.7f, true);

    steam(1.55f + 0.38f, 2.31f + 0.43f + 0.42f, 0.35f + 0.38f, time, 0.2f);
}

// ===================================================================== SHOP 2
// Bamboo tea stall with a deep thatched awning. Front faces +Z.
float bambooRoofC(float z) { return 4.9f - 1.2f * (z + 2.6f) / 7.3f; }       // roof centre
float bambooRoofU(float z) { return bambooRoofC(z) - 0.17f; }                // underside

constexpr Seating BambooSeating{-2.5f, 0.0f, 4.05f, 180.0f, 2.6f};

void drawBambooStatic()
{
    const float fy = 0.32f;

    // Raised bamboo floor on bamboo stilts.
    for (float x : {-3.3f, 0.0f, 3.3f})
        for (float z : {-2.5f, 2.0f})
            drawBamboo(x, 0.0f, z, 0.32f, 0.09f, static_cast<int>(x * 3 + z));
    box(BambooDark, 0.0f, 0.285f, -0.3f, 6.8f, 0.07f, 5.0f);
    for (int i = 0; i < 17; ++i)
        box(vary(BambooC, 40 + i, 0.12f), -3.2f + i * 0.4f, 0.325f, -0.3f, 0.33f, 0.01f, 5.0f);
    strut(BambooC, -3.4f, 0.22f, 2.2f, 3.4f, 0.22f, 2.2f, 0.07f);

    // Main structure.
    for (float x : {-3.4f, -1.1f, 2.9f, 3.4f})
        drawBamboo(x, fy, 2.0f, bambooRoofU(2.0f) - fy, 0.09f, static_cast<int>(x * 5));
    for (float x : {-3.3f, -1.1f, 1.1f, 3.3f})
        drawBamboo(x, fy, -2.6f, bambooRoofU(-2.6f) - fy, 0.09f, static_cast<int>(x * 7));
    for (float x : {-4.0f, 3.8f})
    {
        drawBamboo(x, 0.0f, 4.65f, bambooRoofU(4.65f), 0.10f, static_cast<int>(x * 11));
        const float s = x < 0.0f ? 1.0f : -1.0f;
        strut(BambooDark, x, bambooRoofU(4.65f) - 1.2f, 4.65f, x + s * 1.0f, bambooRoofU(4.65f) - 0.12f, 4.65f, 0.05f);
    }
    strut(BambooC, -3.6f, bambooRoofU(2.0f) - 0.10f, 2.0f, 3.6f, bambooRoofU(2.0f) - 0.10f, 2.0f, 0.09f);
    strut(BambooC, -4.2f, bambooRoofU(4.65f) - 0.10f, 4.65f, 4.0f, bambooRoofU(4.65f) - 0.10f, 4.65f, 0.09f);
    strut(BambooC, -3.6f, bambooRoofU(-2.6f) - 0.10f, -2.6f, 3.6f, bambooRoofU(-2.6f) - 0.10f, -2.6f, 0.09f);
    for (float x : {-4.0f, 0.0f, 3.8f})
        strut(BambooDark, x, bambooRoofU(-2.6f) - 0.10f, -2.6f, x, bambooRoofU(4.65f) - 0.10f, 4.65f, 0.07f);

    // Woven-mat walls.
    glPushMatrix(); glTranslatef(0.0f, fy, -2.7f); drawMatPanel(6.6f, 4.35f, 2); glPopMatrix();
    glPushMatrix(); glTranslatef(-3.35f, fy, -1.2f); glRotatef(90.0f, 0.0f, 1.0f, 0.0f); drawMatPanel(3.0f, 2.4f, 3); glPopMatrix();
    glPushMatrix(); glTranslatef(3.35f, fy, -1.2f); glRotatef(-90.0f, 0.0f, 1.0f, 0.0f); drawMatPanel(3.0f, 2.4f, 4); glPopMatrix();

    // Thatched roof with a deep front awning.
    thatchRoof(-0.05f, 4.30f, 1.05f, 8.9f, 7.35f, 9.3f, 3);

    // Counter: bamboo frame with woven front, plank top.
    box(BambooDark, 0.9f, (fy + 1.70f) * 0.5f, 1.45f, 3.8f, 1.70f - fy, 0.8f);
    glPushMatrix(); glTranslatef(0.9f, fy, 1.88f); drawMatPanel(3.7f, 1.38f, 5); glPopMatrix();
    drawBamboo(-1.0f, fy, 1.90f, 1.45f, 0.07f, 31);
    drawBamboo(2.8f, fy, 1.90f, 1.45f, 0.07f, 32);
    strut(BambooC, -1.0f, 1.0f, 1.92f, 2.8f, 1.0f, 1.92f, 0.05f);
    strut(BambooC, -1.0f, 0.55f, 1.92f, 2.8f, 0.55f, 1.92f, 0.05f);
    box(tint(BambooC, 1.1f), 0.9f, 1.75f, 1.45f, 4.0f, 0.10f, 0.96f);

    // Counter-top set-up.
    drawCupTray(0.0f, 1.80f, 1.45f);
    for (int j = 0; j < 3; ++j)
    {
        glPushMatrix(); glTranslatef(1.1f + j * 0.5f, 1.80f, 1.35f);
        drawBiscuitJar(j + 1); glPopMatrix();
    }
    placeKettle(-0.4f, 1.80f, 1.40f, 0.85f);
    drawSnackPackets(2.35f, 1.80f, 1.50f);

    // Preparation: mud platform, stove, kettle, shelves.
    box(Mud, 1.8f, fy + 0.225f, 0.3f, 1.4f, 0.45f, 1.1f);
    glPushMatrix(); glTranslatef(1.8f, fy + 0.45f, 0.3f); glScalef(0.95f, 0.95f, 0.95f); drawClayStove(); glPopMatrix();
    placeKettle(1.8f, fy + 0.45f + 0.68f, 0.3f, 1.1f);
    glPushMatrix(); glTranslatef(0.6f, 2.0f, -2.40f); drawShelfUnit(4.0f, 2); glPopMatrix();
    box(BambooC, -1.4f, 1.55f, 0.3f, 1.8f, 0.07f, 0.8f);
    for (float lx : {-2.2f, -0.6f})
        drawBamboo(lx, fy, 0.3f, 1.55f - fy, 0.045f, static_cast<int>(lx * 13));
    glPushMatrix(); glTranslatef(-1.9f, 1.585f, 0.3f); drawBiscuitJar(0); glPopMatrix();
    for (int i = 0; i < 3; ++i) drawCup(-1.3f + i * 0.16f, 1.585f, 0.3f);
    column({0.18f, 0.40f, 0.52f}, -0.8f, 1.585f, 0.3f, 0.08f, 0.36f, 8);

    // Sign and hanging bananas on the front beam.
    for (float sx : {-0.55f, 2.15f}) strut(BambooDark, sx + 0.9f - 0.9f + 0.0f, 3.84f, 2.05f, sx + 0.0f, 3.50f, 2.05f, 0.02f, 5);
    drawSign({0.55f, 0.15f, 0.11f}, Cream, 0.8f, 3.20f, 2.05f, 3.0f, 0.60f);
    for (float bx : {-1.8f, 3.3f})
    {
        strut(BambooDark, bx, 3.83f, 2.05f, bx, 3.55f, 2.05f, 0.02f, 5);
        for (int i = 0; i < 8; ++i)
        {
            const float a = i * 0.8f;
            sphere(vary({0.82f, 0.74f, 0.22f}, i + static_cast<int>(bx * 10), 0.12f),
                   bx + std::cos(a) * 0.08f, 3.42f - (i / 4) * 0.12f, 2.05f + std::sin(a) * 0.08f,
                   0.055f, 0.20f, 0.055f, 6, 4);
        }
    }

    // Seating on a beaten-earth floor, approach, props.
    groundPatch({0.48f, 0.35f, 0.20f}, 0.3f, 3.6f, 8.0f, 2.6f);
    groundPatch({0.43f, 0.31f, 0.18f}, -2.5f, 3.3f, 3.2f, 2.4f, 0.03f);
    for (int i = 0; i < 4; ++i)   // plank stepping path to the counter
        box(WoodDark, 1.2f, 0.04f, 2.45f + i * 0.5f, 1.4f, 0.06f, 0.36f);
    drawSeatingStatic(BambooSeating, BambooC, BambooDark, tint(BambooC, 1.05f), 3);
    drawStool(BambooDark, 2.95f, 2.9f);
    drawPlant(-3.6f, fy, 0.9f, 11);
    drawBin(4.3f, 3.4f);
    glPushMatrix(); glTranslatef(4.7f, 0.0f, 1.0f); glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(10.0f, 1.0f, 0.0f, 0.0f); drawBicycle(); glPopMatrix();
}

void drawBambooDynamic(float time, float night)
{
    drawSeatingDynamic(BambooSeating, time, 7, true, true);
    placeVillager(1.4f, 0.0f, 2.65f, 100.0f, false, 3.1f, time, true);
    placeVillager(2.95f, 0.0f, 2.9f, -85.0f, true, 0.9f, time, false);

    // Hurricane lantern, kerosene-style, plus a string of bulbs on the awning beam.
    strut(BambooDark, -1.0f, 3.83f, 2.05f, -1.0f, 3.38f, 2.05f, 0.015f, 5);
    column(Metal, -1.0f, 3.12f, 2.05f, 0.11f, 0.26f, 8);
    emissiveSphere({1.0f, 0.88f, 0.60f}, -1.0f, 3.25f, 2.05f, 0.09f, night);
    strut(Soot, -3.9f, bambooRoofU(4.6f) - 0.30f, 4.55f, 3.7f, bambooRoofU(4.6f) - 0.30f, 4.55f, 0.012f, 4);
    for (int i = 0; i < 7; ++i)
        emissiveSphere({1.0f, 0.93f, 0.72f}, -3.6f + i * 1.1f, bambooRoofU(4.6f) - 0.42f, 4.55f, 0.07f, night);
    fireGlow(1.8f, 0.32f + 0.45f + 0.22f, 0.3f + 0.46f, time);

    glow(0.5f, 0.03f, 3.0f, 3.2f, night, true);
    glow(-2.5f, 0.03f, 3.2f, 2.3f, night * 0.9f, true);
    glow(0.9f, 1.83f, 1.45f, 1.8f, night, true);
    glow(1.0f, 0.33f, 0.4f, 2.2f, night * 0.7f, true);
    glow(0.6f, 2.8f, -2.60f, 2.6f, night * 0.8f, false);

    steam(1.8f + 0.55f, 0.32f + 0.45f + 0.68f + 0.58f, 0.3f, time, 0.55f);
    steam(-0.4f, 1.80f + 0.46f, 1.40f + 0.40f, time, 0.18f);
}

// --------------------------------------------------------------- dispatch
void drawShopStatic(int variant)
{
    switch (variant)
    {
    case 0: drawTimberStatic(); break;
    case 1: drawBrickStatic();  break;
    default: drawBambooStatic(); break;
    }
}

GLuint shopList(int variant)
{
    static GLuint lists[3] = {0, 0, 0};
    if (!lists[variant])
    {
        lists[variant] = glGenLists(1);
        glNewList(lists[variant], GL_COMPILE);
        drawShopStatic(variant);
        glEndList();
    }
    return lists[variant];
}

void drawShop(int variant, float time, float nightAmount)
{
    variant = ((variant % 3) + 3) % 3;
    if (UseDisplayLists) glCallList(shopList(variant));
    else drawShopStatic(variant);

    switch (variant)   // animated / lit parts, drawn opaque first, blended last
    {
    case 0: drawTimberDynamic(time, nightAmount); break;
    case 1: drawBrickDynamic(time, nightAmount);  break;
    default: drawBambooDynamic(time, nightAmount); break;
    }
}

void drawGrassTuft(float x, float z, float height, float rotation, int shade)
{
    static const Color greens[] = {
        {0.10f, 0.38f, 0.08f}, {0.16f, 0.47f, 0.10f},
        {0.22f, 0.55f, 0.13f}, {0.12f, 0.43f, 0.16f}};
    const Color color = greens[shade & 3];
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, 0.04f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    for (int blade = 0; blade < 3; ++blade)
    {
        const float offset = (blade - 1) * 0.12f;
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(offset - 0.055f, 0.0f, 0.0f);
        glVertex3f(offset + 0.055f, 0.0f, 0.0f);
        glVertex3f(offset + (blade - 1) * 0.05f, height, 0.0f);
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, offset - 0.055f);
        glVertex3f(0.0f, 0.0f, offset + 0.055f);
        glVertex3f(0.0f, height * 0.92f, offset);
    }
    glEnd();
    glPopMatrix();
}

void drawBlossom(const Color& color, float x, float y, float z, float size)
{
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLES);
    for (int petal = 0; petal < 5; ++petal)
    {
        const float a = 2.0f * Pi * static_cast<float>(petal) / 5.0f;
        const float b = a + 0.55f;
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(x, y, z);
        glVertex3f(x + std::cos(a) * size, y + size * 0.10f,
                   z + std::sin(a) * size);
        glVertex3f(x + std::cos(b) * size, y + size * 0.10f,
                   z + std::sin(b) * size);
    }
    glEnd();
}

void drawFlower(float x, float z, float height, int seed)
{
    static const Color petals[] = {
        {0.92f, 0.28f, 0.35f}, {0.78f, 0.48f, 0.88f},
        {0.96f, 0.76f, 0.20f}, {0.92f, 0.92f, 0.88f}};
    const Color petal = petals[seed & 3];
    column({0.12f, 0.40f, 0.10f}, x, 0.04f, z, 0.025f, height, 6);
    drawBlossom(petal, x, height + 0.05f, z, 0.17f);
    sphere({0.55f, 0.30f, 0.06f}, x, height + 0.055f, z,
           0.055f, 0.040f, 0.055f, 5, 3);
}

void drawFloweringShrub(float x, float z, float scale, int seed)
{
    const Color leaf{0.10f, 0.32f, 0.09f};
    sphere(leaf, x, 0.48f * scale, z, 0.75f * scale, 0.55f * scale,
           0.65f * scale, 8, 5);
    sphere(tint(leaf, 1.20f), x - 0.45f * scale, 0.42f * scale,
           z + 0.18f * scale, 0.48f * scale, 0.42f * scale,
           0.45f * scale, 7, 5);
    sphere(tint(leaf, 0.86f), x + 0.42f * scale, 0.45f * scale,
           z - 0.12f * scale, 0.50f * scale, 0.44f * scale,
           0.48f * scale, 7, 5);
    for (int flower = 0; flower < 7; ++flower)
    {
        const float angle = 2.0f * Pi * static_cast<float>(flower) / 7.0f;
        const Color c = flower % 2
            ? Color{0.96f, 0.55f, 0.66f} : Color{0.82f, 0.72f, 0.95f};
        drawBlossom(c, x + std::cos(angle) * 0.48f * scale,
                    (0.72f + 0.08f * hash01(seed + flower)) * scale,
                    z + std::sin(angle) * 0.42f * scale, 0.15f * scale);
    }
}

void drawFloweringTree(float x, float z, float scale, int seed)
{
    box(WoodDark, x, 1.7f * scale, z,
        0.48f * scale, 3.4f * scale, 0.48f * scale);
    const Color leaf{0.12f, 0.37f, 0.11f};
    sphere(leaf, x, 4.0f * scale, z, 1.45f * scale, 1.15f * scale,
           1.35f * scale, 8, 6);
    sphere(tint(leaf, 1.16f), x - 1.0f * scale, 3.75f * scale,
           z + 0.1f * scale, 1.05f * scale, 0.90f * scale,
           1.0f * scale, 8, 6);
    sphere(tint(leaf, 0.88f), x + 1.0f * scale, 3.8f * scale,
           z - 0.15f * scale, 1.05f * scale, 0.92f * scale,
           1.0f * scale, 8, 6);
    for (int blossom = 0; blossom < 16; ++blossom)
    {
        const float angle = static_cast<float>(blossom) * 2.39996f;
        const float radius = (0.45f + 0.65f * hash01(seed + blossom * 13)) * scale;
        const Color c = blossom % 3
            ? Color{0.95f, 0.62f, 0.72f} : Color{0.96f, 0.88f, 0.91f};
        drawBlossom(c, x + std::cos(angle) * radius,
                    (3.65f + hash01(seed + blossom * 31) * 0.9f) * scale,
                    z + std::sin(angle) * radius, 0.19f * scale);
    }
}
} // namespace

void drawDriver(float animationTime, float phase, bool detailed)
{
    // Drivers keep a steady wheel-holding pose. Cache the complete shared
    // villager model per clothing variant so adding occupants to traffic does
    // not multiply the immediate-mode character cost every frame.
    static GLuint driverLists[4] = {0, 0, 0, 0};
    static GLuint driverLodLists[4] = {0, 0, 0, 0};
    const int variant = static_cast<int>(std::fabs(phase)) & 3;
    GLuint& list = detailed ? driverLists[variant] : driverLodLists[variant];
    if (list == 0)
    {
        list = glGenLists(1);
        glNewList(list, GL_COMPILE);
        if (detailed)
        {
            drawVillager(true, phase, 0.0f, false, true);
        }
        else
        {
            // Far driver LOD retains the shared palette, proportions, seated
            // joints, and wheel-reaching arm solver with far fewer vertices.
            const Color shirts[] = {
                {0.68f, 0.18f, 0.14f}, {0.16f, 0.42f, 0.26f},
                {0.20f, 0.38f, 0.68f}, {0.78f, 0.56f, 0.15f}};
            const Color shirt = shirts[variant];
            const Color pants{0.16f, 0.22f, 0.34f};
            const Color skin = vary(Skin, variant * 17 + 3, 0.08f);
            for (float side : {-1.0f, 1.0f})
            {
                const V3 hip{side * 0.18f, 1.07f, 0.0f};
                const V3 knee{side * 0.19f, 0.84f, 0.52f};
                const V3 ankle{side * 0.19f, 0.13f, 0.58f};
                taper(pants, hip, knee, 0.12f, 0.09f, 6);
                taper(pants, knee, ankle, 0.09f, 0.06f, 6);
                const V3 shoulder{side * 0.39f, 1.98f, 0.0f};
                drawArm(shoulder, {side * 0.30f, 1.78f, 1.18f},
                        side, shirt, skin, false);
            }
            box(pants, 0.0f, 1.12f, 0.0f, 0.62f, 0.28f, 0.40f);
            box(shirt, 0.0f, 1.67f, 0.0f, 0.72f, 0.82f, 0.42f);
            sphere(skin, 0.0f, 2.43f, 0.02f,
                   0.21f, 0.27f, 0.22f, 8, 5);
            sphere({0.08f, 0.05f, 0.03f}, 0.0f, 2.50f, -0.04f,
                   0.22f, 0.18f, 0.22f, 7, 4);
        }
        glEndList();
    }
    glCallList(list);
    (void)animationTime;
}

void drawWorldVegetation(VisibilityTest visibility, float viewerX, float viewerZ)
{
    constexpr int chunkColumns = 8;
    constexpr int chunkRows = 8;
    constexpr float worldMin = -160.0f;
    constexpr float chunkSize = 40.0f;
    constexpr float drawDistance = 90.0f;
    static GLuint chunkLists[chunkRows][chunkColumns] = {};

    const auto suitable = [](float x, float z)
    {
        if (std::fabs(x) < 12.0f) return false;
        constexpr float farmRows[] =
            {-68.0f, -34.0f, 0.0f, 34.0f, 68.0f, 102.0f};
        for (float row : farmRows)
            if (std::fabs(std::fabs(x) - 38.0f) < 24.5f
                && std::fabs(z - row) < 19.5f)
                return false;

        struct Exclusion { float x, z, rx, rz; };
        constexpr Exclusion exclusions[] = {
            {-82.0f, -42.0f, 24.0f, 19.0f}, {73.0f, 24.0f, 22.0f, 17.5f},
            {-71.0f, 58.0f, 22.0f, 17.5f}, {-65.0f, -68.0f, 15.0f, 15.0f},
            {65.0f, -34.0f, 15.0f, 15.0f}, {-65.0f, 34.0f, 15.0f, 15.0f},
            {65.0f, 68.0f, 15.0f, 15.0f}, {-14.5f, -32.0f, 8.0f, 7.0f},
            {14.5f, 18.0f, 8.0f, 7.0f}, {-14.5f, 66.0f, 8.0f, 7.0f}};
        for (const Exclusion& e : exclusions)
        {
            const float dx = (x - e.x) / e.rx;
            const float dz = (z - e.z) / e.rz;
            if (dx * dx + dz * dz < 1.0f) return false;
        }
        for (int site = 0; site < VillageSimulationSettings::BonfireSiteCount; ++site)
        {
            const float dx = x - VillageSimulationSettings::BonfireSiteX[site];
            const float dz = z - VillageSimulationSettings::BonfireSiteZ[site];
            if (dx * dx + dz * dz < 9.0f * 9.0f) return false;
        }
        return true;
    };

    for (int row = 0; row < chunkRows; ++row)
    {
        for (int columnIndex = 0; columnIndex < chunkColumns; ++columnIndex)
        {
            const float baseX = worldMin + static_cast<float>(columnIndex) * chunkSize;
            const float baseZ = worldMin + static_cast<float>(row) * chunkSize;
            const float centerX = baseX + chunkSize * 0.5f;
            const float centerZ = baseZ + chunkSize * 0.5f;
            const float dx = centerX - viewerX;
            const float dz = centerZ - viewerZ;
            if (dx * dx + dz * dz > drawDistance * drawDistance
                || (visibility && !visibility(centerX, 1.0f, centerZ, 30.0f)))
                continue;

            GLuint& list = chunkLists[row][columnIndex];
            if (list == 0)
            {
                list = glGenLists(1);
                glNewList(list, GL_COMPILE);
                for (int localX = 0; localX < 8; ++localX)
                {
                    for (int localZ = 0; localZ < 7; ++localZ)
                    {
                        const int seed = (columnIndex * 8 + localX + 9) * 131
                            + (row * 7 + localZ + 7) * 47;
                        if (hash01(seed) < 0.62f) continue;
                        const float x = baseX + (localX + 0.5f) * 5.0f
                            + (hash01(seed + 17) - 0.5f) * 3.3f;
                        const float z = baseZ + (localZ + 0.5f) * (chunkSize / 7.0f)
                            + (hash01(seed + 31) - 0.5f) * 3.5f;
                        if (!suitable(x, z)) continue;
                        drawGrassTuft(
                            x, z, 0.22f + hash01(seed + 53) * 0.58f,
                            hash01(seed + 71) * 180.0f, seed);
                        if (hash01(seed + 89) > 0.94f)
                            drawFlower(x + 0.35f, z - 0.22f,
                                       0.32f + hash01(seed + 97) * 0.22f, seed);
                    }
                }
                glEndList();
            }
            glCallList(list);
        }
    }

    constexpr float shrubs[][3] = {
        {-58.0f, -28.0f, 1.0f}, {-60.0f, -57.0f, 0.9f},
        {-106.0f, -35.0f, 1.1f}, {53.0f, 13.0f, 1.0f},
        {95.0f, 18.0f, 0.9f}, {-50.0f, 55.0f, 1.1f},
        {-94.0f, 68.0f, 0.9f}, {96.0f, 48.0f, 1.0f}};
    static GLuint shrubLists[8] = {};
    for (int i = 0; i < 8; ++i)
    {
        const float dx = shrubs[i][0] - viewerX;
        const float dz = shrubs[i][1] - viewerZ;
        if (dx * dx + dz * dz > drawDistance * drawDistance
            || (visibility && !visibility(shrubs[i][0], 1.0f, shrubs[i][1], 2.0f)))
            continue;
        if (shrubLists[i] == 0)
        {
            shrubLists[i] = glGenLists(1);
            glNewList(shrubLists[i], GL_COMPILE);
            drawFloweringShrub(shrubs[i][0], shrubs[i][1], shrubs[i][2], 200 + i);
            glEndList();
        }
        glCallList(shrubLists[i]);
    }

    constexpr float floweringTrees[][3] = {
        {-110.0f, -39.0f, 1.15f}, {101.0f, 22.0f, 1.05f},
        {-98.0f, 72.0f, 1.10f}, {94.0f, -18.0f, 1.00f},
        {-102.0f, -72.0f, 1.08f}};
    static GLuint treeLists[5] = {};
    for (int i = 0; i < 5; ++i)
    {
        const float dx = floweringTrees[i][0] - viewerX;
        const float dz = floweringTrees[i][1] - viewerZ;
        if (dx * dx + dz * dz > drawDistance * drawDistance
            || (visibility
                && !visibility(
                    floweringTrees[i][0], 4.0f, floweringTrees[i][1], 6.0f)))
            continue;
        if (treeLists[i] == 0)
        {
            treeLists[i] = glGenLists(1);
            glNewList(treeLists[i], GL_COMPILE);
            drawFloweringTree(
                floweringTrees[i][0], floweringTrees[i][1],
                floweringTrees[i][2], 310 + i);
            glEndList();
        }
        glCallList(treeLists[i]);
    }
}

void drawBonfireSite(int siteIndex, float animationTime, float nightAmount)
{
    if (siteIndex < 0 || siteIndex >= VillageSimulationSettings::BonfireSiteCount)
        return;

    static GLuint firePitList = 0;
    if (firePitList == 0)
    {
        firePitList = glGenLists(1);
        glNewList(firePitList, GL_COMPILE);
        glColor3f(0.30f, 0.19f, 0.10f);
        glPushMatrix(); glTranslatef(0.0f, 0.055f, 0.0f);
        Primitives::drawPlane(8.2f, 8.2f); glPopMatrix();
        for (int stone = 0; stone < 14; ++stone)
        {
            const float a = 2.0f * Pi * static_cast<float>(stone) / 14.0f;
            sphere(vary({0.34f, 0.32f, 0.29f}, stone, 0.18f),
                   std::cos(a) * 1.35f, 0.23f, std::sin(a) * 1.35f,
                   0.34f, 0.22f, 0.30f, 7, 5);
        }
        sphere({0.16f, 0.13f, 0.11f}, 0.0f, 0.22f, 0.0f,
               1.02f, 0.10f, 1.02f, 10, 4);
        for (int log = 0; log < 4; ++log)
        {
            const float a = (28.0f + static_cast<float>(log) * 47.0f)
                * Pi / 180.0f;
            const float x = std::cos(a) * 1.02f;
            const float z = std::sin(a) * 1.02f;
            const float y = 0.34f + 0.07f * static_cast<float>(log & 1);
            strut(log % 2 ? Wood : WoodDark,
                  -x, y, -z, x, y + 0.04f, z, 0.14f, 9);
            sphere({0.10f, 0.075f, 0.045f}, -x, y, -z,
                   0.15f, 0.15f, 0.15f, 7, 5);
            sphere({0.10f, 0.075f, 0.045f}, x, y + 0.04f, z,
                   0.15f, 0.15f, 0.15f, 7, 5);
        }
        // Split-log seats used by the two seated villagers.
        box(WoodDark, -3.0f, SeatSurfaceY - 0.24f, 0.0f,
            1.7f, 0.48f, 0.62f);
        box(WoodDark,  3.0f, SeatSurfaceY - 0.24f, 0.0f,
            1.7f, 0.48f, 0.62f);
        glEndList();
    }

    glPushMatrix();
    glTranslatef(VillageSimulationSettings::BonfireSiteX[siteIndex], 0.0f,
                 VillageSimulationSettings::BonfireSiteZ[siteIndex]);
    glRotatef(static_cast<float>(siteIndex) * 31.0f, 0.0f, 1.0f, 0.0f);
    glCallList(firePitList);

    const float flickerA = 0.88f + 0.16f
        * std::sin(animationTime * 8.1f + static_cast<float>(siteIndex));
    const float flickerB = 0.86f + 0.14f
        * std::sin(animationTime * 11.3f + static_cast<float>(siteIndex) * 2.2f);
    glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT
                 | GL_COLOR_BUFFER_BIT | GL_POINT_BIT);
    glEnable(GL_LIGHTING);
    const GLfloat emission[] = {
        0.52f + nightAmount * 0.45f, 0.18f + nightAmount * 0.20f,
        0.025f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    // A small ember bed remains readable between the crossed logs.
    for (int ember = 0; ember < 8; ++ember)
    {
        const float a = static_cast<float>(ember) * 2.39996f;
        const float pulse = 0.78f + 0.22f * std::sin(
            animationTime * (5.2f + ember * 0.11f) + ember);
        sphere({1.0f, 0.20f + 0.20f * pulse, 0.025f},
               std::cos(a) * (0.25f + 0.05f * (ember & 1)), 0.43f,
               std::sin(a) * (0.25f + 0.05f * (ember & 1)),
               0.10f, 0.055f, 0.10f, 6, 4);
    }

    const float driftA = 0.12f * std::sin(animationTime * 6.7f + siteIndex);
    const float driftB = 0.10f * std::sin(animationTime * 9.1f + siteIndex * 2.0f);
    sphere({0.92f, 0.20f, 0.035f}, driftA, 0.82f, driftB,
           0.66f * flickerA, 1.18f * flickerB, 0.62f * flickerA, 9, 6);
    sphere({1.00f, 0.55f, 0.06f}, -0.18f - driftB, 0.94f, 0.05f,
           0.36f * flickerB, 0.98f * flickerA, 0.34f * flickerB, 8, 5);
    sphere({1.00f, 0.88f, 0.25f}, 0.12f + driftB, 0.72f, -0.08f,
           0.23f * flickerA, 0.68f * flickerB, 0.22f * flickerA, 7, 5);
    sphere({1.00f, 0.42f, 0.045f}, -0.30f + driftA, 1.14f, 0.10f,
           0.18f * flickerB, 0.62f * flickerA, 0.16f * flickerB, 7, 5);
    const GLfloat noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int puff = 0; puff < 2; ++puff)
    {
        const float life = std::fmod(
            animationTime * 0.16f + static_cast<float>(puff) * 0.5f
            + static_cast<float>(siteIndex) * 0.13f, 1.0f);
        glColor4f(0.36f, 0.34f, 0.32f, 0.16f * (1.0f - life));
        glPushMatrix();
        glTranslatef(std::sin(animationTime + puff) * 0.22f * life,
                     1.55f + life * 2.4f,
                     std::cos(animationTime * 0.7f + puff) * 0.18f * life);
        const float size = 0.25f + life * 0.42f;
        glScalef(size, size, size);
        Primitives::drawSphere(1.0f, 7, 5);
        glPopMatrix();
    }
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (int spark = 0; spark < 5; ++spark)
    {
        const float life = std::fmod(
            animationTime * (0.55f + spark * 0.03f) + spark * 0.21f, 1.0f);
        glColor4f(1.0f, 0.48f + 0.35f * life, 0.08f, 1.0f - life);
        glVertex3f(std::sin(spark * 2.1f + animationTime) * 0.42f * life,
                   1.0f + life * 2.0f,
                   std::cos(spark * 1.7f + animationTime) * 0.35f * life);
    }
    glEnd();
    glDepthMask(GL_TRUE);
    glPopAttrib();

    glPushMatrix(); glTranslatef(-3.0f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    drawVillager(true, 7.2f + siteIndex, animationTime, false); glPopMatrix();
    glPushMatrix(); glTranslatef(3.0f, 0.0f, 0.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    drawVillager(true, 8.7f + siteIndex, animationTime, siteIndex % 2 == 0);
    glPopMatrix();
    if ((siteIndex & 1) == 0)
    {
        glPushMatrix(); glTranslatef(0.0f, 0.0f, -3.35f);
        drawVillager(false, 10.1f + siteIndex, animationTime, false);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawFarmGrass(int seed, float cropX, float cropZ,
                   float houseX, float houseZ, bool hasWindmill)
{
    const float actualHouseX = houseX + 10.0f;
    const float actualHouseZ = houseZ + 7.0f;
    for (int index = 0; index < 92; ++index)
    {
        const float x = -21.5f + hash01(seed * 211 + index * 17) * 43.0f;
        const float z = -16.5f + hash01(seed * 137 + index * 29) * 33.0f;
        if (std::fabs(x - cropX) < 10.0f && std::fabs(z - (cropZ - 7.0f)) < 6.2f) continue;
        if (std::fabs(x - actualHouseX) < 7.0f && std::fabs(z - actualHouseZ) < 6.0f) continue;
        if (std::fabs(x - 10.0f) < 2.4f && z < 8.0f) continue; // access path
        if (hasWindmill)
        {
            const float dx = x + 14.0f, dz = z - 1.0f;
            if (dx * dx + dz * dz < 16.0f) continue;
        }
        drawGrassTuft(x, z, 0.24f + hash01(index * 43 + seed) * 0.42f,
                      hash01(index * 71 + seed) * 180.0f, index + seed);
    }

    // A restrained flower border gives each house colour without placing
    // vegetation on the front access path or inside the building footprint.
    for (int index = 0; index < 8; ++index)
    {
        const float angle = 2.0f * Pi * static_cast<float>(index) / 8.0f;
        const float x = actualHouseX + std::cos(angle) * 6.2f;
        const float z = actualHouseZ + std::sin(angle) * 4.8f;
        if (std::fabs(x - 10.0f) < 2.4f && z < actualHouseZ) continue;
        drawFlower(x, z, 0.34f + hash01(seed * 73 + index) * 0.24f,
                   seed * 11 + index);
    }
}

void drawPondSeating(float animationTime)
{
    // East of the western pond, outside its shoreline. The bench faces west
    // toward the water; its occupants' feet land on the access-side grass.
    glPushMatrix();
    glTranslatef(-61.5f, 0.0f, -42.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    drawBench(4.8f);
    glPushMatrix(); glTranslatef(-1.05f, 0.0f, 0.0f);
    drawVillager(true, 0.6f, animationTime, false); glPopMatrix();
    glPushMatrix(); glTranslatef(1.05f, 0.0f, 0.0f);
    drawVillager(true, 2.2f, animationTime, true); glPopMatrix();
    glPopMatrix();

    // Compact dirt access path from the open field to the bench.
    glColor3f(0.52f, 0.38f, 0.21f);
    glPushMatrix(); glTranslatef(-58.2f, 0.025f, -42.0f);
    Primitives::drawPlane(6.2f, 2.2f); glPopMatrix();
}

void drawRoadsideAmenities(float animationTime, float electricLightAmount,
                           unsigned int benchOccupancy)
{
    struct Placement { float x, z, rotation; int variant; };
    constexpr Placement shops[] = {
        {-14.5f, -32.0f,  90.0f, 0},   // timber stall, tin roof
        { 14.5f,  18.0f, -90.0f, 1},   // brick shop, veranda
        {-14.5f,  66.0f,  90.0f, 2}};  // bamboo stall, deep awning
    for (const Placement& shop : shops)
    {
        glPushMatrix();
        glTranslatef(shop.x, 0.0f, shop.z);
        glRotatef(shop.rotation, 0.0f, 1.0f, 0.0f);
        drawShop(shop.variant, animationTime, electricLightAmount);
        glPopMatrix();
    }

    constexpr Placement benches[] = {
        {-10.5f, -58.0f,  90.0f, 0},
        { 10.5f, -10.0f, -90.0f, 0},
        {-10.5f,  38.0f,  90.0f, 0},
        { 10.5f,  78.0f, -90.0f, 0}};
    for (int index = 0; index < 4; ++index)
    {
        const Placement& bench = benches[index];
        glPushMatrix();
        glTranslatef(bench.x, 0.0f, bench.z);
        glRotatef(bench.rotation + (index % 2 ? 3.0f : -2.0f), 0.0f, 1.0f, 0.0f);
        drawBench(3.8f + (index % 2) * 0.3f);
        const unsigned int occupantCount = (benchOccupancy >> (index * 2)) & 3u;
        if (occupantCount == 1u)
        {
            drawVillager(true, 4.1f + static_cast<float>(index) * 1.7f,
                         animationTime, index % 3 == 0);
        }
        else if (occupantCount >= 2u)
        {
            glPushMatrix(); glTranslatef(-0.82f, 0.0f, 0.0f);
            drawVillager(true, 4.1f + static_cast<float>(index) * 1.7f,
                         animationTime, false); glPopMatrix();
            glPushMatrix(); glTranslatef(0.82f, 0.0f, 0.0f);
            drawVillager(true, 5.3f + static_cast<float>(index) * 1.9f,
                         animationTime, true); glPopMatrix();
        }
        glPopMatrix();
    }
}

} // namespace Village
