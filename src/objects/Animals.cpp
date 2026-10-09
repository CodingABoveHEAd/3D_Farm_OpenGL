#include "objects/Animals.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <algorithm>
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
constexpr Color kCollar   {0.34f, 0.07f, 0.045f};
constexpr Color kBrass    {0.78f, 0.54f, 0.13f};
constexpr Color kMouth    {0.30f, 0.12f, 0.12f};

// Chicken palette
constexpr Color kComb      {0.80f, 0.08f, 0.07f};
constexpr Color kWattle    {0.86f, 0.14f, 0.12f};
constexpr Color kBeak      {0.93f, 0.66f, 0.18f};
constexpr Color kBeakDark  {0.62f, 0.42f, 0.12f};
constexpr Color kLobe      {0.93f, 0.92f, 0.88f};
constexpr Color kIris      {0.95f, 0.50f, 0.05f};
constexpr Color kShank     {0.86f, 0.74f, 0.32f};
constexpr Color kShankDark {0.66f, 0.54f, 0.22f};
constexpr Color kClaw      {0.45f, 0.38f, 0.25f};

struct Plumage { Color body, breast, saddle, hackle, wing, wingTip, tail, tailSheen; };

constexpr Plumage kHen {
    {0.60f, 0.38f, 0.20f}, {0.78f, 0.56f, 0.32f}, {0.52f, 0.32f, 0.16f}, {0.70f, 0.45f, 0.22f},
    {0.50f, 0.30f, 0.15f}, {0.25f, 0.16f, 0.09f}, {0.18f, 0.12f, 0.08f}, {0.22f, 0.16f, 0.10f}};
constexpr Plumage kRooster {
    {0.62f, 0.20f, 0.08f}, {0.10f, 0.07f, 0.06f}, {0.88f, 0.55f, 0.14f}, {0.92f, 0.62f, 0.16f},
    {0.55f, 0.18f, 0.07f}, {0.10f, 0.07f, 0.05f}, {0.04f, 0.10f, 0.08f}, {0.10f, 0.42f, 0.32f}};

constexpr float kRadToDeg = 57.2957795f;

float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
float smooth(float v)  { v = clamp01(v); return v * v * (3.0f - 2.0f * v); }
Color mix(const Color& a, const Color& b, float t)
{
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}

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

    // Paired dewclaws sit above and behind the main hoof.
    drawEllipsoid(kHoof, {x - 0.08f * side, 0.30f, z + 0.11f}, {0.045f, 0.075f, 0.055f}, 22.0f, 0.0f, 0.0f, 6, 4);
    drawEllipsoid(kHoof, {x + 0.08f * side, 0.30f, z + 0.11f}, {0.045f, 0.075f, 0.055f}, 22.0f, 0.0f, 0.0f, 6, 4);

    // Cloven hoof
    drawBox(kHoof, {x - 0.072f, 0.07f, z - 0.07f}, {0.125f, 0.14f, 0.30f}, 0.0f, -4.0f, 0.0f);
    drawBox(kHoof, {x + 0.072f, 0.07f, z - 0.07f}, {0.125f, 0.14f, 0.30f}, 0.0f,  4.0f, 0.0f);
}

