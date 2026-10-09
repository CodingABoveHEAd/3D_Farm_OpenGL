#include "objects/PowerSubstation.h"

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

bool gGridPowered = true;

// Tiny deterministic random generator so the gravel/scatter never changes.
struct Rng
{
    unsigned int state;

    float next()
    {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8) & 0xFFFFFF) / 16777216.0f;
    }

    float range(float lo, float hi) { return lo + (hi - lo) * next(); }
};

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
namespace palette
{
constexpr Color kConcrete   {0.48f, 0.49f, 0.47f};
constexpr Color kPlinth     {0.36f, 0.37f, 0.36f};
constexpr Color kGravel     {0.40f, 0.39f, 0.36f};
constexpr Color kBuilding   {0.42f, 0.45f, 0.43f};
constexpr Color kRoof       {0.16f, 0.18f, 0.19f};
constexpr Color kTrim       {0.72f, 0.74f, 0.69f};
constexpr Color kDoor       {0.18f, 0.21f, 0.20f};
constexpr Color kGlass      {0.18f, 0.43f, 0.52f};
constexpr Color kYellow     {0.95f, 0.72f, 0.08f};
constexpr Color kBlack      {0.04f, 0.04f, 0.03f};
constexpr Color kWood       {0.30f, 0.20f, 0.11f};
constexpr Color kWire       {0.04f, 0.05f, 0.04f};
constexpr Color kSmoke      {0.72f, 0.76f, 0.76f};
constexpr Color kSteel      {0.58f, 0.60f, 0.62f};   // galvanised steel
constexpr Color kDarkSteel  {0.30f, 0.32f, 0.34f};
constexpr Color kTransformer{0.40f, 0.48f, 0.44f};   // grey-green tank paint
constexpr Color kPorcelain  {0.55f, 0.38f, 0.22f};   // insulator brown glaze
constexpr Color kAluminium  {0.78f, 0.80f, 0.82f};
constexpr Color kCopper     {0.72f, 0.44f, 0.24f};
constexpr Color kRed        {0.76f, 0.10f, 0.08f};
constexpr Color kWhite      {0.92f, 0.92f, 0.90f};
constexpr Color kEarth      {0.35f, 0.62f, 0.20f};   // green/yellow earth strap
constexpr Color kLampGlow   {1.00f, 0.93f, 0.62f};
constexpr Color kLampOff    {0.16f, 0.15f, 0.11f};
constexpr Color kBund       {0.31f, 0.31f, 0.30f};
constexpr Color kGenerator  {0.88f, 0.60f, 0.08f};
constexpr Color kDrumRed    {0.70f, 0.10f, 0.07f};
constexpr Color kDrumBlue   {0.12f, 0.28f, 0.60f};
constexpr Color kGreyPaint  {0.62f, 0.64f, 0.62f};
constexpr Color kArrester   {0.70f, 0.71f, 0.69f};
constexpr Color kSilica     {0.20f, 0.35f, 0.75f};
}  // namespace palette

Color lampColor()
{
    return gGridPowered ? palette::kLampGlow : palette::kLampOff;
}

// ---------------------------------------------------------------------------
// Dimensions (substation-local units, before the caller's scale)
// ---------------------------------------------------------------------------
namespace dim
{
// Concrete slab and building
constexpr float kSlabTopY        = 0.36f;
constexpr float kBuildingWidth   = 8.8f;
constexpr float kBuildingDepth   = 5.8f;
constexpr float kBuildingHeight  = 4.0f;
constexpr float kRoofThickness   = 0.28f;
constexpr float kFrontZ          = -kBuildingDepth * 0.5f;    // front wall faces -Z
constexpr float kDoorX           = -2.55f;

// Switchyard
constexpr float kYardHalfWidth   = 6.8f;
constexpr float kYardHalfDepth   = 5.6f;
constexpr float kTransformerZ    = 4.5f;
constexpr float kGantryHeight    = 4.8f;
constexpr float kGantryHalfSpan  = 4.9f;
constexpr float kPadHeight       = 0.20f;
constexpr float kGravelTopY      = 0.04f;

// Transformer details shared with the gantry
constexpr float kLidTopY         = 1.28f;
constexpr float kHvOffsetX       = 0.42f;   // HV bushing row, relative to tank
constexpr float kHvTerminalY     = 2.42f;
constexpr float kLvTerminalY     = 1.72f;
constexpr float kPhaseSpacing    = 0.35f;
constexpr float kStringBottomY   = 3.80f;
constexpr float kCableTrayY      = 3.35f;

// Fence
constexpr float kFenceHeight     = 2.2f;
constexpr float kPostSpacing     = 2.4f;
constexpr float kGateCenterX     = kDoorX;
constexpr float kGateHalfGap     = 1.2f;

// Utility pole line
constexpr float kPoleX           = -8.8f;
constexpr float kPoleHeight      = 8.0f;
constexpr float kCrossarmY       = 7.75f;
constexpr float kInsulatorTopY   = 8.07f;
constexpr float kPi              = 3.14159265358979f;

// Switchyard bay on the east side
constexpr float kBayX            = 5.9f;
}  // namespace dim

float radToDeg(float rad) { return rad * 180.0f / dim::kPi; }

// ---------------------------------------------------------------------------
// GL state captured when geometry is built
// ---------------------------------------------------------------------------
bool gLit = false;
GLuint gStaticList = 0;
GLuint gRoadList = 0;

// ---------------------------------------------------------------------------
// Drawing helpers (all raw GL so they are safe inside display lists)
// ---------------------------------------------------------------------------
void setColor(const Color& color) { glColor3f(color.r, color.g, color.b); }

Color shade(const Color& c, float k) { return {c.r * k, c.g * k, c.b * k}; }

void unitCube()
{
    glBegin(GL_QUADS);
    glNormal3f(1, 0, 0);
    glVertex3f(0.5f, -0.5f, 0.5f);  glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);  glVertex3f(0.5f, 0.5f, 0.5f);
    glNormal3f(-1, 0, 0);
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);   glVertex3f(-0.5f, 0.5f, -0.5f);
    glNormal3f(0, 1, 0);
    glVertex3f(-0.5f, 0.5f, 0.5f);  glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);  glVertex3f(-0.5f, 0.5f, -0.5f);
    glNormal3f(0, -1, 0);
    glVertex3f(-0.5f, -0.5f, -0.5f); glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);   glVertex3f(-0.5f, -0.5f, 0.5f);
    glNormal3f(0, 0, 1);
    glVertex3f(-0.5f, -0.5f, 0.5f); glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);   glVertex3f(-0.5f, 0.5f, 0.5f);
    glNormal3f(0, 0, -1);
    glVertex3f(0.5f, -0.5f, -0.5f); glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f); glVertex3f(0.5f, 0.5f, -0.5f);
    glEnd();
}

// Axis-aligned box centred on `pos`.
void box(const Color& color, const Vec3& pos, const Vec3& size)
{
    setColor(color);
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glScalef(size.x, size.y, size.z);
    unitCube();
    glPopMatrix();
}

// Box rotated around Z, used for braces.
void boxRotatedZ(const Color& color, const Vec3& pos, const Vec3& size, float angleDeg)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);
    box(color, {0.0f, 0.0f, 0.0f}, size);
    glPopMatrix();
}

// Upright (optionally tapered) cylinder. `base` is the centre of the bottom.
void column(const Color& color, const Vec3& base, float r0, float r1, float height,
            int segments = 12)
{
    setColor(color);
    glPushMatrix();
    glTranslatef(base.x, base.y, base.z);

    const float slope = (r0 - r1) / height;
    const float nLen = std::sqrt(1.0f + slope * slope);

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i)
    {
        const float a = 2.0f * dim::kPi * static_cast<float>(i) / segments;
        const float ca = std::cos(a);
        const float sa = std::sin(a);
        glNormal3f(ca / nLen, slope / nLen, sa / nLen);
        glVertex3f(r0 * ca, 0.0f, r0 * sa);
        glVertex3f(r1 * ca, height, r1 * sa);
    }
    glEnd();

    // Top cap (counter-clockwise seen from above).
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0.0f, height, 0.0f);
    for (int i = segments; i >= 0; --i)
    {
        const float a = 2.0f * dim::kPi * static_cast<float>(i) / segments;
        glVertex3f(r1 * std::cos(a), height, r1 * std::sin(a));
    }
    glEnd();

    // Bottom cap.
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= segments; ++i)
    {
        const float a = 2.0f * dim::kPi * static_cast<float>(i) / segments;
        glVertex3f(r0 * std::cos(a), 0.0f, r0 * std::sin(a));
    }
    glEnd();

    glPopMatrix();
}

// Round tube between two arbitrary points.
void tube(const Color& color, const Vec3& a, const Vec3& b, float radius, int segments = 8)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float dz = b.z - a.z;
    const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-5f)
    {
        return;
    }

    const float ny = dy / len;
    float ax = dz / len;   // axis = Y x direction
    float az = -dx / len;
    float angle = radToDeg(std::acos(std::max(-1.0f, std::min(1.0f, ny))));
    if (std::fabs(ax) + std::fabs(az) < 1e-5f)
    {
        ax = 1.0f;
        az = 0.0f;
        angle = ny > 0.0f ? 0.0f : 180.0f;
    }

    glPushMatrix();
    glTranslatef(a.x, a.y, a.z);
    glRotatef(angle, ax, 0.0f, az);
    column(color, {0.0f, 0.0f, 0.0f}, radius, radius, len, segments);
    glPopMatrix();
}

// Square-section member between two arbitrary points (lattice steel, braces).
void beam(const Color& color, const Vec3& a, const Vec3& b, float w, float h)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float dz = b.z - a.z;
    const float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 1e-5f)
    {
        return;
    }

    const float nz = dz / len;
    float ax = -dy / len;   // axis = Z x direction
    float ay = dx / len;
    float angle = radToDeg(std::acos(std::max(-1.0f, std::min(1.0f, nz))));
    if (std::fabs(ax) + std::fabs(ay) < 1e-5f)
    {
        ax = 0.0f;
        ay = 1.0f;
        angle = nz > 0.0f ? 0.0f : 180.0f;
    }

    glPushMatrix();
    glTranslatef((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f);
    glRotatef(angle, ax, ay, 0.0f);
    box(color, {0.0f, 0.0f, 0.0f}, {w, h, len});
    glPopMatrix();
}

// Horizontal ring made of short tubes (corona rings, hoops).
void ring(const Color& color, const Vec3& centre, float radius, float thickness, int segments = 12)
{
    for (int i = 0; i < segments; ++i)
    {
        const float a0 = 2.0f * dim::kPi * static_cast<float>(i) / segments;
        const float a1 = 2.0f * dim::kPi * static_cast<float>(i + 1) / segments;
        tube(color,
             {centre.x + radius * std::cos(a0), centre.y, centre.z + radius * std::sin(a0)},
             {centre.x + radius * std::cos(a1), centre.y, centre.z + radius * std::sin(a1)},
             thickness, 5);
    }
}

// Porcelain-style insulator: core with conical sheds. Grows upward from `base`.
void insulator(const Color& color, const Vec3& base, float coreRadius, float discRadius,
               int discs, float spacing)
{
    const float h = spacing * static_cast<float>(discs + 1);
    column(color, base, coreRadius, coreRadius, h, 10);
    for (int i = 0; i < discs; ++i)
    {
        const float y = base.y + spacing * (static_cast<float>(i) + 0.55f);
        column(shade(color, 1.10f), {base.x, y, base.z}, discRadius, coreRadius * 1.2f,
               spacing * 0.55f, 10);
    }
}

