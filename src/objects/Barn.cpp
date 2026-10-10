#include "objects/Barn.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace
{
// ---------------------------------------------------------------------------
// Basic types
// ---------------------------------------------------------------------------
struct Color
{
    float r, g, b;
};

struct Vec3
{
    float x, y, z;
};

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
namespace palette
{
constexpr Color kWall       {0.55f, 0.12f, 0.07f};  // weathered barn red
constexpr Color kBatten     {0.47f, 0.09f, 0.05f};  // slightly darker vertical battens
constexpr Color kTrim       {0.90f, 0.86f, 0.78f};  // off-white trim
constexpr Color kFoundation {0.45f, 0.44f, 0.42f};  // fieldstone
constexpr Color kRoof       {0.24f, 0.25f, 0.27f};  // metal roofing
constexpr Color kRidgeCap   {0.16f, 0.17f, 0.18f};
constexpr Color kDoor       {0.50f, 0.11f, 0.06f};
constexpr Color kGlass      {0.20f, 0.48f, 0.58f};
constexpr Color kHay        {0.80f, 0.66f, 0.26f};
constexpr Color kHayBand    {0.55f, 0.42f, 0.14f};
constexpr Color kWood       {0.38f, 0.22f, 0.09f};  // fence, cart, barrel
constexpr Color kDarkWood   {0.26f, 0.14f, 0.05f};
constexpr Color kIron       {0.07f, 0.07f, 0.06f};
constexpr Color kFloor      {0.34f, 0.25f, 0.15f};
constexpr Color kFloorLine  {0.23f, 0.15f, 0.08f};
constexpr Color kBeam       {0.25f, 0.13f, 0.045f};
constexpr Color kTrough     {0.31f, 0.19f, 0.08f};
constexpr Color kFeed       {0.57f, 0.43f, 0.15f};
constexpr Color kTool       {0.18f, 0.18f, 0.17f};
constexpr Color kChest      {0.42f, 0.23f, 0.08f};
constexpr Color kLamp       {1.00f, 0.76f, 0.35f};
// Interior additions.
constexpr Color kPlank      {0.33f, 0.20f, 0.10f};  // aged interior boards
constexpr Color kPeg        {0.50f, 0.36f, 0.20f};  // pegboard
constexpr Color kEarth      {0.24f, 0.17f, 0.10f};  // packed stall floor
constexpr Color kStraw      {0.78f, 0.66f, 0.30f};
constexpr Color kBurlap     {0.62f, 0.52f, 0.34f};
constexpr Color kGalv       {0.55f, 0.58f, 0.60f};  // galvanised metal
constexpr Color kLeather    {0.30f, 0.16f, 0.07f};
constexpr Color kRope       {0.62f, 0.52f, 0.32f};
constexpr Color kBrass      {0.70f, 0.55f, 0.20f};
}  // namespace palette

// ---------------------------------------------------------------------------
// Dimensions (barn-local units, before the user supplied scale)
// ---------------------------------------------------------------------------
namespace dim
{
constexpr float kHalfWidth      = 5.0f;   // X
constexpr float kHalfDepth      = 4.0f;   // Z (front of the barn faces -Z)
constexpr float kWallHeight     = 5.5f;
constexpr float kFoundationH    = 0.45f;
constexpr float kFloorTop       = 0.5f;   // finished interior floor level
constexpr float kRoofPitchDeg   = 30.0f;
constexpr float kRoofOverhang   = 0.5f;
constexpr float kRoofThickness  = 0.20f;
constexpr float kPi             = 3.14159265358979f;
constexpr float kBattenSpacing  = 0.625f;
constexpr float kBattenSize     = 0.14f;
constexpr float kBattenRelief   = 0.06f;
constexpr float kDoorHeight     = 4.2f;
constexpr float kDoorLeafWidth  = 3.35f;
constexpr float kDoorwayHalfW   = 3.35f;
}  // namespace dim

float degToRad(float deg) { return deg * dim::kPi / 180.0f; }

float roofRidgeHeight()
{
    return dim::kWallHeight + dim::kHalfWidth * std::tan(degToRad(dim::kRoofPitchDeg));
}

// Height of the underside of the roof plane at horizontal position `x`.
float roofUnderY(float x)
{
    return roofRidgeHeight() - std::fabs(x) * std::tan(degToRad(dim::kRoofPitchDeg)) - 0.03f;
}

float hash01(int value)
{
    const float s = std::sin(value * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

Color tint(const Color& c, float k) { return {c.r * k, c.g * k, c.b * k}; }

Color vary(const Color& c, int seed, float amount = 0.07f)
{
    return tint(c, 1.0f + (hash01(seed) - 0.5f) * 2.0f * amount);
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------
void setColor(const Color& c) { glColor3f(c.r, c.g, c.b); }

// Axis-aligned box centred on `pos`.
void drawBox(const Color& color, const Vec3& pos, const Vec3& size)
{
    setColor(color);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glScalef(size.x, size.y, size.z);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

// Box rotated around the Z axis (used for braces and roof planes).
void drawBoxRotatedZ(const Color& color, const Vec3& pos, const Vec3& size, float angleDeg)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);
    drawBox(color, {0.0f, 0.0f, 0.0f}, size);
    glPopMatrix();
}

// Box rotated around the vertical axis (straw, bales, scattered props).
void drawBoxYaw(const Color& color, const Vec3& pos, const Vec3& size, float yawDeg)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);
    drawBox(color, {0.0f, 0.0f, 0.0f}, size);
    glPopMatrix();
}

// Square-section timber between two points (rafters, braces, ladder rails).
void drawBeam(const Color& color, const Vec3& a, const Vec3& b, float t)
{
    const float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-4f) return;
    glPushMatrix();
    glTranslatef((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f);
    const float axisX = dz, axisZ = -dx;   // (0,1,0) x direction
    if (std::sqrt(axisX * axisX + axisZ * axisZ) > 1e-4f)
        glRotatef(std::acos(std::clamp(dy / len, -1.0f, 1.0f)) * 180.0f / dim::kPi,
                  axisX, 0.0f, axisZ);
    else if (dy < 0.0f)
        glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
    drawBox(color, {0.0f, 0.0f, 0.0f}, {t, len, t});
    glPopMatrix();
}

// Vertical cone frustum, base centred on `base`, rising by h (buckets, cans).
void drawFrustum(const Color& color, const Vec3& base, float r0, float r1, float h,
                 int slices = 12)
{
    setColor(color);
    glPushMatrix();
    glTranslatef(base.x, base.y, base.z);
    const float slope = (r0 - r1) / h;
    const float nl = std::sqrt(1.0f + slope * slope);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= slices; ++i)
    {
        const float a = 2.0f * dim::kPi * i / slices;
        const float cs = std::cos(a), sn = std::sin(a);
        glNormal3f(cs / nl, slope / nl, sn / nl);
        glVertex3f(cs * r0, 0.0f, sn * r0);
        glVertex3f(cs * r1, h, sn * r1);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, h, 0.0f);
    for (int i = slices; i >= 0; --i)
    {
        const float a = 2.0f * dim::kPi * i / slices;
        glVertex3f(std::cos(a) * r1, h, std::sin(a) * r1);
    }
    glEnd();
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, -1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= slices; ++i)
    {
        const float a = 2.0f * dim::kPi * i / slices;
        glVertex3f(std::cos(a) * r0, 0.0f, std::sin(a) * r0);
    }
    glEnd();
    glPopMatrix();
}

// Ring of small boxes in the local XY plane (horse collar, rope coil, brass ring).
void drawLoopZ(const Color& color, const Vec3& c, float radius, float thick, int segments)
{
    const float seg = 2.0f * dim::kPi * radius / segments * 1.08f;
    for (int i = 0; i < segments; ++i)
    {
        const float a = 2.0f * dim::kPi * i / segments;
        drawBoxRotatedZ(color,
                        {c.x + std::cos(a) * radius, c.y + std::sin(a) * radius, c.z},
                        {seg, thick, thick}, a * 180.0f / dim::kPi + 90.0f);
    }
}

