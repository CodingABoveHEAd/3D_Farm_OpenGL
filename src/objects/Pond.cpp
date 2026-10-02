#include "objects/Pond.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

namespace
{
constexpr float Pi = 3.14159265358979323846f;

struct RGB { float r, g, b; };

// Palette
constexpr RGB kSand      {0.76f, 0.64f, 0.40f};
constexpr RGB kDirt      {0.45f, 0.33f, 0.17f};
constexpr RGB kMud       {0.28f, 0.20f, 0.11f};
constexpr RGB kShallow   {0.24f, 0.62f, 0.70f};
constexpr RGB kDeep      {0.05f, 0.22f, 0.44f};
constexpr RGB kWood      {0.48f, 0.28f, 0.11f};
constexpr RGB kWoodLite  {0.60f, 0.38f, 0.16f};
constexpr RGB kWoodDark  {0.28f, 0.14f, 0.05f};
constexpr RGB kReedD     {0.20f, 0.42f, 0.10f};
constexpr RGB kReedL     {0.40f, 0.60f, 0.18f};
constexpr RGB kCattail   {0.36f, 0.19f, 0.08f};
constexpr RGB kPadD      {0.10f, 0.38f, 0.14f};
constexpr RGB kPadL      {0.22f, 0.58f, 0.22f};
constexpr RGB kPetal     {0.98f, 0.70f, 0.80f};
constexpr RGB kYellow    {0.98f, 0.85f, 0.25f};
constexpr RGB kWhite     {0.95f, 0.94f, 0.88f};
constexpr RGB kGrassD    {0.10f, 0.32f, 0.07f};
constexpr RGB kGrassL    {0.36f, 0.64f, 0.16f};

RGB mix(const RGB& a, const RGB& b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}

void setColor(const RGB& c, float a = 1.0f) { glColor4f(c.r, c.g, c.b, a); }

// Deterministic pseudo-random 0..1
float hash01(int n)
{
    const float s = std::sin(n * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

// Organic outline wobble (periodic in angle, so the shape closes cleanly)
float edge(float a)
{
    return 1.0f + 0.06f * std::sin(3.0f * a + 1.0f)
                + 0.04f * std::sin(5.0f * a + 2.0f)
                + 0.025f * std::sin(9.0f * a);
}

void drawBox(const RGB& c, float x, float y, float z, float sx, float sy, float sz)
{
    setColor(c);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void drawBlob(const RGB& c, float x, float y, float z,
              float sx, float sy, float sz, int slices = 10, int stacks = 7)
{
    setColor(c);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawSphere(1.0f, slices, stacks);
    glPopMatrix();
}

// Flat band between two scaled copies of the wobbly outline.
void band(float y, float rx, float rz, float s0, float s1,
          const RGB& c0, const RGB& c1, int seg = 48)
{
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= seg; ++i)
    {
        const float a = 2.0f * Pi * i / seg;
        const float w = edge(a);
        const float x = std::cos(a) * rx * w;
        const float z = std::sin(a) * rz * w;
        setColor(c0);
        glVertex3f(x * s0, y, z * s0);
        setColor(c1);
        glVertex3f(x * s1, y, z * s1);
    }
    glEnd();
}

// ------------------------------------------------------------
// Water: polar mesh with depth gradient and animated shimmer
// ------------------------------------------------------------
void drawWater(float rx, float rz, float t)
{
    constexpr int rings = 8;
    constexpr int segs = 48;

    auto vertex = [&](float s, int i)
    {
        const float a = 2.0f * Pi * i / segs;
        const float w = edge(a);
        const float px = std::cos(a) * rx * w * s;
        const float pz = std::sin(a) * rz * w * s;

        const float wave = std::sin(t * 1.6f + px * 1.4f + pz * 1.1f);
        const float wave2 = std::sin(t * 2.3f - px * 0.9f + pz * 1.7f);
        const float shimmer = 0.05f * wave + 0.03f * wave2;

        RGB c = mix(kShallow, kDeep, 1.0f - s * s * 0.6f - 0.4f * (1.0f - s));
        glColor4f(c.r + shimmer, c.g + shimmer, c.b + shimmer * 0.7f, 0.88f);
        glVertex3f(px, 0.075f + 0.006f * wave, pz);
    };

    for (int r = 0; r < rings; ++r)
    {
        const float s0 = static_cast<float>(r) / rings;
        const float s1 = static_cast<float>(r + 1) / rings;
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= segs; ++i)
        {
            vertex(s0, i);
            vertex(s1, i);
        }
        glEnd();
    }
}

void drawRipples(float rx, float rz, float t)
{
    glLineWidth(1.5f);
    for (int k = 0; k < 3; ++k)
    {
        const float cx = (k == 0 ? -0.18f : k == 1 ? 0.22f : 0.05f) * rx;
        const float cz = (k == 0 ? 0.10f : k == 1 ? -0.22f : 0.30f) * rz;
        float phase = t * 0.22f + k / 3.0f;
        phase -= std::floor(phase);

        const float radius = 0.05f + phase * 0.28f * (rx < rz ? rx : rz) * 0.5f;
        glColor4f(0.90f, 0.97f, 1.00f, (1.0f - phase) * 0.55f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 28; ++i)
        {
            const float a = 2.0f * Pi * i / 28;
            glVertex3f(cx + std::cos(a) * radius, 0.085f, cz + std::sin(a) * radius * 0.8f);
        }
        glEnd();
    }
}

void drawSparkles(float rx, float rz, float t)
{
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 18; ++i)
    {
        const float a = hash01(i * 3 + 1) * 2.0f * Pi;
        const float r = std::sqrt(hash01(i * 3 + 2)) * 0.72f;
        float tw = std::sin(t * 2.0f + i * 1.7f);
        if (tw < 0.0f) continue;
        tw = tw * tw * tw * tw;
        glColor4f(1.0f, 1.0f, 0.95f, tw);
        glVertex3f(std::cos(a) * rx * r, 0.095f, std::sin(a) * rz * r);
    }
    glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ------------------------------------------------------------
// Lily pads, ducks, rocks, plants
// ------------------------------------------------------------
void drawLilyPad(float x, float z, float radius, float rot, bool flower, float t, int seed)
{
    const float bob = 0.004f * std::sin(t * 1.2f + seed);
    glPushMatrix();
    glTranslatef(x, 0.088f + bob, z);
    glRotatef(rot, 0.0f, 1.0f, 0.0f);

    const float notch = 0.35f;
    glBegin(GL_TRIANGLE_FAN);
    setColor(kPadL);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= 24; ++i)
    {
        const float a = notch + (2.0f * Pi - 2.0f * notch) * i / 24;
        setColor(mix(kPadL, kPadD, 0.5f + 0.5f * std::sin(a * 3.0f + seed)));
        glVertex3f(std::cos(a) * radius, 0.0f, std::sin(a) * radius);
    }
    glEnd();

    if (flower)
    {
        for (int p = 0; p < 6; ++p)
        {
            const float a = 2.0f * Pi * p / 6;
            drawBlob(kPetal, std::cos(a) * 0.07f, 0.04f, std::sin(a) * 0.07f,
                     0.045f, 0.03f, 0.045f, 6, 4);
        }
        drawBlob(kYellow, 0.0f, 0.05f, 0.0f, 0.04f, 0.035f, 0.04f, 6, 4);
    }
    glPopMatrix();
}

void drawDuck(float x, float z, float heading, float bob)
{
    const RGB green{0.08f, 0.38f, 0.22f};
    glPushMatrix();
    glTranslatef(x, 0.045f + bob, z);
    glRotatef(heading, 0.0f, 1.0f, 0.0f);   // model faces +Z

    drawBlob(kWhite, 0.0f, 0.18f, 0.0f, 0.20f, 0.15f, 0.32f);              // body
    drawBlob(kWhite, 0.0f, 0.25f, 0.20f, 0.16f, 0.15f, 0.15f);             // chest
    drawBlob({0.55f, 0.48f, 0.36f}, 0.0f, 0.22f, -0.32f, 0.07f, 0.06f, 0.13f); // tail
    for (float s : {-1.0f, 1.0f})
        drawBlob({0.60f, 0.54f, 0.42f}, 0.17f * s, 0.23f, -0.02f, 0.05f, 0.10f, 0.23f);

    drawBlob(green, 0.0f, 0.38f, 0.24f, 0.07f, 0.12f, 0.07f);              // neck
    drawBlob(kWhite, 0.0f, 0.34f, 0.24f, 0.075f, 0.022f, 0.075f, 8, 4);    // neck ring
    drawBlob(green, 0.0f, 0.50f, 0.28f, 0.11f, 0.10f, 0.11f);              // head
    drawBox({0.95f, 0.65f, 0.12f}, 0.0f, 0.47f, 0.42f, 0.07f, 0.025f, 0.11f); // beak
    for (float s : {-1.0f, 1.0f})
        drawBlob({0.02f, 0.02f, 0.02f}, 0.075f * s, 0.53f, 0.34f, 0.022f, 0.022f, 0.022f, 6, 4);
    glPopMatrix();
}

void drawRock(float x, float z, float scale, int seed)
{
    const float tone = 0.30f + 0.12f * hash01(seed);
    const RGB c{tone, tone + 0.01f, tone - 0.04f};
    glPushMatrix();
    glTranslatef(x, scale * 0.16f, z);
    glRotatef(hash01(seed + 5) * 360.0f, 0.0f, 1.0f, 0.0f);
    drawBlob(c, 0.0f, 0.0f, 0.0f, scale, scale * 0.55f, scale * 0.78f, 7, 5);
    drawBlob(mix(c, {0.5f, 0.55f, 0.35f}, 0.35f),        // mossy top
             0.0f, scale * 0.22f, 0.0f, scale * 0.62f, scale * 0.28f, scale * 0.48f, 7, 4);
    glPopMatrix();
}

void drawGrassTuft(float x, float z, float h, int seed)
{
    glPushMatrix();
    glTranslatef(x, 0.05f, z);
    for (int b = 0; b < 5; ++b)
    {
        const float bh = h * (0.65f + 0.5f * hash01(seed * 11 + b));
        const float w = 0.05f * (0.8f + 0.5f * hash01(seed * 7 + b));
        const float lean = (hash01(seed * 5 + b) - 0.5f) * h * 0.9f;
        glPushMatrix();
        glRotatef(hash01(seed * 3 + b) * 360.0f, 0.0f, 1.0f, 0.0f);
        glBegin(GL_TRIANGLES);
        setColor(kGrassD);
        glVertex3f(-w, 0.0f, 0.0f);
        glVertex3f(w, 0.0f, 0.0f);
        setColor(kGrassL);
        glVertex3f(lean, bh, 0.0f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
}

void drawFlower(float x, float z, float h, const RGB& petal)
{
    drawBox(kReedD, x, 0.05f + h * 0.5f, z, 0.015f, h, 0.015f);
    drawBlob(petal, x, 0.05f + h, z, 0.05f, 0.035f, 0.05f, 6, 4);
    drawBlob(kYellow, x, 0.05f + h + 0.02f, z, 0.02f, 0.015f, 0.02f, 5, 3);
}

void drawReedClump(float x, float z, float h, int seed, float t)
{
    for (int j = 0; j < 6; ++j)
    {
        const float ox = (hash01(seed * 13 + j) - 0.5f) * 0.5f;
        const float oz = (hash01(seed * 17 + j) - 0.5f) * 0.5f;
        const float hh = h * (0.7f + 0.5f * hash01(seed * 7 + j));
        const float lean = (hash01(seed * 5 + j) - 0.5f) * 18.0f;
        const float sway = std::sin(t * 1.3f + seed + j * 0.7f) * 3.0f;

        glPushMatrix();
        glTranslatef(x + ox, 0.05f, z + oz);
        glRotatef(lean + sway, 0.0f, 0.0f, 1.0f);
        drawBox(mix(kReedD, kReedL, 0.5f), 0.0f, hh * 0.5f, 0.0f, 0.035f, hh, 0.035f);
        if (j % 3 == 0)   // cattail
        {
            drawBlob(kCattail, 0.0f, hh * 0.88f, 0.0f, 0.045f, 0.14f, 0.045f, 8, 6);
            drawBox(kReedL, 0.0f, hh + 0.06f, 0.0f, 0.012f, 0.12f, 0.012f);
        }
        glPopMatrix();
    }
}

// ------------------------------------------------------------
// Bridge: beams, piles, planks, posts, double arched rails
// ------------------------------------------------------------
void drawArchRail(float side, float baseY, float rise, float thickness, float length)
{
    constexpr int segments = 12;
    for (int s = 0; s < segments; ++s)
    {
        const float t0 = static_cast<float>(s) / segments;
        const float t1 = static_cast<float>(s + 1) / segments;
        const float z0 = (2.0f * t0 - 1.0f) * length * 0.47f;
        const float z1 = (2.0f * t1 - 1.0f) * length * 0.47f;
        const float y0 = baseY + rise * std::sin(t0 * Pi);
        const float y1 = baseY + rise * std::sin(t1 * Pi);
        const float len = std::sqrt((z1 - z0) * (z1 - z0) + (y1 - y0) * (y1 - y0));

        glPushMatrix();
        glTranslatef(side, 0.5f * (y0 + y1), 0.5f * (z0 + z1));
        glRotatef(-std::atan2(y1 - y0, z1 - z0) * 180.0f / Pi, 1.0f, 0.0f, 0.0f);
        drawBox(kWoodDark, 0.0f, 0.0f, 0.0f, thickness, thickness, len + 0.03f);
        glPopMatrix();
    }
}

void drawBridge(float /*width*/, float depth)
{
    const float L = depth * 0.92f;
    const float deckW = 2.4f;

    // Piles standing in the water
    for (float z : {-L * 0.38f, L * 0.38f})
        for (float x : {-0.78f, 0.78f})
            drawBox(kWoodDark, x, 0.0f, z, 0.34f, 0.80f, 0.34f);

    // Side beams + deck base
    for (float x : {-1.2f, 1.2f})
        drawBox(kWoodDark, x, 0.42f, 0.0f, 0.16f, 0.34f, L);
    drawBox(kWood, 0.0f, 0.42f, 0.0f, deckW, 0.28f, L);

    // Planks with alternating tones and small gaps
    constexpr int planks = 13;
    const float pitch = L / planks;
    for (int i = 0; i < planks; ++i)
    {
        const float z = (i - (planks - 1) * 0.5f) * pitch;
        const RGB c = mix(kWood, kWoodLite, (i % 2 ? 0.7f : 0.15f) + 0.25f * hash01(i + 40));
        drawBox(c, 0.0f, 0.60f, z, deckW + 0.10f, 0.08f, pitch * 0.86f);
    }

    // Posts every quarter span, and two arched rails per side
    for (float side : {-1.0f, 1.0f})
    {
        for (int k = 0; k <= 4; ++k)
        {
            const float t = k / 4.0f;
            const float z = (2.0f * t - 1.0f) * L * 0.47f;
            const float top = 1.50f + 0.48f * std::sin(t * Pi) + 0.06f;
            const float bottom = 0.64f;
            drawBox(kWood, side, 0.5f * (top + bottom), z, 0.17f, top - bottom, 0.17f);
            drawBox(kWoodLite, side, top + 0.03f, z, 0.23f, 0.06f, 0.23f);   // cap
        }
        drawArchRail(side, 1.50f, 0.48f, 0.15f, L);
        drawArchRail(side, 1.05f, 0.32f, 0.09f, L);
    }
}
} // namespace

void Pond::draw(float x, float z, float width, float depth,
                float waterTime, bool withBridge)
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                 GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    const float rx = width * 0.5f;
    const float rz = depth * 0.5f;

    // --- Bank: sand fading to dirt, with a wet mud ring at the waterline ---
    band(0.035f, rx, rz, 0.00f, 1.00f, kSand, kDirt);
    band(0.050f, rx, rz, 0.00f, 0.92f, kMud, kSand);

    // --- Water ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawWater(rx * 0.80f, rz * 0.80f, waterTime);
    drawRipples(rx, rz, waterTime);
    drawSparkles(rx * 0.80f, rz * 0.80f, waterTime);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // --- Lily pads ---
    drawLilyPad(-0.22f * rx * 2.0f * 0.5f, -0.30f * rz, 0.30f,  20.0f, true,  waterTime, 1);
    drawLilyPad( 0.30f * rx, 0.34f * rz, 0.24f, 140.0f, false, waterTime, 2);
    drawLilyPad( 0.38f * rx, -0.10f * rz, 0.28f, 250.0f, true,  waterTime, 3);
    drawLilyPad(-0.38f * rx, 0.22f * rz, 0.22f,  70.0f, false, waterTime, 4);
    drawLilyPad(-0.12f * rx, 0.46f * rz, 0.18f, 200.0f, false, waterTime, 5);

    // --- Ducks (swim in opposite loops) ---
    for (int d = 0; d < 2; ++d)
    {
        const float dir = d == 0 ? 1.0f : -1.0f;
        const float a = waterTime * 0.22f * dir + d * 2.4f;
        const float ox = rx * 0.42f, oz = rz * 0.40f;
        const float px = std::cos(a) * ox;
        const float pz = std::sin(a) * oz;
        const float vx = -std::sin(a) * ox * dir;
        const float vz =  std::cos(a) * oz * dir;
        const float heading = std::atan2(vx, vz) * 180.0f / Pi;
        drawDuck(px, pz, heading, 0.012f * std::sin(waterTime * 2.0f + d * 1.5f));
    }

    // --- Rocks around the edge (skipped where the bridge lands) ---
    constexpr int rockCount = 10;
    for (int i = 0; i < rockCount; ++i)
    {
        const float a = 2.0f * Pi * (i + 0.5f * hash01(i)) / rockCount;
        if (std::fabs(std::cos(a)) * rx < 2.0f) continue;
        const float s = 0.93f + 0.05f * hash01(i + 9);
        const float w = edge(a);
        drawRock(std::cos(a) * rx * w * s, std::sin(a) * rz * w * s,
                 0.30f + 0.32f * hash01(i + 20), i + 1);
    }

    // --- Reeds, grass, and wildflowers ---
    const float reedAngles[4] = {0.5f, 2.2f, 3.6f, 5.4f};
    for (int i = 0; i < 4; ++i)
    {
        const float a = reedAngles[i];
        const float w = edge(a) * 0.87f;
        drawReedClump(std::cos(a) * rx * w, std::sin(a) * rz * w,
                      0.75f + 0.25f * hash01(i + 60), i + 1, waterTime);
    }

    for (int i = 0; i < 16; ++i)
    {
        const float a = 2.0f * Pi * i / 16 + hash01(i + 70) * 0.3f;
        const float s = 0.97f + 0.12f * hash01(i + 80);
        const float w = edge(a) * s;
        drawGrassTuft(std::cos(a) * rx * w, std::sin(a) * rz * w,
                      0.35f + 0.20f * hash01(i + 90), i + 1);
    }

    for (int i = 0; i < 7; ++i)
    {
        const float a = 2.0f * Pi * i / 7 + 0.4f;
        const float w = edge(a) * 1.08f;
        drawFlower(std::cos(a) * rx * w, std::sin(a) * rz * w,
                   0.22f + 0.08f * hash01(i + 100),
                   (i % 2) ? kWhite : kPetal);
    }

    // --- Bridge ---
    if (withBridge)
    {
        drawBridge(width, depth);
    }

    glPopMatrix();
    glPopAttrib();
}