// Lines are drawn unlit so thin wires keep a constant dark colour.
void beginLines()
{
    if (gLit)
    {
        glDisable(GL_LIGHTING);
    }
}

void endLines()
{
    if (gLit)
    {
        glEnable(GL_LIGHTING);
    }
}

void line(const Color& color, const Vec3& a, const Vec3& b)
{
    beginLines();
    setColor(color);
    glBegin(GL_LINES);
    glVertex3f(a.x, a.y, a.z);
    glVertex3f(b.x, b.y, b.z);
    glEnd();
    endLines();
}

// Conductor between two points. A parabola approximates a hanging cable: the
// sag is zero at both ends and `sag` below the straight line at mid-span.
void drawWire(const Vec3& a, const Vec3& b, float sag)
{
    constexpr int kSegments = 20;
    beginLines();
    setColor(palette::kWire);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= kSegments; ++i)
    {
        const float t = static_cast<float>(i) / kSegments;
        glVertex3f(a.x + (b.x - a.x) * t,
                   a.y + (b.y - a.y) * t - sag * 4.0f * t * (1.0f - t),
                   a.z + (b.z - a.z) * t);
    }
    glEnd();
    glLineWidth(1.0f);
    endLines();
}

// ---------------------------------------------------------------------------
// Signs and windows
// ---------------------------------------------------------------------------
// Both facing -Z; `z` is the wall surface the sign is mounted on.
void drawWarningSign(float x, float y, float z, float size = 0.65f)
{
    const float k = size / 0.65f;
    box(palette::kYellow, {x, y, z}, {size, size, 0.04f});
    box(palette::kBlack, {x, y + 0.10f * k, z - 0.03f}, {0.08f * k, 0.28f * k, 0.04f});
    box(palette::kBlack, {x, y - 0.17f * k, z - 0.03f}, {0.10f * k, 0.08f * k, 0.04f});
}

// "DANGER - HIGH VOLTAGE" style plate. Yaw 0 faces -Z.
void drawDangerSign(const Vec3& pos, float yawDeg, float size)
{
    glPushMatrix();
    glTranslatef(pos.x, pos.y, pos.z);
    glRotatef(yawDeg, 0.0f, 1.0f, 0.0f);

    box(palette::kWhite, {0.0f, 0.0f, 0.0f}, {size * 1.25f, size * 1.55f, 0.04f});
    box(palette::kRed, {0.0f, size * 0.52f, -0.025f}, {size * 1.15f, size * 0.40f, 0.03f});
    box(palette::kWhite, {0.0f, size * 0.52f, -0.045f}, {size * 0.80f, size * 0.12f, 0.02f});
    box(palette::kYellow, {0.0f, -size * 0.12f, -0.025f}, {size * 0.55f, size * 0.60f, 0.03f});
    boxRotatedZ(palette::kBlack, {-size * 0.04f, -size * 0.02f, -0.045f},
                {size * 0.10f, size * 0.34f, 0.02f}, 22.0f);
    boxRotatedZ(palette::kBlack, {size * 0.04f, -size * 0.24f, -0.045f},
                {size * 0.10f, size * 0.30f, 0.02f}, 22.0f);

    glPopMatrix();
}

void drawWindow(float x, float y, float z)
{
    box(palette::kGlass, {x, y, z}, {0.85f, 0.75f, 0.06f});

    const float fz = z - 0.04f;
    box(palette::kTrim, {x, y + 0.42f, fz}, {1.00f, 0.08f, 0.08f});
    box(palette::kTrim, {x, y - 0.42f, fz}, {1.00f, 0.08f, 0.08f});
    box(palette::kTrim, {x - 0.46f, y, fz}, {0.08f, 0.85f, 0.08f});
    box(palette::kTrim, {x + 0.46f, y, fz}, {0.08f, 0.85f, 0.08f});
    box(palette::kTrim, {x, y, z - 0.05f}, {0.06f, 0.75f, 0.08f});

    // Projecting sill and concrete lintel.
    box(palette::kConcrete, {x, y - 0.50f, fz - 0.04f}, {1.10f, 0.07f, 0.16f});
    box(palette::kConcrete, {x, y + 0.52f, fz - 0.03f}, {1.14f, 0.10f, 0.14f});

    // Security bars.
    for (int i = 0; i < 5; ++i)
    {
        box(palette::kDarkSteel, {x + (static_cast<float>(i) - 2.0f) * 0.17f, y, z - 0.10f},
            {0.025f, 0.80f, 0.025f});
    }
    box(palette::kDarkSteel, {x, y, z - 0.10f}, {0.85f, 0.025f, 0.025f});
}

// Louvre panel on a wall facing +X (outward = 1) or -X (outward = -1).
void drawLouvre(float wallX, float y, float z, float width, float height, float outward)
{
    box(palette::kBlack, {wallX + outward * 0.03f, y, z}, {0.07f, height, width});

    box(palette::kTrim, {wallX + outward * 0.05f, y + height * 0.5f + 0.04f, z}, {0.10f, 0.08f, width + 0.16f});
    box(palette::kTrim, {wallX + outward * 0.05f, y - height * 0.5f - 0.04f, z}, {0.10f, 0.08f, width + 0.16f});
    box(palette::kTrim, {wallX + outward * 0.05f, y, z - width * 0.5f - 0.04f}, {0.10f, height, 0.08f});
    box(palette::kTrim, {wallX + outward * 0.05f, y, z + width * 0.5f + 0.04f}, {0.10f, height, 0.08f});

    constexpr int kSlats = 8;
    for (int i = 0; i < kSlats; ++i)
    {
        const float yy = y - height * 0.5f + (static_cast<float>(i) + 0.5f) * height / kSlats;
        glPushMatrix();
        glTranslatef(wallX + outward * 0.07f, yy, z);
        glRotatef(-30.0f * outward, 0.0f, 0.0f, 1.0f);
        box(palette::kSteel, {0.0f, 0.0f, 0.0f}, {0.20f, 0.035f, width});
        glPopMatrix();
    }
}

// ---------------------------------------------------------------------------
// Control building
// ---------------------------------------------------------------------------
void drawRoofEquipment(float roofTopY)
{
    // Two rooftop air-conditioning units (fans spin in drawDynamic).
    for (float ux : {-2.5f, -0.6f})
    {
        box(palette::kDarkSteel, {ux, roofTopY + 0.08f, 1.0f}, {1.5f, 0.16f, 1.2f});
        box(palette::kGreyPaint, {ux, roofTopY + 0.46f, 1.0f}, {1.3f, 0.60f, 1.0f});
        column(palette::kDarkSteel, {ux, roofTopY + 0.76f, 1.0f}, 0.42f, 0.42f, 0.06f, 16);
        column(palette::kBlack, {ux, roofTopY + 0.77f, 1.0f}, 0.36f, 0.36f, 0.03f, 16);
        box(palette::kDarkSteel, {ux, roofTopY + 0.81f, 1.0f}, {0.78f, 0.02f, 0.03f});
        box(palette::kDarkSteel, {ux, roofTopY + 0.81f, 1.0f}, {0.03f, 0.02f, 0.78f});
        // Side grilles and refrigerant lines.
        box(palette::kBlack, {ux, roofTopY + 0.40f, 1.51f}, {0.9f, 0.30f, 0.03f});
        tube(palette::kCopper, {ux + 0.5f, roofTopY + 0.16f, 0.5f}, {ux + 0.5f, roofTopY + 0.16f, -0.6f}, 0.025);
    }

    // Skylight with curb.
    box(palette::kConcrete, {1.1f, roofTopY + 0.10f, -1.2f}, {1.3f, 0.20f, 1.1f});
    box(palette::kGlass, {1.1f, roofTopY + 0.215f, -1.2f}, {1.1f, 0.03f, 0.9f});
    box(palette::kTrim, {1.1f, roofTopY + 0.235f, -1.2f}, {1.1f, 0.02f, 0.04f});

    // Roof hatch.
    box(palette::kDarkSteel, {-3.3f, roofTopY + 0.12f, -1.6f}, {0.95f, 0.24f, 0.95f});
    box(palette::kSteel, {-3.3f, roofTopY + 0.26f, -1.6f}, {0.85f, 0.04f, 0.85f});
    box(palette::kBlack, {-3.3f, roofTopY + 0.30f, -1.20f}, {0.10f, 0.04f, 0.04f});

    // Satellite dish on a short pedestal.
    column(palette::kDarkSteel, {3.6f, roofTopY, -1.8f}, 0.07f, 0.05f, 0.65f, 8);
    glPushMatrix();
    glTranslatef(3.6f, roofTopY + 0.72f, -1.8f);
    glRotatef(200.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-55.0f, 1.0f, 0.0f, 0.0f);
    column(palette::kTrim, {0.0f, 0.0f, 0.0f}, 0.05f, 0.45f, 0.16f, 16);
    tube(palette::kDarkSteel, {0.0f, 0.05f, 0.0f}, {0.0f, 0.55f, 0.0f}, 0.012, 5);
    box(palette::kBlack, {0.0f, 0.57f, 0.0f}, {0.07f, 0.05f, 0.07f});
    glPopMatrix();

    // Antenna mast with cross elements, guy wires and a beacon base.
    const Vec3 mast{0.9f, roofTopY, 1.9f};
    column(palette::kSteel, mast, 0.05f, 0.03f, 2.4f, 8);
    box(palette::kDarkSteel, {mast.x, roofTopY + 0.08f, mast.z}, {0.3f, 0.16f, 0.3f});
    box(palette::kSteel, {mast.x, roofTopY + 1.0f, mast.z}, {0.9f, 0.025f, 0.025f});
    box(palette::kSteel, {mast.x, roofTopY + 1.4f, mast.z}, {0.7f, 0.025f, 0.025f});
    box(palette::kSteel, {mast.x, roofTopY + 1.8f, mast.z}, {0.5f, 0.025f, 0.025f});
    box(palette::kDarkSteel, {mast.x, roofTopY + 2.42f, mast.z}, {0.10f, 0.06f, 0.10f});
    line(palette::kDarkSteel, {mast.x, roofTopY + 1.6f, mast.z}, {mast.x - 1.2f, roofTopY, mast.z - 0.9f});
    line(palette::kDarkSteel, {mast.x, roofTopY + 1.6f, mast.z}, {mast.x + 1.2f, roofTopY, mast.z - 0.9f});
    line(palette::kDarkSteel, {mast.x, roofTopY + 1.6f, mast.z}, {mast.x, roofTopY, mast.z + 1.2f});

    // Lightning air terminals at the roof corners.
    for (float sx : {-1.0f, 1.0f})
    {
        for (float sz : {-1.0f, 1.0f})
        {
            const float px = sx * 4.45f;
            const float pz = sz * 2.95f;
            tube(palette::kCopper, {px, roofTopY, pz}, {px, roofTopY + 0.65f, pz}, 0.015, 5);
            box(palette::kDarkSteel, {px, roofTopY + 0.03f, pz}, {0.10f, 0.06f, 0.10f});
        }
    }
}

