#include "objects/Animals.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

namespace Animals {
namespace {

struct Vec3 { float x, y, z; };
struct Color { float r, g, b; };

// Palette
constexpr Color kHide     {0.92f, 0.91f, 0.82f};
constexpr Color kHideDark {0.80f, 0.78f, 0.68f};
constexpr Color kBelly    {0.94f, 0.89f, 0.82f};
constexpr Color kPatch    {0.17f, 0.09f, 0.05f};
constexpr Color kPatchAlt {0.26f, 0.14f, 0.08f};
constexpr Color kPink     {0.90f, 0.63f, 0.65f};
constexpr Color kMuzzle   {0.80f, 0.55f, 0.56f};
constexpr Color kHorn     {0.93f, 0.87f, 0.68f};
constexpr Color kHoof     {0.09f, 0.07f, 0.05f};
constexpr Color kEyeWhite {0.96f, 0.95f, 0.90f};
constexpr Color kBlack    {0.03f, 0.02f, 0.02f};
constexpr Color kTag      {0.95f, 0.80f, 0.10f};
constexpr Color kGrass    {0.28f, 0.60f, 0.16f};
constexpr Color kShadow   {0.09f, 0.13f, 0.07f};

// Fake shading: multiply a color by a factor (<1 darker, >1 lighter).
Color shade(const Color& c, float f)
{
    auto cl = [](float v) { return v > 1.0f ? 1.0f : v; };
    return { cl(c.r * f), cl(c.g * f), cl(c.b * f) };
}

void drawBox(const Color& c, Vec3 p, Vec3 s,
             float rx = 0.0f, float ry = 0.0f, float rz = 0.0f)
{
    glColor3f(c.r, c.g, c.b);
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glRotatef(ry, 0.0f, 1.0f, 0.0f);
    glRotatef(rx, 1.0f, 0.0f, 0.0f);
    glRotatef(rz, 0.0f, 0.0f, 1.0f);
    glScalef(s.x, s.y, s.z);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void drawEllipsoid(const Color& c, Vec3 p, Vec3 s,
                   float rx = 0.0f, float ry = 0.0f, float rz = 0.0f,
                   int slices = 16, int stacks = 12)
{
    glColor3f(c.r, c.g, c.b);
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glRotatef(ry, 0.0f, 1.0f, 0.0f);
    glRotatef(rx, 1.0f, 0.0f, 0.0f);
    glRotatef(rz, 0.0f, 0.0f, 1.0f);
    glScalef(s.x, s.y, s.z);
    Primitives::drawSphere(1.0f, slices, stacks);
    glPopMatrix();
}

// Cylinder grows upward (+Y) from its base position.
void drawCyl(const Color& c, Vec3 p, float radius, float height,
             float rx = 0.0f, float rz = 0.0f, int segments = 12)
{
    glColor3f(c.r, c.g, c.b);
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glRotatef(rx, 1.0f, 0.0f, 0.0f);
    glRotatef(rz, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(radius, height, segments);
    glPopMatrix();
}

// Irregular patch made of three overlapping flat blobs (sign of x = which flank).
void drawPatch(const Color& c, float x, float y, float z, float w, float h)
{
    drawEllipsoid(c, {x,          y,            z},            {0.13f, h,         w},         0, 0, 0, 12, 8);
    drawEllipsoid(c, {x * 0.99f,  y + h * 0.55f, z - w * 0.60f}, {0.12f, h * 0.60f, w * 0.55f}, 0, 0, 0, 12, 8);
    drawEllipsoid(c, {x * 0.99f,  y - h * 0.40f, z + w * 0.70f}, {0.12f, h * 0.55f, w * 0.50f}, 0, 0, 0, 12, 8);
}

void drawLeg(float x, float z, bool rear)
{
    const float side = (x < 0.0f) ? -1.0f : 1.0f;
    const Color upper = shade(kHide, 0.95f);
    const Color lower = shade(kHideDark, 0.90f);

    if (rear) {
        drawEllipsoid(upper, {x * 1.05f, 1.75f, z}, {0.38f, 0.80f, 0.58f});         // thigh
        drawCyl(lower, {x, 0.80f, z}, 0.17f, 0.85f);
        drawEllipsoid(lower, {x, 0.84f, z + 0.03f}, {0.19f, 0.22f, 0.24f});          // hock
    } else {
        drawEllipsoid(upper, {x * 1.02f, 1.85f, z}, {0.32f, 0.72f, 0.48f});         // shoulder
        drawCyl(upper, {x, 0.85f, z}, 0.18f, 0.90f);
        drawEllipsoid(lower, {x, 0.86f, z}, {0.17f, 0.17f, 0.17f});                  // knee
    }

    drawCyl(lower, {x, 0.20f, z}, 0.115f, 0.68f);                                    // cannon bone
    drawEllipsoid(lower, {x, 0.22f, z}, {0.14f, 0.11f, 0.15f});                      // fetlock

    // Cloven hoof
    drawBox(kHoof, {x - 0.07f * side, 0.07f, z - 0.05f}, {0.14f, 0.14f, 0.30f});
    drawBox(kHoof, {x + 0.07f * side, 0.07f, z - 0.05f}, {0.14f, 0.14f, 0.30f});
}

void drawHorn(float side)
{
    drawCyl(kHorn,          {0.40f * side, 1.42f, -2.30f}, 0.080f, 0.24f, 0.0f, -60.0f * side, 8);
    drawCyl(shade(kHorn, 0.95f), {0.61f * side, 1.54f, -2.30f}, 0.058f, 0.24f, 0.0f, -20.0f * side, 8);
    drawEllipsoid(shade(kHorn, 0.75f), {0.66f * side, 1.77f, -2.30f}, {0.05f, 0.07f, 0.05f}, 0, 0, 0, 8, 6);
}

void drawEar(float side, float flick)
{
    glPushMatrix();
    glTranslatef(0.55f * side, 1.20f, -2.28f);
    glRotatef(-30.0f * side + flick * side, 0.0f, 0.0f, 1.0f);
    glRotatef(-10.0f, 1.0f, 0.0f, 0.0f);
    drawEllipsoid(kHideDark, {0.22f * side, 0.0f,   0.0f}, {0.30f, 0.06f, 0.16f}, 0, 0, 0, 10, 8);
    drawEllipsoid(kPink,     {0.22f * side, 0.045f, 0.0f}, {0.22f, 0.03f, 0.11f}, 0, 0, 0, 10, 8);
    if (side > 0.0f)  // ear tag on one ear
        drawBox(kTag, {0.36f, -0.07f, 0.02f}, {0.10f, 0.03f, 0.12f});
    glPopMatrix();
}

void drawHead(float bob, float chew, float graze)
{
    glPushMatrix();
    glTranslatef(0.0f, bob, 0.0f);

    // Two-segment neck with dewlap
    drawEllipsoid(kHide,             {0.0f, 1.90f, -1.50f}, {0.66f, 0.70f, 0.80f}, -22.0f);
    drawEllipsoid(shade(kHide, .97f), {0.0f, 1.52f, -2.00f}, {0.50f, 0.55f, 0.68f}, -42.0f);
    drawEllipsoid(shade(kHideDark, .9f), {0.0f, 1.10f, -1.75f}, {0.26f, 0.42f, 0.55f}, -35.0f);

    // Skull, brow, muzzle
    drawEllipsoid(kHide,   {0.0f, 1.05f, -2.55f}, {0.48f, 0.52f, 0.64f}, -35.0f);
    drawEllipsoid(shade(kHide, .93f), {0.0f, 1.32f, -2.42f}, {0.44f, 0.16f, 0.28f}, -20.0f);
    drawEllipsoid(kMuzzle, {0.0f, 0.64f, -2.98f}, {0.40f, 0.32f, 0.38f}, -20.0f);
    drawEllipsoid(kPink,   {0.0f, 0.64f, -3.20f}, {0.32f, 0.24f, 0.14f}, -20.0f);   // nose pad

    // Lower jaw (chews)
    drawEllipsoid(shade(kHideDark, .9f), {0.0f, 0.45f, -2.90f}, {0.30f, 0.12f, 0.30f}, -10.0f + chew);

    // Nostrils
    for (float s : {-1.0f, 1.0f})
        drawEllipsoid(kBlack, {0.13f * s, 0.68f, -3.32f}, {0.07f, 0.05f, 0.04f}, 0, 0, 0, 8, 6);

    // Eyes: sclera, pupil, eyelid
    for (float s : {-1.0f, 1.0f}) {
        drawEllipsoid(kEyeWhite, {0.40f * s, 1.20f, -2.62f}, {0.10f, 0.09f, 0.09f}, 0, 0, 0, 10, 8);
        drawEllipsoid(kBlack,    {0.46f * s, 1.20f, -2.63f}, {0.06f, 0.07f, 0.07f}, 0, 0, 0, 8, 6);
        drawEllipsoid(shade(kHideDark, .85f), {0.41f * s, 1.28f, -2.62f}, {0.12f, 0.04f, 0.11f}, 0, 0, 0, 8, 6);
    }

    // Forelock tuft
    drawEllipsoid(kPatchAlt, {0.0f, 1.45f, -2.38f}, {0.24f, 0.12f, 0.20f}, -20.0f, 0, 0, 10, 8);

    // Grass hanging from the mouth
    for (float s : {-1.0f, 0.0f, 1.0f})
        drawBox(kGrass, {0.06f * s, 0.42f, -3.22f}, {0.03f, 0.30f, 0.02f},
                graze * 0.5f, 0.0f, 12.0f * s);

    glPopMatrix();
}

void drawUdder()
{
    drawEllipsoid(kPink, {0.0f, 1.02f, 0.85f}, {0.34f, 0.28f, 0.36f});
    for (float sx : {-0.12f, 0.12f})
        for (float sz : {0.72f, 0.98f})
            drawCyl(kMuzzle, {sx, 0.66f, sz}, 0.04f, 0.18f, 0.0f, 0.0f, 8);
}

void drawTail(float sway)
{
    glPushMatrix();
    glTranslatef(0.0f, 2.45f, 1.85f);
    glRotatef(sway, 0.0f, 0.0f, 1.0f);
    glRotatef(170.0f, 1.0f, 0.0f, 0.0f);   // hangs down, slightly away from the body

    drawCyl(shade(kHideDark, .9f), {0.0f, 0.0f, 0.0f}, 0.05f, 1.35f, 0.0f, 0.0f, 8);
    drawEllipsoid(kPatchAlt, {0.0f, 1.45f, 0.0f}, {0.13f, 0.32f, 0.13f}, 0, 0, 0, 10, 8);
    glPopMatrix();
}


} // namespace

void drawCow(float x, float z, float scale, float rotation)
{
    const float t        = static_cast<float>(glfwGetTime());
    const float tailSway = 12.0f * std::sin(t * 1.6f);
    const float earFlick = 6.0f  * std::sin(t * 3.1f);
    const float headBob  = 0.03f * std::sin(t * 0.9f);
    const float breath   = 1.0f + 0.012f * std::sin(t * 1.3f);
    const float chew     = 5.0f  * std::sin(t * 4.5f);

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    // --- Torso: barrel + chest + rump, with breathing ---
    drawEllipsoid(kHide,  {0.0f, 1.70f,  0.00f}, {0.98f * breath, 0.95f * breath, 1.75f});
    drawEllipsoid(kHide,  {0.0f, 1.78f, -1.00f}, {0.92f, 0.98f, 0.85f});                       // chest
    drawEllipsoid(kHide,  {0.0f, 1.82f,  1.05f}, {0.90f, 0.95f, 0.85f});                       // rump
    drawEllipsoid(kBelly, {0.0f, 1.28f,  0.10f}, {0.80f, 0.55f, 1.40f});                       // belly
    drawEllipsoid(shade(kHide, 0.82f), {0.0f, 1.05f, 0.10f}, {0.62f, 0.25f, 1.25f});           // underside shadow

    // Flank and rib contour
    for (float s : {-1.0f, 1.0f})
        for (float rz : {-0.35f, 0.05f, 0.45f})
            drawEllipsoid(shade(kHide, 0.94f), {0.93f * s, 1.55f, rz}, {0.06f, 0.30f, 0.14f}, 0, 0, 0, 8, 6);

    // Withers, spine ridge, hip bones
    drawEllipsoid(kHideDark, {0.0f,   2.62f, -0.85f}, {0.32f, 0.16f, 0.45f});
    drawEllipsoid(kHideDark, {0.0f,   2.62f,  0.20f}, {0.20f, 0.08f, 1.10f});
    drawEllipsoid(kHideDark, {-0.55f, 2.45f,  1.05f}, {0.22f, 0.16f, 0.24f});
    drawEllipsoid(kHideDark, { 0.55f, 2.45f,  1.05f}, {0.22f, 0.16f, 0.24f});

    // --- Patches (different on each flank) ---
    drawPatch(kPatch,     0.90f, 2.00f, -0.30f, 0.65f, 0.45f);
    drawPatch(kPatch,     0.86f, 1.55f,  0.70f, 0.40f, 0.32f);
    drawPatch(kPatchAlt, -0.90f, 1.85f,  0.35f, 0.75f, 0.50f);
    drawPatch(kPatch,    -0.80f, 2.25f, -0.90f, 0.42f, 0.32f);
    drawEllipsoid(kPatchAlt, {0.60f, 2.30f, 1.30f}, {0.32f, 0.30f, 0.40f});
    drawEllipsoid(kPatch,    {0.20f, 2.58f, 0.30f}, {0.42f, 0.12f, 0.50f});                    // back

    // --- Head, ears, horns ---
    drawHead(headBob, chew, tailSway);
    drawEar(-1.0f,  earFlick);
    drawEar( 1.0f, -earFlick);
    drawHorn(-1.0f);
    drawHorn( 1.0f);

    // --- Legs ---
    drawLeg(-0.62f, -1.10f, false);
    drawLeg( 0.62f, -1.10f, false);
    drawLeg(-0.62f,  1.15f, true);
    drawLeg( 0.62f,  1.15f, true);

    // --- Udder and tail ---
    drawUdder();
    drawTail(tailSway);

    glPopMatrix();
}

} // namespace Animals