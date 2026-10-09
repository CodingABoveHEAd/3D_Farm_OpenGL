#include "FarmWorld.h"

#include "Camera.h"
#include "Input.h"
#include "graphics/Primitives.h"
#include "graphics/TextureManager.h"
#include "objects/Bridge.h"
#include "objects/PowerPlant.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

using namespace FarmLayout;

// ============================================================
//  OVERVIEW OF THIS FILE
//
//  1. Tunable numbers
//  2. Per-farm data (where each farm stands, how it is turned)
//  3. Small drawing helpers (box, cylinder, trees, rocks ...)
//  4. World data: roads, ponds, fields (built once)
//  5. "Is this spot free?" test used by every scatter routine
//  6. Flat ground decals  -> display list #1  (groundList_)
//  7. 3D scenery          -> display list #2  (sceneryList_)
//  8. Ground that follows the camera (the endless-world trick)
//  9. FarmWorld class functions (atmosphere, travel, update, draw)
// ============================================================

namespace
{
// ------------------------------------------------------------
// 1. Tunable numbers
// ------------------------------------------------------------
constexpr float Pi = 3.14159265358979323846f;
constexpr float DegToRad = Pi / 180.0f;

// Fog: the far ground fades into the horizon colour of the sky dome
// (see sky.cpp: 'horizon'), so there is no visible edge of the world.
constexpr float FogStart = 110.0f;
constexpr float FogEnd   = 420.0f;
constexpr float DayFog[3]   = {0.84f, 0.92f, 0.98f};
constexpr float NightFog[3] = {0.10f, 0.13f, 0.28f};

constexpr float DetailDistance = 85.0f;   // closer than this -> full detail farm
constexpr float DrawDistance   = 380.0f;  // farther than this -> farm not drawn
constexpr float FarmRadius     = 42.0f;   // used for the "is it behind me" test
constexpr float ViewCullAngle  = 62.0f;   // degrees, generous so nothing pops

constexpr float PlayPad   = 70.0f;  // camera may go this far past the outer farms
constexpr float RegionPad = 150.0f; // scenery is generated this far past the farms
constexpr float MinCameraHeight = 1.0f;
constexpr float MaxCameraHeight = 55.0f;

constexpr float GroundY = -0.04f;   // everything flat sits around here

// ------------------------------------------------------------
// 2. Per-farm data
//    (the farm itself is still drawn by Scene.cpp; these numbers
//     only say where it stands and how it is turned / scaled)
// ------------------------------------------------------------
const float kYaw[FarmCount] =
    {0.0f, 7.0f, -5.0f, 12.0f, -9.0f, 4.0f, -13.0f, 8.0f, -6.0f};

const float kScale[FarmCount] =
    {1.00f, 0.96f, 1.04f, 0.98f, 1.03f, 0.95f, 1.06f, 1.01f, 0.97f};

// What surrounds each farm (outside its fence).
struct FarmStyle
{
    int  pondSide;    // -1 pond on the left, +1 pond on the right
    int  hayBales;    // hay bales near the front gate
    bool orchard;     // small orchard on the side opposite the pond
    bool windbreak;   // row of pines behind the farm
};

const FarmStyle kStyle[FarmCount] = {
    {-1, 3, false, false},   // 0
    { 1, 4, true,  false},   // 1
    {-1, 2, false, true },   // 2
    { 1, 3, false, false},   // 3
    {-1, 5, true,  false},   // 4
    { 1, 2, false, true },   // 5
    {-1, 4, false, false},   // 6
    { 1, 3, true,  false},   // 7
    { 1, 2, false, true }    // 8
};

// ------------------------------------------------------------
// 3. Small helpers
// ------------------------------------------------------------
struct Vec2 { float x, z; };

float clamp01(float t) { return std::max(0.0f, std::min(1.0f, t)); }

float smoothstep(float t)
{
    t = clamp01(t);
    return t * t * (3.0f - 2.0f * t);
}

// Deterministic random numbers so every run builds the same world
float rnd(unsigned int& seed)
{
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>((seed >> 8) & 0xFFFFFF) / 16777216.0f;
}

float range(unsigned int& seed, float lo, float hi)
{
    return lo + (hi - lo) * rnd(seed);
}

// Stateless random number from two integers (used for the ground tiles,
// so the same tile always gets the same colour wherever the camera is).
float hash2(int x, int y)
{
    unsigned int h = static_cast<unsigned int>(x) * 374761393u +
                     static_cast<unsigned int>(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return static_cast<float>(h & 0xFFFFFF) / 16777216.0f;
}

void box(float r, float g, float b,
         float x, float y, float z,
         float sx, float sy, float sz)
{
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

// Upright cylinder. y is the CENTRE height, so the cylinder covers
// y - height/2 .. y + height/2.  Written here (instead of using
// Primitives::drawCylinder) so that trees always stand on the ground,
// whatever origin Primitives uses.  The bottom is a little darker and
// the top cap lighter, which gives depth without any lighting.
void cylY(float r, float g, float b,
          float x, float y, float z,
          float radius, float height, int segments)
{
    const float y0 = y - height * 0.5f;
    const float y1 = y + height * 0.5f;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        const float a = 2.0f * Pi * static_cast<float>(i) / static_cast<float>(segments);
        const float px = x + std::cos(a) * radius;
        const float pz = z + std::sin(a) * radius;
        glColor3f(r, g, b);
        glVertex3f(px, y1, pz);
        glColor3f(r * 0.80f, g * 0.80f, b * 0.80f);
        glVertex3f(px, y0, pz);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glColor3f(std::min(1.0f, r * 1.15f), std::min(1.0f, g * 1.15f), std::min(1.0f, b * 1.15f));
    glVertex3f(x, y1, z);
    for (int i = 0; i <= segments; ++i)
    {
        const float a = 2.0f * Pi * static_cast<float>(i) / static_cast<float>(segments);
        glVertex3f(x + std::cos(a) * radius, y1, z + std::sin(a) * radius);
    }
    glEnd();
}

// Cylinder lying along X (hay bales)
void cylX(float r, float g, float b,
          float x, float y, float z,
          float radius, float length, float yawDegrees)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(yawDegrees, 0.0f, 1.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    cylY(r, g, b, 0.0f, 0.0f, 0.0f, radius, length, 10);
    glPopMatrix();
}

// Flat ellipse lying on the ground (dirt patches, pond). It is drawn
// without depth testing, so the order of calls decides what is on top.
void ellipseFlat(float cx, float cz, float rx, float rz, float rotDeg,
                 float r, float g, float b,
                 float er, float eg, float eb, int segs = 20)
{
    glPushMatrix();
    glTranslatef(cx, GroundY + 0.01f, cz);
    glRotatef(rotDeg, 0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(r, g, b);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glColor3f(er, eg, eb);
    for (int i = 0; i <= segs; ++i)
    {
        const float a = 2.0f * Pi * static_cast<float>(i) / static_cast<float>(segs);
        glVertex3f(std::cos(a) * rx, 0.0f, std::sin(a) * rz);
    }
    glEnd();
    glPopMatrix();
}

// Flat rectangle on the ground, corners (x0,z0) - (x1,z1)
void rectFlat(float x0, float z0, float x1, float z1, float r, float g, float b)
{
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(x0, GroundY + 0.01f, z0);
    glVertex3f(x1, GroundY + 0.01f, z0);
    glVertex3f(x1, GroundY + 0.01f, z1);
    glVertex3f(x0, GroundY + 0.01f, z1);
    glEnd();
}

// ---- Scenery pieces (same look as before; segs lets far trees be cheaper)
void drawPine(float x, float z, float s, float shade, int segs = 9)
{
    cylY(0.33f, 0.21f, 0.10f, x, 0.7f * s, z, 0.25f * s, 1.4f * s, 6);

    for (int i = 0; i < 3; ++i)
    {
        const float radius = (1.55f - i * 0.42f) * s;
        const float y = (1.7f + i * 0.95f) * s;

        cylY(0.06f * shade, 0.30f * shade, 0.12f * shade,
             x, y, z, radius, 1.15f * s, segs);
    }
}

void drawRoundTree(float x, float z, float s, float shade, int segs = 10)
{
    cylY(0.38f, 0.25f, 0.12f, x, 1.0f * s, z, 0.28f * s, 2.0f * s, 6);

    cylY(0.16f * shade, 0.45f * shade, 0.12f * shade,
         x, 2.8f * s, z, 1.5f * s, 1.8f * s, segs);

    cylY(0.20f * shade, 0.52f * shade, 0.14f * shade,
         x, 3.9f * s, z, 0.95f * s, 1.1f * s, segs);
}

void drawBush(float x, float z, float s)
{
    cylY(0.12f, 0.38f, 0.10f, x, 0.35f * s, z, 0.75f * s, 0.7f * s, 8);
    cylY(0.16f, 0.45f, 0.12f, x + 0.3f * s, 0.55f * s, z + 0.2f * s,
         0.45f * s, 0.6f * s, 8);
}

void drawRock(float x, float z, float s, float yaw)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);

    box(0.45f, 0.45f, 0.43f, 0.0f, 0.30f * s, 0.0f,
        1.3f * s, 0.6f * s, 0.9f * s);
    box(0.52f, 0.52f, 0.50f, 0.25f * s, 0.55f * s, 0.1f * s,
        0.7f * s, 0.4f * s, 0.6f * s);

    glPopMatrix();
}

void drawHayBale(float x, float z, float yaw)
{
    cylX(0.86f, 0.72f, 0.30f, x, 0.62f, z, 0.62f, 1.2f, yaw);
}

// Reeds along one edge of a pond (the water itself is a flat decal)
void drawReeds(float x, float z, float w, float d)
{
    for (int i = 0; i < 6; ++i)
    {
        box(0.30f, 0.45f, 0.18f,
            x - w * 0.5f + i * (w / 5.0f), 0.45f, z - d * 0.5f - 0.6f,
            0.08f, 0.9f, 0.08f);
    }
}

// A little clump of grass blades (3 thin boxes leaning apart)
void drawTuft(float x, float z, float s)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    for (int i = 0; i < 3; ++i)
    {
        glPushMatrix();
        glRotatef(i * 60.0f, 0.0f, 1.0f, 0.0f);
        glRotatef((i - 1) * 14.0f, 0.0f, 0.0f, 1.0f);
        box(0.26f, 0.55f, 0.16f, 0.0f, 0.25f * s, 0.0f, 0.07f, 0.5f * s, 0.07f);
        glPopMatrix();
    }
    glPopMatrix();
}

// A few tiny flowers
void drawFlowers(float x, float z, unsigned int& seed)
{
    static const float palette[4][3] = {
        {0.95f, 0.85f, 0.20f}, {0.96f, 0.96f, 0.92f},
        {0.90f, 0.40f, 0.60f}, {0.60f, 0.45f, 0.85f}};

    const float* c = palette[static_cast<int>(rnd(seed) * 3.99f)];
    for (int i = 0; i < 7; ++i)
    {
        const float px = x + range(seed, -1.3f, 1.3f);
        const float pz = z + range(seed, -1.3f, 1.3f);
        box(0.20f, 0.45f, 0.14f, px, 0.12f, pz, 0.04f, 0.24f, 0.04f);
        box(c[0], c[1], c[2], px, 0.27f, pz, 0.13f, 0.09f, 0.13f);
    }
}

void drawHill(float x, float z, float radius, float height, float shade)
{
    // Stepped low-poly hill (as before, one more step and more sides)
    for (int i = 0; i < 4; ++i)
    {
        const float k = 1.0f - i * 0.24f;
        cylY(0.18f * shade, 0.42f * shade + i * 0.03f, 0.16f * shade,
             x, height * (0.125f + i * 0.25f), z,
             radius * k, height * 0.26f, 14);
    }
}

// Power pole turned so its cross-arm is square to the line direction
void drawPowerPole(float x, float z, float angleDeg)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(angleDeg, 0.0f, 1.0f, 0.0f);