void drawBuildingFrontDetails(float wallTopY)
{
    const float doorZ = dim::kFrontZ - 0.05f;

    // Concrete canopy over the entrance on two diagonal brackets.
    box(palette::kConcrete, {dim::kDoorX, 3.45f, dim::kFrontZ - 0.45f}, {2.3f, 0.12f, 0.90f});
    box(palette::kTrim, {dim::kDoorX, 3.53f, dim::kFrontZ - 0.88f}, {2.3f, 0.06f, 0.05f});
    for (float side : {-1.0f, 1.0f})
    {
        beam(palette::kDarkSteel, {dim::kDoorX + side * 0.95f, 3.38f, dim::kFrontZ - 0.80f},
             {dim::kDoorX + side * 0.95f, 2.95f, dim::kFrontZ - 0.05f}, 0.07f, 0.07f);
    }
    box(lampColor(), {dim::kDoorX, 3.37f, dim::kFrontZ - 0.55f}, {0.40f, 0.03f, 0.30f});

    // Door furniture: vision panel, louvre, kick plate, hinges, closer, lever.
    box(palette::kGlass, {dim::kDoorX - 0.30f, 2.20f, doorZ - 0.07f}, {0.34f, 0.55f, 0.03f});
    for (int i = 0; i < 5; ++i)
    {
        box(palette::kBlack, {dim::kDoorX, 0.95f + static_cast<float>(i) * 0.10f, doorZ - 0.07f},
            {0.70f, 0.03f, 0.03f});
    }
    box(palette::kSteel, {dim::kDoorX, 0.55f, doorZ - 0.07f}, {1.35f, 0.30f, 0.03f});
    for (float hy : {0.80f, 1.80f, 2.80f})
    {
        box(palette::kDarkSteel, {dim::kDoorX - 0.72f, hy, doorZ - 0.08f}, {0.05f, 0.14f, 0.07f});
    }
    box(palette::kDarkSteel, {dim::kDoorX + 0.35f, 2.98f, doorZ - 0.09f}, {0.40f, 0.10f, 0.08f});
    box(palette::kSteel, {dim::kDoorX + 0.50f, 1.55f, doorZ - 0.13f}, {0.14f, 0.04f, 0.06f});

    // Keypad / intercom beside the door.
    box(palette::kDarkSteel, {dim::kDoorX + 1.15f, 1.50f, dim::kFrontZ - 0.05f}, {0.16f, 0.26f, 0.06f});
    box(palette::kGlass, {dim::kDoorX + 1.15f, 1.56f, dim::kFrontZ - 0.09f}, {0.10f, 0.08f, 0.02f});
    box(palette::kRed, {dim::kDoorX + 1.15f, 1.44f, dim::kFrontZ - 0.09f}, {0.05f, 0.05f, 0.02f});

    // Fire extinguisher on a bracket.
    column(palette::kRed, {dim::kDoorX - 1.30f, 0.95f, dim::kFrontZ - 0.12f}, 0.07f, 0.07f, 0.34f, 10);
    column(palette::kBlack, {dim::kDoorX - 1.30f, 1.29f, dim::kFrontZ - 0.12f}, 0.025f, 0.025f, 0.07f, 6);
    box(palette::kDarkSteel, {dim::kDoorX - 1.30f, 1.10f, dim::kFrontZ - 0.04f}, {0.16f, 0.04f, 0.04f});

    // Wall-pack lights near the corners.
    for (float lx : {-3.9f, 3.9f})
    {
        box(palette::kDarkSteel, {lx, 3.4f, dim::kFrontZ - 0.12f}, {0.34f, 0.20f, 0.20f});
        box(lampColor(), {lx, 3.38f, dim::kFrontZ - 0.23f}, {0.28f, 0.14f, 0.02f});
        box(palette::kDarkSteel, {lx, 3.52f, dim::kFrontZ - 0.15f}, {0.36f, 0.03f, 0.26f});
    }

    // Electricity meter box with conduit run.
    box(palette::kGreyPaint, {3.7f, 1.5f, dim::kFrontZ - 0.07f}, {0.42f, 0.58f, 0.14f});
    column(palette::kWhite, {3.7f, 1.62f, dim::kFrontZ - 0.15f}, 0.12f, 0.12f, 0.03f, 14);
    box(palette::kGlass, {3.7f, 1.62f, dim::kFrontZ - 0.18f}, {0.18f, 0.18f, 0.01f});
    tube(palette::kDarkSteel, {3.6f, 1.80f, dim::kFrontZ - 0.04f}, {3.6f, wallTopY - 0.20f, dim::kFrontZ - 0.04f}, 0.03);
    tube(palette::kDarkSteel, {3.8f, 1.80f, dim::kFrontZ - 0.04f}, {3.8f, wallTopY - 0.20f, dim::kFrontZ - 0.04f}, 0.03);

    // Earth strap running down the corner to a rod.
    box(palette::kEarth, {-4.12f, 0.9f, dim::kFrontZ - 0.01f}, {0.04f, 1.1f, 0.015f});
    tube(palette::kCopper, {-4.12f, 0.36f, dim::kFrontZ - 0.01f}, {-4.12f, 0.0f, dim::kFrontZ - 0.01f}, 0.02, 5);

    // Wall panel joints and a horizontal trim band.
    for (float gx : {-1.15f, 1.15f, 3.35f, -4.0f})
    {
        box(shade(palette::kBuilding, 0.65f), {gx, dim::kSlabTopY + dim::kBuildingHeight * 0.5f, dim::kFrontZ - 0.012f},
            {0.04f, dim::kBuildingHeight, 0.03f});
    }
    box(palette::kTrim, {0.0f, wallTopY - 0.40f, dim::kFrontZ - 0.03f}, {dim::kBuildingWidth, 0.12f, 0.06f});
}

void drawBuildingSides(float wallTopY)
{
    const float hw = dim::kBuildingWidth * 0.5f;
    const float hd = dim::kBuildingDepth * 0.5f;

    // --- West wall: two louvres and air-conditioning condensers.
    drawLouvre(-hw, 2.2f, -1.2f, 1.3f, 1.0f, -1.0f);
    drawLouvre(-hw, 2.2f, 1.2f, 1.3f, 1.0f, -1.0f);
    for (float cz : {-1.2f, 1.2f})
    {
        box(palette::kConcrete, {-hw - 0.55f, 0.40f, cz}, {0.8f, 0.08f, 1.1f});
        box(palette::kGreyPaint, {-hw - 0.55f, 0.90f, cz}, {0.7f, 0.90f, 0.95f});
        column(palette::kBlack, {-hw - 0.55f, 1.35f, cz}, 0.28f, 0.28f, 0.03f, 14);
        box(palette::kDarkSteel, {-hw - 0.55f, 1.375f, cz}, {0.58f, 0.02f, 0.03f});
        box(palette::kDarkSteel, {-hw - 0.55f, 1.375f, cz}, {0.03f, 0.02f, 0.58f});
        tube(palette::kCopper, {-hw - 0.2f, 0.7f, cz - 0.3f}, {-hw - 0.02f, 0.7f, cz - 0.3f}, 0.025);
    }

    // --- East wall: roller shutter door and roof-access ladder with cage.
    const float doorZ = -0.3f;
    box(palette::kTrim, {hw + 0.03f, dim::kSlabTopY + 1.5f, doorZ}, {0.12f, 3.0f, 2.7f});
    for (int i = 0; i < 14; ++i)
    {
        const float y = dim::kSlabTopY + 0.12f + static_cast<float>(i) * 0.195f;
        box(shade(palette::kSteel, i % 2 == 0 ? 1.0f : 0.90f), {hw + 0.06f, y, doorZ}, {0.05f, 0.18f, 2.4f});
    }
    for (float side : {-1.0f, 1.0f})
    {
        box(palette::kDarkSteel, {hw + 0.08f, dim::kSlabTopY + 1.45f, doorZ + side * 1.28f}, {0.10f, 2.9f, 0.10f});
    }
    box(palette::kDarkSteel, {hw + 0.12f, dim::kSlabTopY + 3.0f, doorZ}, {0.30f, 0.36f, 2.7f});
    drawDangerSign({hw + 0.05f, 2.9f, 1.55f}, -90.0f, 0.42f);

    const float ladderX = hw + 0.14f;
    const float ladderZ = 2.0f;
    for (float side : {-1.0f, 1.0f})
    {
        tube(palette::kSteel, {ladderX, dim::kSlabTopY, ladderZ + side * 0.2f},
             {ladderX, 5.0f, ladderZ + side * 0.2f}, 0.025, 6);
    }
    for (float y = 0.8f; y < 4.9f; y += 0.3f)
    {
        box(palette::kSteel, {ladderX, y, ladderZ}, {0.03f, 0.03f, 0.40f});
    }
    for (float y : {1.0f, 2.2f, 3.4f, 4.6f})
    {
        box(palette::kDarkSteel, {ladderX - 0.07f, y, ladderZ - 0.2f}, {0.14f, 0.04f, 0.04f});
        box(palette::kDarkSteel, {ladderX - 0.07f, y, ladderZ + 0.2f}, {0.14f, 0.04f, 0.04f});
    }
    for (int i = 0; i < 5; ++i)
    {
        const float y = 2.4f + static_cast<float>(i) * 0.55f;
        box(palette::kDarkSteel, {ladderX + 0.30f, y, ladderZ - 0.30f}, {0.60f, 0.025f, 0.025f});
        box(palette::kDarkSteel, {ladderX + 0.30f, y, ladderZ + 0.30f}, {0.60f, 0.025f, 0.025f});
        box(palette::kDarkSteel, {ladderX + 0.60f, y, ladderZ}, {0.025f, 0.025f, 0.60f});
    }
    for (float dz : {-0.3f, 0.0f, 0.3f})
    {
        tube(palette::kDarkSteel, {ladderX + 0.60f, 2.4f, ladderZ + dz}, {ladderX + 0.60f, 5.0f, ladderZ + dz}, 0.012, 4);
    }

    // --- Rear wall: cable tray fed by the transformer LV cables, service door.
    const float rearZ = hd;
    box(palette::kSteel, {0.0f, dim::kCableTrayY, rearZ + 0.17f}, {8.2f, 0.05f, 0.34f});
    box(palette::kSteel, {0.0f, dim::kCableTrayY + 0.06f, rearZ + 0.33f}, {8.2f, 0.12f, 0.03f});
    box(palette::kSteel, {0.0f, dim::kCableTrayY + 0.06f, rearZ + 0.01f}, {8.2f, 0.12f, 0.03f});
    for (float dz : {0.08f, 0.17f, 0.26f})
    {
        tube(palette::kBlack, {-4.0f, dim::kCableTrayY + 0.06f, rearZ + dz},
             {4.0f, dim::kCableTrayY + 0.06f, rearZ + dz}, 0.03, 6);
    }
    for (float bx = -3.6f; bx <= 3.7f; bx += 1.2f)
    {
        box(palette::kDarkSteel, {bx, dim::kCableTrayY - 0.12f, rearZ + 0.17f}, {0.05f, 0.22f, 0.34f});
    }

    const float doorX = 3.2f;
    box(palette::kTrim, {doorX, dim::kSlabTopY + 1.3f, rearZ + 0.02f}, {1.3f, 2.6f, 0.08f});
    box(palette::kDoor, {doorX, dim::kSlabTopY + 1.25f, rearZ + 0.06f}, {1.05f, 2.5f, 0.06f});
    box(palette::kSteel, {doorX + 0.38f, 1.45f, rearZ + 0.11f}, {0.14f, 0.04f, 0.05f});
    box(palette::kConcrete, {doorX, dim::kSlabTopY + 0.05f, rearZ + 0.35f}, {1.4f, 0.10f, 0.6f});

    (void)wallTopY;
}