void drawMound(const Color& color, const Vec3& pos, float sx, float sy, float sz)
{
    setColor(color);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glScalef(sx, sy, sz);
    Primitives::drawSphere(1.0f, 12, 8);
    glPopMatrix();
}

// Additive warm light pool on a horizontal surface (lamp spill).
void drawLightPool(const Vec3& c, float radius, float strength)
{
    if (strength < 0.02f) return;
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    constexpr int kSeg = 24;
    const Color& l = palette::kLamp;
    const float a0 = 0.38f * strength;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(l.r, l.g * 0.95f, l.b * 0.8f, a0);
    glVertex3f(c.x, c.y, c.z);
    for (int i = 0; i <= kSeg; ++i)
    {
        const float a = 2.0f * dim::kPi * i / kSeg;
        glColor4f(l.r, l.g * 0.9f, l.b * 0.6f, a0 * 0.28f);
        glVertex3f(c.x + std::cos(a) * radius * 0.5f, c.y, c.z + std::sin(a) * radius * 0.5f);
    }
    glEnd();
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= kSeg; ++i)
    {
        const float a = 2.0f * dim::kPi * i / kSeg;
        const float cs = std::cos(a), sn = std::sin(a);
        glColor4f(l.r, l.g * 0.9f, l.b * 0.6f, a0 * 0.28f);
        glVertex3f(c.x + cs * radius * 0.5f, c.y, c.z + sn * radius * 0.5f);
        glColor4f(l.r, l.g * 0.9f, l.b * 0.6f, 0.0f);
        glVertex3f(c.x + cs * radius, c.y, c.z + sn * radius);
    }
    glEnd();
    glPopAttrib();
}

// Triangular gable wall. `facing` is -1 for the front (-Z) and +1 for the back.
void drawGable(float z, float facing)
{
    const float ridge = roofRidgeHeight();
    const float base  = dim::kWallHeight;
    const float hw    = dim::kHalfWidth;

    setColor(palette::kWall);
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 0.0f, facing);
    if (facing < 0.0f)
    {
        glVertex3f(0.0f, ridge, z);
        glVertex3f(hw, base, z);
        glVertex3f(-hw, base, z);
    }
    else
    {
        glVertex3f(0.0f, ridge, z);
        glVertex3f(-hw, base, z);
        glVertex3f(hw, base, z);
    }
    glEnd();
}

// Window drawn in a local XY plane, then yawed to face the requested wall.
// yawDeg = 0 for front/back walls, 90 for side walls.
void drawWindow(const Vec3& pos, float yawDeg)
{
    constexpr float kW = 1.10f;
    constexpr float kH = 1.30f;
    constexpr float kBar = 0.10f;
    constexpr float kDepth = 0.08f;

    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);

    drawBox(palette::kGlass, {0.0f, 0.0f, 0.0f}, {kW, kH, kDepth});

    // Outer frame.
    drawBox(palette::kTrim, {0.0f,  kH * 0.5f, -0.03f}, {kW + kBar, kBar, 0.10f});
    drawBox(palette::kTrim, {0.0f, -kH * 0.5f, -0.03f}, {kW + kBar, kBar, 0.10f});
    drawBox(palette::kTrim, { kW * 0.5f, 0.0f, -0.03f}, {kBar, kH, 0.10f});
    drawBox(palette::kTrim, {-kW * 0.5f, 0.0f, -0.03f}, {kBar, kH, 0.10f});

    // Muntins.
    drawBox(palette::kTrim, {0.0f, 0.0f, -0.05f}, {0.05f, kH, 0.05f});
    drawBox(palette::kTrim, {0.0f, 0.0f, -0.05f}, {kW, 0.05f, 0.05f});

    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Barn shell (unchanged)
// ---------------------------------------------------------------------------
void drawFoundation()
{
    drawBox(palette::kFoundation,
            {0.0f, dim::kFoundationH * 0.5f, 0.0f},
            {dim::kHalfWidth * 2.0f + 0.4f, dim::kFoundationH, dim::kHalfDepth * 2.0f + 0.4f});
}

void drawWalls()
{
    const float w = dim::kHalfWidth * 2.0f;
    const float d = dim::kHalfDepth * 2.0f;
    const float h = dim::kWallHeight;

    // A true shell instead of a solid cube: from inside the barn the wall
    // faces, roof structure and entrance remain visible and the doorway is
    // physically open. The thin slabs are lit on both sides by Lighting.
    drawBox(palette::kWall, {0.0f, h * 0.5f, dim::kHalfDepth},
            {w, h, 0.18f});
    drawBox(palette::kWall, {-dim::kHalfWidth, h * 0.5f, 0.0f},
            {0.18f, h, d});
    drawBox(palette::kWall, { dim::kHalfWidth, h * 0.5f, 0.0f},
            {0.18f, h, d});

    const float sideWidth = dim::kHalfWidth - dim::kDoorwayHalfW;
    for (float side : {-1.0f, 1.0f})
    {
        drawBox(palette::kWall,
                {side * (dim::kDoorwayHalfW + sideWidth * 0.5f),
                 h * 0.5f, -dim::kHalfDepth},
                {sideWidth, h, 0.18f});
    }
    drawBox(palette::kWall,
            {0.0f, dim::kDoorHeight + (h - dim::kDoorHeight) * 0.5f,
             -dim::kHalfDepth},
            {dim::kDoorwayHalfW * 2.0f, h - dim::kDoorHeight, 0.18f});

    // Triangular gable ends sit just proud of the box so they do not z-fight.
    drawGable(-dim::kHalfDepth - 0.01f, -1.0f);
    drawGable(dim::kHalfDepth + 0.01f, 1.0f);

    // Corner boards and a belt course give the box some structure.
    for (float sx : {-1.0f, 1.0f})
    {
        for (float sz : {-1.0f, 1.0f})
        {
            drawBox(palette::kTrim,
                    {sx * (dim::kHalfWidth + 0.05f), h * 0.5f, sz * (dim::kHalfDepth + 0.05f)},
                    {0.20f, h, 0.20f});
        }
    }
}

void drawBattens()
{
    const float centerY = dim::kFoundationH + (dim::kWallHeight - dim::kFoundationH) * 0.5f;
    const float height  = dim::kWallHeight - dim::kFoundationH;
    const float inset   = 0.45f;

    // Front and back walls.
    for (float x = -dim::kHalfWidth + inset; x <= dim::kHalfWidth - inset + 0.001f;
         x += dim::kBattenSpacing)
    {
        for (float sz : {-1.0f, 1.0f})
        {
            if (sz < 0.0f && std::fabs(x) < dim::kDoorwayHalfW)
                continue;
            drawBox(palette::kBatten,
                    {x, centerY, sz * (dim::kHalfDepth + dim::kBattenRelief * 0.5f)},
                    {dim::kBattenSize, height, dim::kBattenRelief});
        }
    }

    // Side walls.
    for (float z = -dim::kHalfDepth + inset; z <= dim::kHalfDepth - inset + 0.001f;
         z += dim::kBattenSpacing)
    {
        for (float sx : {-1.0f, 1.0f})
        {
            drawBox(palette::kBatten,
                    {sx * (dim::kHalfWidth + dim::kBattenRelief * 0.5f), centerY, z},
                    {dim::kBattenRelief, height, dim::kBattenSize});
        }
    }
}

