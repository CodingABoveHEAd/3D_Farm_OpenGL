#include "objects/Barn.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

namespace Barn {
namespace {

constexpr float Pi = 3.14159265358979323846f;

struct C { float r, g, b; };
constexpr C BarnRed    {0.72f, 0.12f, 0.10f};
constexpr C BarnRedDrk {0.50f, 0.08f, 0.07f};
constexpr C RoofRed    {0.58f, 0.14f, 0.10f};
constexpr C RoofDrk    {0.38f, 0.08f, 0.05f};
constexpr C TrimWhite  {0.92f, 0.92f, 0.88f};
constexpr C TrimBeige  {0.78f, 0.76f, 0.68f};
constexpr C DoorBrown  {0.38f, 0.22f, 0.12f};
constexpr C DoorDrk    {0.26f, 0.14f, 0.07f};
constexpr C WoodLoft   {0.55f, 0.36f, 0.18f};
constexpr C FoundStone {0.45f, 0.45f, 0.42f};
constexpr C WindowDrk  {0.12f, 0.14f, 0.18f};
constexpr C CupolaRed  {0.62f, 0.15f, 0.12f};

void col(float r, float g, float b) { glColor3f(r, g, b); }

void box(float r, float g, float b, float x, float y, float z, float sx, float sy, float sz)
{
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void cylY(float r, float g, float b, float x, float y, float z, float rad, float h, int seg = 10)
{
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCylinder(rad, h, seg);
    glPopMatrix();
}

// --- Gambrel roof helpers ---
// Lower pitch (steep): wall top -> bend line
// Upper pitch (shallow): bend line -> ridge
void gambrelRoof(float halfW, float wallTop, float bendY, float ridgeY, float length, int side)
{
    const float sx = side * 1.0f; // -1 left, +1 right

    // Lower panel
    {
        const float x0 = sx * halfW, y0 = wallTop;
        const float x1 = sx * halfW * 0.45f, y1 = bendY;
        const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
        const float dx = x1 - x0, dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        const float ang = std::atan2(dx, dy) * 180.0f / Pi;

        glPushMatrix();
        glTranslatef(cx, cy, 0.0f);
        glRotatef(ang, 0.0f, 0.0f, 1.0f);
        // slope panel
        box(RoofRed.r, RoofRed.g, RoofRed.b, 0, 0, 0, 0.28f, len, length + 0.3f);
        // shingle lines
        const int courses = 6;
        for (int i = 0; i < courses; ++i)
        {
            float yy = -len * 0.5f + len * (i + 0.5f) / courses;
            box(RoofDrk.r, RoofDrk.g, RoofDrk.b, 0, yy, 0.10f, 0.20f, 0.02f, length + 0.25f);
        }
        glPopMatrix();
    }
    // Upper panel
    {
        const float x0 = sx * halfW * 0.45f, y0 = bendY;
        const float x1 = 0.0f, y1 = ridgeY;
        const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
        const float dx = x1 - x0, dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        const float ang = std::atan2(dx, dy) * 180.0f / Pi;
        glPushMatrix();
        glTranslatef(cx, cy, 0.0f);
        glRotatef(ang, 0.0f, 0.0f, 1.0f);
        box(RoofRed.r + 0.04f, RoofRed.g + 0.04f, RoofRed.b + 0.04f, 0, 0, 0, 0.28f, len, length + 0.3f);
        const int courses = 4;
        for (int i = 0; i < courses; ++i)
        {
            float yy = -len * 0.5f + len * (i + 0.5f) / courses;
            box(RoofDrk.r, RoofDrk.g, RoofDrk.b, 0, yy, 0.10f, 0.20f, 0.02f, length + 0.25f);
        }
        glPopMatrix();
    }
}

} // anonymous

void drawBarn(float x, float z, float scale, float yaw)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    const float W = 7.0f;        // total width (X)
    const float D = 6.0f;        // depth (Z)
    const float wallH = 4.2f;    // wall height
    const float bendY = 6.0f;
    const float ridgeY = 8.0f;

    // --- Foundation ---
    box(FoundStone.r, FoundStone.g, FoundStone.b, 0, 0.30f, 0, W + 0.4f, 0.60f, D + 0.4f);
    // stone pattern line
    box(0.35f, 0.35f, 0.33f, 0, 0.12f, D * 0.5f + 0.02f, W + 0.2f, 0.04f, 0.04f);
    box(0.35f, 0.35f, 0.33f, 0, 0.12f, -D * 0.5f - 0.02f, W + 0.2f, 0.04f, 0.04f);