    cylY(0.35f, 0.25f, 0.15f, 0.0f, 4.0f, 0.0f, 0.16f, 8.0f, 6);
    box(0.30f, 0.22f, 0.12f, 0.0f, 7.3f, 0.0f, 3.0f, 0.14f, 0.14f);

    for (int i = -1; i <= 1; i += 2)
    {
        box(0.85f, 0.85f, 0.88f, i * 1.3f, 7.5f, 0.0f, 0.12f, 0.22f, 0.12f);
    }
    glPopMatrix();
}

// ============================================================
// BÉZIER CURVES — Required Grading Feature
//
// Power line sagging catenary modeled using Quadratic Bézier curves:
// B(t) = (1-t)² P₀ + 2(1-t)t P₁ + t² P₂   for 0 ≤ t ≤ 1
//
// P₀ = top of pole 1 insulator
// P₂ = top of pole 2 insulator
// P₁ = mid-span control point dropped downwards by gravity sag
// ============================================================
void drawWire(float x1, float z1, float x2, float z2)
{
    const float dx = x2 - x1;
    const float dz = z2 - z1;
    const float length = std::sqrt(dx * dx + dz * dz);
    if (length < 0.1f) return;

    // Perpendicular normal in XZ plane for dual wire spacing
    const float nx = -dz / length;
    const float nz = dx / length;

    constexpr int segments = 12;
    const float sag = std::min(1.85f, length * 0.045f);

    for (int i = -1; i <= 1; i += 2)
    {
        const float armOffset = i * 1.3f;
        const float p0x = x1 + nx * armOffset;
        const float p0z = z1 + nz * armOffset;
        const float p0y = 7.6f;

        const float p2x = x2 + nx * armOffset;
        const float p2z = z2 + nz * armOffset;
        const float p2y = 7.6f;

        // Control point dropped down by gravity
        const float p1x = (p0x + p2x) * 0.5f;
        const float p1z = (p0z + p2z) * 0.5f;
        const float p1y = 7.6f - sag;

        for (int s = 0; s < segments; ++s)
        {
            const float tA = static_cast<float>(s) / segments;
            const float tB = static_cast<float>(s + 1) / segments;
            const float uA = 1.0f - tA;
            const float uB = 1.0f - tB;

            // B(t) = u^2*P0 + 2*u*t*P1 + t^2*P2
            const float ax = uA*uA*p0x + 2.0f*uA*tA*p1x + tA*tA*p2x;
            const float ay = uA*uA*p0y + 2.0f*uA*tA*p1y + tA*tA*p2y;
            const float az = uA*uA*p0z + 2.0f*uA*tA*p1z + tA*tA*p2z;

            const float bx = uB*uB*p0x + 2.0f*uB*tB*p1x + tB*tB*p2x;
            const float by = uB*uB*p0y + 2.0f*uB*tB*p1y + tB*tB*p2y;
            const float bz = uB*uB*p0z + 2.0f*uB*tB*p1z + tB*tB*p2z;

            const float segLen = std::sqrt((bx-ax)*(bx-ax) + (by-ay)*(by-ay) + (bz-az)*(bz-az));
            const float midX = (ax + bx) * 0.5f;
            const float midY = (ay + by) * 0.5f;
            const float midZ = (az + bz) * 0.5f;
            const float angleY = std::atan2(bx - ax, bz - az) * 180.0f / Pi;

            glPushMatrix();
            glTranslatef(midX, midY, midZ);
            glRotatef(angleY, 0.0f, 1.0f, 0.0f);
            box(0.06f, 0.06f, 0.06f, 0.0f, 0.0f, 0.0f, 0.04f, 0.04f, segLen * 1.02f);
            glPopMatrix();
        }
    }
}

// Coloured sign at each farm entrance with N dots = farm number
void drawFarmSign(int index, float x, float z)
{
    static const float palette[9][3] = {
        {0.85f, 0.20f, 0.15f}, {0.15f, 0.45f, 0.85f}, {0.95f, 0.75f, 0.10f},
        {0.20f, 0.65f, 0.30f}, {0.65f, 0.25f, 0.75f}, {0.95f, 0.50f, 0.10f},
        {0.10f, 0.70f, 0.75f}, {0.85f, 0.35f, 0.55f}, {0.45f, 0.45f, 0.50f}};

    const float* c = palette[index % 9];

    // Post
    box(0.35f, 0.24f, 0.12f, x, 1.6f, z, 0.18f, 3.2f, 0.18f);

    // Banner
    box(c[0], c[1], c[2], x, 3.3f, z, 2.2f, 1.2f, 0.12f);
    box(0.95f, 0.95f, 0.95f, x, 3.3f, z + 0.07f, 2.3f, 0.06f, 0.03f);

    // Number dots (1..9 as a 3x3 grid)
    const int n = (index % 9) + 1;
    for (int i = 0; i < n; ++i)
    {
        const int col = i % 3;
        const int row = i / 3;

        box(1.0f, 1.0f, 1.0f,
            x - 0.55f + col * 0.55f,
            3.65f - row * 0.32f,
            z + 0.08f,
            0.20f, 0.20f, 0.04f);
    }
}

