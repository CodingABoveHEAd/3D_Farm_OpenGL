#include "objects/Farmhouse.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <initializer_list>

namespace
{
constexpr float HouseX = 10.0f;
constexpr float HouseZ = 7.0f;

constexpr float WallTop = 5.35f;   // top of the main wall box
constexpr float WinY    = 2.95f;   // window centre height

// ------------------------------------------------------------
// Palette
// ------------------------------------------------------------
struct Color
{
    float r, g, b;
};

constexpr Color Cream       {0.88f, 0.83f, 0.70f};   // trim, frames, columns
constexpr Color CreamShade  {0.80f, 0.75f, 0.62f};   // sills, bases, balusters
constexpr Color Glass       {0.15f, 0.48f, 0.66f};
constexpr Color ShutterGreen{0.13f, 0.28f, 0.20f};
constexpr Color ShutterSlat {0.09f, 0.20f, 0.14f};
constexpr Color Gold        {0.92f, 0.70f, 0.20f};

// ------------------------------------------------------------
// Small helpers for drawing transformed cubes.
// ------------------------------------------------------------
void drawBox(
    float r, float g, float b,
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

void drawBox(
    const Color& c,
    float x, float y, float z,
    float sx, float sy, float sz)
{
    drawBox(c.r, c.g, c.b, x, y, z, sx, sy, sz);
}

// ------------------------------------------------------------
// Foundation: irregular stone courses on one face.
// Local frame: wall surface at z = 0, facing +z, centred on x = 0.
// ------------------------------------------------------------
void drawStoneFace(float length)
{
    for (int r = 0; r < 3; ++r)
    {
        const float y = 0.13f + 0.22f * r;

        float x = -0.5f * length - ((r % 2) ? 0.0f : 0.35f);
        int i = 0;

        while (x < 0.5f * length)
        {
            const float w  = 0.60f + 0.22f * ((i * 5 + r * 3) % 4);
            const float x0 = std::max(x, -0.5f * length);
            const float x1 = std::min(x + w, 0.5f * length);

            if (x1 - x0 > 0.05f)
            {
                const float s = 0.42f + 0.05f * ((i * 3 + r * 5) % 4);

                drawBox(
                    s, s - 0.02f, s - 0.07f,
                    0.5f * (x0 + x1), y, 0.015f,
                    (x1 - x0) - 0.05f, 0.19f, 0.05f);
            }

            x += w;
            ++i;
        }
    }
}

// ------------------------------------------------------------
// Window pieces. Local frame: wall surface at z = 0, facing +z.
// ------------------------------------------------------------
void drawMuntins(float x, float y, float z)
{
    // Six-pane layout: one vertical + two horizontal bars.
    drawBox(Cream, x, y,          z, 0.06f, 1.45f, 0.04f);
    drawBox(Cream, x, y + 0.24f,  z, 1.35f, 0.05f, 0.04f);
    drawBox(Cream, x, y - 0.24f,  z, 1.35f, 0.05f, 0.04f);
}

void drawWindowUnit()
{
    // Frame.
    drawBox(Cream, 0.0f, WinY, 0.16f, 1.70f, 1.85f, 0.20f);

    // Glass.
    drawBox(Glass, 0.0f, WinY, 0.27f, 1.35f, 1.45f, 0.06f);

    // Reflection streak.
    drawBox(
        0.45f, 0.78f, 0.88f,
        -0.38f, WinY + 0.30f, 0.305f,
        0.07f, 0.55f, 0.02f);

    drawMuntins(0.0f, WinY, 0.315f);

    // Projecting sill.
    drawBox(CreamShade, 0.0f, WinY - 0.985f, 0.20f, 1.95f, 0.10f, 0.34f);

    // Lintel with a small cap moulding.
    drawBox(Cream,      0.0f, WinY + 1.005f, 0.13f, 1.95f, 0.16f, 0.26f);
    drawBox(CreamShade, 0.0f, WinY + 1.110f, 0.15f, 2.10f, 0.07f, 0.30f);
}

void drawShutterPair()
{
    for (float s : {-1.0f, 1.0f})
    {
        const float sx = s * 1.02f;

        drawBox(ShutterGreen, sx, WinY, 0.13f, 0.30f, 1.85f, 0.16f);

        // Louvre slats.
        for (int i = 0; i < 6; ++i)
        {
            drawBox(
                ShutterSlat,
                sx, WinY - 0.75f + 0.30f * i, 0.225f,
                0.24f, 0.05f, 0.04f);
        }
    }
}

// Small attic window for the gable ends (local frame as above).
void drawAtticWindowUnit()
{
    const float y = 6.05f;

    drawBox(Cream, 0.0f, y, 0.09f, 1.05f, 1.00f, 0.14f);
    drawBox(0.18f, 0.50f, 0.68f, 0.0f, y, 0.16f, 0.80f, 0.75f, 0.04f);
    drawBox(Cream, 0.0f, y, 0.19f, 0.05f, 0.75f, 0.03f);
    drawBox(Cream, 0.0f, y, 0.19f, 0.80f, 0.05f, 0.03f);
    drawBox(CreamShade, 0.0f, y - 0.56f, 0.12f, 1.25f, 0.08f, 0.22f);
}

// ------------------------------------------------------------
// Roof
// ------------------------------------------------------------

// One 30-degree roof plane with shingle courses, fascia and barge boards.
// side = -1 for the back slope, +1 for the front slope.
void drawRoofSlab(float side)
{
    constexpr float Length = 9.0f;
    constexpr float Thick  = 0.30f;
    constexpr float Depth  = 4.2f;

    glPushMatrix();

    glTranslatef(0.0f, 6.313f, side * 1.631f);
    glRotatef(side * 30.0f, 1.0f, 0.0f, 0.0f);

    // Dark underlay (shows through as grooves between courses).
    drawBox(0.20f, 0.04f, 0.028f, 0.0f, 0.0f, 0.0f, Length, Thick, Depth);

    // Shingle courses, starting at the eave.
    for (int i = 0; i < 13; ++i)
    {
        const float dist = 0.16f + 0.30f * i;
        const float z    = side * (0.5f * Depth - dist);
        const float v    = 0.018f * ((i * 3) % 4);

        drawBox(
            0.30f + v, 0.062f + 0.4f * v, 0.040f + 0.2f * v,
            0.0f, 0.175f, z,
            Length, 0.05f, 0.27f);
    }

    // Eave fascia.
    drawBox(Cream, 0.0f, -0.02f, side * 0.5f * Depth, Length + 0.05f, 0.36f, 0.07f);

    // Barge boards along the gable edges.
    drawBox(Cream,  4.5f, -0.02f, 0.0f, 0.07f, 0.36f, Depth);
    drawBox(Cream, -4.5f, -0.02f, 0.0f, 0.07f, 0.36f, Depth);

    glPopMatrix();
}

// Triangular gable wall at one end of the house.
// sx = -1 (left) or +1 (right).
void drawGableEnd(float sx)
{
    const float x = sx * 3.995f;

    // Draw the triangle with culling off so winding doesn't matter.
    const GLboolean culling = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glColor3f(0.60f, 0.36f, 0.16f);

    glBegin(GL_TRIANGLES);
    glNormal3f(sx, 0.0f, 0.0f);
    glVertex3f(x, WallTop - 0.02f, -3.0f);
    glVertex3f(x, WallTop - 0.02f,  3.0f);
    glVertex3f(x, 7.06f,            0.0f);
    glEnd();

    if (culling)
    {
        glEnable(GL_CULL_FACE);
    }

    // Horizontal siding boards that follow the triangle.
    for (float y : {5.70f, 6.05f, 6.40f, 6.75f})
    {
        const float half = (7.06f - y) * 3.0f / 1.71f;

        drawBox(
            0.72f, 0.45f, 0.22f,
            sx * 4.01f, y, 0.0f,
            0.04f, 0.05f, 2.0f * half - 0.05f);
    }
}

// ------------------------------------------------------------
// Porch railing pieces
// ------------------------------------------------------------
void drawRailAlongX(float x0, float x1, float z)
{
    const float len = x1 - x0;
    const float cx  = 0.5f * (x0 + x1);

    drawBox(Cream, cx, 1.75f, z, len, 0.10f, 0.10f);   // top rail
    drawBox(Cream, cx, 1.05f, z, len, 0.08f, 0.08f);   // bottom rail

    const int n = static_cast<int>(len / 0.16f);
    for (int i = 0; i < n; ++i)
    {
        drawBox(CreamShade, x0 + 0.08f + 0.16f * i, 1.40f, z, 0.05f, 0.66f, 0.05f);
    }
}

void drawRailAlongZ(float x, float z0, float z1)
{
    const float len = z1 - z0;
    const float cz  = 0.5f * (z0 + z1);

    drawBox(Cream, x, 1.75f, cz, 0.10f, 0.10f, len);
    drawBox(Cream, x, 1.05f, cz, 0.08f, 0.08f, len);

    const int n = static_cast<int>(len / 0.16f);
    for (int i = 0; i < n; ++i)
    {
        drawBox(CreamShade, x, 1.40f, z0 + 0.08f + 0.16f * i, 0.05f, 0.66f, 0.05f);
    }
}
}

// ============================================================
// MAIN FARMHOUSE
// ============================================================

void Farmhouse::render() const
{
    glPushMatrix();

    // Position the complete farmhouse in the farm.
    glTranslatef(
        HouseX,
        0.0f,
        HouseZ
    );
    glScalef(1.30f, 1.30f, 1.30f);

    renderFoundation();
    renderBody();
    renderWallTrim();
    renderRoof();
    renderDoor();
    renderWindows();
    renderWindowShutters();
    renderAtticWindow();
    renderChimney();
    renderPorch();
    renderPorchSteps();
    renderPorchRailings();

    glPopMatrix();
}

// ============================================================
// FOUNDATION
// ============================================================

void Farmhouse::renderFoundation() const
{
    // Dark mortar-coloured core.
    drawBox(
        0.25f, 0.24f, 0.21f,
        0.0f, 0.35f, 0.0f,
        8.35f, 0.70f, 6.35f
    );

    // Lighter capping course (water table) around the top.
    drawBox(
        0.50f, 0.48f, 0.42f,
        0.0f, 0.72f, 0.0f,
        8.45f, 0.14f, 6.45f
    );

    // Front stones.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 3.175f);
    drawStoneFace(8.35f);
    glPopMatrix();

    // Back stones.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -3.175f);
    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
    drawStoneFace(8.35f);
    glPopMatrix();

    // Side stones.
    for (float s : {-1.0f, 1.0f})
    {
        glPushMatrix();
        glTranslatef(s * 4.175f, 0.0f, 0.0f);
        glRotatef(s * 90.0f, 0.0f, 1.0f, 0.0f);
        drawStoneFace(6.35f);
        glPopMatrix();
    }
}

// ============================================================
// MAIN BODY
// ============================================================

void Farmhouse::renderBody() const
{
    // Wall core (visible as dark grooves between the siding boards).
    drawBox(
        0.56f, 0.33f, 0.14f,
        0.0f, 2.85f, 0.0f,
        8.0f, 5.0f, 6.0f
    );

    // Darker wainscot band around the base.
    drawBox(
        0.46f, 0.26f, 0.11f,
        0.0f, 1.05f, 0.0f,
        8.06f, 0.55f, 6.10f
    );

    // Cream belt course capping the wainscot.
    drawBox(
        Cream,
        0.0f, 1.37f, 0.0f,
        8.14f, 0.07f, 6.18f
    );

    // Clapboard siding on all four walls, alternating shade.
    for (int i = 0; i < 12; ++i)
    {
        const float y   = 1.58f + 0.30f * i;
        const bool  odd = (i % 2) == 1;

        const float r = odd ? 0.70f : 0.75f;
        const float g = odd ? 0.43f : 0.47f;
        const float b = odd ? 0.20f : 0.23f;

        drawBox(r, g, b,  0.0f,  y,  3.025f, 7.60f, 0.26f, 0.05f);   // front
        drawBox(r, g, b,  0.0f,  y, -3.025f, 7.60f, 0.26f, 0.05f);   // back
        drawBox(r, g, b,  4.025f, y, 0.0f,   0.05f, 0.26f, 5.60f);   // right
        drawBox(r, g, b, -4.025f, y, 0.0f,   0.05f, 0.26f, 5.60f);   // left
    }
}

// ============================================================
// WALL TRIM
// ============================================================

void Farmhouse::renderWallTrim() const
{
    // Square cream corner posts.
    for (float sx : {-3.96f, 3.96f})
    {
        for (float sz : {-2.96f, 2.96f})
        {
            drawBox(
                Cream,
                sx, 2.85f, sz,
                0.28f, 4.65f, 0.28f
            );
        }
    }

    // Frieze board under the eaves, all the way around.
    drawBox(Cream,  0.0f,  5.20f,  3.05f, 8.30f, 0.30f, 0.14f);
    drawBox(Cream,  0.0f,  5.20f, -3.05f, 8.30f, 0.30f, 0.14f);
    drawBox(Cream,  4.05f, 5.20f,  0.0f,  0.14f, 0.30f, 6.30f);
    drawBox(Cream, -4.05f, 5.20f,  0.0f,  0.14f, 0.30f, 6.30f);
}

// ============================================================
// ROOF
// ============================================================

void Farmhouse::renderRoof() const
{
    // Two 30-degree shingled slopes sitting on top of the walls,
    // with an eave overhang on all sides.
    drawRoofSlab(-1.0f);
    drawRoofSlab(1.0f);

    // Dark ridge cap.
    drawBox(
        0.20f, 0.04f, 0.03f,
        0.0f, 7.44f, 0.0f,
        9.12f, 0.24f, 0.44f
    );

    // Filled-in gable walls at both ends.
    drawGableEnd(-1.0f);
    drawGableEnd(1.0f);
}

// ============================================================
// DOOR
// ============================================================

void Farmhouse::renderDoor() const
{
    // Cream door surround.
    drawBox(
        Cream,
        0.0f, 2.27f, 3.16f,
        2.05f, 3.05f, 0.20f
    );

    // Main door leaf (deep green).
    drawBox(
        0.14f, 0.30f, 0.22f,
        0.0f, 2.20f, 3.29f,
        1.60f, 2.80f, 0.18f
    );

    for (float x : {-0.42f, 0.42f})
    {
        // Lower raised panels.
        drawBox(
            0.09f, 0.21f, 0.15f,
            x, 1.65f, 3.40f,
            0.55f, 0.72f, 0.06f
        );

        // Upper glazed panels.
        drawBox(
            0.20f, 0.45f, 0.60f,
            x, 2.70f, 3.40f,
            0.55f, 0.85f, 0.06f
        );

        // Glazing bars.
        drawBox(Cream, x, 2.70f, 3.44f, 0.04f, 0.85f, 0.03f);
        drawBox(Cream, x, 2.70f, 3.44f, 0.55f, 0.04f, 0.03f);
    }

    // Brass back plate and handle.
    drawBox(Gold, 0.55f, 2.14f, 3.435f, 0.10f, 0.30f, 0.03f);

    glColor3f(
        0.92f,
        0.70f,
        0.20f
    );

    glPushMatrix();

    glTranslatef(
        0.55f,
        2.14f,
        3.47f
    );

    Primitives::drawCylinder(
        0.09f,
        0.10f,
        10
    );

    glPopMatrix();

    // Cornice above the door.
    drawBox(Cream,      0.0f, 3.86f, 3.20f, 2.45f, 0.14f, 0.30f);
    drawBox(CreamShade, 0.0f, 3.98f, 3.18f, 2.20f, 0.10f, 0.24f);

    // Wall lanterns either side of the door.
    for (float x : {-1.30f, 1.30f})
    {
        drawBox(0.98f, 0.85f, 0.45f, x, 2.70f, 3.14f, 0.14f, 0.28f, 0.14f);
        drawBox(0.15f, 0.12f, 0.10f, x, 2.865f, 3.14f, 0.18f, 0.05f, 0.18f);
        drawBox(0.15f, 0.12f, 0.10f, x, 2.535f, 3.14f, 0.16f, 0.04f, 0.16f);
    }

    // Doormat.
    drawBox(0.20f, 0.16f, 0.12f, 0.0f, 0.825f, 3.55f, 1.50f, 0.04f, 0.60f);
}

// ============================================================
// WINDOWS
// ============================================================

void Farmhouse::renderWindows() const
{
    // Front pair.
    drawWindow(-2.6f);
    drawWindow(2.6f);

    // Side windows (two per side).
    for (float sx : {-1.0f, 1.0f})
    {
        for (float z : {-1.5f, 1.5f})
        {
            glPushMatrix();
            glTranslatef(sx * 4.0f, 0.0f, z);
            glRotatef(sx * 90.0f, 0.0f, 1.0f, 0.0f);
            drawWindowUnit();
            glPopMatrix();
        }
    }

    // Back windows.
    for (float x : {-2.6f, 0.0f, 2.6f})
    {
        glPushMatrix();
        glTranslatef(x, 0.0f, -3.0f);
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        drawWindowUnit();
        glPopMatrix();
    }
}

void Farmhouse::drawWindow(float x) const
{
    glPushMatrix();

    // Wall surface of the front face is at z = 3.0.
    glTranslatef(x, 0.0f, 3.0f);
    drawWindowUnit();

    glPopMatrix();
}

void Farmhouse::drawWindowCross(
    float x,
    float y,
    float z) const
{
    drawMuntins(x, y, z);
}

// ============================================================
// WINDOW SHUTTERS
// ============================================================

void Farmhouse::renderWindowShutters() const
{
    // Front.
    for (float x : {-2.6f, 2.6f})
    {
        glPushMatrix();
        glTranslatef(x, 0.0f, 3.0f);
        drawShutterPair();
        glPopMatrix();
    }

    // Sides.
    for (float sx : {-1.0f, 1.0f})
    {
        for (float z : {-1.5f, 1.5f})
        {
            glPushMatrix();
            glTranslatef(sx * 4.0f, 0.0f, z);
            glRotatef(sx * 90.0f, 0.0f, 1.0f, 0.0f);
            drawShutterPair();
            glPopMatrix();
        }
    }

    // Back (outer windows only).
    for (float x : {-2.6f, 2.6f})
    {
        glPushMatrix();
        glTranslatef(x, 0.0f, -3.0f);
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        drawShutterPair();
        glPopMatrix();
    }
}

// ============================================================
// ATTIC WINDOWS
// ============================================================

void Farmhouse::renderAtticWindow() const
{
    // One in each gable end, centred in the triangle.
    for (float sx : {-1.0f, 1.0f})
    {
        glPushMatrix();
        glTranslatef(sx * 4.0f, 0.0f, 0.0f);
        glRotatef(sx * 90.0f, 0.0f, 1.0f, 0.0f);
        drawAtticWindowUnit();
        glPopMatrix();
    }
}

// ============================================================
// CHIMNEY
// ============================================================

void Farmhouse::renderChimney() const
{
    const float cx = 2.5f;
    const float cz = -0.8f;

    // Brick stack (extends down into the roof).
    drawBox(
        0.55f, 0.21f, 0.15f,
        cx, 6.85f, cz,
        0.85f, 2.50f, 0.85f
    );

    // Mortar bands.
    for (int i = 0; i < 10; ++i)
    {
        drawBox(
            0.78f, 0.74f, 0.66f,
            cx, 5.95f + 0.22f * i, cz,
            0.87f, 0.035f, 0.87f
        );
    }

    // Corbelled brick collar.
    drawBox(0.42f, 0.16f, 0.11f, cx, 8.02f, cz, 1.00f, 0.12f, 1.00f);

    // Concrete cap.
    drawBox(0.55f, 0.53f, 0.50f, cx, 8.16f, cz, 1.06f, 0.14f, 1.06f);

    // Clay flue pot.
    drawBox(0.22f, 0.09f, 0.06f, cx, 8.42f, cz, 0.42f, 0.36f, 0.42f);
}

// ============================================================
// PORCH
// ============================================================

void Farmhouse::renderPorch() const
{
    // Deck base.
    drawBox(
        0.48f, 0.25f, 0.09f,
        0.0f, 0.58f, 3.78f,
        3.9f, 0.45f, 1.65f
    );

    // Deck boards.
    for (int i = 0; i < 12; ++i)
    {
        const bool odd = (i % 2) == 1;

        drawBox(
            odd ? 0.50f : 0.55f,
            odd ? 0.28f : 0.31f,
            odd ? 0.11f : 0.13f,
            -1.7875f + 0.325f * i, 0.815f, 3.78f,
            0.30f, 0.02f, 1.62f
        );
    }

    // Cream skirt along the deck front.
    drawBox(Cream, 0.0f, 0.56f, 4.62f, 3.94f, 0.30f, 0.04f);

    // Front beam.
    drawBox(Cream, 0.0f, 3.90f, 4.35f, 4.00f, 0.22f, 0.24f);

    // Porch roof (slight forward tilt) with shingles and fascia.
    glPushMatrix();

    glTranslatef(0.0f, 4.22f, 3.85f);
    glRotatef(7.0f, 1.0f, 0.0f, 0.0f);

    drawBox(0.20f, 0.04f, 0.028f, 0.0f, 0.0f, 0.0f, 4.6f, 0.26f, 1.9f);

    for (int i = 0; i < 7; ++i)
    {
        const float z = 0.95f - (0.14f + 0.27f * i);
        const float v = 0.018f * ((i * 3) % 4);

        drawBox(
            0.30f + v, 0.062f + 0.4f * v, 0.040f + 0.2f * v,
            0.0f, 0.15f, z,
            4.6f, 0.04f, 0.24f
        );
    }

    drawBox(Cream, 0.0f, -0.02f, 0.95f, 4.66f, 0.32f, 0.06f);
    drawBox(Cream,  2.3f, -0.02f, 0.0f, 0.06f, 0.32f, 1.9f);
    drawBox(Cream, -2.3f, -0.02f, 0.0f, 0.06f, 0.32f, 1.9f);

    glPopMatrix();

    // Columns with base and capital.
    for (float x : {-1.6f, 1.6f})
    {
        drawBox(Cream,      x, 2.50f, 4.35f, 0.26f, 3.10f, 0.26f);
        drawBox(CreamShade, x, 0.90f, 4.35f, 0.50f, 0.18f, 0.50f);
        drawBox(CreamShade, x, 3.78f, 4.35f, 0.40f, 0.14f, 0.40f);
    }
}

// ============================================================
// PORCH STEPS
// ============================================================

void Farmhouse::renderPorchSteps() const
{
    // Upper step.
    drawBox(
        0.42f, 0.21f, 0.07f,
        0.0f, 0.27f, 4.78f,
        1.8f, 0.54f, 0.50f
    );

    drawBox(0.52f, 0.29f, 0.11f, 0.0f, 0.545f, 4.80f, 1.86f, 0.04f, 0.54f);

    // Lower step.
    drawBox(
        0.35f, 0.17f, 0.055f,
        0.0f, 0.14f, 5.20f,
        1.8f, 0.28f, 0.50f
    );

    drawBox(0.52f, 0.29f, 0.11f, 0.0f, 0.285f, 5.21f, 1.86f, 0.04f, 0.54f);
}

// ============================================================
// PORCH RAILINGS
// ============================================================

void Farmhouse::renderPorchRailings() const
{
    // Front rails, leaving a gap for the steps.
    drawRailAlongX(0.95f, 1.47f, 4.35f);
    drawRailAlongX(-1.47f, -0.95f, 4.35f);

    // Side rails.
    drawRailAlongZ(1.6f, 3.05f, 4.22f);
    drawRailAlongZ(-1.6f, 3.05f, 4.22f);

    // Newel posts at the top of the steps.
    for (float x : {-0.95f, 0.95f})
    {
        drawBox(Cream, x, 1.45f, 4.35f, 0.14f, 1.20f, 0.14f);
        drawBox(CreamShade, x, 2.08f, 4.35f, 0.18f, 0.06f, 0.18f);
    }
}