    // --- Front face (Z+) wood siding ---
    // Main wall
    box(BarnRed.r, BarnRed.g, BarnRed.b, 0, wallH * 0.5f + 0.6f, D * 0.5f, W, wallH, 0.20f);
    // Vertical batten strips
    for (float bx = -W * 0.5f + 0.9f; bx < W * 0.5f; bx += 1.15f)
        box(BarnRedDrk.r, BarnRedDrk.g, BarnRedDrk.b, bx, wallH * 0.5f + 0.6f, D * 0.5f + 0.08f, 0.14f, wallH, 0.04f);
    // White trim around front
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, -W * 0.5f, wallH * 0.5f + 0.6f, D * 0.5f + 0.06f, 0.22f, wallH + 0.1f, 0.08f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, W * 0.5f, wallH * 0.5f + 0.6f, D * 0.5f + 0.06f, 0.22f, wallH + 0.1f, 0.08f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, 0, wallH + 0.6f, D * 0.5f + 0.06f, W + 0.1f, 0.16f, 0.08f);

    // Double sliding doors (closed)
    box(DoorBrown.r, DoorBrown.g, DoorBrown.b, -1.25f, 1.85f, D * 0.5f + 0.12f, 2.35f, 3.0f, 0.08f);
    box(DoorBrown.r, DoorBrown.g, DoorBrown.b, 1.25f, 1.85f, D * 0.5f + 0.12f, 2.35f, 3.0f, 0.08f);
    // Cross braces on doors (X)
    for (float sx : {-1.25f, 1.25f})
    {
        box(DoorDrk.r, DoorDrk.g, DoorDrk.b, sx, 1.85f, D * 0.5f + 0.18f, 2.1f, 0.16f, 0.04f);
        glPushMatrix();
        glTranslatef(sx, 1.85f, D * 0.5f + 0.20f);
        glRotatef(38.0f, 0, 0, 1);
        box(DoorDrk.r, DoorDrk.g, DoorDrk.b, 0, 0, 0, 0.14f, 2.6f, 0.04f);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(sx, 1.85f, D * 0.5f + 0.20f);
        glRotatef(-38.0f, 0, 0, 1);
        box(DoorDrk.r, DoorDrk.g, DoorDrk.b, 0, 0, 0, 0.14f, 2.6f, 0.04f);
        glPopMatrix();
    }
    // Top rail for sliding doors
    box(TrimBeige.r, TrimBeige.g, TrimBeige.b, 0, 3.50f, D * 0.5f + 0.15f, W - 0.4f, 0.12f, 0.10f);
    // Small transom windows above doors
    box(WindowDrk.r, WindowDrk.g, WindowDrk.b, -1.25f, 3.92f, D * 0.5f + 0.08f, 1.6f, 0.55f, 0.04f);
    box(WindowDrk.r, WindowDrk.g, WindowDrk.b, 1.25f, 3.92f, D * 0.5f + 0.08f, 1.6f, 0.55f, 0.04f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, -1.25f, 3.92f, D * 0.5f + 0.10f, 1.85f, 0.75f, 0.06f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, 1.25f, 3.92f, D * 0.5f + 0.10f, 1.85f, 0.75f, 0.06f);
    // Mullions
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, -1.25f, 3.92f, D * 0.5f + 0.14f, 0.06f, 0.55f, 0.04f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, 1.25f, 3.92f, D * 0.5f + 0.14f, 0.06f, 0.55f, 0.04f);

    // Loft door near ridge (for hay)
    box(DoorBrown.r, DoorBrown.g, DoorBrown.b, 0, 6.35f, D * 0.5f + 0.10f, 1.45f, 1.55f, 0.08f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, 0, 6.35f, D * 0.5f + 0.12f, 1.65f, 1.75f, 0.06f);
    // Circular window in gable
    cylY(TrimWhite.r, TrimWhite.g, TrimWhite.b, 0, 6.45f, D * 0.5f + 0.14f, 0.45f, 0.08f, 12);
    cylY(WindowDrk.r, WindowDrk.g, WindowDrk.b, 0, 6.45f, D * 0.5f + 0.16f, 0.36f, 0.06f, 12);
    // Window cross
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, 0, 6.45f, D * 0.5f + 0.18f, 0.06f, 0.72f, 0.04f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, 0, 6.45f, D * 0.5f + 0.18f, 0.72f, 0.06f, 0.04f);

    // Hayloft pulley arm
    box(WoodLoft.r, WoodLoft.g, WoodLoft.b, 0, 6.95f, D * 0.5f + 0.65f, 0.18f, 0.18f, 1.05f);
    // pulley
    cylY(0.25f, 0.25f, 0.24f, 0, 6.80f, D * 0.5f + 1.10f, 0.14f, 0.08f, 10);

    // --- Side walls ---
    for (float side : {-1.0f, 1.0f})
    {
        float sx = side * W * 0.5f;
        box(BarnRed.r, BarnRed.g, BarnRed.b, sx, wallH * 0.5f + 0.6f, 0, 0.20f, wallH, D);
        for (float bz = -D * 0.5f + 1.0f; bz < D * 0.5f; bz += 1.2f)
            box(BarnRedDrk.r, BarnRedDrk.g, BarnRedDrk.b, sx + side * 0.06f, wallH * 0.5f + 0.6f, bz, 0.04f, wallH, 0.14f);
        // Side window
        box(TrimWhite.r, TrimWhite.g, TrimWhite.b, sx + side * 0.10f, 2.10f, 0.0f, 0.06f, 1.1f, 1.4f);
        box(WindowDrk.r, WindowDrk.g, WindowDrk.b, sx + side * 0.12f, 2.10f, 0.0f, 0.04f, 0.90f, 1.15f);
        box(TrimWhite.r, TrimWhite.g, TrimWhite.b, sx + side * 0.13f, 2.10f, 0.0f, 0.02f, 0.90f, 0.06f);
        // lower side vent
        box(TrimWhite.r, TrimWhite.g, TrimWhite.b, sx + side * 0.10f, 1.25f, 1.55f, 0.06f, 0.45f, 0.55f);
        box(WindowDrk.r, WindowDrk.g, WindowDrk.b, sx + side * 0.12f, 1.25f, 1.55f, 0.04f, 0.32f, 0.42f);
    }

    // --- Back wall ---
    box(BarnRed.r, BarnRed.g, BarnRed.b, 0, wallH * 0.5f + 0.6f, -D * 0.5f, W, wallH, 0.20f);
    for (float bx2 = -W * 0.5f + 0.9f; bx2 < W * 0.5f; bx2 += 1.15f)
        box(BarnRedDrk.r, BarnRedDrk.g, BarnRedDrk.b, bx2, wallH * 0.5f + 0.6f, -D * 0.5f - 0.08f, 0.14f, wallH, 0.04f);
    box(TrimWhite.r, TrimWhite.g, TrimWhite.b, 0, wallH + 0.6f, -D * 0.5f - 0.06f, W + 0.1f, 0.16f, 0.08f);

    // --- Gable end walls (triangular fill under gambrel) ---
    // front gable fill (simple two-triangle approach via sloped top)
    // Using flat color with trim to hide seam
    for (float s : {-1.0f, 1.0f})
    {
        float sx = s * W * 0.5f;
        // gable infill at front
        glDisable(GL_CULL_FACE);
        glColor3f(BarnRed.r, BarnRed.g, BarnRed.b);
        glBegin(GL_TRIANGLES);
        glVertex3f(sx, wallH + 0.6f, D * 0.5f);
        glVertex3f(s * W * 0.45f * 0.5f, bendY, D * 0.5f);
        glVertex3f(0, ridgeY, D * 0.5f);
        glEnd();
        glColor3f(BarnRed.r, BarnRed.g, BarnRed.b);
        glBegin(GL_TRIANGLES);
        glVertex3f(sx, wallH + 0.6f, -D * 0.5f);
        glVertex3f(s * W * 0.45f * 0.5f, bendY, -D * 0.5f);
        glVertex3f(0, ridgeY, -D * 0.5f);
        glEnd();
    }

    // --- Gambrel roof ---
    {
        const float halfW = W * 0.5f;
        // subtract overhang
        gambrelRoof(halfW + 0.25f, wallH + 0.6f + 0.10f, bendY + 0.15f, ridgeY + 0.12f, D + 0.6f, -1);
        gambrelRoof(halfW + 0.25f, wallH + 0.6f + 0.10f, bendY + 0.15f, ridgeY + 0.12f, D + 0.6f, 1);
        // ridge cap
        box(RoofDrk.r, RoofDrk.g, RoofDrk.b, 0, ridgeY + 0.16f, 0, 0.36f, 0.18f, D + 0.65f);
    }

    // --- Cupola on ridge ---
    {
        const float cy = ridgeY + 0.45f;
        const float cw = 1.25f, cd = 1.25f, ch = 1.10f;
        box(BarnRed.r, BarnRed.g, BarnRed.b, 0, cy + ch * 0.5f, 0.2f, cw, ch, cd);
        // louvers
        for (float sx : {-1.0f, 1.0f})
            box(WindowDrk.r, WindowDrk.g, WindowDrk.b, sx * cw * 0.5f + sx * 0.06f, cy + ch * 0.5f, 0.2f, 0.06f, ch * 0.65f, cd * 0.70f);
        box(WindowDrk.r, WindowDrk.g, WindowDrk.b, 0, cy + ch * 0.5f, cd * 0.5f + 0.06f, cw * 0.70f, ch * 0.65f, 0.06f);
        box(WindowDrk.r, WindowDrk.g, WindowDrk.b, 0, cy + ch * 0.5f, -cd * 0.5f - 0.06f + 0.4f, cw * 0.70f, ch * 0.65f, 0.06f);
        // cupola roof (small pyramid)
        box(CupolaRed.r, CupolaRed.g, CupolaRed.b, 0, cy + ch + 0.22f, 0.2f, cw + 0.45f, 0.45f, cd + 0.45f);
        // tiny weathervane on cupola
        box(0.25f, 0.25f, 0.24f, 0, cy + ch + 0.70f, 0.2f, 0.04f, 0.55f, 0.04f);
        box(0.85f, 0.78f, 0.10f, 0.08f, cy + ch + 0.90f, 0.2f, 0.55f, 0.06f, 0.04f);
    }

    glPopMatrix();
}

} // namespace Barn