void drawBuilding()
{
    const float wallCenterY = dim::kSlabTopY + dim::kBuildingHeight * 0.5f;
    const float wallTopY    = dim::kSlabTopY + dim::kBuildingHeight;
    const float roofY       = wallTopY + dim::kRoofThickness * 0.5f;
    const float roofTopY    = wallTopY + dim::kRoofThickness;
    const float roofW       = dim::kBuildingWidth + 0.5f;
    const float roofD       = dim::kBuildingDepth + 0.5f;

    // Walls and darker concrete plinth around the base.
    box(palette::kBuilding, {0.0f, wallCenterY, 0.0f},
        {dim::kBuildingWidth, dim::kBuildingHeight, dim::kBuildingDepth});
    box(palette::kPlinth, {0.0f, dim::kSlabTopY + 0.25f, 0.0f},
        {dim::kBuildingWidth + 0.15f, 0.50f, dim::kBuildingDepth + 0.15f});

    // Corner pilasters give the walls some depth.
    for (float sx : {-1.0f, 1.0f})
    {
        for (float sz : {-1.0f, 1.0f})
        {
            box(palette::kConcrete,
                {sx * (dim::kBuildingWidth * 0.5f - 0.06f), wallCenterY,
                 sz * (dim::kBuildingDepth * 0.5f - 0.06f)},
                {0.40f, dim::kBuildingHeight, 0.40f});
        }
    }

    // Flat roof with a low parapet, coping and seams.
    box(palette::kRoof, {0.0f, roofY, 0.0f}, {roofW, dim::kRoofThickness, roofD});
    const float parapetY = roofTopY + 0.09f;
    for (float sz : {-1.0f, 1.0f})
    {
        box(palette::kTrim, {0.0f, parapetY, sz * (roofD * 0.5f - 0.06f)}, {roofW, 0.18f, 0.12f});
        box(shade(palette::kTrim, 1.06f), {0.0f, parapetY + 0.10f, sz * (roofD * 0.5f - 0.06f)},
            {roofW + 0.06f, 0.04f, 0.20f});
    }
    for (float sx : {-1.0f, 1.0f})
    {
        box(palette::kTrim, {sx * (roofW * 0.5f - 0.06f), parapetY, 0.0f}, {0.12f, 0.18f, roofD});
        box(shade(palette::kTrim, 1.06f), {sx * (roofW * 0.5f - 0.06f), parapetY + 0.10f, 0.0f},
            {0.20f, 0.04f, roofD + 0.06f});
    }
    for (float sx : {-3.5f, 3.5f})
    {
        box(palette::kBlack, {sx, roofTopY + 0.05f, -roofD * 0.5f + 0.04f}, {0.30f, 0.10f, 0.22f});
    }
    for (int i = -3; i <= 3; ++i)
    {
        box(shade(palette::kRoof, 1.30f), {0.0f, roofTopY + 0.006f, static_cast<float>(i) * 0.8f},
            {roofW - 0.6f, 0.012f, 0.05f});
    }

    // Entrance step on a solid block, lower step and walkway lead-in.
    box(palette::kConcrete, {dim::kDoorX, dim::kSlabTopY * 0.5f, dim::kFrontZ - 0.55f}, {1.9f, dim::kSlabTopY, 0.50f});
    box(palette::kConcrete, {dim::kDoorX, dim::kSlabTopY + 0.09f, dim::kFrontZ - 0.40f}, {1.9f, 0.18f, 0.60f});
    box(palette::kConcrete, {dim::kDoorX, 0.13f, dim::kFrontZ - 0.85f}, {1.9f, 0.26f, 0.30f});

    // Entrance: frame, leaf, lamp, sign.
    const float doorZ = dim::kFrontZ - 0.05f;
    const float doorH = 2.8f;
    const float doorY = dim::kSlabTopY + doorH * 0.5f;
    box(palette::kTrim, {dim::kDoorX, dim::kSlabTopY + doorH + 0.08f, doorZ - 0.02f},
        {1.75f, 0.16f, 0.10f});
    for (float side : {-1.0f, 1.0f})
    {
        box(palette::kTrim, {dim::kDoorX + side * 0.80f, doorY, doorZ - 0.02f}, {0.14f, doorH, 0.10f});
    }
    box(palette::kDoor, {dim::kDoorX, doorY, doorZ}, {1.45f, doorH, 0.12f});
    box(palette::kYellow, {dim::kDoorX, wallTopY - 0.55f, dim::kFrontZ - 0.10f}, {0.30f, 0.18f, 0.20f});
    drawWarningSign(dim::kDoorX, 3.95f, dim::kFrontZ - 0.03f);

    // Windows.
    drawWindow(0.0f, 2.65f, dim::kFrontZ - 0.03f);
    drawWindow(2.30f, 2.65f, dim::kFrontZ - 0.03f);

    drawBuildingFrontDetails(wallTopY);
    drawBuildingSides(wallTopY);
    drawRoofEquipment(roofTopY);
}

// Banded exhaust stack (smoke is drawn in drawDynamic).
void drawStack()
{
    const float roofTopY = dim::kSlabTopY + dim::kBuildingHeight + dim::kRoofThickness;
    constexpr float kX = 3.25f;
    constexpr float kZ = 1.55f;
    constexpr float kHeight = 1.9f;
    const float baseY = roofTopY + 0.12f;

    box(palette::kConcrete, {kX, roofTopY + 0.06f, kZ}, {0.85f, 0.12f, 0.85f});
    column(palette::kConcrete, {kX, baseY, kZ}, 0.30f, 0.22f, kHeight, 16);

    // Aviation-style red and white bands near the top.
    for (int i = 0; i < 4; ++i)
    {
        const float y = baseY + kHeight - 0.30f * static_cast<float>(i + 1);
        const float t = (y - baseY) / kHeight;
        const float r = 0.30f - 0.08f * t + 0.006f;
        column(i % 2 == 0 ? palette::kRed : palette::kWhite, {kX, y, kZ}, r, r - 0.012f, 0.30f, 16);
    }

    column(palette::kDarkSteel, {kX, baseY + kHeight, kZ}, 0.27f, 0.27f, 0.08f, 16);
    column(palette::kBlack, {kX, baseY + kHeight + 0.08f, kZ}, 0.19f, 0.19f, 0.02f, 16);

    // Maintenance ladder on the east side.
    for (float side : {-1.0f, 1.0f})
    {
        tube(palette::kSteel, {kX + 0.30f, baseY, kZ + side * 0.09f},
             {kX + 0.22f, baseY + kHeight, kZ + side * 0.09f}, 0.015, 5);
    }
    for (float y = baseY + 0.2f; y < baseY + kHeight; y += 0.28f)
    {
        box(palette::kSteel, {kX + 0.26f - (y - baseY) * 0.02f, y, kZ}, {0.03f, 0.025f, 0.18f});
    }
}

// ---------------------------------------------------------------------------
// Yard
// ---------------------------------------------------------------------------
void drawYard()
{
    const float w = dim::kYardHalfWidth;
    const float d = dim::kYardHalfDepth;

    // Gravel yard with scattered stones.
    box(palette::kGravel, {0.0f, 0.02f, 0.0f}, {w * 2.0f + 0.2f, 0.04f, d * 2.0f + 0.2f});

    Rng rng{1234u};
    for (int i = 0; i < 420; ++i)
    {
        const float x = rng.range(-w, w);
        const float z = rng.range(-d, d);
        if (std::fabs(x) < 5.1f && std::fabs(z) < 3.5f)
        {
            continue;
        }
        const float s = rng.range(0.05f, 0.13f);
        const float g = rng.range(0.78f, 1.25f);
        box(shade(palette::kGravel, g), {x, dim::kGravelTopY + s * 0.25f, z}, {s * 1.4f, s * 0.6f, s});
    }

    // Control-building slab with a yellow safety edge line.
    box(palette::kConcrete, {0.0f, dim::kSlabTopY * 0.5f, 0.0f}, {10.0f, dim::kSlabTopY, 6.8f});
    box(palette::kYellow, {0.0f, dim::kSlabTopY + 0.003f, -3.30f}, {10.0f, 0.006f, 0.07f});

    // Concrete walkway from the gate to the door, with control joints.
    box(palette::kConcrete, {dim::kDoorX, 0.07f, -4.55f}, {1.9f, 0.14f, 2.1f});
    for (float jz : {-4.1f, -4.7f, -5.3f})
    {
        box(palette::kPlinth, {dim::kDoorX, 0.0725f, jz}, {1.9f, 0.145f, 0.02f});
    }

    // Yellow and black bollards beside the entrance.
    for (float bx : {dim::kDoorX - 1.35f, dim::kDoorX + 1.35f})
    {
        column(palette::kYellow, {bx, 0.0f, -4.0f}, 0.09f, 0.09f, 0.95f, 10);
        column(palette::kBlack, {bx, 0.55f, -4.0f}, 0.095f, 0.095f, 0.12f, 10);
        column(palette::kDarkSteel, {bx, 0.95f, -4.0f}, 0.06f, 0.03f, 0.04f, 10);
    }

    // Manhole cover.
    column(palette::kDarkSteel, {2.2f, dim::kGravelTopY, -4.6f}, 0.40f, 0.40f, 0.03f, 16);
    box(palette::kBlack, {2.2f, 0.078f, -4.6f}, {0.60f, 0.006f, 0.03f});
    box(palette::kBlack, {2.2f, 0.078f, -4.6f}, {0.03f, 0.006f, 0.60f});
}