// Put the current matrix into farm "f"'s own frame
// (same translate / turn / scale that draw() uses for the farm itself)
void farmFrame(int f)
{
    float cx, cz;
    FarmWorld::farmCenter(f, cx, cz);
    glTranslatef(cx, 0.0f, cz);
    glRotatef(FarmWorld::farmYaw(f), 0.0f, 1.0f, 0.0f);
    const float s = FarmWorld::farmScale(f);
    glScalef(s, s, s);
}

// Farm-local point -> world point (matches glRotatef about Y)
void localToWorld(int f, float lx, float lz, float& wx, float& wz)
{
    float cx, cz;
    FarmWorld::farmCenter(f, cx, cz);
    const float a = FarmWorld::farmYaw(f) * DegToRad;
    const float s = FarmWorld::farmScale(f);
    const float c = std::cos(a);
    const float sn = std::sin(a);
    wx = cx + s * (lx * c + lz * sn);
    wz = cz + s * (-lx * sn + lz * c);
}

// Is the world point inside farm f's fence rectangle (+ margin)?
bool inFarm(int f, float x, float z, float margin)
{
    float cx, cz;
    FarmWorld::farmCenter(f, cx, cz);
    const float a = FarmWorld::farmYaw(f) * DegToRad;
    const float s = FarmWorld::farmScale(f);
    const float c = std::cos(a);
    const float sn = std::sin(a);
    const float dx = x - cx;
    const float dz = z - cz;
    const float lx = (c * dx - sn * dz) / s;
    const float lz = (sn * dx + c * dz) / s;
    const float m = margin / s;
    return std::fabs(lx) < FarmHalfX + m && std::fabs(lz) < FarmHalfZ + m;
}

// ------------------------------------------------------------
// 4. World data (roads, ponds, fields) - built once
// ------------------------------------------------------------
enum RoadFlags { RoadPower = 1, RoadAvenue = 2 };

struct Road
{
    std::vector<Vec2>  pts;     // sampled centre line
    std::vector<float> cum;     // distance from the start to each sample
    float width = 6.0f;
    bool  isMain = true;        // false = short drive to a farm gate
    int   flags = 0;
};

struct FieldRect          // a rectangular crop field next to a road
{
    float cx, cz;         // centre
    float hx, hz;         // half size (x runs along the road)
    float angleDeg;       // glRotatef angle about Y
    float c, s;           // cos / sin of that angle (for inside tests)
    int   type;           // 0 wheat, 1 young crop, 2 plowed soil, 3 pasture
    int   side;           // which side of the road (+1 / -1)
    int   border;         // 0 fence, 1 hedge
};

struct Pond { float x, z, r; };

struct WorldData
{
    std::vector<Road>      roads;
    std::vector<FieldRect> fields;
    std::vector<Pond>      ponds;
    float minX = 0, maxX = 0, minZ = 0, maxZ = 0;   // farm centres
};

WorldData gWorld;

// Road centre lines (x, z). They are smoothed with a Catmull-Rom
// spline, so the roads bend naturally instead of forming a grid.
// The ends run far outside the farms and disappear into the fog.
const float kRoadA[][2] = {   // east-west, between farm row 1 and 2
    {-300, 64}, {-220, 58}, {-140, 66}, {-70, 56}, {-20, 50}, {35, 47},
    {95, 44}, {150, 54}, {200, 63}, {270, 60}, {350, 52}, {430, 58}};

const float kRoadB[][2] = {   // east-west, in front of farm row 2
    {-260, 108}, {-170, 116}, {-100, 110}, {-58, 110}, {-18, 116},
    {30, 126}, {80, 124}, {125, 124}, {156, 132}};

const float kRoadC[][2] = {   // east-west, in front of farm row 3
    {-270, 205}, {-180, 196}, {-110, 201}, {-55, 198}, {-10, 200},
    {40, 204}, {80, 208}, {112, 222}, {150, 238}, {205, 238},
    {280, 244}, {380, 236}};

const float kRoadNorth[][2] = {   // leaves road A towards the north-west
    {-70, -150}, {-62, -80}, {-58, -20}, {-54, 28}, {-46, 56}};

const float kRoadSpine[][2] = {   // tree-lined road running south
    {-20, 50}, {-17, 115}, {-14, 160}, {-10, 200}, {-6, 260}, {-12, 340}};

const float kRoadEast[][2] = {    // links A, B and C on the east side
    {158, 58}, {160, 100}, {156, 132}, {146, 160}, {132, 190},
    {124, 215}, {112, 222}};

const float kRoadFarm5[][2] = {   // branch to the south-east farm
    {156, 132}, {178, 148}, {200, 152}};

// Where each farm's drive meets the main road (snapped to the nearest road)
const float kDriveTarget[FarmCount][2] = {
    {8, 56}, {96, 44}, {190, 62}, {36, 126}, {112, 124},
    {200, 152}, {-55, 198}, {80, 208}, {176, 238}};

