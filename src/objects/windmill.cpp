#include "objects/Windmil.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

namespace Windmill {

// ------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------
namespace {

// Small positioned box
void box(float x, float y, float z,
         float w, float h, float d)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCube(w, h, d);
    glPopMatrix();
}

// Positioned + rotated box (rotation around Z axis)
void rotBox(float x, float y, float z,
            float w, float h, float d,
            float angleZ)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(angleZ, 0.0f, 0.0f, 1.0f);
    Primitives::drawCube(w, h, d);
    glPopMatrix();
}

// A single blade: frame (wooden lattice) + sail (cloth).
// Drawn along +Y, from (0,0) to (0,length).
// The whole thing is rotated by `angle` around Z, then translated
// to the hub position by the caller.
void drawBlade(float length, float width)
{
    // ---- Blade FRAME (main spar) ----
    glColor3f(0.55f, 0.38f, 0.20f);   // light wood
    box(0.0f, length * 0.5f, 0.0f,
        width * 0.20f, length, 0.10f);

    // ---- Side rails (two thin long strips) ----
    glColor3f(0.48f, 0.32f, 0.16f);
    box( width * 0.5f, length * 0.5f, 0.0f,
        width * 0.10f, length * 0.95f, 0.08f);
    box(-width * 0.5f, length * 0.5f, 0.0f,
        width * 0.10f, length * 0.95f, 0.08f);

    // ---- Cross-bars (lattice look) ----
    glColor3f(0.42f, 0.28f, 0.14f);
    for (int i = 1; i <= 5; ++i) {
        float t = (float)i / 6.0f;
        box(0.0f, length * t, 0.0f,
            width, 0.06f, 0.09f);
    }

    // ---- Sail / cloth (pale canvas, offset forward) ----
    glColor3f(0.92f, 0.88f, 0.78f);
    glPushMatrix();
    glTranslatef(width * 0.15f, length * 0.55f, 0.08f);
    Primitives::drawCube(width * 0.75f, length * 0.82f, 0.04f);
    glPopMatrix();

    // Slight shading strip on the sail (adds depth)
    glColor3f(0.80f, 0.75f, 0.65f);
    glPushMatrix();
    glTranslatef(width * 0.15f, length * 0.55f, 0.11f);
    Primitives::drawCube(width * 0.75f, length * 0.82f, 0.01f);
    glPopMatrix();
}

} // anonymous namespace


// ============================================================
// WINDMILL
// ============================================================
//
//  Local origin is at the base of the tower (y = 0).
//  The whole windmill sits at world position (x, z).
//  The blade hub is at local (0, hubY, hubZ) — a bit forward of
//  the tower's front face — and rotates around the Z axis.
// ============================================================