// ---------------------------------------------------------------------------
// Switchyard
// ---------------------------------------------------------------------------
void drawTransformer(float x, float z)
{
    const float pad = dim::kPadHeight;
    const float tankH = 0.95f;
    const float tankY = pad + 0.07f + tankH * 0.5f;
    const float lidTop = dim::kLidTopY;

    // Oil containment bund filled with gravel.
    box(palette::kBund, {x, 0.19f, z - 1.0f}, {2.4f, 0.30f, 0.12f});
    box(palette::kBund, {x, 0.19f, z + 1.0f}, {2.4f, 0.30f, 0.12f});
    box(palette::kBund, {x - 1.2f, 0.19f, z}, {0.12f, 0.30f, 2.0f});
    box(palette::kBund, {x + 1.2f, 0.19f, z}, {0.12f, 0.30f, 2.0f});
    box(shade(palette::kGravel, 0.7f), {x, 0.05f, z}, {2.28f, 0.06f, 1.88f});

    // Concrete plinth and steel skid.
    box(palette::kConcrete, {x, pad * 0.5f, z}, {1.9f, pad, 1.55f});
    box(palette::kDarkSteel, {x, pad + 0.035f, z - 0.4f}, {1.4f, 0.07f, 0.12f});
    box(palette::kDarkSteel, {x, pad + 0.035f, z + 0.4f}, {1.4f, 0.07f, 0.12f});

    // Main tank, lid flange and stiffening ribs.
    box(palette::kTransformer, {x, tankY, z}, {1.2f, tankH, 1.3f});
    box(shade(palette::kTransformer, 0.85f), {x, lidTop - 0.03f, z}, {1.28f, 0.06f, 1.38f});
    for (int i = 0; i < 6; ++i)
    {
        const float rx = x + (static_cast<float>(i) - 2.5f) * 0.20f;
        box(shade(palette::kTransformer, 0.82f), {rx, tankY, z - 0.665f}, {0.05f, tankH - 0.06f, 0.04f});
        box(shade(palette::kTransformer, 0.82f), {rx, tankY, z + 0.665f}, {0.05f, tankH - 0.06f, 0.04f});
    }
    for (float ry : {tankY - 0.25f, tankY + 0.25f})
    {
        box(shade(palette::kTransformer, 0.82f), {x, ry, z - 0.665f}, {1.2f, 0.04f, 0.04f});
        box(shade(palette::kTransformer, 0.82f), {x, ry, z + 0.665f}, {1.2f, 0.04f, 0.04f});
    }

    // Radiator banks with headers and fan housings on both sides.
    for (float side : {-1.0f, 1.0f})
    {
        const float fx = x + side * 0.90f;
        for (int fin = 0; fin < 6; ++fin)
        {
            box(palette::kTransformer, {fx, tankY, z + (static_cast<float>(fin) - 2.5f) * 0.22f},
                {0.10f, 0.72f, 0.20f});
        }
        tube(palette::kDarkSteel, {x + side * 0.6f, tankY + 0.33f, z}, {fx + side * 0.05f, tankY + 0.33f, z}, 0.04);
        tube(palette::kDarkSteel, {x + side * 0.6f, tankY - 0.33f, z}, {fx + side * 0.05f, tankY - 0.33f, z}, 0.04);
        tube(palette::kDarkSteel, {fx, tankY + 0.40f, z - 0.55f}, {fx, tankY + 0.40f, z + 0.55f}, 0.035);
        tube(palette::kDarkSteel, {fx, tankY - 0.40f, z - 0.55f}, {fx, tankY - 0.40f, z + 0.55f}, 0.035);
        for (float fz : {-0.3f, 0.3f})
        {
            column(palette::kBlack, {fx, pad + 0.02f, z + fz}, 0.12f, 0.12f, 0.06f, 12);
        }
    }

    // Conservator (oil expansion tank) with supports, breather and gauge.
    const float consX = x + 0.15f;
    const float consY = lidTop + 0.38f;
    tube(palette::kTransformer, {consX, consY, z - 0.55f}, {consX, consY, z + 0.55f}, 0.15, 14);
    for (float sz : {-0.35f, 0.35f})
    {
        box(palette::kDarkSteel, {consX, lidTop + 0.12f, z + sz}, {0.30f, 0.24f, 0.05f});
    }
    tube(palette::kDarkSteel, {consX, consY - 0.15f, z - 0.2f}, {consX, lidTop, z - 0.2f}, 0.035);
    box(palette::kDarkSteel, {consX, lidTop + 0.12f, z - 0.2f}, {0.12f, 0.10f, 0.10f});   // Buchholz relay
    tube(palette::kWhite, {consX, consY, z + 0.55f}, {consX, consY, z + 0.59f}, 0.085, 12);
    column(palette::kSilica, {consX, lidTop - 0.12f, z - 0.74f}, 0.065f, 0.065f, 0.24f, 10);
    tube(palette::kDarkSteel, {consX, consY - 0.05f, z - 0.55f}, {consX, lidTop + 0.10f, z - 0.74f}, 0.015, 5);

    // HV bushings: three in a row, with sheds and terminal caps.
    for (int k = -1; k <= 1; ++k)
    {
        const float bz = z + static_cast<float>(k) * dim::kPhaseSpacing;
        insulator(palette::kPorcelain, {x + dim::kHvOffsetX, lidTop, bz}, 0.055f, 0.12f, 7, 0.13f);
        column(palette::kCopper, {x + dim::kHvOffsetX, lidTop + 1.04f, bz}, 0.05f, 0.04f, 0.10f, 8);
        column(palette::kDarkSteel, {x + dim::kHvOffsetX, lidTop - 0.02f, bz}, 0.12f, 0.12f, 0.06f, 10);
    }

    // LV bushings (shorter) and the cable boxes behind them.
    for (int k = 0; k < 3; ++k)
    {
        const float bx = x - 0.45f + static_cast<float>(k) * 0.18f;
        insulator(palette::kPorcelain, {bx, lidTop, z - 0.42f}, 0.04f, 0.075f, 3, 0.10f);
        column(palette::kCopper, {bx, lidTop + 0.40f, z - 0.42f}, 0.035f, 0.03f, 0.04f, 8);
        tube(palette::kBlack, {bx, dim::kLvTerminalY - 0.02f, z - 0.42f},
             {bx, dim::kCableTrayY + 0.06f, dim::kBuildingDepth * 0.5f + 0.17f}, 0.03, 6);
    }

    // Tap-changer cabinet and warning sign on the front.
    box(palette::kGreyPaint, {x, tankY, z - 0.74f}, {0.55f, 0.55f, 0.18f});
    box(palette::kDarkSteel, {x, tankY, z - 0.835f}, {0.02f, 0.50f, 0.02f});
    box(palette::kWhite, {x - 0.14f, tankY + 0.10f, z - 0.84f}, {0.12f, 0.12f, 0.01f});
    drawWarningSign(x + 0.18f, tankY - 0.12f, z - 0.85f, 0.18f);

    // Lifting lugs and earth strap.
    for (float lx : {-0.55f, 0.55f})
    {
        for (float lz : {-0.60f, 0.60f})
        {
            box(palette::kDarkSteel, {x + lx, lidTop + 0.04f, z + lz}, {0.10f, 0.08f, 0.06f});
        }
    }
    box(palette::kEarth, {x - 0.62f, pad + 0.35f, z - 0.55f}, {0.03f, 0.70f, 0.015f});
}

// Concrete fire walls between the transformer bays.
void drawFireWalls()
{
    for (float wx : {-1.6f, 1.6f})
    {
        box(palette::kConcrete, {wx, 1.24f, dim::kTransformerZ}, {0.14f, 2.40f, 2.20f});
        box(palette::kTrim, {wx, 2.46f, dim::kTransformerZ}, {0.20f, 0.06f, 2.26f});
        for (int i = 0; i < 4; ++i)
        {
            box(shade(palette::kConcrete, 0.8f), {wx, 0.5f + static_cast<float>(i) * 0.55f, dim::kTransformerZ},
                {0.16f, 0.02f, 2.20f});
        }
    }
}

// Steel portal with a lattice beam, cross-arms and hanging insulator strings.
// `siteX` are the three transformer connection points.
void drawGantry(const float (&siteX)[3])
{
    const float z = dim::kTransformerZ;
    const float h = dim::kGantryHeight;
    const float half = dim::kGantryHalfSpan;
    const float botY = h - 0.35f;
    const float armY = botY - 0.07f;

    // Lattice towers.
    for (float side : {-1.0f, 1.0f})
    {
        const float tx = side * half;
        box(palette::kConcrete, {tx, 0.12f, z}, {1.0f, 0.24f, 1.1f});

        for (float dz : {-0.42f, 0.42f})
        {
            column(palette::kSteel, {tx, 0.2f, z + dz}, 0.055f, 0.045f, h - 0.1f, 8);
            box(palette::kDarkSteel, {tx, 0.22f, z + dz}, {0.20f, 0.04f, 0.20f});
        }

        constexpr int kBays = 7;
        const float bayH = (h - 0.3f) / kBays;
        for (int i = 0; i < kBays; ++i)
        {
            const float y0 = 0.3f + static_cast<float>(i) * bayH;
            const float y1 = y0 + bayH;
            const float s = (i % 2 == 0) ? 1.0f : -1.0f;
            beam(palette::kSteel, {tx, y0, z - 0.42f * s}, {tx, y1, z + 0.42f * s}, 0.04f, 0.04f);
            box(palette::kSteel, {tx, y1, z}, {0.04f, 0.04f, 0.84f});
        }
        box(palette::kDarkSteel, {tx, h + 0.04f, z}, {0.18f, 0.06f, 1.0f});
    }

    // Lattice top beam: two parallel planar trusses tied together.
    constexpr int kBeamBays = 14;
    const float dx = (half * 2.0f) / kBeamBays;
    for (float zo : {-0.22f, 0.22f})
    {
        beam(palette::kSteel, {-half, h, z + zo}, {half, h, z + zo}, 0.07f, 0.07f);
        beam(palette::kSteel, {-half, botY, z + zo}, {half, botY, z + zo}, 0.07f, 0.07f);

        for (int i = 0; i <= kBeamBays; ++i)
        {
            const float bx = -half + static_cast<float>(i) * dx;
            beam(palette::kSteel, {bx, botY, z + zo}, {bx, h, z + zo}, 0.04f, 0.04f);

            if (i < kBeamBays)
            {
                const float s = (i % 2 == 0) ? 1.0f : -1.0f;
                beam(palette::kSteel, {bx, s > 0 ? botY : h, z + zo},
                     {bx + dx, s > 0 ? h : botY, z + zo}, 0.035f, 0.035f);
            }
        }
    }
    for (int i = 0; i <= kBeamBays; i += 2)
    {
        const float bx = -half + static_cast<float>(i) * dx;
        beam(palette::kSteel, {bx, h, z - 0.22f}, {bx, h, z + 0.22f}, 0.04f, 0.04f);
        beam(palette::kSteel, {bx, botY, z - 0.22f}, {bx, botY, z + 0.22f}, 0.04f, 0.04f);
    }

    // Connection sites: two end sites on the towers plus one per transformer.
    const float sites[5] = {-half, siteX[0], siteX[1], siteX[2], half};
    for (float sx : sites)
    {
        box(palette::kDarkSteel, {sx, armY, z}, {0.14f, 0.10f, 1.30f});

        for (int k = -1; k <= 1; ++k)
        {
            const float sz = z + static_cast<float>(k) * dim::kPhaseSpacing;
            box(palette::kSteel, {sx, armY - 0.07f, sz}, {0.06f, 0.10f, 0.06f});
            insulator(palette::kPorcelain, {sx, dim::kStringBottomY + 0.03f, sz}, 0.04f, 0.10f, 6, 0.075f);
            box(palette::kAluminium, {sx, dim::kStringBottomY, sz}, {0.10f, 0.08f, 0.10f});
        }
    }

    // Phase conductors between the sites.
    for (int k = -1; k <= 1; ++k)
    {
        const float sz = z + static_cast<float>(k) * dim::kPhaseSpacing;
        for (int i = 0; i < 4; ++i)
        {
            drawWire({sites[i], dim::kStringBottomY, sz}, {sites[i + 1], dim::kStringBottomY, sz}, 0.12f);
        }

        // Vertical jumpers down to each transformer's HV bushing.
        for (int t = 0; t < 3; ++t)
        {
            tube(palette::kAluminium, {siteX[t], dim::kStringBottomY - 0.04f, sz},
                 {siteX[t], dim::kHvTerminalY, sz}, 0.022, 6);
            box(palette::kAluminium, {siteX[t], dim::kHvTerminalY, sz}, {0.10f, 0.06f, 0.10f});
        }
    }
}