void drawRoof()
{
    const float pitch    = dim::kRoofPitchDeg;
    const float ridge    = roofRidgeHeight();
    const float run      = dim::kHalfWidth + dim::kRoofOverhang;   // horizontal run, ridge to eave
    const float halfRun  = run * 0.5f;
    const float length   = run / std::cos(degToRad(pitch));
    const float roofDepth = dim::kHalfDepth * 2.0f + dim::kRoofOverhang * 2.0f;
    const float centerY  = ridge - halfRun * std::tan(degToRad(pitch)) + dim::kRoofThickness * 0.5f;

    // Each plane slopes downward away from the ridge.
    for (float side : {-1.0f, 1.0f})
    {
        drawBoxRotatedZ(palette::kRoof,
                        {side * halfRun, centerY, 0.0f},
                        {length, dim::kRoofThickness, roofDepth},
                        -side * pitch);
    }

    // Ridge cap.
    drawBox(palette::kRidgeCap,
            {0.0f, ridge + dim::kRoofThickness * 0.7f, 0.0f},
            {0.45f, 0.18f, roofDepth + 0.05f});

    // Fascia boards along the front and back roof edges.
    for (float sz : {-1.0f, 1.0f})
    {
        for (float side : {-1.0f, 1.0f})
        {
            drawBoxRotatedZ(palette::kTrim,
                            {side * halfRun, centerY - 0.12f, sz * (roofDepth * 0.5f)},
                            {length, 0.22f, 0.08f},
                            -side * pitch);
        }
    }
}

void drawFrontDoors(float openAmount)
{
    const float z = -dim::kHalfDepth - 0.08f;
    const float cy = dim::kDoorHeight * 0.5f;

    const float innerW = dim::kDoorLeafWidth - 0.35f;
    const float innerH = dim::kDoorHeight - 0.35f;
    const float diagLen = std::sqrt(innerW * innerW + innerH * innerH);
    const float diagDeg = std::atan2(innerH, innerW) * 180.0f / dim::kPi;

    for (float side : {-1.0f, 1.0f})
    {
        const float closedX = side * dim::kDoorLeafWidth * 0.5f;
        const float openX = side * (dim::kHalfWidth + dim::kDoorLeafWidth * 0.46f);
        const float cx = closedX + (openX - closedX) * openAmount;

        // Door leaf.
        drawBox(palette::kDoor, {cx, cy, z}, {dim::kDoorLeafWidth, dim::kDoorHeight, 0.14f});

        // Perimeter trim.
        const float tz = z - 0.09f;
        drawBox(palette::kTrim, {cx, 0.12f, tz}, {dim::kDoorLeafWidth, 0.22f, 0.06f});
        drawBox(palette::kTrim, {cx, dim::kDoorHeight - 0.10f, tz}, {dim::kDoorLeafWidth, 0.22f, 0.06f});
        drawBox(palette::kTrim, {cx - dim::kDoorLeafWidth * 0.5f + 0.10f, cy, tz},
                {0.20f, dim::kDoorHeight, 0.06f});
        drawBox(palette::kTrim, {cx + dim::kDoorLeafWidth * 0.5f - 0.10f, cy, tz},
                {0.20f, dim::kDoorHeight, 0.06f});

        // X-brace.
        drawBoxRotatedZ(palette::kTrim, {cx, cy, tz - 0.02f}, {diagLen, 0.16f, 0.05f}, diagDeg);
        drawBoxRotatedZ(palette::kTrim, {cx, cy, tz - 0.02f}, {diagLen, 0.16f, 0.05f}, -diagDeg);

        // Handle.
        drawBox(palette::kIron, {cx - side * 0.25f, 2.0f, tz - 0.06f}, {0.08f, 0.45f, 0.08f});
    }

    // Overhead track the sliding doors hang from.
    drawBox(palette::kIron,
            {0.0f, dim::kDoorHeight + 0.18f, z - 0.15f},
            {dim::kHalfWidth * 2.0f + dim::kDoorLeafWidth, 0.10f, 0.10f});
}

void drawSideDoor()
{
    const float x = dim::kHalfWidth + 0.08f;
    drawBox(palette::kDoor, {x, 1.15f, -0.2f}, {0.14f, 2.30f, 1.40f});
    drawBox(palette::kTrim, {x + 0.05f, 2.35f, -0.2f}, {0.08f, 0.20f, 1.65f});
    drawBox(palette::kTrim, {x + 0.05f, 1.15f, -0.95f}, {0.08f, 2.40f, 0.18f});
    drawBox(palette::kTrim, {x + 0.05f, 1.15f, 0.55f}, {0.08f, 2.40f, 0.18f});
    drawBox(palette::kIron, {x + 0.10f, 1.10f, 0.25f}, {0.08f, 0.10f, 0.10f});
}

void drawHayloftDoor()
{
    // Small double door in the front gable, above the main entrance.
    const float z = -dim::kHalfDepth - 0.06f;
    drawBox(palette::kTrim, {0.0f, 6.30f, z}, {1.65f, 1.75f, 0.10f});
    drawBox(palette::kDoor, {-0.38f, 6.30f, z - 0.05f}, {0.72f, 1.55f, 0.06f});
    drawBox(palette::kDoor, { 0.38f, 6.30f, z - 0.05f}, {0.72f, 1.55f, 0.06f});
    // Hay hoist beam projecting from the peak.
    drawBox(palette::kDarkWood, {0.0f, roofRidgeHeight() - 0.35f, z - 0.45f}, {0.14f, 0.14f, 1.0f});
}

void drawWindows()
{
    const float z = dim::kHalfDepth + 0.06f;
    drawWindow({-3.2f, 3.6f, -z}, 0.0f);
    drawWindow({ 3.2f, 3.6f, -z}, 0.0f);
    drawWindow({-3.2f, 3.6f,  z}, 0.0f);
    drawWindow({ 3.2f, 3.6f,  z}, 0.0f);
    drawWindow({-dim::kHalfWidth - 0.06f, 3.5f, 0.8f}, 90.0f);
}

// ---------------------------------------------------------------------------
// Interior: floor
// ---------------------------------------------------------------------------
// Straw bedding with scattered wisps and a trampled darker patch.
void drawStrawBed(float cx, float cz, float w, float d, int seed)
{
    drawBox(vary(palette::kStraw, seed, 0.08f), {cx, 0.525f, cz}, {w, 0.05f, d});
    for (int i = 0; i < 18; ++i)
    {
        const float px = cx + (hash01(seed * 13 + i * 3) - 0.5f) * (w - 0.4f);
        const float pz = cz + (hash01(seed * 17 + i * 5) - 0.5f) * (d - 0.4f);
        drawBoxYaw(vary(palette::kStraw, seed * 7 + i, 0.18f),
                   {px, 0.555f + 0.004f * (i % 4), pz},
                   {0.05f, 0.02f, 0.55f + 0.35f * hash01(seed + i)},
                   hash01(seed * 3 + i * 11) * 180.0f);
    }
    drawBox(tint(palette::kStraw, 0.55f),
            {cx + (hash01(seed) - 0.5f) * w * 0.3f, 0.553f, cz},
            {w * 0.4f, 0.006f, d * 0.4f});
}

void drawInteriorFloor()
{
    const float fl = dim::kFoundationH;

    // Packed-earth stall floors on both sides.
    for (float side : {-1.0f, 1.0f})
        drawBox(palette::kEarth, {side * 3.45f, fl + 0.025f, 0.0f}, {2.9f, 0.05f, 7.8f});

    // Central aisle: long boards with staggered butt joints.
    for (int i = 0; i < 10; ++i)
    {
        const float x = -1.8f + 0.4f * i;
        float z = -3.9f;
        float len = 1.2f + (i % 3) * 0.9f;
        int seg = 0;
        while (z < 3.89f)
        {
            const float l = std::min(len, 3.9f - z);
            if (l < 0.15f) break;
            drawBox(vary(palette::kFloor, i * 31 + seg * 7, 0.12f),
                    {x, fl + 0.025f, z + l * 0.5f}, {0.37f, 0.05f, l - 0.02f});
            z += l;
            len = 2.7f;
            ++seg;
        }
    }

    // Fieldstone kerbs separate the aisle from the stalls.
    for (float side : {-1.0f, 1.0f})
        drawBox(palette::kFoundation, {side * 2.0f, fl + 0.05f, 0.0f}, {0.14f, 0.10f, 7.8f});

    // Old oil stains on the boards.
    drawBox(tint(palette::kFloorLine, 0.45f), {0.45f, fl + 0.052f, -1.6f}, {0.8f, 0.004f, 0.55f});
    drawBox(tint(palette::kFloorLine, 0.50f), {-1.05f, fl + 0.052f, 0.9f}, {0.6f, 0.004f, 0.8f});
    drawBox(tint(palette::kFloorLine, 0.50f), {1.1f, fl + 0.052f, 1.5f}, {0.5f, 0.004f, 0.4f});
}