void drawHorn(float side)
{
    drawCyl(kHorn,          {0.40f * side, 1.42f, -2.30f}, 0.080f, 0.24f, 0.0f, -60.0f * side, 8);
    drawEllipsoid(shade(kHorn, 0.72f), {0.45f * side, 1.45f, -2.30f}, {0.09f, 0.045f, 0.09f}, 0, 0, 0, 8, 4);
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

void drawHead(float bob, float chew, float graze, float blink, float bellSway)
{
    glPushMatrix();
    glTranslatef(0.0f, bob, 0.0f);

    // Two-segment neck with dewlap
    drawEllipsoid(kHide,             {0.0f, 1.90f, -1.50f}, {0.66f, 0.70f, 0.80f}, -22.0f);
    drawEllipsoid(shade(kHide, .97f), {0.0f, 1.52f, -2.00f}, {0.50f, 0.55f, 0.68f}, -42.0f);
    drawEllipsoid(shade(kHideDark, .9f), {0.0f, 1.10f, -1.75f}, {0.26f, 0.42f, 0.55f}, -35.0f);

    // A low-profile collar follows the neck contour. The bell hangs from the
    // throat and moves only a little, keeping the silhouette readable.
    drawEllipsoid(kCollar, {0.0f, 1.50f, -1.94f}, {0.52f, 0.10f, 0.51f}, -42.0f, 0.0f, 0.0f, 14, 8);
    glPushMatrix();
    glTranslatef(0.0f, 1.08f, -2.03f);
    glRotatef(bellSway, 0.0f, 0.0f, 1.0f);
    drawBox(shade(kCollar, 0.72f), {0.0f, 0.08f, 0.0f}, {0.075f, 0.20f, 0.07f});
    drawEllipsoid(kBrass, {0.0f, -0.08f, 0.0f}, {0.15f, 0.17f, 0.13f}, 0, 0, 0, 10, 8);
    drawEllipsoid(shade(kBrass, 0.58f), {0.0f, -0.22f, 0.0f}, {0.055f, 0.055f, 0.05f}, 0, 0, 0, 8, 6);
    glPopMatrix();

    // Skull, brow, muzzle
    drawEllipsoid(kHide,   {0.0f, 1.05f, -2.55f}, {0.48f, 0.52f, 0.64f}, -35.0f);
    drawEllipsoid(shade(kHide, .93f), {0.0f, 1.32f, -2.42f}, {0.44f, 0.16f, 0.28f}, -20.0f);
    drawEllipsoid(kMuzzle, {0.0f, 0.64f, -2.98f}, {0.40f, 0.32f, 0.38f}, -20.0f);
    drawEllipsoid(kPink,   {0.0f, 0.64f, -3.20f}, {0.32f, 0.24f, 0.14f}, -20.0f);   // nose pad

    // Lower jaw (chews)
    drawEllipsoid(shade(kHideDark, .9f), {0.0f, 0.45f, -2.90f}, {0.30f, 0.12f, 0.30f}, -10.0f + chew);
    drawEllipsoid(kMouth, {0.0f, 0.50f, -3.22f}, {0.23f, 0.025f, 0.035f}, -12.0f, 0.0f, 0.0f, 10, 4);

    // Nostrils
    for (float s : {-1.0f, 1.0f})
        drawEllipsoid(kBlack, {0.13f * s, 0.68f, -3.32f}, {0.07f, 0.05f, 0.04f}, 0, 0, 0, 8, 6);

    // Eyes: sclera, pupil, eyelid
    for (float s : {-1.0f, 1.0f}) {
        drawEllipsoid(kEyeWhite, {0.40f * s, 1.20f, -2.62f}, {0.10f, 0.09f, 0.09f}, 0, 0, 0, 10, 8);
        drawEllipsoid(kBlack,    {0.46f * s, 1.20f, -2.63f}, {0.06f, 0.07f, 0.07f}, 0, 0, 0, 8, 6);
        drawEllipsoid(kEyeWhite, {0.50f * s, 1.23f, -2.66f}, {0.018f, 0.018f, 0.018f}, 0, 0, 0, 6, 4);
        drawEllipsoid(shade(kHideDark, .85f), {0.41f * s, 1.28f, -2.62f}, {0.12f, 0.04f, 0.11f}, 0, 0, 0, 8, 6);
        if (blink > 0.0f)
            drawEllipsoid(kHideDark, {0.46f * s, 1.20f, -2.64f},
                          {0.075f, 0.075f * blink, 0.075f}, 0, 0, 0, 8, 6);
    }

    // Forelock tuft
    drawEllipsoid(kPatchAlt, {0.0f, 1.45f, -2.38f}, {0.24f, 0.12f, 0.20f}, -20.0f, 0, 0, 10, 8);

    // Grass only hangs from the mouth while the cow is actually grazing.
    if (graze > 0.35f)
        for (float s : {-1.0f, 0.0f, 1.0f})
            drawBox(kGrass, {0.06f * s, 0.42f, -3.22f},
                    {0.03f, 0.22f + 0.10f * graze, 0.02f},
                    std::sin(graze * 4.0f) * 5.0f, 0.0f, 12.0f * s);

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

// ---------------------------------------------------------------------------
// Chicken
// ---------------------------------------------------------------------------

struct ChickenPose {
    float legSwing;    // hip swing in degrees (left leg; right is mirrored)
    float bodyBob;     // small vertical bob
    float bodyPitch;   // forward lean about the hips (pecking)
    float headPeck;    // neck rotation relative to the body
    float headThrust;  // head pushed forward while walking
    float beakOpen;    // degrees
    float blink;       // 0..1
    float wingLift;    // degrees
    float tailSway;    // degrees
    float breath;      // ~1.0
};

void drawChickenLeg(const Plumage& p, float side, float swing, bool rooster)
{
    glPushMatrix();
    glTranslatef(0.20f * side, 0.66f, 0.04f);
    glRotatef(swing, 1.0f, 0.0f, 0.0f);

    // Feathered thigh and "trouser" feathers over the hock
    drawEllipsoid(p.body,              {0.0f, -0.02f, 0.00f}, {0.16f, 0.24f, 0.22f}, -10.0f);
    drawEllipsoid(shade(p.body, 0.90f), {0.0f, -0.20f, 0.05f}, {0.10f, 0.12f, 0.12f}, 15.0f, 0, 0, 10, 8);

    // Scaly shank with scute bands
    drawCyl(kShank, {0.0f, -0.64f, 0.0f}, 0.032f, 0.50f, 0.0f, 0.0f, 8);
    for (int i = 0; i < 4; ++i)
        drawBox(kShankDark, {0.0f, -0.24f - i * 0.10f, 0.0f}, {0.07f, 0.012f, 0.07f});
    if (rooster)  // spur
        drawEllipsoid(kClaw, {0.0f, -0.42f, 0.045f}, {0.016f, 0.045f, 0.016f}, 40.0f, 0, 0, 6, 4);

    // Foot stays roughly flat on the ground
    glTranslatef(0.0f, -0.64f, 0.0f);
    glRotatef(-swing * 0.75f, 1.0f, 0.0f, 0.0f);
    drawEllipsoid(kShankDark, {0.0f, 0.0f, 0.0f}, {0.045f, 0.03f, 0.05f}, 0, 0, 0, 8, 6);

    const float toeAngles[3] = {-30.0f, 0.0f, 30.0f};
    for (float a : toeAngles) {
        glPushMatrix();
        glRotatef(a, 0.0f, 1.0f, 0.0f);
        drawBox(kShank, {0.0f, 0.0f, -0.095f}, {0.028f, 0.022f, 0.19f});
        drawEllipsoid(kClaw, {0.0f, -0.002f, -0.195f}, {0.015f, 0.013f, 0.035f}, 0, 0, 0, 6, 4);
        glPopMatrix();
    }
    drawBox(kShank, {0.0f, 0.0f, 0.05f}, {0.026f, 0.022f, 0.10f});                          // hind toe
    drawEllipsoid(kClaw, {0.0f, -0.002f, 0.105f}, {0.014f, 0.012f, 0.03f}, 0, 0, 0, 6, 4);
    glPopMatrix();
}

// Fan of tail feathers, each built from segments along a curved arc.
void drawTailFan(const Color& c, const Color& sheen, int count, float spread,
                 float radius, float arc, int segs, float width, float sway)
{
    glPushMatrix();
    glTranslatef(0.0f, 0.98f, 0.60f);
    const float step = radius * arc / segs;
    for (int f = 0; f < count; ++f) {
        const float k = (count > 1) ? (static_cast<float>(f) / (count - 1)) * 2.0f - 1.0f : 0.0f;
        glPushMatrix();
        glRotatef(k * spread + sway, 0.0f, 0.0f, 1.0f);
        for (int i = 0; i < segs; ++i) {
            const float u = (i + 0.5f) / segs;
            const float a = u * arc;
            const Color col = shade(mix(c, sheen, u), (i % 2) ? 1.0f : 0.88f);
            drawEllipsoid(col,
                          {0.0f, radius * std::sin(a), radius * (1.0f - std::cos(a)) + f * 0.004f},
                          {width * (1.0f - 0.35f * u), step * 0.70f, 0.045f},
                          a * kRadToDeg, 0.0f, 0.0f, 6, 4);
        }
        glPopMatrix();
    }
    glPopMatrix();
}

void drawChickenWing(const Plumage& p, float side, float lift)
{
    glPushMatrix();
    glTranslatef(0.42f * side, 1.06f, -0.14f);
    glRotatef(lift * side, 0.0f, 0.0f, 1.0f);
    glRotatef(8.0f, 1.0f, 0.0f, 0.0f);

    drawEllipsoid(p.wing, {0.04f * side, -0.20f, 0.14f}, {0.08f, 0.30f, 0.40f}, 10.0f, 0, 0, 14, 10);
    for (int i = 0; i < 3; ++i)   // covert scallops
        drawEllipsoid(shade(p.wing, (i % 2) ? 1.10f : 0.92f),
                      {0.09f * side, -0.10f - i * 0.10f, 0.02f + i * 0.08f},
                      {0.035f, 0.08f, 0.12f}, 10.0f, 0, 0, 8, 6);
    for (int i = 0; i < 4; ++i)   // primaries trailing back
        drawEllipsoid((i % 2) ? p.wingTip : shade(p.wingTip, 1.25f),
                      {0.07f * side, -0.30f - i * 0.015f, 0.30f + i * 0.09f},
                      {0.04f, 0.09f, 0.16f}, 14.0f + i * 3.0f, 0, 0, 8, 6);
    glPopMatrix();
}

void drawChickenBody(const Plumage& p, bool rooster, float breath)
{
    drawEllipsoid(p.body,   {0.0f, 0.88f,  0.06f}, {0.48f * breath, 0.44f * breath, 0.66f}, -6.0f, 0, 0, 18, 14);
    drawEllipsoid(p.breast, {0.0f, 0.86f, -0.36f}, {0.40f * breath, 0.42f * breath, 0.34f}, 0, 0, 0, 16, 12);
    drawEllipsoid(shade(p.breast, 0.88f), {0.0f, 0.62f, 0.04f}, {0.40f, 0.22f, 0.52f}, 0, 0, 0, 14, 10);
    drawEllipsoid(p.saddle, {0.0f, 1.08f,  0.32f}, {0.34f, 0.20f, 0.40f}, -12.0f, 0, 0, 14, 10);
    drawEllipsoid(p.saddle, {0.0f, 1.12f, -0.16f}, {0.30f, 0.14f, 0.28f}, 0, 0, 0, 12, 8);
    drawEllipsoid(shade(p.body, 0.95f), {0.0f, 0.84f, 0.64f}, {0.30f, 0.28f, 0.26f}, 0, 0, 0, 12, 10);

    if (rooster)  // long saddle hackles draped over the flanks
        for (float s : {-1.0f, 1.0f})
            drawEllipsoid(p.saddle, {0.30f * s, 0.95f, 0.40f}, {0.08f, 0.20f, 0.30f}, 0, 0, 20.0f * s, 10, 8);

    // Overlapping feather scales that follow the flank contour
    for (float s : {-1.0f, 1.0f}) {
        for (int i = 0; i < 6; ++i) {
            const float dz = -0.45f + i * 0.17f;
            const float k  = std::sqrt(std::max(0.0f, 1.0f - (dz / 0.66f) * (dz / 0.66f)));
            for (int j = 0; j < 3; ++j) {
                const float phi = (-25.0f + j * 30.0f) / kRadToDeg;
                const Color base = (i < 2) ? p.breast : p.body;
                drawEllipsoid(shade(base, ((i + j) % 2) ? 0.90f : 1.06f),
                              {s * 0.48f * std::cos(phi) * k * 0.98f,
                               0.88f + 0.44f * std::sin(phi) * k,
                               0.06f + dz},
                              {0.035f, 0.075f, 0.12f},
                              0.0f, 0.0f, s * phi * kRadToDeg, 8, 6);
            }
        }
    }
}

void drawChickenHead(const Plumage& p, bool rooster, const ChickenPose& k)
{
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -k.headThrust);
    glTranslatef(0.0f, 1.00f, -0.45f);          // pivot at the base of the neck
    glRotatef(-k.headPeck, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -1.00f, 0.45f);

    // S-curved neck
    drawEllipsoid(p.hackle,             {0.0f, 1.12f, -0.58f}, {0.20f, 0.26f, 0.20f}, -15.0f, 0, 0, 12, 10);
    drawEllipsoid(shade(p.hackle, 0.95f), {0.0f, 1.34f, -0.66f}, {0.14f, 0.22f, 0.14f}, -12.0f, 0, 0, 12, 10);

    // Hackle collar
    const float hLen = rooster ? 0.20f : 0.15f;
    for (int i = 0; i < 10; ++i) {
        const float a = i * 36.0f;
        const float r = a / kRadToDeg;
        drawEllipsoid(shade(p.hackle, (i % 2) ? 1.0f : 0.88f),
                      {0.17f * std::sin(r), 1.06f, -0.58f + 0.17f * std::cos(r)},
                      {0.065f, hLen, 0.05f}, 30.0f, a, 0.0f, 6, 4);
    }
    for (int i = 0; i < 8; ++i) {
        const float a = i * 45.0f + 20.0f;
        const float r = a / kRadToDeg;
        drawEllipsoid(shade(p.hackle, (i % 2) ? 0.92f : 1.05f),
                      {0.12f * std::sin(r), 1.26f, -0.66f + 0.12f * std::cos(r)},
                      {0.05f, hLen * 0.75f, 0.04f}, 25.0f, a, 0.0f, 6, 4);
    }

    // Head, face patch, ear lobes
    drawEllipsoid(shade(p.hackle, 1.05f), {0.0f, 1.55f, -0.72f}, {0.13f, 0.14f, 0.16f}, 0, 0, 0, 14, 10);
    for (float s : {-1.0f, 1.0f}) {
        drawEllipsoid(kComb, {0.098f * s, 1.54f, -0.82f}, {0.03f, 0.065f, 0.07f}, 0, 0, 0, 8, 6);
        drawEllipsoid(kLobe, {0.122f * s, 1.49f, -0.70f}, {0.02f, 0.04f, 0.04f}, 0, 0, 0, 8, 6);
    }

    // Eyes with iris, pupil and eyelid
    for (float s : {-1.0f, 1.0f}) {
        drawEllipsoid(kIris,  {0.116f * s, 1.57f, -0.78f}, {0.022f, 0.030f, 0.030f}, 0, 0, 0, 8, 6);
        drawEllipsoid(kBlack, {0.130f * s, 1.57f, -0.78f}, {0.012f, 0.018f, 0.018f}, 0, 0, 0, 6, 4);
        drawEllipsoid(shade(kComb, 0.8f),
                      {0.120f * s, 1.595f - 0.025f * k.blink, -0.78f},
                      {0.026f, 0.020f + 0.022f * k.blink, 0.034f}, 0, 0, 0, 8, 6);
    }

    // Beak (lower half opens when pecking)
    drawEllipsoid(kBeak,     {0.0f, 1.52f, -0.90f}, {0.050f, 0.040f, 0.120f}, -10.0f, 0, 0, 10, 8);
    drawEllipsoid(kBeakDark, {0.0f, 1.505f, -1.00f}, {0.025f, 0.020f, 0.050f}, -14.0f, 0, 0, 8, 6);
    drawEllipsoid(kBeak,     {0.0f, 1.465f, -0.88f}, {0.040f, 0.025f, 0.090f}, -k.beakOpen, 0, 0, 8, 6);

    // Serrated comb and wattles
    const int   n    = rooster ? 5 : 3;
    const float size = rooster ? 1.8f : 1.0f;
    for (int i = 0; i < n; ++i) {
        const float h  = 0.5f + 0.5f * std::sin(3.14159f * i / (n - 1));
        const float sy = 0.035f + h * 0.045f * size;
        drawEllipsoid(kComb, {0.0f, 1.675f + sy * 0.5f, -0.80f + i * (0.17f / (n - 1))},
                      {0.020f, sy, 0.035f}, 0, 0, 0, 8, 6);
    }
    for (float s : {-1.0f, 1.0f})
        drawEllipsoid(kWattle, {0.025f * s, 1.39f, -0.86f},
                      {0.022f, rooster ? 0.08f : 0.045f, 0.03f}, 0, 0, 0, 8, 6);

    glPopMatrix();
}

void drawChickenModel(const Plumage& p, bool rooster, const ChickenPose& k)
{
    drawChickenLeg(p, -1.0f,  k.legSwing, rooster);
    drawChickenLeg(p,  1.0f, -k.legSwing, rooster);

    glPushMatrix();
    glTranslatef(0.0f, k.bodyBob, 0.0f);
    glTranslatef(0.0f, 0.66f, 0.04f);                 // lean about the hips
    glRotatef(-k.bodyPitch, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, -0.66f, -0.04f);

    drawChickenBody(p, rooster, k.breath);
    drawChickenWing(p, -1.0f, k.wingLift);
    drawChickenWing(p,  1.0f, k.wingLift);

    if (rooster) {
        drawTailFan(p.tail, p.tailSheen, 5, 24.0f, 0.32f, 1.0f, 4, 0.07f, k.tailSway);
        drawTailFan(p.tail, p.tailSheen, 3, 14.0f, 0.55f, 1.9f, 8, 0.045f, k.tailSway * 1.4f);
    } else {
        drawTailFan(p.tail, p.tailSheen, 5, 16.0f, 0.30f, 0.9f, 4, 0.07f, k.tailSway);
    }

    drawChickenHead(p, rooster, k);
    glPopMatrix();
}

} // namespace