void catmull(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
             float t, Vec2& out)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t +
                    (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                    (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    out.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t +
                    (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
                    (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);
}

void buildRoad(Road& road, const std::vector<Vec2>& ctrl)
{
    std::vector<Vec2> p;
    p.push_back(ctrl.front());
    p.insert(p.end(), ctrl.begin(), ctrl.end());
    p.push_back(ctrl.back());

    road.pts.clear();
    for (std::size_t i = 1; i + 2 < p.size(); ++i)
    {
        const float dx = p[i + 1].x - p[i].x;
        const float dz = p[i + 1].z - p[i].z;
        const int steps = std::max(2, static_cast<int>(std::sqrt(dx * dx + dz * dz) / 3.0f));
        for (int k = 0; k < steps; ++k)
        {
            Vec2 q;
            catmull(p[i - 1], p[i], p[i + 1], p[i + 2],
                    static_cast<float>(k) / static_cast<float>(steps), q);
            road.pts.push_back(q);
        }
    }
    road.pts.push_back(ctrl.back());

    road.cum.assign(road.pts.size(), 0.0f);
    for (std::size_t i = 1; i < road.pts.size(); ++i)
    {
        const float dx = road.pts[i].x - road.pts[i - 1].x;
        const float dz = road.pts[i].z - road.pts[i - 1].z;
        road.cum[i] = road.cum[i - 1] + std::sqrt(dx * dx + dz * dz);
    }
}

template <std::size_t N>
void addRoad(const float (&pts)[N][2], float width, int flags)
{
    std::vector<Vec2> ctrl;
    for (std::size_t i = 0; i < N; ++i)
        ctrl.push_back({pts[i][0], pts[i][1]});

    Road road;
    road.width = width;
    road.flags = flags;
    buildRoad(road, ctrl);
    gWorld.roads.push_back(road);
}

// Point and direction at distance s along a road
void pointAtLength(const Road& r, float s, Vec2& p, Vec2& t)
{
    std::size_t i = 1;
    while (i + 1 < r.pts.size() && r.cum[i] < s)
        ++i;

    const float seg = std::max(0.0001f, r.cum[i] - r.cum[i - 1]);
    const float k = clamp01((s - r.cum[i - 1]) / seg);
    p.x = r.pts[i - 1].x + (r.pts[i].x - r.pts[i - 1].x) * k;
    p.z = r.pts[i - 1].z + (r.pts[i].z - r.pts[i - 1].z) * k;

    float tx = r.pts[i].x - r.pts[i - 1].x;
    float tz = r.pts[i].z - r.pts[i - 1].z;
    const float len = std::max(0.0001f, std::sqrt(tx * tx + tz * tz));
    t.x = tx / len;
    t.z = tz / len;
}

// Distance from a point to the EDGE of the nearest road (0 = on the edge)
float roadEdgeDistance(float x, float z)
{
    float best = 1e30f;
    for (const Road& r : gWorld.roads)
    {
        const float h = r.width * 0.5f;
        for (const Vec2& p : r.pts)
        {
            const float dx = x - p.x;
            const float dz = z - p.z;
            // cheap reject before the square root
            if (dx * dx + dz * dz > (best + h) * (best + h))
                continue;
            best = std::min(best, std::sqrt(dx * dx + dz * dz) - h);
        }
    }
    return best;
}

// Closest point on any MAIN road (used to attach farm drives)
Vec2 nearestMainRoadPoint(float x, float z)
{
    Vec2 best = {x, z};
    float bestD = 1e30f;
    for (const Road& r : gWorld.roads)
    {
        if (!r.isMain)
            continue;
        for (const Vec2& p : r.pts)
        {
            const float d = (p.x - x) * (p.x - x) + (p.z - z) * (p.z - z);
            if (d < bestD)
            {
                bestD = d;
                best = p;
            }
        }
    }
    return best;
}

bool inField(const FieldRect& f, float x, float z, float margin)
{
    const float dx = x - f.cx;
    const float dz = z - f.cz;
    const float lx = f.c * dx - f.s * dz;
    const float lz = f.s * dx + f.c * dz;
    return std::fabs(lx) < f.hx + margin && std::fabs(lz) < f.hz + margin;
}

// ------------------------------------------------------------
// 5. "Is this spot free?"
//    Every tree / bush / rock / field asks this before it is placed,
//    so nothing grows through a farm, a road, a pond or a field.
// ------------------------------------------------------------
bool isFree(float x, float z, float margin, int ignoreFarm = -1)
{
    for (int f = 0; f < FarmCount; ++f)
    {
        if (f != ignoreFarm && inFarm(f, x, z, margin + 3.0f))
            return false;
    }

    for (const Pond& p : gWorld.ponds)
    {
        const float dx = x - p.x;
        const float dz = z - p.z;
        const float r = p.r + margin;
        if (dx * dx + dz * dz < r * r)
            return false;
    }

    for (const FieldRect& f : gWorld.fields)
    {
        if (inField(f, x, z, margin))
            return false;
    }

    return roadEdgeDistance(x, z) >= margin;
}

bool inRegion(float x, float z)
{
    return x > gWorld.minX - RegionPad && x < gWorld.maxX + RegionPad &&
           z > gWorld.minZ - RegionPad && z < gWorld.maxZ + RegionPad;
}

// Where farm f's pond is (farm-local position)
void pondLocal(int f, float& lx, float& lz)
{
    const int side = kStyle[f].pondSide;
    lx = side * (FarmHalfX + 11.0f);
    lz = (side < 0) ? -9.0f : 12.0f;
}

constexpr float PondW = 12.0f;
constexpr float PondD = 8.0f;

void placeFields()
{
    unsigned int seed = 31337u;

    for (const Road& road : gWorld.roads)
    {
        if (!road.isMain || road.width < 5.0f)
            continue;                     // fields only along the real roads

        const float total = road.cum.back();

        for (float s0 = 28.0f; s0 < total - 28.0f; s0 += range(seed, 55.0f, 85.0f))
        {
            Vec2 p, t;
            pointAtLength(road, s0, p, t);
            if (!inRegion(p.x, p.z))
                continue;

            // right-hand normal of the road direction
            const float nx = -t.z;
            const float nz = t.x;

            for (int side = -1; side <= 1; side += 2)
            {
                if (rnd(seed) > 0.72f)
                    continue;

                FieldRect f;
                f.hx = range(seed, 15.0f, 23.0f);
                f.hz = range(seed, 10.0f, 15.0f);
                const float off = road.width * 0.5f + 5.0f + f.hz;
                f.cx = p.x + nx * side * off;
                f.cz = p.z + nz * side * off;
                f.angleDeg = std::atan2(-t.z, t.x) / DegToRad;
                f.c = std::cos(f.angleDeg * DegToRad);
                f.s = std::sin(f.angleDeg * DegToRad);
                f.type = static_cast<int>(rnd(seed) * 3.99f);
                f.side = side;
                f.border = (rnd(seed) < 0.5f) ? 1 : 0;

                // test a 5 x 3 grid of points over the field
                bool fits = true;
                for (int iu = -2; iu <= 2 && fits; ++iu)
                {
                    for (int iv = -1; iv <= 1 && fits; ++iv)
                    {
                        const float lx = iu * 0.5f * f.hx;
                        const float lz = iv * f.hz;
                        // local (lx,lz) -> world with the field's rotation
                        const float wx = f.cx + lx * f.c + lz * f.s;
                        const float wz = f.cz - lx * f.s + lz * f.c;
                        fits = isFree(wx, wz, 2.0f);
                    }
                }

                if (fits)
                    gWorld.fields.push_back(f);
            }
        }
    }
}

void buildWorldData()
{
    gWorld = WorldData();

    // Bounds of the farm centres
    gWorld.minX = gWorld.minZ = 1e30f;
    gWorld.maxX = gWorld.maxZ = -1e30f;
    for (int i = 0; i < FarmCount; ++i)
    {
        float x, z;
        FarmWorld::farmCenter(i, x, z);
        gWorld.minX = std::min(gWorld.minX, x);
        gWorld.maxX = std::max(gWorld.maxX, x);
        gWorld.minZ = std::min(gWorld.minZ, z);
        gWorld.maxZ = std::max(gWorld.maxZ, z);
    }

    // ---- Main roads
    addRoad(kRoadA,     RoadWidth, RoadPower);
    addRoad(kRoadB,     RoadWidth, RoadPower);
    addRoad(kRoadC,     RoadWidth, RoadPower);
    addRoad(kRoadNorth, 5.5f, 0);
    addRoad(kRoadSpine, 5.5f, RoadAvenue);
    addRoad(kRoadEast,  5.5f, 0);
    addRoad(kRoadFarm5, 4.5f, 0);

    // ---- Ponds (so scatter code knows to keep away)
    for (int f = 0; f < FarmCount; ++f)
    {
        float lx, lz, wx, wz;
        pondLocal(f, lx, lz);
        localToWorld(f, lx, lz, wx, wz);
        gWorld.ponds.push_back({wx, wz, 7.5f * kScale[f]});
    }

    // ---- Farm drives: gate -> main road
    //      The gate is in the middle of the front fence (farm-local x=10,
    //      z=19, in line with the farmhouse door).
    const std::size_t mainRoads = gWorld.roads.size();
    for (int f = 0; f < FarmCount; ++f)
    {
        float gx, gz;
        localToWorld(f, 10.0f, FarmHalfZ, gx, gz);

        const float yaw = kYaw[f] * DegToRad;
        const float ox = std::sin(yaw);     // "out of the gate" direction
        const float oz = std::cos(yaw);

        const Vec2 j = nearestMainRoadPoint(kDriveTarget[f][0], kDriveTarget[f][1]);

        const Vec2 start = {gx - ox * 1.5f, gz - oz * 1.5f};
        const Vec2 p1 = {gx + ox * 9.0f, gz + oz * 9.0f};

        // bend the middle of the drive sideways a little, different per farm
        const float mx = (p1.x + j.x) * 0.5f;
        const float mz = (p1.z + j.z) * 0.5f;
        float px = -(j.z - p1.z);
        float pz = (j.x - p1.x);
        const float pl = std::max(0.001f, std::sqrt(px * px + pz * pz));
        const float wob = ((f % 2) ? 1.0f : -1.0f) * 2.5f;
        const Vec2 p2 = {mx + px / pl * wob, mz + pz / pl * wob};

        std::vector<Vec2> ctrl;
        ctrl.push_back(start);
        ctrl.push_back(p1);
        ctrl.push_back(p2);
        ctrl.push_back(j);

        Road drive;
        drive.width = 3.8f;
        drive.isMain = false;
        buildRoad(drive, ctrl);
        gWorld.roads.push_back(drive);
    }
    (void)mainRoads;

    placeFields();
}

// ------------------------------------------------------------
// 6. FLAT GROUND DECALS  (display list #1)
//    Drawn with the depth test OFF, in painter's order, so thin layers
//    never flicker against each other ("z-fighting").
// ------------------------------------------------------------
void drawRibbon(const Road& r, float extra, float cr, float cg, float cb, float jitter)
{
    const float half = r.width * 0.5f + extra;
    const int n = static_cast<int>(r.pts.size());

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < n; ++i)
    {
        const Vec2& a = r.pts[std::max(0, i - 1)];
        const Vec2& b = r.pts[std::min(n - 1, i + 1)];
        float tx = b.x - a.x;
        float tz = b.z - a.z;
        float len = std::sqrt(tx * tx + tz * tz);
        if (len < 0.0001f)
        {
            tx = 1.0f;
            tz = 0.0f;
            len = 1.0f;
        }
        tx /= len;
        tz /= len;

        const float nx = -tz;
        const float nz = tx;
        const float k = 1.0f + jitter * (hash2(i / 5, n) - 0.5f) * 2.0f;

        glColor3f(cr * k, cg * k, cb * k);
        glVertex3f(r.pts[i].x + nx * half, GroundY + 0.01f, r.pts[i].z + nz * half);
        glVertex3f(r.pts[i].x - nx * half, GroundY + 0.01f, r.pts[i].z - nz * half);
    }
    glEnd();
}

// A thin dark strip alongside a road (wheel ruts)
void drawRut(const Road& r, float offset, float width)
{
    const int n = static_cast<int>(r.pts.size());
    glColor3f(0.40f, 0.33f, 0.23f);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i < n; ++i)
    {
        const Vec2& a = r.pts[std::max(0, i - 1)];
        const Vec2& b = r.pts[std::min(n - 1, i + 1)];
        float tx = b.x - a.x;
        float tz = b.z - a.z;
        const float len = std::max(0.0001f, std::sqrt(tx * tx + tz * tz));
        tx /= len;
        tz /= len;
        const float nx = -tz;
        const float nz = tx;
        glVertex3f(r.pts[i].x + nx * (offset + width), GroundY + 0.012f, r.pts[i].z + nz * (offset + width));
        glVertex3f(r.pts[i].x + nx * (offset - width), GroundY + 0.012f, r.pts[i].z + nz * (offset - width));
    }
    glEnd();
}