// ---------------------------------------------------------------------------
// Interior: wall lining
// ---------------------------------------------------------------------------
void drawInteriorWalls()
{
    // Horizontal wainscot boards on both side walls, studs, girts, loft ledger.
    for (float side : {-1.0f, 1.0f})
    {
        for (int c = 0; c < 4; ++c)
            drawBox(vary(palette::kPlank, c * 5 + (side > 0.0f ? 20 : 0), 0.10f),
                    {side * 4.87f, 0.65f + 0.3f * c, 0.0f}, {0.07f, 0.28f, 7.8f});
        drawBox(palette::kBeam, {side * 4.84f, 2.65f, 0.0f}, {0.16f, 0.20f, 7.8f});
        drawBox(palette::kBeam, {side * 4.84f, 4.20f, 0.0f}, {0.16f, 0.20f, 7.8f});
        for (float z = -3.6f; z <= 3.61f; z += 1.2f)
        {
            if (side < 0.0f && std::fabs(z - 0.8f) < 0.8f)
                continue;   // keep the side window clear
            drawBox(palette::kBeam, {side * 4.84f, 3.6f, z}, {0.16f, 3.8f, 0.16f});
        }
        // Corner posts.
        for (float sz : {-1.0f, 1.0f})
            drawBox(palette::kBeam, {side * 4.82f, 3.0f, sz * 3.82f}, {0.20f, 5.0f, 0.20f});
    }

    // Rear wall.
    for (int c = 0; c < 4; ++c)
        drawBox(vary(palette::kPlank, c * 7 + 40, 0.10f),
                {0.0f, 0.65f + 0.3f * c, 3.87f}, {9.6f, 0.28f, 0.07f});
    drawBox(palette::kBeam, {0.0f, 2.65f, 3.85f}, {9.6f, 0.20f, 0.12f});
    for (float x : {-1.25f, 0.0f, 1.25f})
        drawBox(palette::kBeam, {x, 3.6f, 3.85f}, {0.16f, 3.8f, 0.12f});
    drawBox(palette::kBeam, {0.0f, 6.9f, 3.86f}, {0.16f, 2.8f, 0.12f});   // gable king stud

    // Front wall, either side of the doorway.
    for (float sx : {-1.0f, 1.0f})
        for (int c = 0; c < 4; ++c)
            drawBox(vary(palette::kPlank, c * 3 + (sx > 0.0f ? 60 : 50), 0.10f),
                    {sx * 4.1f, 0.65f + 0.3f * c, -3.87f}, {1.5f, 0.28f, 0.07f});
}

// ---------------------------------------------------------------------------
// Interior: roof framing
// ---------------------------------------------------------------------------
void drawRoofFraming()
{
    const float ridge = roofRidgeHeight();

    // Ridge beam and wall plates.
    drawBox(palette::kBeam, {0.0f, ridge - 0.15f, 0.0f}, {0.22f, 0.22f, 8.2f});
    for (float side : {-1.0f, 1.0f})
        drawBox(palette::kBeam, {side * 4.88f, 5.4f, 0.0f}, {0.24f, 0.22f, 8.0f});

    // Common rafters tight under the roof.
    for (int i = 0; i < 9; ++i)
    {
        const float z = -3.6f + 0.9f * i;
        for (float side : {-1.0f, 1.0f})
            drawBeam(tint(palette::kBeam, 1.15f),
                     {side * 4.95f, roofUnderY(4.95f) - 0.07f, z},
                     {0.0f, roofUnderY(0.0f) - 0.07f, z}, 0.12f);
    }

    // Purlins running along the barn.
    for (float x : {-3.2f, -1.6f, 1.6f, 3.2f})
        drawBox(palette::kBeam, {x, roofUnderY(x) - 0.21f, 0.0f}, {0.14f, 0.14f, 7.9f});

    // Principal king-post trusses at the three bays.
    for (float z : {-3.25f, 0.0f, 3.25f})
    {
        drawBox(palette::kBeam, {0.0f, 5.05f, z}, {8.65f, 0.25f, 0.25f});   // tie beam
        for (float side : {-1.0f, 1.0f})
        {
            const float x = side * 4.25f;
            drawBox(palette::kFoundation, {x, 0.55f, z}, {0.50f, 0.10f, 0.50f});  // footing
            drawBox(palette::kBeam, {x, 2.75f, z}, {0.28f, 5.5f, 0.28f});          // post
            drawBeam(palette::kBeam, {x, 4.1f, z}, {side * 3.05f, 5.05f, z}, 0.16f);   // knee brace
            drawBeam(palette::kBeam, {side * 4.95f, roofUnderY(4.95f) - 0.10f, z},
                     {0.0f, roofUnderY(0.0f) - 0.10f, z}, 0.20f);                  // principal rafter
            drawBeam(palette::kBeam, {0.0f, 6.2f, z}, {side * 2.4f, 5.175f, z}, 0.16f);  // strut
        }
        drawBox(palette::kBeam, {0.0f, 6.65f, z}, {0.24f, 2.95f, 0.24f});          // king post
        drawBeam(palette::kBeam, {-2.4f, 6.75f, z}, {2.4f, 6.75f, z}, 0.14f);      // collar tie
    }
}

// ---------------------------------------------------------------------------
// Interior: stalls
// ---------------------------------------------------------------------------
void drawBucket(const Vec3& p)
{
    drawFrustum(palette::kGalv, p, 0.17f, 0.22f, 0.38f, 14);
    drawFrustum(tint(palette::kGalv, 0.7f), {p.x, p.y + 0.38f, p.z}, 0.23f, 0.23f, 0.03f, 14);
    const Vec3 h[] = {{-0.21f, 0.34f, 0.0f}, {-0.15f, 0.52f, 0.0f}, {0.0f, 0.58f, 0.0f},
                      {0.15f, 0.52f, 0.0f}, {0.21f, 0.34f, 0.0f}};
    for (int i = 0; i < 4; ++i)
        drawBeam(palette::kIron, {p.x + h[i].x, p.y + h[i].y, p.z},
                 {p.x + h[i + 1].x, p.y + h[i + 1].y, p.z}, 0.02f);
}

void drawStool(const Vec3& p)
{
    drawFrustum(palette::kWood, {p.x, p.y + 0.36f, p.z}, 0.22f, 0.22f, 0.07f, 12);
    for (int i = 0; i < 3; ++i)
    {
        const float a = i * 2.094f + 0.5f;
        drawBeam(palette::kDarkWood,
                 {p.x + std::cos(a) * 0.12f, p.y + 0.38f, p.z + std::sin(a) * 0.12f},
                 {p.x + std::cos(a) * 0.20f, p.y, p.z + std::sin(a) * 0.20f}, 0.045f);
    }
}

void drawMilkCan(const Vec3& p)
{
    drawFrustum(palette::kGalv, p, 0.20f, 0.18f, 0.62f, 14);
    drawFrustum(palette::kGalv, {p.x, p.y + 0.62f, p.z}, 0.18f, 0.10f, 0.14f, 14);
    drawFrustum(tint(palette::kGalv, 0.9f), {p.x, p.y + 0.76f, p.z}, 0.10f, 0.10f, 0.12f, 12);
    drawFrustum(tint(palette::kGalv, 0.75f), {p.x, p.y + 0.88f, p.z}, 0.14f, 0.12f, 0.06f, 12);
    for (float s : {-1.0f, 1.0f})
        drawBox(palette::kIron, {p.x + s * 0.20f, p.y + 0.50f, p.z}, {0.04f, 0.22f, 0.06f});
}

