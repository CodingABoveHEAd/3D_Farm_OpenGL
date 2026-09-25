#include "objects/Farmhouse.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <initializer_list>

namespace
{
constexpr float HouseX = 10.0f;
constexpr float HouseZ = 7.0f;

// ------------------------------------------------------------
// Small helper for drawing transformed cubes.
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
    // Dark stone/concrete foundation.
    drawBox(
        0.25f,
        0.24f,
        0.21f,

        0.0f,
        0.35f,
        0.0f,

        8.35f,
        0.70f,
        6.35f
    );

    // Slightly lighter upper foundation strip.
    drawBox(
        0.38f,
        0.36f,
        0.30f,

        0.0f,
        0.70f,
        0.0f,

        8.20f,
        0.22f,
        6.15f
    );
}

// ============================================================
// MAIN BODY
// ============================================================

void Farmhouse::renderBody() const
{
    // Main wooden wall.
    glColor3f(
        0.72f,
        0.45f,
        0.22f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        2.85f,
        0.0f
    );

    Primitives::drawCube(
        8.0f,
        5.0f,
        6.0f
    );

    glPopMatrix();

    // Slightly darker lower wooden section.
    drawBox(
        0.62f,
        0.34f,
        0.15f,

        0.0f,
        1.05f,
        0.0f,

        7.95f,
        0.55f,
        6.02f
    );

    // Horizontal wooden wall stripe.
    drawBox(
        0.48f,
        0.25f,
        0.10f,

        0.0f,
        2.10f,
        3.04f,

        7.80f,
        0.12f,
        0.10f
    );

    drawBox(
        0.48f,
        0.25f,
        0.10f,

        0.0f,
        3.45f,
        3.04f,

        7.80f,
        0.12f,
        0.10f
    );
}

// ============================================================
// WALL TRIM
// ============================================================

void Farmhouse::renderWallTrim() const
{
    // Four vertical corner beams.

    const float beamWidth = 0.22f;

    // Front-left
    drawBox(
        0.38f,
        0.18f,
        0.06f,
        -3.90f,
        2.85f,
        3.05f,
        beamWidth,
        4.65f,
        0.18f
    );

    // Front-right
    drawBox(
        0.38f,
        0.18f,
        0.06f,
         3.90f,
        2.85f,
        3.05f,
        beamWidth,
        4.65f,
        0.18f
    );

    // Rear-left
    drawBox(
        0.38f,
        0.18f,
        0.06f,
        -3.90f,
        2.85f,
        -3.05f,
        beamWidth,
        4.65f,
        0.18f
    );

    // Rear-right
    drawBox(
        0.38f,
        0.18f,
        0.06f,
         3.90f,
        2.85f,
        -3.05f,
        beamWidth,
        4.65f,
        0.18f
    );

    // Front upper trim.
    drawBox(
        0.35f,
        0.17f,
        0.05f,
        0.0f,
        5.15f,
        3.05f,
        8.20f,
        0.22f,
        0.20f
    );
}

// ============================================================
// ROOF
// ============================================================

void Farmhouse::renderRoof() const
{
    // Left roof slope.
    glColor3f(
        0.30f,
        0.055f,
        0.035f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        5.35f,
        -1.35f
    );

    glRotatef(
        -30.0f,
        1.0f,
        0.0f,
        0.0f
    );

    Primitives::drawCube(
        8.8f,
        0.40f,
        4.10f
    );

    glPopMatrix();

    // Right roof slope.
    glPushMatrix();

    glTranslatef(
        0.0f,
        5.35f,
        1.35f
    );

    glRotatef(
        30.0f,
        1.0f,
        0.0f,
        0.0f
    );

    Primitives::drawCube(
        8.8f,
        0.40f,
        4.10f
    );

    glPopMatrix();

    // Dark roof ridge.
    drawBox(
        0.18f,
        0.035f,
        0.025f,

        0.0f,
        6.55f,
        0.0f,

        9.0f,
        0.28f,
        0.30f
    );

    // Front roof trim.
    drawBox(
        0.48f,
        0.12f,
        0.05f,

        0.0f,
        5.55f,
        3.25f,

        8.95f,
        0.18f,
        0.18f
    );

    // Back roof trim.
    drawBox(
        0.48f,
        0.12f,
        0.05f,

        0.0f,
        5.55f,
        -3.25f,

        8.95f,
        0.18f,
        0.18f
    );
}