void emitFieldDecal(const FieldRect& f)
{
    // base colour, row colour
    static const float base[4][3] = {
        {0.78f, 0.65f, 0.24f},   // ripe wheat
        {0.36f, 0.58f, 0.18f},   // young crop
        {0.45f, 0.31f, 0.16f},   // plowed soil
        {0.40f, 0.62f, 0.26f}};  // pasture
    static const float row[4][3] = {
        {0.68f, 0.53f, 0.16f},
        {0.27f, 0.47f, 0.12f},
        {0.34f, 0.22f, 0.10f},
        {0.40f, 0.62f, 0.26f}};

    glPushMatrix();
    glTranslatef(f.cx, 0.0f, f.cz);
    glRotatef(f.angleDeg, 0.0f, 1.0f, 0.0f);

    rectFlat(-f.hx, -f.hz, f.hx, f.hz, base[f.type][0], base[f.type][1], base[f.type][2]);

    if (f.type != 3)   // crop rows run along the road
    {
        for (float z = -f.hz + 0.7f; z < f.hz - 1.0f; z += 1.8f)
        {
            rectFlat(-f.hx + 0.6f, z, f.hx - 0.6f, z + 0.9f,
                     row[f.type][0], row[f.type][1], row[f.type][2]);
        }
    }

    glPopMatrix();
}

void emitGroundDecals()
{
    // ---- Fields
    for (const FieldRect& f : gWorld.fields)
        emitFieldDecal(f);

    // ---- Ponds (sandy bank + water) and the dirt yard in front of each farm
    unsigned int seed = 8080u;
    for (int f = 0; f < FarmCount; ++f)
    {
        glPushMatrix();
        farmFrame(f);

        float lx, lz;
        pondLocal(f, lx, lz);
        ellipseFlat(lx, lz, PondW * 0.5f + 1.6f, PondD * 0.5f + 1.6f, 0.0f,
                    0.72f, 0.64f, 0.45f, 0.62f, 0.58f, 0.38f, 24);
        ellipseFlat(lx, lz, PondW * 0.5f, PondD * 0.5f, 0.0f,
                    0.27f, 0.55f, 0.76f, 0.14f, 0.38f, 0.60f, 24);

        // trodden dirt in front of the gate
        ellipseFlat(10.0f, FarmHalfZ + 5.0f, 8.0f, 5.5f, range(seed, -15.0f, 15.0f),
                    0.47f, 0.38f, 0.26f, 0.44f, 0.38f, 0.25f);
        ellipseFlat(10.0f + range(seed, -4.0f, 4.0f), FarmHalfZ + 11.0f, 4.5f, 3.0f,
                    range(seed, 0.0f, 90.0f), 0.50f, 0.41f, 0.28f, 0.46f, 0.38f, 0.26f);

        glPopMatrix();
    }

    // ---- Dirt patches beside the roads and in a few fields
    for (const Road& r : gWorld.roads)
    {
        if (!r.isMain)
            continue;
        const float total = r.cum.back();
        for (float s0 = 20.0f; s0 < total; s0 += range(seed, 38.0f, 70.0f))
        {
            Vec2 p, t;
            pointAtLength(r, s0, p, t);
            if (!inRegion(p.x, p.z))
                continue;
            const float side = (rnd(seed) < 0.5f) ? -1.0f : 1.0f;
            const float off = r.width * 0.5f + range(seed, 1.5f, 4.0f);
            ellipseFlat(p.x - t.z * side * off, p.z + t.x * side * off,
                        range(seed, 1.6f, 3.4f), range(seed, 1.2f, 2.4f),
                        range(seed, 0.0f, 180.0f),
                        0.47f, 0.38f, 0.26f, 0.42f, 0.40f, 0.24f);
        }
    }

    // ---- Roads: grass verge first, then the road surface on top of it,
    //      so junctions blend together without seams.
    for (const Road& r : gWorld.roads)
        drawRibbon(r, r.isMain ? 1.1f : 0.7f, 0.43f, 0.50f, 0.25f, 0.0f);

    for (const Road& r : gWorld.roads)
        drawRibbon(r, 0.0f, 0.47f, 0.39f, 0.28f, 0.035f);

    // Wheel ruts on the long roads
    for (const Road& r : gWorld.roads)
    {
        if (r.isMain && r.width > 5.0f)
        {
            drawRut(r, -1.2f, 0.28f);
            drawRut(r,  1.2f, 0.28f);
        }
    }
}

// ------------------------------------------------------------
// 7. 3D SCENERY  (display list #2)
// ------------------------------------------------------------
void emitHills()
{
    const float ccx = (gWorld.minX + gWorld.maxX) * 0.5f;
    const float ccz = (gWorld.minZ + gWorld.maxZ) * 0.5f;
    const float hx = (gWorld.maxX - gWorld.minX) * 0.5f;
    const float hz = (gWorld.maxZ - gWorld.minZ) * 0.5f;

    struct Ring
    {
        float dist;     // distance beyond the farms
        int   count;
        float rMin, rMax, hMin, hMax;
        float gap;      // chance to leave a gap (so it is not a closed wall)
    };

    // a low ring of hills and a faint, tall ring of mountains behind it
    const Ring rings[2] = {
        {130.0f, 36, 24.0f, 40.0f,  9.0f, 19.0f, 0.15f},
        {225.0f, 28, 45.0f, 70.0f, 28.0f, 55.0f, 0.08f}};

    unsigned int seed = 777u;
    for (const Ring& ring : rings)
    {
        for (int k = 0; k < ring.count; ++k)
        {
            const float theta = 2.0f * Pi * (static_cast<float>(k) + range(seed, -0.3f, 0.3f)) /
                                static_cast<float>(ring.count);
            const float c = std::cos(theta);
            const float s = std::sin(theta);

            // rounded rectangle instead of a perfect circle
            const float ex = (c < 0 ? -1.0f : 1.0f) * std::pow(std::fabs(c), 0.55f);
            const float ez = (s < 0 ? -1.0f : 1.0f) * std::pow(std::fabs(s), 0.55f);

            const float x = ccx + (hx + ring.dist + range(seed, -15.0f, 15.0f)) * ex;
            const float z = ccz + (hz + ring.dist + range(seed, -15.0f, 15.0f)) * ez;
            const float radius = range(seed, ring.rMin, ring.rMax);
            const float height = range(seed, ring.hMin, ring.hMax);
            const float shade = range(seed, 0.8f, 1.1f);

            if (rnd(seed) < ring.gap)
                continue;
            if (roadEdgeDistance(x, z) < radius * 0.8f)
                continue;               // leave a gap where a road passes

            drawHill(x, z, radius, height, shade);
        }
    }
}