void drawHayRack(float side, float z)
{
    drawBox(palette::kPlank, {side * 4.84f, 2.25f, z}, {0.08f, 1.0f, 1.8f});
    drawBox(palette::kWood, {side * 4.55f, 1.78f, z}, {0.55f, 0.08f, 1.8f});
    for (int i = 0; i < 8; ++i)
        drawBox(palette::kDarkWood, {side * 4.30f, 2.2f, z + (i - 3.5f) * 0.24f},
                {0.05f, 0.80f, 0.05f});
    drawBox(palette::kDarkWood, {side * 4.30f, 2.62f, z}, {0.08f, 0.06f, 1.8f});
    drawBox(palette::kHay, {side * 4.57f, 2.0f, z}, {0.42f, 0.50f, 1.6f});
    for (int i = 0; i < 5; ++i)
        drawBoxYaw(vary(palette::kHay, i * 11 + static_cast<int>(z * 10.0f), 0.15f),
                   {side * 4.40f, 2.30f + 0.03f * i, z + (i - 2.0f) * 0.32f},
                   {0.16f, 0.03f, 0.4f}, hash01(i + static_cast<int>(z * 5.0f)) * 40.0f);
}

void drawTrough(float side, float z0, float z1)
{
    const float cx = side * 2.66f;
    const float len = z1 - z0;
    const float cz = (z0 + z1) * 0.5f;

    drawBox(palette::kTrough, {cx, 0.58f, cz}, {0.62f, 0.42f, len});
    for (float s : {-1.0f, 1.0f})
        drawBox(tint(palette::kTrough, 1.25f), {cx + s * 0.30f, 0.81f, cz}, {0.07f, 0.06f, len + 0.04f});
    for (float ez : {z0, z1})
        drawBox(tint(palette::kTrough, 1.25f), {cx, 0.81f, ez}, {0.68f, 0.06f, 0.07f});

    const int chunks = std::max(1, static_cast<int>(len / 1.2f));
    for (int i = 0; i < chunks; ++i)
    {
        const float zc = z0 + (i + 0.5f) * len / chunks;
        drawBox((i % 2 == 0) ? palette::kHay : palette::kFeed,
                {cx, 0.82f + 0.01f * (i % 3), zc},
                {0.50f, 0.06f + 0.03f * (i % 3), len / chunks - 0.05f});
    }
    for (float z = z0 + 0.5f; z < z1 - 0.2f; z += 1.15f)
        drawBox(palette::kIron, {cx, 0.58f, z}, {0.65f, 0.44f, 0.05f});
}

// One stall divider running from the aisle posts to the side wall. When
// `gateOnly` is set (the animated feeding gate occupies this line) only the
// posts are built so nothing is left behind when the gate swings open.
void drawStallPartition(float side, float z, bool gateOnly)
{
    for (float x : {side * 2.25f, side * 4.78f})
    {
        drawBox(palette::kBeam, {x, 1.45f, z}, {0.18f, 1.9f, 0.18f});
        drawBox(palette::kDarkWood, {x, 2.42f, z}, {0.24f, 0.06f, 0.24f});
    }
    if (gateOnly) return;

    const int base = static_cast<int>(z * 10.0f) + (side > 0.0f ? 100 : 0);
    for (int i = 0; i < 3; ++i)   // solid kickboards towards the wall
        drawBox(vary(palette::kWood, base + i * 3, 0.10f),
                {side * 3.93f, 0.68f + 0.34f * i, z}, {1.55f, 0.32f, 0.08f});
    drawBox(palette::kDarkWood, {side * 3.515f, 1.92f, z}, {2.53f, 0.12f, 0.12f});   // top rail
    drawBox(palette::kDarkWood, {side * 2.70f, 1.00f, z}, {0.90f, 0.08f, 0.10f});    // mid rail over trough
    for (int i = 0; i < 4; ++i)
        drawBox(palette::kDarkWood, {side * (2.45f + 0.2f * i), 1.45f, z}, {0.04f, 0.82f, 0.04f});
    for (int i = 0; i < 5; ++i)
        drawBox(palette::kDarkWood, {side * (3.3f + 0.35f * i), 1.69f, z}, {0.04f, 0.34f, 0.04f});
}

void drawStalls()
{
    for (float side : {-1.0f, 1.0f})
    {
        for (float z : {-2.15f, 0.15f, 2.45f})
            drawStallPartition(side, z, side > 0.0f && z < -2.0f);
        drawTrough(side, -2.7f, 2.0f);
        for (float z : {-1.0f, 1.3f})
            drawHayRack(side, z);
    }

    // Straw bedding.
    drawStrawBed( 3.95f, -1.0f, 1.75f, 1.9f, 1);
    drawStrawBed( 3.95f,  1.3f, 1.75f, 2.0f, 2);
    drawStrawBed(-3.95f, -1.0f, 1.75f, 1.9f, 3);
    drawStrawBed(-3.85f,  0.8f, 1.90f, 1.2f, 4);

    // Stall furniture.
    drawBucket({ 3.40f, 0.55f, -0.20f});
    drawBucket({ 3.50f, 0.55f,  1.90f});
    drawBucket({-3.60f, 0.55f, -1.50f});
    drawStool({-4.10f, 0.55f, -0.40f});

    // Milk cans in the front-right bay.
    drawMilkCan({3.70f, 0.50f, -3.40f});
    drawMilkCan({4.15f, 0.50f, -3.00f});
    drawMilkCan({3.35f, 0.50f, -3.05f});
}

// ---------------------------------------------------------------------------
// Interior: hayloft, ladder, gable platform
// ---------------------------------------------------------------------------
void drawHayBale(const Vec3& pos, float yaw, int seed)
{
    const Vec3 size{1.25f, 0.72f, 1.05f};
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    drawBox(vary(palette::kHay, seed, 0.08f), {0.0f, 0.0f, 0.0f}, size);
    for (float strap : {-0.30f, 0.30f})
        drawBox(palette::kHayBand, {strap, 0.0f, 0.0f}, {0.04f, size.y + 0.02f, size.z + 0.02f});
    for (int i = 0; i < 6; ++i)   // loose stalks on top
        drawBoxYaw(vary(palette::kHay, seed * 5 + i, 0.18f),
                   {(hash01(seed + i * 7) - 0.5f) * 1.1f, size.y * 0.5f + 0.01f,
                    (hash01(seed + i * 13) - 0.5f) * 0.9f},
                   {0.05f, 0.015f, 0.40f}, hash01(seed * 3 + i) * 180.0f);
    glPopMatrix();
}

void drawLadder()
{
    const float zA = -3.7f, zB = -3.1f;
    const Vec3 bottom{-3.1f, 0.5f, 0.0f}, top{-2.3f, 4.85f, 0.0f};
    for (float z : {zA, zB})
        drawBeam(palette::kWood, {bottom.x, bottom.y, z}, {top.x, top.y, z}, 0.09f);
    for (int i = 1; i <= 13; ++i)
    {
        const float t = i / 14.0f;
        drawBox(palette::kDarkWood,
                {bottom.x + (top.x - bottom.x) * t, bottom.y + (top.y - bottom.y) * t, (zA + zB) * 0.5f},
                {0.06f, 0.06f, zB - zA});
    }
}