// ============================================================
// DOOR
// ============================================================

void Farmhouse::renderDoor() const
{
    // Door frame.
    drawBox(
        0.30f,
        0.14f,
        0.045f,

        0.0f,
        1.95f,
        3.16f,

        2.05f,
        3.30f,
        0.20f
    );

    // Main door.
    drawBox(
        0.30f,
        0.11f,
        0.035f,

        0.0f,
        1.95f,
        3.29f,

        1.60f,
        2.75f,
        0.18f
    );

    // Upper door panel.
    drawBox(
        0.42f,
        0.18f,
        0.07f,

        -0.42f,
        2.45f,
        3.40f,

        0.48f,
        0.80f,
        0.06f
    );

    drawBox(
        0.42f,
        0.18f,
        0.07f,

        0.42f,
        2.45f,
        3.40f,

        0.48f,
        0.80f,
        0.06f
    );

    // Lower door panels.
    drawBox(
        0.24f,
        0.085f,
        0.025f,

        -0.42f,
        1.40f,
        3.40f,

        0.48f,
        0.72f,
        0.06f
    );

    drawBox(
        0.24f,
        0.085f,
        0.025f,

        0.42f,
        1.40f,
        3.40f,

        0.48f,
        0.72f,
        0.06f
    );

    // Golden door handle.
    glColor3f(
        0.92f,
        0.70f,
        0.20f
    );

    glPushMatrix();

    glTranslatef(
        0.52f,
        1.95f,
        3.47f
    );

    Primitives::drawCylinder(
        0.09f,
        0.10f,
        10
    );

    glPopMatrix();

    // Small step.
    drawBox(
        0.38f,
        0.20f,
        0.08f,

        0.0f,
        0.82f,
        3.50f,

        2.0f,
        0.18f,
        0.65f
    );
}

// ============================================================
// WINDOWS
// ============================================================

void Farmhouse::renderWindows() const
{
    drawWindow(-2.35f);
    drawWindow(2.35f);
}

void Farmhouse::drawWindow(float x) const
{
    // Outer wooden frame.
    drawBox(
        0.34f,
        0.16f,
        0.05f,

        x,
        2.95f,
        3.16f,

        2.05f,
        1.85f,
        0.20f
    );

    // Blue glass.
    glColor3f(
        0.15f,
        0.48f,
        0.66f
    );

    glPushMatrix();

    glTranslatef(
        x,
        2.95f,
        3.30f
    );

    Primitives::drawCube(
        1.65f,
        1.45f,
        0.12f
    );

    glPopMatrix();

    // Window cross.
    drawWindowCross(
        x,
        2.95f,
        3.40f
    );

    // Slight highlight on glass.
    drawBox(
        0.45f,
        0.78f,
        0.88f,

        x - 0.38f,
        3.25f,
        3.46f,

        0.08f,
        0.60f,
        0.025f
    );
}

void Farmhouse::drawWindowCross(
    float x,
    float y,
    float z) const
{
    glColor3f(
        0.90f,
        0.72f,
        0.35f
    );

    // Vertical divider.
    glPushMatrix();

    glTranslatef(
        x,
        y,
        z
    );

    glScalef(
        0.08f,
        1.0f,
        1.0f
    );

    Primitives::drawCube(
        1.0f,
        1.45f,
        0.06f
    );

    glPopMatrix();

    // Horizontal divider.
    glPushMatrix();

    glTranslatef(
        x,
        y,
        z + 0.01f
    );

    glScalef(
        1.0f,
        0.08f,
        1.0f
    );

    Primitives::drawCube(
        1.65f,
        1.0f,
        0.06f
    );

    glPopMatrix();
}

// ============================================================
// WINDOW SHUTTERS
// ============================================================

void Farmhouse::renderWindowShutters() const
{
    for (float x : {-2.35f, 2.35f})
    {
        // Left shutter.
        drawBox(
            0.28f,
            0.13f,
            0.045f,

            x - 1.15f,
            2.95f,
            3.22f,

            0.38f,
            1.70f,
            0.12f
        );

        // Right shutter.
        drawBox(
            0.28f,
            0.13f,
            0.045f,

            x + 1.15f,
            2.95f,
            3.22f,

            0.38f,
            1.70f,
            0.12f
        );
    }
}

