#include "objects/Village.h"

#include "DayNightSettings.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace Village {
namespace {
constexpr float Pi = 3.14159265358979323846f;
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
    box(seat, 0.0f, 0.86f, 0.02f, length, 0.20f, 0.78f);
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
void limb(const Color& color, float x, float y, float z,
          float length, float angleX, float angleZ, float radius)
{
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(angleZ, 0.0f, 0.0f, 1.0f);
    glRotatef(angleX, 1.0f, 0.0f, 0.0f);
    Primitives::drawCylinder(radius, length, 9);
    glPopMatrix();
}

void drawVillager(bool seated, float phase, float time, bool holdingCup)
{
    const Color shirts[] = {
        {0.68f, 0.18f, 0.14f}, {0.16f, 0.42f, 0.26f},
        {0.20f, 0.38f, 0.68f}, {0.78f, 0.56f, 0.15f}};
    const Color shirt = shirts[static_cast<int>(phase * 3.0f) & 3];
    const float gesture = std::sin(time * (0.75f + phase * 0.08f) + phase) * 10.0f;
    const float sip = holdingCup
        ? 0.5f + 0.5f * std::sin(time * 0.48f + phase * 2.3f) : 0.0f;
    const float headTurn = std::sin(time * 0.31f + phase) * 14.0f;

    if (seated)
    {
        for (float side : {-1.0f, 1.0f})
        {
            limb({0.16f, 0.22f, 0.34f}, side * 0.18f, 1.05f, 0.0f,
                 0.58f, -67.0f, 0.0f, 0.105f);
            limb({0.18f, 0.14f, 0.09f}, side * 0.18f, 0.82f, 0.52f,
                 0.72f, 8.0f, 0.0f, 0.09f);
            box({0.12f, 0.07f, 0.035f}, side * 0.18f, 0.08f, 0.63f,
                0.24f, 0.13f, 0.40f);
        }
    }
    else
    {
        for (float side : {-1.0f, 1.0f})
        {
            limb({0.16f, 0.22f, 0.34f}, side * 0.17f, 1.10f, 0.0f,
                 0.93f, 0.0f, 0.0f, 0.105f);
            box({0.12f, 0.07f, 0.035f}, side * 0.17f, 0.07f, 0.10f,
                0.24f, 0.13f, 0.38f);
        }
    }

    const float hipY = seated ? 1.12f : 1.45f;
    sphere(shirt, 0.0f, hipY + 0.53f, 0.0f, 0.38f, 0.58f, 0.25f);
    box({0.16f, 0.22f, 0.34f}, 0.0f, hipY + 0.10f, 0.0f, 0.68f, 0.26f, 0.46f);

    for (float side : {-1.0f, 1.0f})
    {
        const bool cupArm = holdingCup && side > 0.0f;
        const float armAngle = cupArm ? -58.0f * sip : gesture * side;
        limb(shirt, side * 0.45f, hipY + 0.82f, 0.0f,
             0.62f, armAngle, side * 8.0f, 0.09f);
        sphere(Skin, side * 0.45f, hipY + 0.25f + (cupArm ? 0.45f * sip : 0.0f),
               cupArm ? 0.30f * sip : 0.05f,
               0.11f, 0.11f, 0.11f, 8, 6);
        if (cupArm) drawCup(side * 0.45f, hipY + 0.30f + 0.43f * sip, 0.28f * sip);
    }

    glPushMatrix();
    glTranslatef(0.0f, hipY + 1.25f, 0.0f);
    glRotatef(headTurn, 0.0f, 1.0f, 0.0f);
    sphere(Skin, 0.0f, 0.0f, 0.0f, 0.25f, 0.30f, 0.25f);
    sphere({0.16f, 0.09f, 0.04f}, 0.0f, 0.19f, -0.04f, 0.26f, 0.14f, 0.24f);
    sphere({0.03f, 0.03f, 0.025f}, -0.08f, 0.04f, 0.235f, 0.025f, 0.025f, 0.018f, 6, 4);
    sphere({0.03f, 0.03f, 0.025f},  0.08f, 0.04f, 0.235f, 0.025f, 0.025f, 0.018f, 6, 4);
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
} // namespace

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
}

void drawPondSeating(float animationTime)
{
    // East of the western pond, outside its shoreline. The bench faces west
    // toward the water; its occupants' feet land on the access-side grass.
    glPushMatrix();
    glTranslatef(-57.5f, 0.0f, -42.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    drawBench(4.8f);
    glPushMatrix(); glTranslatef(-1.05f, 0.0f, 0.0f);
    drawVillager(true, 0.6f, animationTime, false); glPopMatrix();
    glPushMatrix(); glTranslatef(1.05f, 0.0f, 0.0f);
    drawVillager(true, 2.2f, animationTime, true); glPopMatrix();
    glPopMatrix();

    // Compact dirt access path from the open field to the bench.
    glColor3f(0.52f, 0.38f, 0.21f);
    glPushMatrix(); glTranslatef(-54.2f, 0.025f, -42.0f);
    Primitives::drawPlane(6.2f, 2.2f); glPopMatrix();
}

void drawRoadsideAmenities(float animationTime, float nightAmount)
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
        drawShop(shop.variant, animationTime, nightAmount);
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
        glPopMatrix();
    }
}

} // namespace Village