// Forest clumps, small groves and open meadows spread over the whole region
void emitLandscapeCells()
{
    const float cell = 40.0f;
    const float x0 = gWorld.minX - RegionPad;
    const float x1 = gWorld.maxX + RegionPad;
    const float z0 = gWorld.minZ - RegionPad;
    const float z1 = gWorld.maxZ + RegionPad;

    int treeBudget = 1900;     // hard cap so the scene stays light

    for (float gx = x0; gx < x1; gx += cell)
    {
        for (float gz = z0; gz < z1; gz += cell)
        {
            unsigned int seed = 12345u +
                (static_cast<unsigned int>(static_cast<int>(gx)) * 73856093u ^
                 static_cast<unsigned int>(static_cast<int>(gz)) * 19349663u);

            const float cx = gx + range(seed, 6.0f, cell - 6.0f);
            const float cz = gz + range(seed, 6.0f, cell - 6.0f);

            if (!isFree(cx, cz, 6.0f))
                continue;

            // distance to the nearest farm centre
            float nearest = 1e30f;
            for (int f = 0; f < FarmCount; ++f)
            {
                float fx, fz;
                FarmWorld::farmCenter(f, fx, fz);
                nearest = std::min(nearest, std::sqrt((cx - fx) * (cx - fx) + (cz - fz) * (cz - fz)));
            }

            // how far outside the farm area are we?
            const float outX = std::max(std::max(gWorld.minX - cx, cx - gWorld.maxX), 0.0f);
            const float outZ = std::max(std::max(gWorld.minZ - cz, cz - gWorld.maxZ), 0.0f);
            const float outside = std::sqrt(outX * outX + outZ * outZ);

            enum { Forest, Grove, Meadow } kind;
            const float u = rnd(seed);
            if (outside > 50.0f)
                kind = (u < 0.85f) ? Forest : Grove;       // dense belt at the edge
            else if (nearest < 62.0f)
                kind = (u < 0.70f) ? Meadow : Grove;       // keep farms airy
            else
                kind = (u < 0.35f) ? Forest : (u < 0.60f ? Grove : Meadow);

            const float pineChance = range(seed, 0.2f, 0.8f);
            const int segs = (outside > 50.0f) ? 7 : 9;

            if (kind == Forest || kind == Grove)
            {
                const int count = (kind == Forest) ? 12 + static_cast<int>(rnd(seed) * 7.0f)
                                                   : 4 + static_cast<int>(rnd(seed) * 4.0f);
                const float spread = (kind == Forest) ? 11.0f : 5.5f;

                for (int t = 0; t < count && treeBudget > 0; ++t)
                {
                    const float a = range(seed, 0.0f, 2.0f * Pi);
                    const float d = std::sqrt(rnd(seed)) * spread;
                    const float x = cx + std::cos(a) * d;
                    const float z = cz + std::sin(a) * d;
                    const float s = range(seed, 0.75f, 1.35f);
                    const float shade = range(seed, 0.8f, 1.2f);
                    const bool pine = rnd(seed) < pineChance;

                    if (!isFree(x, z, 2.2f))
                        continue;

                    if (pine)
                        drawPine(x, z, s, shade, segs);
                    else
                        drawRoundTree(x, z, s, shade, segs + 1);
                    --treeBudget;
                }

                // undergrowth at the edge of the clump
                for (int b = 0; b < 3; ++b)
                {
                    const float a = range(seed, 0.0f, 2.0f * Pi);
                    const float x = cx + std::cos(a) * spread * 1.1f;
                    const float z = cz + std::sin(a) * spread * 1.1f;
                    if (isFree(x, z, 1.5f))
                        drawBush(x, z, range(seed, 0.7f, 1.3f));
                }
            }
            else   // Meadow: lone trees, bushes, rocks, grass, flowers
            {
                for (int t = 0; t < 2; ++t)
                {
                    const float x = cx + range(seed, -14.0f, 14.0f);
                    const float z = cz + range(seed, -14.0f, 14.0f);
                    const float s = range(seed, 0.8f, 1.3f);
                    const float shade = range(seed, 0.85f, 1.15f);
                    if (rnd(seed) < 0.6f && treeBudget > 0 && isFree(x, z, 2.5f))
                    {
                        drawRoundTree(x, z, s, shade);
                        --treeBudget;
                    }
                }

                // bush cluster
                const float bx = cx + range(seed, -10.0f, 10.0f);
                const float bz = cz + range(seed, -10.0f, 10.0f);
                for (int b = 0; b < 3; ++b)
                {
                    const float x = bx + range(seed, -2.2f, 2.2f);
                    const float z = bz + range(seed, -2.2f, 2.2f);
                    if (isFree(x, z, 1.5f))
                        drawBush(x, z, range(seed, 0.7f, 1.3f));
                }

                if (rnd(seed) < 0.45f)
                {
                    const float x = cx + range(seed, -12.0f, 12.0f);
                    const float z = cz + range(seed, -12.0f, 12.0f);
                    if (isFree(x, z, 2.0f))
                        drawRock(x, z, range(seed, 0.6f, 1.4f), range(seed, 0.0f, 180.0f));
                }

                for (int g = 0; g < 4; ++g)
                {
                    const float x = cx + range(seed, -13.0f, 13.0f);
                    const float z = cz + range(seed, -13.0f, 13.0f);
                    if (isFree(x, z, 1.0f))
                        drawTuft(x, z, range(seed, 0.8f, 1.5f));
                }

                if (rnd(seed) < 0.5f)
                {
                    const float x = cx + range(seed, -12.0f, 12.0f);
                    const float z = cz + range(seed, -12.0f, 12.0f);
                    if (isFree(x, z, 1.5f))
                        drawFlowers(x, z, seed);
                }
            }
        }
    }
}

// Bushes, trees, rocks and grass tufts along the roads, plus the tree
// avenue on the long south road.
void emitRoadside()
{
    unsigned int seed = 2468u;

    for (const Road& r : gWorld.roads)
    {
        if (!r.isMain)
            continue;

        const float total = r.cum.back();
        for (float s0 = 6.0f; s0 < total; s0 += range(seed, 5.0f, 9.0f))
        {
            Vec2 p, t;
            pointAtLength(r, s0, p, t);
            if (!inRegion(p.x, p.z))
                continue;

            const float side = (rnd(seed) < 0.5f) ? -1.0f : 1.0f;
            const float off = r.width * 0.5f + range(seed, 2.6f, 6.5f);
            const float x = p.x - t.z * side * off;
            const float z = p.z + t.x * side * off;
            const float u = rnd(seed);

            if (u > 0.60f || !isFree(x, z, 1.2f))
                continue;

            if (u < 0.28f)
                drawBush(x, z, range(seed, 0.7f, 1.2f));
            else if (u < 0.40f)
                drawRoundTree(x, z, range(seed, 0.8f, 1.2f), range(seed, 0.85f, 1.15f));
            else if (u < 0.45f)
                drawPine(x, z, range(seed, 0.8f, 1.2f), range(seed, 0.85f, 1.15f));
            else if (u < 0.50f)
                drawRock(x, z, range(seed, 0.5f, 1.0f), range(seed, 0.0f, 180.0f));
            else
            {
                drawTuft(x, z, range(seed, 0.9f, 1.6f));
                drawTuft(x + 0.7f, z + 0.4f, range(seed, 0.9f, 1.6f));
            }
        }

        // tree avenue
        if (r.flags & RoadAvenue)
        {
            for (float s0 = 12.0f; s0 < total; s0 += 15.0f)
            {
                Vec2 p, t;
                pointAtLength(r, s0, p, t);
                if (!inRegion(p.x, p.z))
                    continue;
                for (int side = -1; side <= 1; side += 2)
                {
                    const float off = r.width * 0.5f + 3.4f;
                    const float x = p.x - t.z * side * off;
                    const float z = p.z + t.x * side * off;
                    if (isFree(x, z, 1.0f))
                        drawRoundTree(x, z, range(seed, 0.9f, 1.1f), range(seed, 0.95f, 1.1f));
                }
            }
        }
    }
}

// Poles and wires follow the roads (the old version used fixed east-west
// lines that crossed straight through the farms).
void emitPowerLines()
{
    for (const Road& r : gWorld.roads)
    {
        if (!(r.flags & RoadPower))
            continue;

        std::vector<Vec2> poles;
        const float total = r.cum.back();
        const float off = r.width * 0.5f + 2.8f;

        for (float s0 = 18.0f; s0 < total; s0 += 24.0f)
        {
            bool placed = false;
            const float tries[3] = {0.0f, 5.0f, -5.0f};
            for (float shift : tries)
            {
                Vec2 p, t;
                pointAtLength(r, std::max(0.0f, std::min(total, s0 + shift)), p, t);
                const float x = p.x - t.z * off;      // south / right-hand side
                const float z = p.z + t.x * off;
                if (inRegion(x, z) && isFree(x, z, 0.6f))
                {
                    poles.push_back({x, z});
                    placed = true;
                    break;
                }
            }
            (void)placed;
        }

        if (poles.size() < 2)
            continue;

        for (std::size_t i = 0; i < poles.size(); ++i)
        {
            const Vec2& a = poles[i];
            const Vec2& b = (i + 1 < poles.size()) ? poles[i + 1] : poles[i];
            const Vec2& c = (i + 1 < poles.size()) ? poles[i] : poles[i - 1];
            const float dx = (i + 1 < poles.size()) ? b.x - a.x : a.x - c.x;
            const float dz = (i + 1 < poles.size()) ? b.z - a.z : a.z - c.z;
            const float angle = std::atan2(dx, dz) * 180.0f / Pi;

            drawPowerPole(a.x, a.z, angle);
            if (i + 1 < poles.size())
                drawWire(a.x, a.z, b.x, b.z);
        }
    }
}