void drawHayloft()
{
    const float postZ[] = {-2.9f, -0.95f, 1.3f, 3.3f};

    for (float side : {-1.0f, 1.0f})
    {
        const float cx = side * 2.2f;
        for (float z : postZ)
        {
            drawBox(palette::kFoundation, {cx, 0.54f, z}, {0.42f, 0.08f, 0.42f});
            drawBox(palette::kBeam, {cx, 2.185f, z}, {0.26f, 3.37f, 0.26f});
            for (float dz : {-0.7f, 0.7f})
                if (std::fabs(z + dz) < 3.8f)
                    drawBeam(palette::kBeam, {cx, 3.2f, z}, {cx, 3.85f, z + dz}, 0.12f);
        }
        drawBox(palette::kBeam, {cx, 4.02f, 0.0f}, {0.30f, 0.30f, 7.8f});   // carrier beam

        for (int i = 0; i < 13; ++i)                                          // joists
            drawBox(palette::kWood, {side * 3.55f, 4.27f, -3.6f + 0.6f * i}, {2.7f, 0.20f, 0.14f});
        for (int j = 0; j < 10; ++j)                                          // deck boards
            drawBox(vary(palette::kPlank, j * 7 + (side > 0.0f ? 30 : 0), 0.12f),
                    {side * (2.335f + 0.27f * j), 4.405f, 0.0f}, {0.25f, 0.05f, 7.6f});
        drawBox(palette::kDarkWood, {side * 2.13f, 4.27f, 0.0f}, {0.06f, 0.34f, 7.7f});   // fascia

        // Guard rail (open at the ladder on the left side).
        for (float z = -3.6f; z <= 3.61f; z += 1.2f)
        {
            if (side < 0.0f && z < -3.0f) continue;
            drawBox(palette::kBeam, {side * 2.12f, 4.9f, z}, {0.10f, 0.95f, 0.10f});
        }
        const float railStart = (side < 0.0f) ? -2.4f : -3.6f;
        const float railLen = 3.6f - railStart;
        for (float y : {4.85f, 5.2f})
            drawBox(palette::kWood, {side * 2.12f, y, (railStart + 3.6f) * 0.5f}, {0.07f, 0.08f, railLen});

        // Loose hay mounds.
        drawMound(vary(palette::kHay, 71 + (side > 0.0f ? 1 : 0), 0.1f),
                  {side * 3.9f, 4.45f, 1.1f}, 1.2f, 0.55f, 1.4f);
        drawMound(vary(palette::kHay, 73 + (side > 0.0f ? 1 : 0), 0.1f),
                  {side * 3.2f, 4.45f, 0.1f}, 0.9f, 0.40f, 1.0f);
    }

    // Stored bales on the loft.
    struct LoftBale { float x, z; int level; };
    const LoftBale loftBales[] = {{3.1f, -2.6f, 0}, {3.1f, -1.45f, 0}, {4.25f, -2.0f, 0},
                                  {3.1f, -2.0f, 1}, {3.3f, 2.2f, 0}, {3.3f, 3.3f, 0}, {4.3f, 2.7f, 0}};
    int n = 0;
    for (float side : {-1.0f, 1.0f})
    {
        for (const LoftBale& b : loftBales)
        {
            if (side < 0.0f && b.z < -2.3f) continue;   // keep the ladder landing clear
            drawHayBale({side * b.x, 4.43f + 0.36f + 0.72f * b.level, b.z},
                        (hash01(n * 3 + 1) - 0.5f) * 6.0f, 200 + n);
            ++n;
        }
    }

    drawLadder();
}

// Hay-hoist platform behind the front gable door.
void drawGablePlatform()
{
    drawBox(palette::kBeam, {0.0f, 5.05f, -3.80f}, {8.65f, 0.25f, 0.22f});
    for (int i = 0; i < 6; ++i)
        drawBox(vary(palette::kPlank, i * 9 + 3, 0.12f),
                {0.0f, 5.225f, -3.88f + 0.2f * i}, {4.8f, 0.05f, 0.19f});

    // Hoist rope, pulley block and hay hook.
    drawBox(palette::kIron, {0.0f, 8.0f, -3.45f}, {0.20f, 0.26f, 0.10f});
    drawBeam(palette::kRope, {0.0f, 8.0f, -3.45f}, {0.05f, 6.1f, -3.45f}, 0.035f);
    drawBox(palette::kIron, {0.05f, 5.95f, -3.45f}, {0.05f, 0.28f, 0.05f});
}

// ---------------------------------------------------------------------------
// Interior: workshop, shelving, tack and storage
// ---------------------------------------------------------------------------
void drawShelfItem(int kind, float x, float y, float z, int seed)
{
    switch (kind % 4)
    {
    case 0:   // slatted crate
        drawBox(vary(palette::kWood, seed, 0.12f), {x, y + 0.17f, z}, {0.50f, 0.34f, 0.40f});
        drawBox(palette::kDarkWood, {x, y + 0.10f, z}, {0.52f, 0.04f, 0.42f});
        drawBox(palette::kDarkWood, {x, y + 0.26f, z}, {0.52f, 0.04f, 0.42f});
        break;
    case 1:   // galvanised pail
        drawFrustum(palette::kGalv, {x, y, z}, 0.17f, 0.20f, 0.32f, 12);
        drawFrustum(tint(palette::kGalv, 0.7f), {x, y + 0.32f, z}, 0.21f, 0.21f, 0.025f, 12);
        break;
    case 2:   // preserve jars
        drawFrustum({0.30f, 0.52f, 0.34f}, {x - 0.10f, y, z}, 0.07f, 0.07f, 0.22f, 10);
        drawFrustum({0.62f, 0.42f, 0.14f}, {x + 0.10f, y, z}, 0.07f, 0.07f, 0.20f, 10);
        drawFrustum(palette::kIron, {x - 0.10f, y + 0.22f, z}, 0.075f, 0.075f, 0.03f, 10);
        drawFrustum(palette::kIron, {x + 0.10f, y + 0.20f, z}, 0.075f, 0.075f, 0.03f, 10);
        break;
    default:  // oil can
        drawFrustum({0.65f, 0.12f, 0.08f}, {x, y, z}, 0.13f, 0.13f, 0.26f, 10);
        drawBeam(palette::kIron, {x + 0.10f, y + 0.26f, z}, {x + 0.28f, y + 0.42f, z}, 0.03f);
        break;
    }
}

void drawShelving()
{
    // Rear-left storage shelves (same footprint as before).
    for (float y : {1.0f, 2.05f, 3.10f})
        drawBox(palette::kWood, {-3.55f, y, 3.55f}, {2.35f, 0.16f, 0.58f});
    for (float x : {-4.45f, -2.65f})
        drawBox(palette::kDarkWood, {x, 1.65f, 3.55f}, {0.16f, 3.3f, 0.16f});

    const float slots[] = {-4.2f, -3.8f, -3.4f, -3.0f};
    int level = 0;
    for (float y : {1.0f, 2.05f, 3.10f})
    {
        for (int s = 0; s < 4; ++s)
            drawShelfItem(s + level, slots[s], y + 0.08f, 3.52f, level * 11 + s);
        ++level;
    }
}

void drawWallTools()
{
    // Tools leaning against the left wall: shovel, fork, rake.
    const float z0[] = {2.70f, 2.95f, 3.20f};
    for (int i = 0; i < 3; ++i)
    {
        const float z = z0[i];
        drawBeam(palette::kWood, {-4.58f, 0.6f, z}, {-4.82f, 3.0f, z}, 0.07f);
        if (i == 0)       // shovel
            drawBox(palette::kTool, {-4.58f, 0.80f, z}, {0.03f, 0.50f, 0.32f});
        else if (i == 1)  // pitchfork
        {
            drawBox(palette::kTool, {-4.58f, 0.95f, z}, {0.04f, 0.05f, 0.30f});
            for (float dz : {-0.12f, 0.0f, 0.12f})
                drawBox(palette::kTool, {-4.58f, 0.75f, z + dz}, {0.03f, 0.40f, 0.03f});
        }
        else              // rake
        {
            drawBox(palette::kTool, {-4.58f, 0.78f, z}, {0.04f, 0.06f, 0.55f});
            for (int t = 0; t < 7; ++t)
                drawBox(palette::kTool, {-4.58f, 0.62f, z - 0.24f + t * 0.08f}, {0.025f, 0.14f, 0.02f});
        }
    }
}

