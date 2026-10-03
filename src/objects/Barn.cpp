#include "objects/Barn.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

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
constexpr float kRoofPitchDeg   = 30.0f;
constexpr float kRoofOverhang   = 0.5f;
constexpr float kRoofThickness  = 0.20f;
constexpr float kPi             = 3.14159265358979f;
constexpr float kBattenSpacing  = 0.625f;
constexpr float kBattenSize     = 0.14f;
constexpr float kBattenRelief   = 0.06f;
constexpr float kDoorHeight     = 4.2f;
constexpr float kDoorLeafWidth  = 3.55f;
constexpr float kDoorCenterX    = 2.25f;
}  // namespace dim

float degToRad(float deg) { return deg * dim::kPi / 180.0f; }

float roofRidgeHeight()
{
    return dim::kWallHeight + dim::kHalfWidth * std::tan(degToRad(dim::kRoofPitchDeg));
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
// Barn parts
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

    drawBox(palette::kWall, {0.0f, h * 0.5f, 0.0f}, {w, h, d});

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

void drawFrontDoors()
{
    const float z = -dim::kHalfDepth - 0.08f;
    const float cy = dim::kDoorHeight * 0.5f;

    const float innerW = dim::kDoorLeafWidth - 0.35f;
    const float innerH = dim::kDoorHeight - 0.35f;
    const float diagLen = std::sqrt(innerW * innerW + innerH * innerH);
    const float diagDeg = std::atan2(innerH, innerW) * 180.0f / dim::kPi;

    for (float side : {-1.0f, 1.0f})
    {
        const float cx = side * dim::kDoorCenterX;

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
            {dim::kDoorCenterX * 4.0f, 0.10f, 0.10f});
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

void drawHayBales()
{
    constexpr int kRows = 2;
    constexpr int kColumns = 3;
    const Vec3 baleSize{1.25f, 0.72f, 1.05f};

    for (int row = 0; row < kRows; ++row)
    {
        for (int col = 0; col < kColumns; ++col)
        {
            // Stagger the upper row so the stack interlocks.
            const float stagger = (row % 2) ? 0.35f : 0.0f;
            const Vec3 pos{-2.2f + col * 1.55f + stagger,
                           dim::kFoundationH + baleSize.y * 0.5f + row * baleSize.y,
                           2.65f};
            drawBox(palette::kHay, pos, baleSize);

            // Two binding straps per bale.
            for (float strap : {-0.30f, 0.30f})
            {
                drawBox(palette::kHayBand,
                        {pos.x + strap, pos.y, pos.z},
                        {0.04f, baleSize.y + 0.02f, baleSize.z + 0.02f});
            }
        }
    }
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
}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void Barn::draw(float x, float z, float scale, float rotation, bool fenced)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    drawFoundation();
    drawWalls();
    drawBattens();
    drawRoof();
    drawFrontDoors();
    drawSideDoor();
    drawHayloftDoor();
    drawWindows();
    drawHayBales();
    drawEquipment();

    if (fenced)
    {
        drawFence(7.0f, 6.0f);
    }

    glPopMatrix();
}