// Hedge or fence on the road side of every field, and hay in pastures
void emitFieldBorders()
{
    unsigned int seed = 9191u;

    for (const FieldRect& f : gWorld.fields)
    {
        glPushMatrix();
        glTranslatef(f.cx, 0.0f, f.cz);
        glRotatef(f.angleDeg, 0.0f, 1.0f, 0.0f);

        const float zEdge = -f.side * f.hz;     // the edge that faces the road

        if (f.border == 1)
        {
            for (float x = -f.hx + 1.0f; x < f.hx; x += range(seed, 2.8f, 3.8f))
                drawBush(x, zEdge, range(seed, 0.8f, 1.3f));
        }
        else
        {
            box(0.35f, 0.18f, 0.06f, 0.0f, 0.95f, zEdge, f.hx * 2.0f, 0.12f, 0.12f);
            for (float x = -f.hx; x <= f.hx + 0.1f; x += 6.0f)
                box(0.35f, 0.18f, 0.06f, x, 0.7f, zEdge, 0.2f, 1.4f, 0.2f);
        }

        if (f.type == 3)    // pasture: a couple of hay bales
        {
            for (int i = 0; i < 3; ++i)
                drawHayBale(range(seed, -f.hx + 4.0f, f.hx - 4.0f),
                            range(seed, -f.hz + 3.0f, f.hz - 3.0f),
                            range(seed, 0.0f, 180.0f));
        }

        glPopMatrix();
    }
}

// Pond reeds, sign, hay, orchard and windbreak around each farm.
// Each farm gets a different mix, so they do not look copy-pasted.
void emitFarmSurroundings()
{
    for (int f = 0; f < FarmCount; ++f)
    {
        unsigned int seed = 5000u + static_cast<unsigned int>(f) * 7919u;
        const FarmStyle& st = kStyle[f];

        glPushMatrix();
        farmFrame(f);

        drawFarmSign(f, 4.2f, FarmHalfZ + 4.0f);

        float px, pz;
        pondLocal(f, px, pz);
        drawReeds(px, pz, PondW, PondD);

        // hay bales on both sides of the drive
        for (int i = 0; i < st.hayBales; ++i)
        {
            const float side = (i % 2 == 0) ? -1.0f : 1.0f;
            const float lx = 10.0f + side * range(seed, 5.5f, 8.5f);
            const float lz = FarmHalfZ + 3.0f + range(seed, 0.0f, 8.0f);
            drawHayBale(lx, lz, range(seed, 0.0f, 180.0f));
        }

        // small orchard on the side without the pond
        if (st.orchard)
        {
            const float side = -static_cast<float>(st.pondSide);
            for (int c = 0; c < 3; ++c)
            {
                for (int r = 0; r < 4; ++r)
                {
                    const float lx = side * (28.5f + c * 4.5f);
                    const float lz = -8.0f + r * 4.6f;
                    float wx, wz;
                    localToWorld(f, lx, lz, wx, wz);
                    if (isFree(wx, wz, 1.5f, f))
                        drawRoundTree(lx, lz, range(seed, 0.65f, 0.80f), range(seed, 0.9f, 1.15f));
                }
            }
        }

        // row of pines behind the farm
        if (st.windbreak)
        {
            for (float lx = -22.0f; lx <= 22.0f; lx += 4.4f)
            {
                const float lz = -(FarmHalfZ + 5.0f) + range(seed, -0.8f, 0.8f);
                float wx, wz;
                localToWorld(f, lx, lz, wx, wz);
                if (isFree(wx, wz, 1.5f, f))
                    drawPine(lx, lz, range(seed, 1.0f, 1.25f), range(seed, 0.85f, 1.15f));
            }
        }

        // a few tufts of grass and flowers around the property
        for (int i = 0; i < 6; ++i)
        {
            const float lx = range(seed, -FarmHalfX - 9.0f, FarmHalfX + 9.0f);
            const float lz = range(seed, -FarmHalfZ - 9.0f, FarmHalfZ + 9.0f);
            float wx, wz;
            localToWorld(f, lx, lz, wx, wz);
            if (isFree(wx, wz, 1.0f, f) && !inFarm(f, wx, wz, 1.0f))
                drawTuft(lx, lz, range(seed, 0.9f, 1.5f));
        }

        glPopMatrix();
    }
}

void emitScenery()
{
    emitHills();
    emitLandscapeCells();
    emitRoadside();
    emitPowerLines();
    emitFieldBorders();
    emitFarmSurroundings();

    // Rural Power Substation connected near road intersection
    PowerPlant::drawSubstation(185.0f, 85.0f, -15.0f);

    // Arched wooden pond bridge with Bézier railings
    Bridge::drawBridge(-120.0f, -45.0f, 35.0f, 14.0f, 3.8f);
}

// ------------------------------------------------------------
// 8. Ground that follows the camera
//    A grid of coloured tiles is drawn around the camera every
//    frame.  The tiles are placed on a fixed (slightly turned) grid
//    in the world and each tile's colour comes from a hash of its
//    grid number, so the ground does not move when the camera moves.
//    Beyond the fog distance the tiles are invisible, so the ground
//    never seems to end.
// ------------------------------------------------------------
void grassColor(int a, int b, float& r, float& g, float& bl)
{
    const float h = hash2(a * 3 + 11, b * 7 + 5);
    const float t = hash2(a * 5 + 2, b * 3 + 9);
    const float k = 0.90f + 0.20f * h;
    r  = (0.28f + 0.09f * t) * k;
    g  = (0.52f + 0.04f * t) * k;
    bl = (0.21f - 0.03f * t) * k;
}

// Some far tiles become "fields" of other colours
bool cropTileColor(int a, int b, float cellX, float cellZ, float& r, float& g, float& bl)
{
    if (hash2(a * 13 + 7, b * 17 + 3) < 0.74f)
        return false;

    // keep the area right around the farms plain grass
    for (int f = 0; f < FarmCount; ++f)
    {
        float fx, fz;
        FarmWorld::farmCenter(f, fx, fz);
        if ((cellX - fx) * (cellX - fx) + (cellZ - fz) * (cellZ - fz) < 70.0f * 70.0f)
            return false;
    }

    static const float cols[4][3] = {
        {0.74f, 0.62f, 0.24f}, {0.38f, 0.60f, 0.20f},
        {0.43f, 0.31f, 0.17f}, {0.52f, 0.66f, 0.30f}};
    const int i = static_cast<int>(hash2(a * 29 + 1, b * 31 + 8) * 3.99f);
    r = cols[i][0];
    g = cols[i][1];
    bl = cols[i][2];
    return true;
}

void drawGroundCells(float camX, float camZ)
{
    constexpr float cell = 44.0f;
    constexpr int   N = 11;               // tiles each side of the camera
    const float phi = 0.35f;              // grid turned so it does not line up with the farms
    const float c = std::cos(phi);
    const float s = std::sin(phi);

    const float u0 = camX * c + camZ * s;
    const float v0 = -camX * s + camZ * c;
    const int iu = static_cast<int>(std::floor(u0 / cell));
    const int iv = static_cast<int>(std::floor(v0 / cell));

    TextureManager::bind(TextureManager::Grass);
    glBegin(GL_QUADS);
    for (int j = -N; j <= N; ++j)
    {
        for (int i = -N; i <= N; ++i)
        {
            const int a = iu + i;
            const int b = iv + j;

            const float cu = (a + 0.5f) * cell;
            const float cv = (b + 0.5f) * cell;
            const float cellX = cu * c - cv * s;
            const float cellZ = cu * s + cv * c;

            float cr = 0.28f, cg = 0.52f, cb = 0.21f;
            const bool crop = cropTileColor(a, b, cellX, cellZ, cr, cg, cb);

            const int ca[4] = {a, a + 1, a + 1, a};
            const int cb2[4] = {b, b, b + 1, b + 1};
            const float texU[4] = {0.0f, 1.0f, 1.0f, 0.0f};
            const float texV[4] = {0.0f, 0.0f, 1.0f, 1.0f};

            for (int k = 0; k < 4; ++k)
            {
                if (!crop)
                    grassColor(ca[k], cb2[k], cr, cg, cb);
                glColor3f(cr, cg, cb);
                glTexCoord2f(texU[k], texV[k]);

                const float u = ca[k] * cell;
                const float v = cb2[k] * cell;
                glVertex3f(u * c - v * s, GroundY, u * s + v * c);
            }
        }
    }
    glEnd();
    TextureManager::unbind();
}
} // namespace

// ============================================================
// 9. FarmWorld
// ============================================================

FarmWorld::~FarmWorld()
{
    release();
}