void drawWorkbench()
{
    const float cx = 3.55f, cz = 3.5f;
    drawBox(palette::kWood, {cx, 1.45f, cz}, {2.3f, 0.12f, 0.75f});            // top
    drawBox(palette::kDarkWood, {cx, 0.85f, cz}, {2.1f, 0.06f, 0.62f});        // lower shelf
    for (float dx : {-1.05f, 1.05f})
        for (float dz : {-0.30f, 0.30f})
            drawBox(palette::kBeam, {cx + dx, 0.98f, cz + dz}, {0.10f, 0.96f, 0.10f});

    // Vise at the left end.
    drawBox(palette::kIron, {cx - 1.0f, 1.58f, cz + 0.28f}, {0.22f, 0.16f, 0.12f});
    drawBox(palette::kIron, {cx - 1.0f, 1.55f, cz + 0.38f}, {0.05f, 0.05f, 0.25f});

    // On the bench: hammer, nail jar, board offcuts.
    drawBox(palette::kWood, {cx - 0.2f, 1.53f, cz + 0.1f}, {0.40f, 0.04f, 0.04f});
    drawBox(palette::kTool, {cx - 0.4f, 1.56f, cz + 0.1f}, {0.10f, 0.10f, 0.07f});
    drawFrustum({0.55f, 0.62f, 0.58f}, {cx + 0.5f, 1.51f, cz - 0.1f}, 0.09f, 0.09f, 0.22f, 10);
    drawBoxYaw(palette::kWood, {cx + 0.1f, 1.53f, cz - 0.2f}, {0.7f, 0.04f, 0.14f}, 12.0f);
    // Under the bench: stack of boards.
    for (int i = 0; i < 4; ++i)
        drawBox(vary(palette::kWood, 90 + i, 0.15f), {cx + 0.1f, 0.91f + i * 0.05f, cz}, {1.7f, 0.05f, 0.30f});

    // Pegboard with outlined tools.
    drawBox(palette::kPeg, {cx, 2.7f, 3.755f}, {2.3f, 1.4f, 0.05f});
    drawBox(palette::kWood, {cx - 0.8f, 2.6f, 3.70f}, {0.05f, 0.55f, 0.04f});     // hammer
    drawBox(palette::kTool, {cx - 0.8f, 2.9f, 3.70f}, {0.22f, 0.10f, 0.07f});
    drawBox(palette::kTool, {cx - 0.45f, 2.55f, 3.70f}, {0.07f, 0.70f, 0.025f});  // wrench
    drawBox(palette::kTool, {cx - 0.45f, 2.92f, 3.70f}, {0.16f, 0.10f, 0.03f});
    drawBox(palette::kGalv, {cx + 0.15f, 2.45f, 3.70f}, {0.70f, 0.18f, 0.015f});  // saw
    drawBox(palette::kWood, {cx + 0.58f, 2.5f, 3.70f}, {0.18f, 0.24f, 0.04f});
    for (int i = 0; i < 3; ++i)                                                   // chisels
    {
        drawBox(palette::kTool, {cx + 0.75f + i * 0.14f, 2.65f, 3.70f}, {0.04f, 0.45f, 0.02f});
        drawBox(palette::kWood, {cx + 0.75f + i * 0.14f, 2.95f, 3.70f}, {0.06f, 0.18f, 0.04f});
    }
    drawLoopZ(palette::kIron, {cx - 0.1f, 3.15f, 3.70f}, 0.13f, 0.03f, 10);        // coil of wire
}

void drawSack(const Vec3& p, int seed)
{
    drawFrustum(vary(palette::kBurlap, seed, 0.10f), p, 0.28f, 0.23f, 0.62f, 10);
    drawFrustum(tint(palette::kBurlap, 0.8f), {p.x, p.y + 0.62f, p.z}, 0.23f, 0.11f, 0.12f, 10);
    drawFrustum(palette::kRope, {p.x, p.y + 0.74f, p.z}, 0.10f, 0.07f, 0.07f, 8);
}

void drawTackWall()
{
    // Harness hung on pegs, front-left of the entrance.
    drawBox(palette::kPlank, {-4.25f, 3.3f, -3.85f}, {1.2f, 0.22f, 0.07f});
    for (float x : {-4.65f, -4.25f, -3.9f})
        drawBox(palette::kIron, {x, 3.3f, -3.78f}, {0.05f, 0.05f, 0.14f});

    drawBeam(palette::kLeather, {-4.65f, 3.3f, -3.78f}, {-4.65f, 2.89f, -3.78f}, 0.04f);
    drawLoopZ(palette::kLeather, {-4.65f, 2.55f, -3.78f}, 0.34f, 0.11f, 12);       // horse collar

    drawBeam(palette::kLeather, {-4.25f, 3.3f, -3.80f}, {-4.25f, 2.3f, -3.80f}, 0.06f);   // bridle
    drawBeam(palette::kLeather, {-4.25f, 2.7f, -3.80f}, {-3.98f, 2.2f, -3.80f}, 0.05f);
    drawBeam(palette::kLeather, {-4.25f, 2.7f, -3.80f}, {-4.52f, 2.2f, -3.80f}, 0.05f);
    drawLoopZ(palette::kBrass, {-4.25f, 2.3f, -3.78f}, 0.07f, 0.025f, 8);

    drawLoopZ(palette::kRope, {-3.9f, 2.85f, -3.78f}, 0.20f, 0.07f, 10);           // rope coil
    drawLoopZ(tint(palette::kRope, 0.9f), {-3.9f, 2.85f, -3.76f}, 0.13f, 0.07f, 8);
}

void drawInteriorProps()
{
    drawShelving();
    drawWallTools();
    drawWorkbench();
    drawTackWall();

    // Grain sacks along the rear wall behind the hay.
    for (int i = 0; i < 6; ++i)
        drawSack({-1.5f + 0.6f * i, 0.50f, 3.62f}, 300 + i);
}

// ---------------------------------------------------------------------------
// Hay bales at the end of the aisle
// ---------------------------------------------------------------------------
void drawHayBales()
{
    const float baseY = dim::kFoundationH + 0.36f;
    const float z = 2.65f;
    // Bottom row of three, interlocked second row of two, one on top.
    for (int col = 0; col < 3; ++col)
        drawHayBale({-1.4f + col * 1.4f, baseY, z}, (hash01(col + 1) - 0.5f) * 4.0f, 10 + col);
    for (int col = 0; col < 2; ++col)
        drawHayBale({-0.7f + col * 1.4f, baseY + 0.72f, z}, (hash01(col + 7) - 0.5f) * 4.0f, 20 + col);
    drawHayBale({0.0f, baseY + 1.44f, z}, 2.0f, 30);

    // Loose hay spilled around the stack.
    for (int i = 0; i < 10; ++i)
        drawBoxYaw(vary(palette::kHay, 400 + i, 0.18f),
                   {-2.0f + hash01(i * 5 + 1) * 4.0f, 0.505f, 1.7f + hash01(i * 3 + 2) * 0.5f},
                   {0.06f, 0.02f, 0.5f + 0.4f * hash01(i)}, hash01(i * 9) * 180.0f);
}