// ---------------------------------------------------------------------------
// East switchyard bay: instrument transformers, surge arresters, rigid bus
// ---------------------------------------------------------------------------
void drawSupportStand(float x, float z, float h)
{
    for (float sx : {-1.0f, 1.0f})
    {
        for (float sz : {-1.0f, 1.0f})
        {
            box(palette::kConcrete, {x + sx * 0.25f, 0.07f, z + sz * 0.25f}, {0.16f, 0.14f, 0.16f});
            column(palette::kSteel, {x + sx * 0.25f, 0.14f, z + sz * 0.25f}, 0.035f, 0.03f, h - 0.14f, 6);
        }
    }

    for (float side : {-1.0f, 1.0f})
    {
        beam(palette::kSteel, {x + side * 0.25f, 0.2f, z - 0.25f}, {x + side * 0.25f, h - 0.1f, z + 0.25f}, 0.025f, 0.025f);
        beam(palette::kSteel, {x - 0.25f, 0.2f, z + side * 0.25f}, {x + 0.25f, h - 0.1f, z + side * 0.25f}, 0.025f, 0.025f);
    }
    box(palette::kDarkSteel, {x, h + 0.025f, z}, {0.62f, 0.05f, 0.62f});
}

void drawCurrentTransformer(float x, float z)
{
    drawSupportStand(x, z, 0.90f);

    insulator(palette::kPorcelain, {x, 0.95f, z}, 0.09f, 0.17f, 6, 0.10f);
    column(palette::kAluminium, {x, 1.65f, z}, 0.20f, 0.20f, 0.26f, 14);
    column(palette::kAluminium, {x, 1.91f, z}, 0.20f, 0.10f, 0.10f, 14);
    box(palette::kCopper, {x - 0.25f, 1.78f, z}, {0.14f, 0.06f, 0.06f});
    box(palette::kCopper, {x + 0.25f, 1.78f, z}, {0.14f, 0.06f, 0.06f});
    box(palette::kGreyPaint, {x, 0.95f, z - 0.22f}, {0.14f, 0.10f, 0.05f});   // junction box
    tube(palette::kEarth, {x + 0.25f, 0.9f, z + 0.25f}, {x + 0.25f, 0.0f, z + 0.38f}, 0.012, 4);
}

void drawVoltageTransformer(float x, float z)
{
    drawSupportStand(x, z, 0.90f);

    box(palette::kGreyPaint, {x, 1.17f, z}, {0.42f, 0.44f, 0.42f});
    box(palette::kDarkSteel, {x, 1.17f, z - 0.215f}, {0.02f, 0.40f, 0.01f});
    insulator(palette::kArrester, {x, 1.39f, z}, 0.08f, 0.14f, 6, 0.09f);
    insulator(palette::kArrester, {x, 1.39f + 0.63f, z}, 0.08f, 0.14f, 6, 0.09f);
    column(palette::kAluminium, {x, 2.02f + 0.0f, z}, 0.14f, 0.14f, 0.08f, 12);
    ring(palette::kAluminium, {x, 2.10f, z}, 0.22f, 0.012f, 12);
    box(palette::kCopper, {x, 2.16f, z}, {0.10f, 0.05f, 0.10f});
}

void drawSurgeArrester(float x, float z)
{
    drawSupportStand(x, z, 0.60f);

    insulator(palette::kArrester, {x, 0.65f, z}, 0.07f, 0.13f, 8, 0.09f);
    ring(palette::kAluminium, {x, 1.44f, z}, 0.19f, 0.012f, 12);
    box(palette::kCopper, {x, 1.50f, z}, {0.08f, 0.05f, 0.08f});
    box(palette::kGreyPaint, {x + 0.30f, 0.45f, z}, {0.14f, 0.18f, 0.10f});    // discharge counter
    tube(palette::kEarth, {x - 0.25f, 0.60f, z - 0.25f}, {x - 0.25f, 0.0f, z - 0.40f}, 0.012, 4);
}

void drawBay()
{
    const float x = dim::kBayX;
    constexpr float kBusY = 2.9f;

    drawCurrentTransformer(x, -3.4f);
    drawVoltageTransformer(x, -1.8f);
    for (float az : {0.2f, 0.8f, 1.4f})
    {
        drawSurgeArrester(x, az);
    }

    // Rigid aluminium bus and risers to each device.
    tube(palette::kAluminium, {x, kBusY, -3.4f}, {x, kBusY, 1.4f}, 0.045, 10);
    tube(palette::kAluminium, {x, 1.9f, -3.4f}, {x, kBusY, -3.4f}, 0.03, 6);
    tube(palette::kAluminium, {x, 2.16f, -1.8f}, {x, kBusY, -1.8f}, 0.03, 6);
    for (float az : {0.2f, 0.8f, 1.4f})
    {
        tube(palette::kAluminium, {x, 1.50f, az}, {x, kBusY, az}, 0.03, 6);
        box(palette::kAluminium, {x, kBusY, az}, {0.12f, 0.08f, 0.12f});
    }
    box(palette::kAluminium, {x, kBusY, -3.4f}, {0.12f, 0.08f, 0.12f});
    box(palette::kAluminium, {x, kBusY, -1.8f}, {0.12f, 0.08f, 0.12f});

    // Feeder to the gantry's east tower.
    drawWire({x, kBusY, 1.4f}, {dim::kGantryHalfSpan, dim::kStringBottomY, dim::kTransformerZ}, 0.30f);
}

// ---------------------------------------------------------------------------
// Yard furniture: floodlight masts, generator, drums, kiosk
// ---------------------------------------------------------------------------
void drawFloodlightMast(float x, float z)
{
    box(palette::kConcrete, {x, 0.12f, z}, {0.55f, 0.24f, 0.55f});
    column(palette::kSteel, {x, 0.2f, z}, 0.14f, 0.07f, 6.8f, 10);
    for (int i = 0; i < 4; ++i)
    {
        const float a = static_cast<float>(i) * 0.5f * dim::kPi;
        box(palette::kDarkSteel, {x + 0.2f * std::cos(a), 0.26f, z + 0.2f * std::sin(a)}, {0.06f, 0.05f, 0.06f});
    }
    box(palette::kDarkSteel, {x + 0.16f, 1.2f, z}, {0.04f, 0.5f, 0.12f});   // cable box

    const float yaw = radToDeg(std::atan2(-x, -z));
    glPushMatrix();
    glTranslatef(x, 7.0f, z);
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    box(palette::kDarkSteel, {0.0f, 0.0f, 0.0f}, {1.30f, 0.08f, 0.10f});
    for (int i = -1; i <= 1; ++i)
    {
        glPushMatrix();
        glTranslatef(static_cast<float>(i) * 0.42f, 0.05f, 0.06f);
        glRotatef(28.0f, 1.0f, 0.0f, 0.0f);
        box(palette::kDarkSteel, {0.0f, 0.0f, 0.0f}, {0.34f, 0.20f, 0.28f});
        box(lampColor(), {0.0f, -0.02f, 0.15f}, {0.30f, 0.16f, 0.02f});
        box(palette::kDarkSteel, {0.0f, 0.11f, 0.02f}, {0.38f, 0.03f, 0.34f});
        glPopMatrix();
    }
    glPopMatrix();
}

void drawGenerator(float x, float z)
{
    // Steel skid with fuel tank, canopy, louvres and exhaust.
    box(palette::kBlack, {x, 0.14f, z}, {1.35f, 0.20f, 2.35f});
    box(palette::kGenerator, {x, 0.80f, z}, {1.20f, 1.10f, 2.10f});
    box(shade(palette::kGenerator, 0.85f), {x, 1.37f, z}, {1.26f, 0.05f, 2.16f});

    for (int i = 0; i < 6; ++i)
    {
        box(palette::kBlack, {x + 0.61f, 0.45f + static_cast<float>(i) * 0.10f, z + 0.45f}, {0.03f, 0.05f, 0.80f});
    }
    box(palette::kGreyPaint, {x + 0.61f, 0.95f, z - 0.55f}, {0.03f, 0.55f, 0.55f});
    box(palette::kGlass, {x + 0.63f, 1.08f, z - 0.55f}, {0.02f, 0.15f, 0.30f});
    box(palette::kRed, {x + 0.63f, 0.80f, z - 0.55f}, {0.02f, 0.08f, 0.08f});

    column(palette::kDarkSteel, {x - 0.30f, 1.40f, z + 0.70f}, 0.12f, 0.12f, 0.40f, 12);
    tube(palette::kDarkSteel, {x - 0.30f, 1.80f, z + 0.70f}, {x - 0.30f, 2.50f, z + 0.70f}, 0.06, 10);
    column(palette::kBlack, {x - 0.30f, 2.50f, z + 0.70f}, 0.075f, 0.075f, 0.04f, 10);
    drawDangerSign({x + 0.63f, 1.15f, z + 0.90f}, -90.0f, 0.32f);
}

void drawDrum(float x, float z, const Color& colour)
{
    column(colour, {x, 0.0f, z}, 0.26f, 0.26f, 0.75f, 14);
    for (float y : {0.08f, 0.37f, 0.66f})
    {
        column(shade(colour, 0.80f), {x, y, z}, 0.27f, 0.27f, 0.03f, 14);
    }
    column(palette::kDarkSteel, {x, 0.75f, z}, 0.20f, 0.20f, 0.015f, 12);
    column(palette::kBlack, {x + 0.08f, 0.765f, z}, 0.035f, 0.035f, 0.012f, 8);
}

void drawStorageCorner()
{
    box(palette::kWood, {-5.95f, 0.10f, -2.75f}, {1.20f, 0.12f, 1.20f});
    drawDrum(-6.20f, -3.05f, palette::kDrumRed);
    drawDrum(-5.70f, -3.05f, palette::kDrumBlue);
    drawDrum(-5.95f, -2.50f, palette::kDrumRed);

    // Spill-kit bin.
    box(palette::kYellow, {-5.9f, 0.35f, -1.3f}, {0.60f, 0.70f, 0.45f});
    box(palette::kBlack, {-5.9f, 0.72f, -1.3f}, {0.64f, 0.05f, 0.49f});
}

void drawKiosk(float x, float z)
{
    box(palette::kConcrete, {x, 0.14f, z}, {1.10f, 0.20f, 0.75f});
    box(palette::kGreyPaint, {x, 0.95f, z}, {0.95f, 1.55f, 0.60f});
    box(palette::kRoof, {x, 1.78f, z}, {1.10f, 0.08f, 0.76f});
    box(palette::kDarkSteel, {x, 0.95f, z - 0.305f}, {0.02f, 1.45f, 0.02f});
    box(palette::kSteel, {x - 0.08f, 0.95f, z - 0.32f}, {0.03f, 0.18f, 0.03f});
    box(palette::kSteel, {x + 0.08f, 0.95f, z - 0.32f}, {0.03f, 0.18f, 0.03f});
    for (int i = 0; i < 4; ++i)
    {
        box(palette::kBlack, {x + 0.24f, 1.30f + static_cast<float>(i) * 0.07f, z - 0.31f}, {0.18f, 0.03f, 0.02f});
    }
    drawWarningSign(x - 0.25f, 1.45f, z - 0.31f, 0.20f);
    tube(palette::kEarth, {x - 0.45f, 0.3f, z - 0.2f}, {x - 0.45f, 0.0f, z - 0.4f}, 0.012, 4);
}