void drawWindmill(float x, float z, float bladeAngle, float scale)
{
    glPushMatrix();

    // -- World placement + uniform scale --
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);

    // ----------------------------------------------------------
    // 1. STONE BASE  (slightly wider than tower)
    // ----------------------------------------------------------
    glColor3f(0.55f, 0.52f, 0.48f);   // light gray stone
    glPushMatrix();
    glTranslatef(0.0f, 0.35f, 0.0f);
    glScalef(1.15f, 0.55f, 1.15f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // Base trim (darker ring)
    glColor3f(0.40f, 0.38f, 0.35f);
    glPushMatrix();
    glTranslatef(0.0f, 0.65f, 0.0f);
    glScalef(1.20f, 0.10f, 1.20f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // ----------------------------------------------------------
    // 2. TAPERED TOWER  (stacked square segments, shrinking)
    // ----------------------------------------------------------
    glColor3f(0.85f, 0.82f, 0.72f);   // cream plaster

    // Segment parameters: (y, size, height)
    struct Seg { float y, size, h; };
    const Seg segs[] = {
        {1.40f, 1.10f, 1.50f},
        {2.85f, 1.00f, 1.40f},
        {4.20f, 0.90f, 1.30f},
        {5.45f, 0.80f, 1.20f},
    };

    for (int i = 0; i < 4; ++i) {
        // Main body of segment
        glPushMatrix();
        glTranslatef(0.0f, segs[i].y, 0.0f);
        Primitives::drawCube(segs[i].size,
                             segs[i].h,
                             segs[i].size);
        glPopMatrix();

        // Dark trim at the bottom of each segment (bands)
        glColor3f(0.55f, 0.42f, 0.28f);   // wood/stone band
        glPushMatrix();
        glTranslatef(0.0f, segs[i].y - segs[i].h * 0.5f + 0.03f, 0.0f);
        Primitives::drawCube(segs[i].size + 0.05f,
                             0.08f,
                             segs[i].size + 0.05f);
        glPopMatrix();

        glColor3f(0.85f, 0.82f, 0.72f);   // back to plaster
    }

    // ----------------------------------------------------------
    // 3. FRONT SHADOW STRIP  (adds depth — darker side of tower)
    // ----------------------------------------------------------
    glColor3f(0.68f, 0.65f, 0.58f);
    glPushMatrix();
    glTranslatef(-0.45f, 3.20f, 0.55f);
    Primitives::drawCube(0.25f, 4.20f, 0.02f);
    glPopMatrix();

    // Sunlit strip on the other side
    glColor3f(0.95f, 0.92f, 0.85f);
    glPushMatrix();
    glTranslatef(0.42f, 3.20f, 0.56f);
    Primitives::drawCube(0.22f, 4.20f, 0.02f);
    glPopMatrix();

    // ----------------------------------------------------------
    // 4. WINDOW  (small dark square on the front of the tower)
    // ----------------------------------------------------------
    glColor3f(0.20f, 0.12f, 0.06f);   // dark wood frame
    glPushMatrix();
    glTranslatef(0.0f, 2.30f, 0.56f);
    Primitives::drawCube(0.35f, 0.45f, 0.05f);
    glPopMatrix();

    glColor3f(0.15f, 0.20f, 0.25f);   // glass
    glPushMatrix();
    glTranslatef(0.0f, 2.30f, 0.59f);
    Primitives::drawCube(0.26f, 0.34f, 0.03f);
    glPopMatrix();

    // Window cross bars
    glColor3f(0.20f, 0.12f, 0.06f);
    glPushMatrix();
    glTranslatef(0.0f, 2.30f, 0.61f);
    Primitives::drawCube(0.26f, 0.03f, 0.02f);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(0.0f, 2.30f, 0.61f);
    Primitives::drawCube(0.03f, 0.34f, 0.02f);
    glPopMatrix();

    // ----------------------------------------------------------
    // 5. CAP / TOP SECTION  (peaked roof)
    // ----------------------------------------------------------
    glColor3f(0.65f, 0.22f, 0.15f);   // reddish-brown roof

    // Main cap block
    glPushMatrix();
    glTranslatef(0.0f, 6.35f, 0.0f);
    Primitives::drawCube(1.00f, 0.55f, 1.00f);
    glPopMatrix();

    // Roof pyramid (stacked shrinking boxes for a peaked look)
    struct CapStep { float y, size; };
    const CapStep cap[] = {
        {6.72f, 0.85f},
        {6.90f, 0.60f},
        {7.02f, 0.35f},
        {7.10f, 0.14f},
    };
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(0.0f, cap[i].y, 0.0f);
        Primitives::drawCube(cap[i].size, 0.16f, cap[i].size);
        glPopMatrix();
    }

    // ----------------------------------------------------------
    // 6. HUB SUPPORT  (a small wooden arm that pushes the hub
    //    forward of the tower so the blades don't clip it)
    // ----------------------------------------------------------
    glColor3f(0.45f, 0.28f, 0.14f);
    glPushMatrix();
    glTranslatef(0.0f, 5.00f, 0.60f);
    Primitives::drawCube(0.32f, 0.32f, 0.85f);
    glPopMatrix();

    // ----------------------------------------------------------
    // 7. HUB  (central rotating block, drawn BEFORE the blades
    //    so blades visually emerge from it)
    // ----------------------------------------------------------
    float hubY = 5.00f;
    float hubZ = 1.00f;

    glColor3f(0.30f, 0.18f, 0.08f);   // dark wood
    glPushMatrix();
    glTranslatef(0.0f, hubY, hubZ);
    Primitives::drawCube(0.42f, 0.42f, 0.28f);
    glPopMatrix();

    // Metal ring (slightly lighter)
    glColor3f(0.55f, 0.55f, 0.58f);
    glPushMatrix();
    glTranslatef(0.0f, hubY, hubZ - 0.16f);
    Primitives::drawCube(0.30f, 0.30f, 0.06f);
    glPopMatrix();

    // ----------------------------------------------------------
    // 8. BLADES  (the ONLY thing that rotates)
    // ----------------------------------------------------------
    //
    //  Hierarchy:
    //      glTranslatef(hub)        ← put us at the hub
    //      glRotatef(bladeAngle)    ← spin around Z
    //      for i in 0..3:
    //          glRotatef(i*90, Z)   ← orient blade in place
    //          drawBlade(...)       ← draw along +Y
    //
    //  The tower is NOT inside this block, so it never moves.
    //  Only the blades (and the hub visuals drawn inside) rotate.
    // ----------------------------------------------------------
    glPushMatrix();

        // Move to the hub position in LOCAL windmill space
        glTranslatef(0.0f, hubY, hubZ + 0.10f);

        // Apply the accumulated blade angle. This rotates everything
        // drawn inside this push/pop around the Z axis.
        glRotatef(bladeAngle, 0.0f, 0.0f, 1.0f);

        // Front hub cap (drawn inside the rotating frame — spins with it)
        glColor3f(0.35f, 0.22f, 0.10f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.06f);
        Primitives::drawCube(0.24f, 0.24f, 0.10f);
        glPopMatrix();

        // Draw the four blades, each starting at the hub and
        // extending outward. Blade #0 points up (+Y) at angle=0.
        const float bladeLen   = 2.40f;
        const float bladeWidth = 0.55f;

        for (int i = 0; i < 4; ++i) {
            glPushMatrix();
                glRotatef(i * 90.0f, 0.0f, 0.0f, 1.0f);
                drawBlade(bladeLen, bladeWidth);
            glPopMatrix();
        }

    glPopMatrix();   // end rotating frame

    glPopMatrix();   // end world placement
}

} // namespace Windmill