// ============================================================
// ATTIC WINDOW
// ============================================================

void Farmhouse::renderAtticWindow() const
{
    // Small attic window under the roof.
    drawBox(
        0.30f,
        0.15f,
        0.05f,

        0.0f,
        5.25f,
        3.05f,

        1.45f,
        0.85f,
        0.18f
    );

    glColor3f(
        0.18f,
        0.50f,
        0.68f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        5.25f,
        3.18f
    );

    Primitives::drawCube(
        1.05f,
        0.55f,
        0.10f
    );

    glPopMatrix();

    // Attic window divider.
    drawBox(
        0.92f,
        0.72f,
        0.25f,

        0.0f,
        5.25f,
        3.25f,

        0.07f,
        0.60f,
        0.06f
    );
}

// ============================================================
// CHIMNEY
// ============================================================

void Farmhouse::renderChimney() const
{
    // Main chimney.
    drawBox(
        0.40f,
        0.16f,
        0.09f,

        2.5f,
        6.15f,
        -0.80f,

        0.85f,
        2.10f,
        0.85f
    );

    // Chimney cap.
    drawBox(
        0.22f,
        0.09f,
        0.05f,

        2.5f,
        7.25f,
        -0.80f,

        1.05f,
        0.20f,
        1.05f
    );
}

// ============================================================
// PORCH
// ============================================================

void Farmhouse::renderPorch() const
{
    // Porch floor.
    drawBox(
        0.48f,
        0.25f,
        0.09f,

        0.0f,
        0.58f,
        3.78f,

        3.9f,
        0.45f,
        1.65f
    );

    // Porch front beam.
    drawBox(
        0.30f,
        0.14f,
        0.045f,

        0.0f,
        2.75f,
        4.55f,

        4.15f,
        0.22f,
        0.22f
    );

    // Porch roof.
    glColor3f(
        0.38f,
        0.16f,
        0.06f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        3.10f,
        4.20f
    );

    glRotatef(
        7.0f,
        1.0f,
        0.0f,
        0.0f
    );

    Primitives::drawCube(
        4.20f,
        0.28f,
        1.80f
    );

    glPopMatrix();

    // Porch columns.
    for (float x : {-1.55f, 1.55f})
    {
        drawBox(
            0.27f,
            0.12f,
            0.035f,

            x,
            1.70f,
            4.30f,

            0.28f,
            2.80f,
            0.28f
        );

        // Column base.
        drawBox(
            0.20f,
            0.09f,
            0.025f,

            x,
            0.60f,
            4.30f,

            0.55f,
            0.18f,
            0.55f
        );
    }
}

// ============================================================
// PORCH STEPS
// ============================================================

void Farmhouse::renderPorchSteps() const
{
    // Upper step.
    drawBox(
        0.42f,
        0.21f,
        0.07f,

        0.0f,
        0.38f,
        4.60f,

        3.0f,
        0.30f,
        0.75f
    );

    // Lower step.
    drawBox(
        0.35f,
        0.17f,
        0.055f,

        0.0f,
        0.18f,
        4.95f,

        3.35f,
        0.20f,
        0.65f
    );
}

// ============================================================
// PORCH RAILINGS
// ============================================================

void Farmhouse::renderPorchRailings() const
{
    glColor3f(
        0.30f,
        0.14f,
        0.055f
    );

    // Horizontal rail.
    drawBox(
        0.30f,
        0.13f,
        0.04f,

        -1.55f,
        1.55f,
        4.30f,

        0.25f,
        0.16f,
        1.70f
    );

    drawBox(
        0.30f,
        0.13f,
        0.04f,

        1.55f,
        1.55f,
        4.30f,

        0.25f,
        0.16f,
        1.70f
    );

    // Small decorative railing posts.
    for (float x : {-1.10f, -0.55f, 0.55f, 1.10f})
    {
        drawBox(
            0.32f,
            0.14f,
            0.05f,

            x,
            1.15f,
            4.55f,

            0.12f,
            1.0f,
            0.12f
        );
    }

    // Top railing.
    drawBox(
        0.32f,
        0.14f,
        0.05f,

        0.0f,
        1.70f,
        4.55f,

        2.80f,
        0.15f,
        0.15f
    );
}