// ---------------------------------------------------------------------------
// Fence
// ---------------------------------------------------------------------------
// Chain-link fence along one axis-aligned run. (ox, oz) points to the outside
// and carries the outward-leaning barbed-wire arms.
void drawFenceRun(float x0, float z0, float x1, float z1, float ox, float oz)
{
    const float dx = x1 - x0;
    const float dz = z1 - z0;
    const float length = std::fabs(dx) + std::fabs(dz);
    const float ux = dx / length;
    const float uz = dz / length;

    constexpr float kRailY0 = 0.12f;
    const float railY1 = dim::kFenceHeight - 0.05f;

    // Posts with concrete footings, caps and barbed-wire arms.
    const int bays = std::max(1, static_cast<int>(std::ceil(length / dim::kPostSpacing)));
    for (int i = 0; i <= bays; ++i)
    {
        const float d = length * static_cast<float>(i) / bays;
        const float px = x0 + ux * d;
        const float pz = z0 + uz * d;

        box(palette::kConcrete, {px, 0.05f, pz}, {0.22f, 0.10f, 0.22f});
        column(palette::kSteel, {px, 0.0f, pz}, 0.07f, 0.065f, dim::kFenceHeight, 8);
        box(palette::kDarkSteel, {px, dim::kFenceHeight + 0.02f, pz}, {0.10f, 0.05f, 0.10f});
        beam(palette::kDarkSteel, {px, dim::kFenceHeight, pz},
             {px + ox * 0.32f, dim::kFenceHeight + 0.32f, pz + oz * 0.32f}, 0.04f, 0.04f);
    }

    // Top and bottom rails.
    const Vec3 mid{(x0 + x1) * 0.5f, 0.0f, (z0 + z1) * 0.5f};
    const Vec3 railSize{std::fabs(dx) + 0.06f, 0.06f, std::fabs(dz) + 0.06f};
    for (float y : {kRailY0, railY1})
    {
        box(palette::kSteel, {mid.x, y, mid.z}, railSize);
    }

    // Diamond mesh between the rails.
    constexpr float kCell = 0.50f;
    const int cells = std::max(1, static_cast<int>(length / kCell));
    const float step = length / cells;
    beginLines();
    setColor(shade(palette::kSteel, 0.85f));
    glBegin(GL_LINES);
    for (int i = 0; i < cells; ++i)
    {
        const float a = step * static_cast<float>(i);
        const float b = a + step;
        glVertex3f(x0 + ux * a, kRailY0, z0 + uz * a);
        glVertex3f(x0 + ux * b, railY1, z0 + uz * b);
        glVertex3f(x0 + ux * a, railY1, z0 + uz * a);
        glVertex3f(x0 + ux * b, kRailY0, z0 + uz * b);
    }
    glEnd();
    endLines();

    // Three barbed-wire strands running along the outward-leaning arms.
    for (int k = 0; k < 3; ++k)
    {
        const float t = static_cast<float>(k) * 0.5f;
        const float y = dim::kFenceHeight + 0.32f * t;
        const float off = 0.32f * t;
        line(palette::kDarkSteel, {x0 + ox * off, y, z0 + oz * off}, {x1 + ox * off, y, z1 + oz * off});
    }
}

// One swing-gate leaf. `dirSign` = +1 extends toward +X from the hinge, -1 toward -X.
void drawGateLeaf(float hingeX, float hingeZ, float dirSign, float openDeg)
{
    constexpr float kW = 1.08f;
    constexpr float kTop = 2.0f;
    constexpr float kBottom = 0.25f;

    glPushMatrix();
    glTranslatef(hingeX, 0.0f, hingeZ);
    glRotatef(openDeg * dirSign, 0.0f, 1.0f, 0.0f);

    const float u = dirSign;
    box(palette::kDarkSteel, {u * kW * 0.5f, kTop, 0.0f}, {kW, 0.07f, 0.07f});
    box(palette::kDarkSteel, {u * kW * 0.5f, kBottom, 0.0f}, {kW, 0.07f, 0.07f});
    box(palette::kDarkSteel, {u * kW, (kTop + kBottom) * 0.5f, 0.0f}, {0.07f, kTop - kBottom, 0.07f});
    beam(palette::kDarkSteel, {u * 0.06f, kBottom, 0.0f}, {u * kW, kTop, 0.0f}, 0.04f, 0.04f);

    beginLines();
    setColor(shade(palette::kSteel, 0.85f));
    glBegin(GL_LINES);
    constexpr int kCells = 6;
    for (int i = 0; i < kCells; ++i)
    {
        const float a = u * kW * static_cast<float>(i) / kCells;
        const float b = u * kW * static_cast<float>(i + 1) / kCells;
        glVertex3f(a, kBottom, 0.0f);
        glVertex3f(b, kTop, 0.0f);
        glVertex3f(a, kTop, 0.0f);
        glVertex3f(b, kBottom, 0.0f);
    }
    glEnd();
    endLines();

    box(palette::kSteel, {u * kW, 1.10f, 0.0f}, {0.10f, 0.05f, 0.10f});   // latch
    box(palette::kYellow, {u * kW, 0.95f, 0.0f}, {0.08f, 0.10f, 0.06f});  // padlock
    glPopMatrix();
}

void drawFence()
{
    const float w = dim::kYardHalfWidth;
    const float d = dim::kYardHalfDepth;
    const float gateL = dim::kGateCenterX - dim::kGateHalfGap;
    const float gateR = dim::kGateCenterX + dim::kGateHalfGap;

    drawFenceRun(-w, d, w, d, 0.0f, 1.0f);          // rear
    drawFenceRun(-w, -d, -w, d, -1.0f, 0.0f);       // left
    drawFenceRun(w, -d, w, d, 1.0f, 0.0f);          // right
    drawFenceRun(-w, -d, gateL, -d, 0.0f, -1.0f);   // front, left of the gate
    drawFenceRun(gateR, -d, w, -d, 0.0f, -1.0f);    // front, right of the gate

    // Heavier gate posts, open gate leaves and signage.
    for (float gx : {gateL, gateR})
    {
        box(palette::kConcrete, {gx, 0.08f, -d}, {0.34f, 0.16f, 0.34f});
        column(palette::kDarkSteel, {gx, 0.0f, -d}, 0.12f, 0.11f, dim::kFenceHeight + 0.2f, 10);
        box(palette::kDarkSteel, {gx, dim::kFenceHeight + 0.22f, -d}, {0.20f, 0.06f, 0.20f});
    }
    drawGateLeaf(gateL + 0.05f, -d, 1.0f, 72.0f);
    drawGateLeaf(gateR - 0.05f, -d, -1.0f, 72.0f);

    drawWarningSign(gateR + 1.1f, 1.45f, -d - 0.08f);
    drawDangerSign({gateL - 1.1f, 1.45f, -d - 0.08f}, 0.0f, 0.50f);
    drawDangerSign({w + 0.08f, 1.45f, -1.5f}, -90.0f, 0.45f);
    drawDangerSign({-w - 0.08f, 1.45f, -1.5f}, 90.0f, 0.45f);
    drawDangerSign({2.0f, 1.45f, d + 0.08f}, 180.0f, 0.45f);
}


// Street-lamp style luminaire on one end of the crossarm. `side` = -1 or +1 (X direction).
// The lens is drawn unlit so it looks like it is glowing, even with lighting enabled.
void drawPoleLamp(float side)
{
    const float postX  = side * 1.42f;
    const float baseY  = dim::kCrossarmY + 0.07f;
    const float topY   = baseY + 0.60f;
    const float headX  = side * 2.00f;
    const float headY  = topY + 0.06f;

    // Mounting plate on the crossarm and a short vertical post
    box(palette::kDarkSteel, {postX, baseY, 0.0f}, {0.16f, 0.04f, 0.16f});
    column(palette::kDarkSteel, {postX, baseY, 0.0f}, 0.035f, 0.028f, 0.60f, 8);

    // Curved-looking arm: rising bracket to the head plus a diagonal brace
    tube(palette::kDarkSteel, {postX, topY, 0.0f}, {headX, headY, 0.0f}, 0.026f, 6);
    tube(palette::kDarkSteel, {postX, baseY + 0.30f, 0.0f}, {headX - side * 0.18f, headY - 0.02f, 0.0f}, 0.014f, 5);

    // Luminaire housing: shallow hood, body and a reflector rim
    box(palette::kGreyPaint, {headX, headY + 0.045f, 0.0f}, {0.52f, 0.06f, 0.26f});
    box(palette::kDarkSteel, {headX, headY, 0.0f},           {0.46f, 0.07f, 0.22f});
    box(palette::kDarkSteel, {headX, headY - 0.045f, 0.0f},  {0.50f, 0.02f, 0.26f});
    box(palette::kDarkSteel, {headX + side * 0.24f, headY, 0.0f}, {0.04f, 0.09f, 0.14f});   // end cap

    // Glowing lens (unlit) with a brighter bulb in the middle
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    box(lampColor(), {headX, headY - 0.062f, 0.0f}, {0.40f, 0.025f, 0.18f});
    box(gGridPowered ? Color{1.0f, 1.0f, 0.90f} : palette::kLampOff,
        {headX, headY - 0.070f, 0.0f}, {0.22f, 0.012f, 0.10f});
    glPopAttrib();
}

// ---------------------------------------------------------------------------
// Utility pole line
// ---------------------------------------------------------------------------
void drawPoleAt(float x, float z, bool detailed)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    // Tapered timber pole with a concrete collar and a capped top.
    column(palette::kWood, {0.0f, 0.0f, 0.0f}, 0.16f, 0.105f, dim::kPoleHeight, 12);
    column(palette::kConcrete, {0.0f, 0.0f, 0.0f}, 0.20f, 0.19f, 0.30f, 12);
    column(palette::kWood, {0.0f, dim::kPoleHeight, 0.0f}, 0.105f, 0.02f, 0.10f, 12);

    // Crossarm with steel band fittings.
    box(palette::kWood, {0.0f, dim::kCrossarmY, 0.0f}, {3.1f, 0.14f, 0.14f});
    column(palette::kDarkSteel, {0.0f, dim::kCrossarmY - 0.30f, 0.0f}, 0.125f, 0.125f, 0.05f, 10);
    column(palette::kDarkSteel, {0.0f, dim::kCrossarmY - 0.65f, 0.0f}, 0.13f, 0.13f, 0.05f, 10);

    // Diagonal braces under the crossarm.
    const float rise = 0.35f;
    const float run = 0.90f;
    const float braceLen = std::sqrt(rise * rise + run * run);
    const float braceDeg = radToDeg(std::atan2(rise, run));
    boxRotatedZ(palette::kWood, {-run * 0.5f, dim::kCrossarmY - rise * 0.5f, 0.0f},
                {braceLen, 0.08f, 0.08f}, -braceDeg);
    boxRotatedZ(palette::kWood, { run * 0.5f, dim::kCrossarmY - rise * 0.5f, 0.0f},
                {braceLen, 0.08f, 0.08f}, braceDeg);

    // Pin insulators for the three phases (top ends at kInsulatorTopY).
    for (float side : {-1.0f, 0.0f, 1.0f})
    {
        insulator(palette::kPorcelain, {side * 1.05f, dim::kCrossarmY + 0.07f, 0.0f}, 0.045f, 0.10f, 4, 0.05f);
        box(palette::kDarkSteel, {side * 1.05f, dim::kCrossarmY + 0.075f, 0.0f}, {0.12f, 0.02f, 0.12f});
    }

        // Street lamps on both ends of the crossarm.
    drawPoleLamp(-1.0f);
    drawPoleLamp( 1.0f);

    if (detailed)
    {
        // Climbing steps, pole tag and earth conductor.
        for (int i = 0; i < 10; ++i)
        {
            const float y = 2.4f + static_cast<float>(i) * 0.5f;
            const float s = (i % 2 == 0) ? 1.0f : -1.0f;
            box(palette::kSteel, {s * 0.17f, y, 0.0f}, {0.18f, 0.03f, 0.03f});
        }
        box(palette::kWhite, {0.0f, 2.6f, -0.15f}, {0.10f, 0.14f, 0.02f});
        tube(palette::kCopper, {0.0f, 0.1f, 0.17f}, {0.0f, 7.9f, 0.12f}, 0.012, 4);
    }

    glPopMatrix();
}