void drawCow(float x, float z, float scale, float rotation, float animationTime)
{
    const float t        = animationTime;
    const float seed     = x * 0.37f + z * 0.23f;
    const int coatVariant = std::abs(static_cast<int>(seed * 10.0f)) % 3;
    const float grazeRate = 0.34f
        + 0.13f * (0.5f + 0.5f * std::sin(seed * 2.17f));
    const float grazeWave = 0.5f + 0.5f * std::sin(
        t * grazeRate + x * 0.31f + z * 0.17f);
    const float grazing = clamp01((grazeWave - 0.20f) / 0.42f);
    const float flySwat = std::pow(std::max(0.0f, std::sin(t * 0.73f + seed)), 10.0f);
    const float tailSway = 7.0f * std::sin(t * 1.15f + seed) + 20.0f * flySwat;
    const float earFlick = 18.0f * std::pow(
        std::max(0.0f, std::sin(t * 0.61f + seed * 1.7f)), 16.0f);
    const float headBob  = 0.03f * std::sin(t * 0.9f + seed)
        + (1.0f - grazing) * 0.90f;
    const float breath   = 1.0f + 0.012f * std::sin(t * 1.3f + seed);
    const float chew     = grazing * 5.0f * std::sin(t * 4.5f + seed);
    const float blinkCycle = std::fmod(t * 0.29f + std::fabs(seed), 1.0f);
    const float blink    = blinkCycle < 0.045f
        ? std::sin(blinkCycle / 0.045f * 3.14159265f) : 0.0f;
    const float bellSway = 3.0f * std::sin(t * 0.8f + seed) * (0.25f + grazing);

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    // A soft contact patch grounds the hooves and makes the leg spacing much
    // easier to read, especially under the low night lighting.
    drawEllipsoid(kShadow, {0.0f, 0.018f, 0.05f}, {0.92f, 0.018f, 1.72f}, 0, 0, 0, 16, 6);

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
    drawPatch(kPatch, 0.90f, 2.00f, -0.30f, 0.65f, 0.45f);
    if (coatVariant != 1)
        drawPatch(kPatch, 0.86f, 1.55f, 0.70f, 0.40f, 0.32f);
    if (coatVariant != 2)
        drawPatch(kPatchAlt, -0.90f, 1.85f, 0.35f, 0.75f, 0.50f);
    if (coatVariant == 0)
        drawPatch(kPatch, -0.80f, 2.25f, -0.90f, 0.42f, 0.32f);
    else if (coatVariant == 1)
        drawPatch(kPatchAlt, -0.86f, 1.55f, -0.62f, 0.52f, 0.40f);
    else
        drawPatch(kPatch, -0.88f, 2.02f, 0.82f, 0.58f, 0.36f);
    if (coatVariant != 2)
        drawEllipsoid(kPatchAlt, {0.60f, 2.30f, 1.30f}, {0.32f, 0.30f, 0.40f});
    if (coatVariant != 1)
        drawEllipsoid(kPatch, {0.20f, 2.58f, 0.30f}, {0.42f, 0.12f, 0.50f});

    // --- Head, ears, horns ---
    // A small grazed patch makes the feeding action readable at a distance.
    for (int blade = -2; blade <= 2; ++blade)
        drawBox(kGrass, {blade * 0.18f, 0.16f, -3.05f + 0.08f * (blade & 1)},
                {0.035f, 0.32f, 0.035f}, blade * 8.0f, 0.0f, blade * 4.0f);
    drawHead(headBob, chew, grazing, blink, bellSway);
    glPushMatrix();
    glTranslatef(0.0f, headBob, 0.0f);
    drawEar(-1.0f,  earFlick);
    drawEar( 1.0f, -earFlick);
    drawHorn(-1.0f);
    drawHorn( 1.0f);
    glPopMatrix();

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

void drawChicken(float x, float z, float scale, float rotation,
                 float animationTime, bool rooster)
{
    const float t    = animationTime;
    const float seed = x * 0.37f + z * 0.19f;

    // Wander on a circle, speeding up and slowing down; legs are driven by
    // distance travelled, so they stop when the bird stops to forage.
    constexpr float kWander = 0.55f, kOmega = 0.9f, kRadius = 1.1f;
    const float amp   = 0.8f * kWander / kOmega;
    const float ang   = rotation / kRadToDeg + seed + kWander * t + amp * std::sin(kOmega * t + seed);
    const float speed = 1.0f + 0.8f * std::cos(kOmega * t + seed);       // 0.2 .. 1.8

    const float px  = x + kRadius * std::sin(ang);
    const float pz  = z + kRadius * std::cos(ang);
    const float yaw = std::atan2(-std::cos(ang), std::sin(ang)) * kRadToDeg
                    + 6.0f * std::sin(t * 1.3f + seed);

    const float forage  = smooth((0.6f - speed) / 0.4f);                  // 1 = pecking
    const float walkAmp = clamp01(speed * 0.9f);
    const float legPh   = ang * 23.0f;
    const float pulse   = std::pow(0.5f + 0.5f * std::sin(t * 9.0f + seed * 3.0f), 3.0f) * forage;
    const float blinkCy = std::fmod(t * 0.37f + seed, 1.0f);

    ChickenPose pose;
    pose.legSwing   = 24.0f * walkAmp * std::sin(legPh);
    pose.bodyBob    = 0.015f * walkAmp * std::sin(legPh * 2.0f);
    pose.bodyPitch  = 8.0f * forage + 20.0f * pulse;
    pose.headPeck   = 20.0f * forage + 62.0f * pulse;
    pose.headThrust = 0.06f * walkAmp * std::sin(legPh * 2.0f + 1.0f);
    pose.beakOpen   = 16.0f * pulse;
    pose.blink      = (blinkCy < 0.04f) ? 1.0f : 0.0f;
    pose.wingLift   = 3.0f + 2.5f * walkAmp * std::sin(legPh * 2.0f) + 1.5f * std::sin(t * 0.8f + seed);
    pose.tailSway   = 4.0f * std::sin(t * 1.7f + seed) + 5.0f * walkAmp * std::sin(legPh);
    pose.breath     = 1.0f + 0.012f * std::sin(t * 2.1f + seed);

    glPushMatrix();
    glTranslatef(px, 0.0f, pz);
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);
    drawChickenModel(rooster ? kRooster : kHen, rooster, pose);
    glPopMatrix();
}

} // namespace Animals