// ---------------------------------------------------------------------------
// Animated pieces
// ---------------------------------------------------------------------------
void drawWheelbarrow(float offset, float wheelRotation)
{
    // Rides up onto the raised floor when it is inside the barn.
    const float zWorld = -0.15f + offset;
    const float t = std::clamp((zWorld + 4.6f) / 0.9f, 0.0f, 1.0f);
    const float lift = dim::kFoundationH * t * t * (3.0f - 2.0f * t);

    glPushMatrix();
    glTranslatef(0.65f, lift, -0.15f + offset);

    // Tray and flared side boards.
    drawBox(palette::kIron, {0.0f, 0.92f, 0.0f}, {1.15f, 0.18f, 1.55f});
    for (float side : {-1.0f, 1.0f})
        drawBoxRotatedZ(palette::kTool, {side * 0.55f, 1.18f, 0.0f},
                        {0.12f, 0.58f, 1.48f}, -side * 16.0f);
    drawBox(palette::kTool, {0.0f, 1.15f, 0.72f}, {1.12f, 0.52f, 0.10f});

    // Handles point toward the rear (+Z); legs stop the tray tipping.
    for (float side : {-1.0f, 1.0f})
    {
        drawBox(palette::kWood, {side * 0.43f, 0.72f, 1.10f},
                {0.10f, 0.10f, 2.00f});
        drawBoxRotatedZ(palette::kWood, {side * 0.43f, 0.36f, 0.68f},
                        {0.09f, 0.70f, 0.09f}, side * 8.0f);
    }

    glColor3f(palette::kIron.r, palette::kIron.g, palette::kIron.b);
    glPushMatrix();
    glTranslatef(0.0f, 0.43f, -1.02f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(wheelRotation, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(0.38f, 0.20f, 18);
    glPopMatrix();
    glPopMatrix();
}

void drawStorageChest(float openAmount)
{
    const Vec3 base{-3.75f, 0.77f, 1.90f};
    drawBox(palette::kChest, base, {1.55f, 0.62f, 0.82f});
    drawBox(palette::kIron, {base.x, base.y, base.z - 0.43f},
            {0.15f, 0.48f, 0.05f});
    for (float x : {-4.35f, -3.15f})
        drawBox(palette::kIron, {x, base.y, base.z}, {0.07f, 0.66f, 0.86f});

    // Lid rotates around its rear edge without changing position when the
    // target is reversed halfway through the animation.
    glPushMatrix();
    glTranslatef(base.x, 1.10f, base.z + 0.39f);
    glRotatef(-92.0f * openAmount, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, -0.39f);
    drawBox(palette::kChest, {0.0f, 0.0f, 0.0f}, {1.62f, 0.16f, 0.84f});
    glPopMatrix();
}

void drawFeedingGate(float openAmount)
{
    glPushMatrix();
    glTranslatef(2.18f, 0.0f, -2.20f);
    glRotatef(-82.0f * openAmount, 0.0f, 1.0f, 0.0f);
    for (float y : {0.55f, 1.15f, 1.75f})
        drawBox(palette::kWood, {1.15f, y, 0.0f}, {2.3f, 0.13f, 0.13f});
    for (float x : {0.08f, 1.15f, 2.22f})
        drawBox(palette::kBeam, {x, 1.15f, 0.0f}, {0.13f, 1.85f, 0.13f});
    glPopMatrix();
    drawBox(palette::kIron, {2.15f, 1.15f, -2.20f}, {0.20f, 2.15f, 0.20f});
}

void drawInteriorLamp(float amount)
{
    drawBox(palette::kIron, {0.0f, 4.98f, 0.0f}, {0.08f, 0.38f, 0.08f});
    drawBox(palette::kTool, {0.0f, 4.72f, 0.0f}, {0.58f, 0.12f, 0.58f});
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT);
    const GLfloat emission[] = {
        palette::kLamp.r * amount, palette::kLamp.g * amount,
        palette::kLamp.b * amount, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glColor3f(0.22f + 0.78f * amount,
              0.20f + 0.56f * amount,
              0.16f + 0.18f * amount);
    glPushMatrix();
    glTranslatef(0.0f, 4.54f, 0.0f);
    Primitives::drawSphere(0.20f, 12, 8);
    glPopMatrix();
    glPopAttrib();

    // Warm light spilling over the aisle and stall floors (blended, drawn last).
    drawLightPool({0.0f, 0.53f, 0.0f}, 3.8f, amount);
    drawLightPool({0.0f, 4.45f, 0.0f}, 3.0f, amount * 0.35f);
}

void drawEquipment()
{
    // Hand cart and barrel parked beside the entrance.
    constexpr float kZ = -5.0f;
    constexpr float kCartX = 6.0f;
    constexpr float kBarrelX = -6.0f;

    // Cart bed with side boards.
    drawBox(palette::kDarkWood, {kCartX, 0.65f, kZ}, {1.8f, 0.18f, 1.0f});
    drawBox(palette::kWood, {kCartX, 1.05f, kZ}, {1.2f, 0.55f, 0.80f});
    // Handle.
    drawBox(palette::kDarkWood, {kCartX + 1.2f, 0.85f, kZ}, {0.9f, 0.08f, 0.08f});

    setColor(palette::kIron);
    for (float dx : {-0.55f, 0.55f})
    {
        glPushMatrix();
        glTranslatef(kCartX + dx, 0.35f, kZ);
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        Primitives::drawCylinder(0.28f, 0.16f, 16);
        glPopMatrix();
    }

    // Barrel with two iron hoops.
    drawBox(palette::kDarkWood, {kBarrelX, 0.55f, kZ}, {1.0f, 1.1f, 1.0f});
    for (float y : {0.25f, 0.85f})
    {
        drawBox(palette::kIron, {kBarrelX, y, kZ}, {1.04f, 0.07f, 1.04f});
    }
}

void drawFence(float halfWidth, float halfDepth)
{
    constexpr float kPostSize = 0.22f;
    constexpr float kPostHeight = 1.15f;
    constexpr float kRailThickness = 0.10f;
    constexpr float kPostSpacing = 3.5f;
    constexpr float kGateHalfGap = 1.8f;   // opening centred on the front side

    const float postY = kPostHeight * 0.5f;
    const float railHeights[] = {0.45f, 0.90f};

    // Posts around the perimeter.
    for (float x = -halfWidth; x <= halfWidth + 0.001f; x += kPostSpacing)
    {
        drawBox(palette::kWood, {x, postY, -halfDepth}, {kPostSize, kPostHeight, kPostSize});
        drawBox(palette::kWood, {x, postY,  halfDepth}, {kPostSize, kPostHeight, kPostSize});
    }
    for (float z = -halfDepth + kPostSpacing; z < halfDepth - 0.001f; z += kPostSpacing)
    {
        drawBox(palette::kWood, {-halfWidth, postY, z}, {kPostSize, kPostHeight, kPostSize});
        drawBox(palette::kWood, { halfWidth, postY, z}, {kPostSize, kPostHeight, kPostSize});
    }
    // Gate posts flanking the front opening.
    for (float sx : {-1.0f, 1.0f})
    {
        drawBox(palette::kDarkWood, {sx * kGateHalfGap, postY + 0.1f, -halfDepth},
                {kPostSize * 1.4f, kPostHeight + 0.2f, kPostSize * 1.4f});
    }

    // Rails.
    for (float y : railHeights)
    {
        // Back and side rails run the full length.
        drawBox(palette::kWood, {0.0f, y, halfDepth}, {halfWidth * 2.0f, kRailThickness, kRailThickness});
        drawBox(palette::kWood, {-halfWidth, y, 0.0f}, {kRailThickness, kRailThickness, halfDepth * 2.0f});
        drawBox(palette::kWood, { halfWidth, y, 0.0f}, {kRailThickness, kRailThickness, halfDepth * 2.0f});

        // Front rails stop short of the gate opening.
        const float segLen = halfWidth - kGateHalfGap;
        const float segCenter = kGateHalfGap + segLen * 0.5f;
        for (float sx : {-1.0f, 1.0f})
        {
            drawBox(palette::kWood, {sx * segCenter, y, -halfDepth},
                    {segLen, kRailThickness, kRailThickness});
        }
    }
}

void drawStaticBarn(bool fenced)
{
    drawFoundation();
    drawWalls();
    drawBattens();
    drawRoof();

    drawInteriorFloor();
    drawInteriorWalls();
    drawRoofFraming();
    drawStalls();
    drawHayloft();
    drawGablePlatform();
    drawInteriorProps();

    drawSideDoor();
    drawHayloftDoor();
    drawWindows();
    drawHayBales();
    drawEquipment();
    if (fenced)
        drawFence(7.0f, 6.0f);
}

void drawCachedStaticBarn(bool fenced)
{
    static GLuint lists[2] = {0, 0};
    const int index = fenced ? 1 : 0;
    if (lists[index] == 0)
    {
        lists[index] = glGenLists(1);
        glNewList(lists[index], GL_COMPILE);
        drawStaticBarn(fenced);
        glEndList();
    }
    glCallList(lists[index]);
}
}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void Barn::draw(float x, float z, float scale, float rotation, bool fenced,
                const Barn::State& state)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    drawCachedStaticBarn(fenced);
    drawFrontDoors(std::clamp(state.doorOpen, 0.0f, 1.0f));
    drawWheelbarrow(state.wheelbarrowOffset, state.wheelRotation);
    drawStorageChest(std::clamp(state.chestOpen, 0.0f, 1.0f));
    drawFeedingGate(std::clamp(state.feedingGateOpen, 0.0f, 1.0f));
    drawInteriorLamp(std::clamp(state.electricLight, 0.0f, 1.0f));

    glPopMatrix();
}