// Pole-mounted distribution transformer with drop-out fuses.
void drawPoleTransformer(float x, float z)
{
    const float cy = 6.35f;
    box(palette::kDarkSteel, {x + 0.22f, cy + 0.38f, z}, {0.32f, 0.06f, 0.50f});
    box(palette::kDarkSteel, {x + 0.22f, cy - 0.04f, z}, {0.32f, 0.06f, 0.50f});
    column(palette::kTransformer, {x + 0.52f, cy, z}, 0.26f, 0.26f, 0.72f, 14);
    column(shade(palette::kTransformer, 0.85f), {x + 0.52f, cy + 0.72f, z}, 0.26f, 0.12f, 0.08f, 14);
    for (float dz : {-0.12f, 0.12f})
    {
        insulator(palette::kPorcelain, {x + 0.52f, cy + 0.78f, z + dz}, 0.03f, 0.065f, 2, 0.05f);
    }
    for (float dz : {-0.30f, 0.30f})
    {
        tube(palette::kWhite, {x + 0.35f, dim::kCrossarmY - 0.55f, z + dz}, {x + 0.52f, dim::kCrossarmY - 0.85f, z + dz}, 0.03, 6);
    }
    for (float dz : {-0.15f, 0.15f})
    {
        drawWire({x + 0.52f, cy + 0.30f, z + dz}, {x + 1.2f, 3.5f, z + dz * 3.0f}, 0.4f);
    }
}

// Guy wire from the pole top to a ground anchor.
void drawGuy(float x, float z, float directionZ)
{
    const float ax = x;
    const float az = z + directionZ * 4.2f;

    drawWire({x, 7.2f, z}, {ax, 0.15f, az}, 0.06f);
    box(palette::kConcrete, {ax, 0.06f, az}, {0.40f, 0.12f, 0.40f});
    tube(palette::kSteel, {ax, 0.06f, az}, {ax, 0.65f, az - directionZ * 0.15f}, 0.025, 5);
    box(palette::kYellow, {ax, 0.9f, az - directionZ * 0.2f}, {0.10f, 1.4f, 0.04f});   // guy guard
}

void drawPole(float z)
{
    drawPoleAt(dim::kPoleX, z, true);
}

// ---------------------------------------------------------------------------
// Compose the cached geometry
// ---------------------------------------------------------------------------
void drawPoleLine()
{
    const float poleZ[3] = {-14.0f, 0.0f, 14.0f};
    for (float pz : poleZ)
    {
        drawPole(pz);
    }

    drawPoleTransformer(dim::kPoleX, poleZ[1]);
    drawGuy(dim::kPoleX, poleZ[0], -1.0f);
    drawGuy(dim::kPoleX, poleZ[2], 1.0f);

    for (float phase : {-1.0f, 0.0f, 1.0f})
    {
        const float px = dim::kPoleX + phase * 1.05f;
        drawWire({px, dim::kInsulatorTopY, poleZ[0]}, {px, dim::kInsulatorTopY, poleZ[1]}, 1.6f);
        drawWire({px, dim::kInsulatorTopY, poleZ[1]}, {px, dim::kInsulatorTopY, poleZ[2]}, 1.6f);
    }

    // Service drop from the centre pole to the gantry (one conductor per phase).
    for (int k = -1; k <= 1; ++k)
    {
        const float px = dim::kPoleX + static_cast<float>(k) * 1.05f;
        drawWire({px, dim::kInsulatorTopY, poleZ[1]},
                 {-dim::kGantryHalfSpan, dim::kStringBottomY,
                  dim::kTransformerZ + static_cast<float>(k) * dim::kPhaseSpacing},
                 1.6f);
    }
}

void drawSubstationStatic()
{
    drawYard();
    drawBuilding();
    drawStack();

    const float transformerX[3] = {-3.2f, 0.0f, 3.2f};
    float siteX[3];
    for (int i = 0; i < 3; ++i)
    {
        drawTransformer(transformerX[i], dim::kTransformerZ);
        siteX[i] = transformerX[i] + dim::kHvOffsetX;
    }
    drawFireWalls();
    drawGantry(siteX);

    drawBay();
    drawFloodlightMast(-5.9f, -4.7f);
    drawFloodlightMast(5.9f, -4.7f);
    drawGenerator(-5.95f, 1.2f);
    drawStorageCorner();
    drawKiosk(4.2f, -4.5f);

    drawFence();
    drawPoleLine();
}

void drawRoadUtilitiesStatic()
{
    constexpr float poleX[] = {-7.2f, 7.2f};
    constexpr float poleSpacing = 24.0f;
    constexpr int firstPole = -20;
    constexpr int lastPole = 20;

    for (float x : poleX)
    {
        for (int index = firstPole; index <= lastPole; ++index)
        {
            drawPoleAt(x, static_cast<float>(index) * poleSpacing, false);
        }

        for (float phase : {-1.0f, 0.0f, 1.0f})
        {
            const float wireX = x + phase * 1.05f;
            for (int index = firstPole; index < lastPole; ++index)
            {
                const float z0 = static_cast<float>(index) * poleSpacing;
                const float z1 = static_cast<float>(index + 1) * poleSpacing;
                drawWire({wireX, dim::kInsulatorTopY, z0}, {wireX, dim::kInsulatorTopY, z1}, 1.6f);
            }
        }
    }

    // A long service connection visually ties the right-hand road line to
    // the remote substation behind the farms.
    drawWire({7.2f, dim::kInsulatorTopY, 0.0f}, {70.0f, dim::kInsulatorTopY, 92.0f}, 4.0f);
}

// ---------------------------------------------------------------------------
// Animated parts (drawn every frame)
// ---------------------------------------------------------------------------
void drawRoofFan(float x, float y, float z, float time, float direction)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(time * direction * 600.0f, 0.0f, 1.0f, 0.0f);
    box(palette::kSteel, {0.0f, 0.0f, 0.0f}, {0.66f, 0.012f, 0.10f});
    box(palette::kSteel, {0.0f, 0.0f, 0.0f}, {0.10f, 0.012f, 0.66f});
    column(palette::kDarkSteel, {0.0f, -0.01f, 0.0f}, 0.06f, 0.06f, 0.04f, 8);
    glPopMatrix();
}

void drawBeacon(const Vec3& p, float time)
{
    const float phase = std::fmod(time, 1.5f);
    const float k = gGridPowered ? (phase < 0.22f ? 1.0f : 0.22f) : 0.05f;

    glColor3f(1.0f * k, 0.08f * k, 0.05f * k);
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    Primitives::drawSphere(0.10f, 10, 7);
    glPopMatrix();
}

void drawStackSmoke(float time)
{
    const float roofTopY = dim::kSlabTopY + dim::kBuildingHeight + dim::kRoofThickness;
    const Vec3 top{3.25f, roofTopY + 0.12f + 1.9f + 0.12f, 1.55f};

    const GLboolean blendWas = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Smoke puffs rise, widen, fade and loop; each is offset in the cycle.
    constexpr int kPuffs = 6;
    for (int i = 0; i < kPuffs; ++i)
    {
        const float raw = time * 0.12f + static_cast<float>(i) / kPuffs;
        const float life = raw - std::floor(raw);   // 0..1
        const float phase = time * 0.5f + static_cast<float>(i) * 1.7f;
        const float radius = 0.16f + 0.42f * life;
        const float alpha = 0.70f * (1.0f - life);

        glColor4f(palette::kSmoke.r, palette::kSmoke.g, palette::kSmoke.b, alpha);
        glPushMatrix();
        glTranslatef(top.x + std::sin(phase) * 0.30f * life + life * 0.35f,
                     top.y + life * 2.8f,
                     top.z + std::cos(phase) * 0.18f * life);
        glScalef(radius, radius, radius);
        Primitives::drawSphere(1.0f, 10, 7);
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    if (!blendWas)
    {
        glDisable(GL_BLEND);
    }
    glColor3f(1.0f, 1.0f, 1.0f);
}

void drawSubstationDynamic(float time)
{
    const float roofTopY = dim::kSlabTopY + dim::kBuildingHeight + dim::kRoofThickness;

    drawRoofFan(-2.5f, roofTopY + 0.80f, 1.0f, time, 1.0f);
    drawRoofFan(-0.6f, roofTopY + 0.80f, 1.0f, time, -1.0f);

    drawBeacon({0.9f, roofTopY + 2.50f, 1.9f}, time);
    drawBeacon({-5.9f, 7.25f, -4.7f}, time + 0.5f);

    drawStackSmoke(time);
}
}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
namespace PowerSubstation
{
void setPowerAmount(float amount)
{
    const bool powered = amount >= 0.5f;
    if (powered == gGridPowered)
        return;

    gGridPowered = powered;
    // Lamp colors are part of otherwise-static display lists. Rebuild each
    // list only at the on/off threshold, never on every transition frame.
    if (gStaticList != 0)
    {
        glDeleteLists(gStaticList, 1);
        gStaticList = 0;
    }
    if (gRoadList != 0)
    {
        glDeleteLists(gRoadList, 1);
        gRoadList = 0;
    }
}

void draw(float x, float z, float scale, float time)
{
    gLit = glIsEnabled(GL_LIGHTING) == GL_TRUE;

    // The static geometry is compiled once into a display list.
    if (gStaticList == 0)
    {
        gStaticList = glGenLists(1);
        glNewList(gStaticList, GL_COMPILE);
        drawSubstationStatic();
        glEndList();
    }

    // Scaled normals stay unit length while the substation is drawn.
    const GLboolean normalizeWas = glIsEnabled(GL_NORMALIZE);
    if (!normalizeWas)
    {
        glEnable(GL_NORMALIZE);
    }

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);

    glCallList(gStaticList);
    drawSubstationDynamic(time);

    glPopMatrix();

    if (!normalizeWas)
    {
        glDisable(GL_NORMALIZE);
    }
}

void drawRoadUtilities(float time)
{
    gLit = glIsEnabled(GL_LIGHTING) == GL_TRUE;

    if (gRoadList == 0)
    {
        gRoadList = glGenLists(1);
        glNewList(gRoadList, GL_COMPILE);
        drawRoadUtilitiesStatic();
        glEndList();
    }

    const GLboolean normalizeWas = glIsEnabled(GL_NORMALIZE);
    if (!normalizeWas)
    {
        glEnable(GL_NORMALIZE);
    }

    glPushMatrix();
    glCallList(gRoadList);
    glPopMatrix();

    if (!normalizeWas)
    {
        glDisable(GL_NORMALIZE);
    }

    (void)time;
}
}  // namespace PowerSubstation