void FarmWorld::release()
{
    if (groundList_ != 0)
    {
        glDeleteLists(groundList_, 1);
        groundList_ = 0;
    }
    if (sceneryList_ != 0)
    {
        glDeleteLists(sceneryList_, 1);
        sceneryList_ = 0;
    }
}

void FarmWorld::setupAtmosphere()
{
    glClearColor(DayFog[0], DayFog[1], DayFog[2], 1.0f);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, DayFog);
    glFogf(GL_FOG_START, FogStart);
    glFogf(GL_FOG_END, FogEnd);
    glHint(GL_FOG_HINT, GL_NICEST);
}

void FarmWorld::updateAtmosphere(bool night)
{
    glFogfv(GL_FOG_COLOR, night ? NightFog : DayFog);
}

void FarmWorld::farmCenter(int index, float& x, float& z)
{
    static const float centers[FarmCount][2] = {
        {  0.0f,   0.0f},
        { 88.0f,  -8.0f},
        {178.0f,  27.0f},
        { 25.0f,  86.0f},
        {108.0f,  76.0f},
        {204.0f, 116.0f},
        {-58.0f, 151.0f},
        { 65.0f, 165.0f},
        {168.0f, 205.0f}
    };

    const int safeIndex = std::max(0, std::min(FarmCount - 1, index));
    x = centers[safeIndex][0];
    z = centers[safeIndex][1];
}

float FarmWorld::farmYaw(int index)
{
    return kYaw[std::max(0, std::min(FarmCount - 1, index))];
}

float FarmWorld::farmScale(int index)
{
    return kScale[std::max(0, std::min(FarmCount - 1, index))];
}

int FarmWorld::nearestFarm(float x, float z)
{
    int best = 0;
    float bestDistance = 1e30f;

    for (int i = 0; i < FarmCount; ++i)
    {
        float cx, cz;
        farmCenter(i, cx, cz);

        const float d = (x - cx) * (x - cx) + (z - cz) * (z - cz);
        if (d < bestDistance)
        {
            bestDistance = d;
            best = i;
        }
    }

    return best;
}

void FarmWorld::travelTo(int farmIndex, const Camera& camera)
{
    farmIndex = std::max(0, std::min(FarmCount - 1, farmIndex));

    fromPos_[0] = camera.posX();
    fromPos_[1] = camera.posY();
    fromPos_[2] = camera.posZ();
    fromYaw_ = camera.yawDegrees();
    fromPitch_ = camera.pitchDegrees();

    // Same viewpoint as the default start (slightly in front of the farm,
    // looking at it), but turned with the farm.
    float wx, wz;
    localToWorld(farmIndex, 0.0f, 10.0f, wx, wz);
    toPos_[0] = wx;
    toPos_[1] = 4.0f;
    toPos_[2] = wz;
    toYaw_ = -90.0f - farmYaw(farmIndex);
    toPitch_ = -15.0f;

    // Take the shortest way around for the turn
    float delta = std::fmod(toYaw_ - fromYaw_, 360.0f);
    if (delta > 180.0f) delta -= 360.0f;
    if (delta < -180.0f) delta += 360.0f;
    toYaw_ = fromYaw_ + delta;

    const float dx = toPos_[0] - fromPos_[0];
    const float dz = toPos_[2] - fromPos_[2];
    const float distance = std::sqrt(dx * dx + dz * dz);

    duration_ = std::max(1.2f, std::min(4.0f, distance / 35.0f));
    arcHeight_ = std::min(14.0f, distance * 0.15f);

    time_ = 0.0f;
    target_ = farmIndex;
    traveling_ = true;
}

void FarmWorld::update(float deltaTime, Camera& camera)
{
    // Which farm do M / comma step from? B is reserved for bonfires.
    const int base = traveling_
        ? target_
        : nearestFarm(camera.posX(), camera.posZ());

    int requested = -1;

    if (Input::wasPressed(GLFW_KEY_M))
        requested = (base + 1) % FarmCount;             // next farm

    if (Input::wasPressed(GLFW_KEY_COMMA))
        requested = (base + FarmCount - 1) % FarmCount; // previous farm

    for (int i = 0; i < 9 && i < FarmCount; ++i)
    {
        if (Input::wasPressed(GLFW_KEY_1 + i))
            requested = i;
    }

    if (requested >= 0)
        travelTo(requested, camera);

    if (!traveling_)
    {
        // ---- Keep the camera inside the playable area.
        // The scenery (forests, hills, fog) continues well beyond this
        // limit, so the player never reaches the "end" of the world.
        float minX = 1e30f, maxX = -1e30f, minZ = 1e30f, maxZ = -1e30f;
        for (int i = 0; i < FarmCount; ++i)
        {
            float x, z;
            farmCenter(i, x, z);
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minZ = std::min(minZ, z);
            maxZ = std::max(maxZ, z);
        }

        const float x = camera.posX();
        const float y = camera.posY();
        const float z = camera.posZ();
        const float nx = std::max(minX - PlayPad, std::min(maxX + PlayPad, x));
        const float ny = std::max(MinCameraHeight, std::min(MaxCameraHeight, y));
        const float nz = std::max(minZ - PlayPad, std::min(maxZ + PlayPad, z));

        if (nx != x || ny != y || nz != z)
            camera.setPose(nx, ny, nz, camera.yawDegrees(), camera.pitchDegrees());
        return;
    }

    time_ += std::min(deltaTime, 0.05f);
    const float t = std::min(1.0f, time_ / duration_);
    const float s = smoothstep(t);

    const float x = fromPos_[0] + (toPos_[0] - fromPos_[0]) * s;
    const float z = fromPos_[2] + (toPos_[2] - fromPos_[2]) * s;
    float y = fromPos_[1] + (toPos_[1] - fromPos_[1]) * s;

    // Fly up in an arc so the trip feels like a flight
    y += std::sin(s * Pi) * arcHeight_;

    const float yaw = fromYaw_ + (toYaw_ - fromYaw_) * s;
    const float pitch = fromPitch_ + (toPitch_ - fromPitch_) * s;

    camera.setPose(x, y, z, yaw, pitch);

    if (t >= 1.0f)
        traveling_ = false;
}

void FarmWorld::buildStatic()
{
    release();
    buildWorldData();

    groundList_ = glGenLists(1);
    glNewList(groundList_, GL_COMPILE);
    emitGroundDecals();
    glEndList();

    sceneryList_ = glGenLists(1);
    glNewList(sceneryList_, GL_COMPILE);
    emitScenery();
    glEndList();
}

void FarmWorld::draw(const Camera& camera, const DrawFarmFn& drawFarm)
{
    // Static scenery is compiled once into display lists
    if (sceneryList_ == 0)
    {
        buildStatic();
    }

    const float camX = camera.posX();
    const float camY = camera.posY();
    const float camZ = camera.posZ();

    // ---- 1. Flat ground (tiles that follow the camera + roads, fields,
    //         ponds).  Depth test off: painter's order, no flicker.
    glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);

    drawGroundCells(camX, camZ);
    glCallList(groundList_);
    glPopAttrib();

    // ---- 2. 3D scenery (trees, hills, poles ...)
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glCallList(sceneryList_);
    glPopAttrib();

    // ---- 3. Farms (only the ones that matter)
    const float yaw = camera.yawDegrees() * DegToRad;
    const float pitch = camera.pitchDegrees() * DegToRad;
    const float fx = std::cos(pitch) * std::cos(yaw);
    const float fy = std::sin(pitch);
    const float fz = std::cos(pitch) * std::sin(yaw);

    for (int i = 0; i < FarmCount; ++i)
    {
        float cx, cz;
        farmCenter(i, cx, cz);

        const float dx = cx - camX;
        const float dy = -camY;
        const float dz = cz - camZ;
        const float flat2 = dx * dx + dz * dz;

        // Too far away to matter
        if (flat2 > DrawDistance * DrawDistance)
            continue;

        // Behind the camera?  (angle between view direction and the farm,
        // minus the angle the farm itself covers)
        const float dist = std::sqrt(flat2 + dy * dy);
        if (dist > FarmRadius)
        {
            const float cosA = (dx * fx + dy * fy + dz * fz) / dist;
            const float angle = std::acos(std::max(-1.0f, std::min(1.0f, cosA)));
            const float covered = std::asin(std::min(1.0f, FarmRadius / dist));
            if (angle > ViewCullAngle * DegToRad + covered)
                continue;
        }

        const bool detailed = flat2 < DetailDistance * DetailDistance;

        glPushMatrix();
        farmFrame(i);
        drawFarm(i, detailed);
        glPopMatrix();
    }
}
