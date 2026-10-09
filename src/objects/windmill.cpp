#include "objects/Windmill.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace Windmill {

// ------------------------------------------------------------
// Internal helpers  (NO OpenGL lighting is used anywhere)
//
// Depth is faked by "baked shading": every big surface is drawn
// with a per-face brightness multiplier (front brightest, back
// darkest), so the model still reads as 3D with flat colours.
// ------------------------------------------------------------
namespace {

const float PI = 3.14159265f;

struct C { float r, g, b; };

// ---- Palette ------------------------------------------------
const C STONE      = {0.55f, 0.52f, 0.48f};
const C STONE_DARK = {0.42f, 0.40f, 0.37f};
const C STONE_LITE = {0.68f, 0.65f, 0.60f};
const C PLASTER    = {0.86f, 0.83f, 0.73f};
const C PLASTER_LN = {0.74f, 0.70f, 0.60f};
const C WOOD       = {0.50f, 0.34f, 0.18f};
const C WOOD_DARK  = {0.30f, 0.18f, 0.09f};
const C WOOD_LITE  = {0.62f, 0.44f, 0.24f};
const C ROOF       = {0.66f, 0.22f, 0.15f};
const C METAL      = {0.45f, 0.46f, 0.50f};
const C GLASS      = {0.20f, 0.30f, 0.38f};
const C SHUTTER    = {0.22f, 0.40f, 0.30f};
const C SHUTTER_LN = {0.15f, 0.29f, 0.21f};
const C CANVAS_A   = {0.93f, 0.89f, 0.79f};
const C CANVAS_B   = {0.85f, 0.80f, 0.69f};

// Set colour with a brightness multiplier (clamped)
void col(const C& c, float k = 1.0f)
{
    float r = c.r * k, g = c.g * k, b = c.b * k;
    glColor3f(r > 1.f ? 1.f : r, g > 1.f ? 1.f : g, b > 1.f ? 1.f : b);
}

// Small positioned box (uniform colour must be set by caller)
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

// Square frustum (tapered box) standing on y0 with height h.
// bh / th = half-width at bottom / top.  Each face gets its own
// brightness so it looks shaded without any lighting.
void frustum(float y0, float h, float bh, float th, const C& c)
{
    float y1 = y0 + h;
    glBegin(GL_QUADS);
        // front (+Z)  - brightest
        glNormal3f(0.0f, 0.0f, 1.0f);
        col(c, 1.00f);
        glVertex3f(-bh, y0,  bh); glVertex3f( bh, y0,  bh);
        glVertex3f( th, y1,  th); glVertex3f(-th, y1,  th);
        // right (+X)
        glNormal3f(1.0f, 0.0f, 0.0f);
        col(c, 0.86f);
        glVertex3f( bh, y0,  bh); glVertex3f( bh, y0, -bh);
        glVertex3f( th, y1, -th); glVertex3f( th, y1,  th);
        // back (-Z)  - darkest
        glNormal3f(0.0f, 0.0f, -1.0f);
        col(c, 0.60f);
        glVertex3f( bh, y0, -bh); glVertex3f(-bh, y0, -bh);
        glVertex3f(-th, y1, -th); glVertex3f( th, y1, -th);
        // left (-X)
        glNormal3f(-1.0f, 0.0f, 0.0f);
        col(c, 0.72f);
        glVertex3f(-bh, y0, -bh); glVertex3f(-bh, y0,  bh);
        glVertex3f(-th, y1,  th); glVertex3f(-th, y1, -th);
        // top
        glNormal3f(0.0f, 1.0f, 0.0f);
        col(c, 1.08f);
        glVertex3f(-th, y1,  th); glVertex3f( th, y1,  th);
        glVertex3f( th, y1, -th); glVertex3f(-th, y1, -th);
    glEnd();
}

// Cylinder along the Z axis, starting at z0 and extending `len`.
// Alternating segment brightness gives a faceted, shaded look.
void cylinderZ(float cx, float cy, float z0, float radius, float len,
               const C& c, int segs = 16)
{
    float z1 = z0 + len;
    glBegin(GL_QUADS);
    for (int i = 0; i < segs; ++i) {
        float a0 = 2.0f * PI * i / segs;
        float a1 = 2.0f * PI * (i + 1) / segs;
        glNormal3f(cosf((a0 + a1) * 0.5f), sinf((a0 + a1) * 0.5f), 0.0f);
        col(c, (i % 2) ? 0.88f : 1.0f);
        glVertex3f(cx + radius * cosf(a0), cy + radius * sinf(a0), z0);
        glVertex3f(cx + radius * cosf(a1), cy + radius * sinf(a1), z0);
        glVertex3f(cx + radius * cosf(a1), cy + radius * sinf(a1), z1);
        glVertex3f(cx + radius * cosf(a0), cy + radius * sinf(a0), z1);
    }
    glEnd();

    // front disc
    col(c, 1.12f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(cx, cy, z1);
    for (int i = 0; i <= segs; ++i) {
        float a = 2.0f * PI * i / segs;
        glVertex3f(cx + radius * cosf(a), cy + radius * sinf(a), z1);
    }
    glEnd();
}

// Flat half-disc (arch top) in the XY plane at depth z
void archFan(float cx, float cy, float z, float radius,
             const C& c, float k = 1.0f)
{
    col(c, k);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(cx, cy, z);
    for (int i = 0; i <= 14; ++i) {
        float a = PI * i / 14.0f;
        glVertex3f(cx + radius * cosf(a), cy + radius * sinf(a), z);
    }
    glEnd();
}

// Flat full disc in the XY plane at depth z
void discFan(float cx, float cy, float z, float radius,
             const C& c, float k = 1.0f)
{
    col(c, k);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(cx, cy, z);
    for (int i = 0; i <= 18; ++i) {
        float a = 2.0f * PI * i / 18.0f;
        glVertex3f(cx + radius * cosf(a), cy + radius * sinf(a), z);
    }
    glEnd();
}

// ---- Tower / base profile helpers ---------------------------
const float BASE_TOP    = 0.90f;
const float TOWER_BOT   = 0.90f;
const float TOWER_TOP   = 5.90f;
const float BASE_HALF_B = 0.80f;   // half-width at y = 0
const float BASE_HALF_T = 0.68f;   // half-width at y = BASE_TOP
const float TOWER_HALF_B = 0.64f;
const float TOWER_HALF_T = 0.44f;

float baseHalf(float y)
{
    return BASE_HALF_B + (BASE_HALF_T - BASE_HALF_B) * (y / BASE_TOP);
}

float towerHalf(float y)
{
    float t = (y - TOWER_BOT) / (TOWER_TOP - TOWER_BOT);
    return TOWER_HALF_B + (TOWER_HALF_T - TOWER_HALF_B) * t;
}

// ---- Window (drawn on the local +Z face) --------------------
void drawWindow(float y)
{
    float z0 = towerHalf(y);

    // stone lintel + sill
    col(STONE_LITE);
    box(0.0f, y + 0.34f, z0, 0.52f, 0.07f, 0.09f);
    col(STONE);
    box(0.0f, y - 0.30f, z0 + 0.01f, 0.54f, 0.05f, 0.13f);

    // wooden frame
    col(WOOD_DARK);
    box(0.0f, y, z0, 0.38f, 0.54f, 0.06f);

    // glass
    col(GLASS);
    box(0.0f, y, z0 + 0.02f, 0.30f, 0.46f, 0.04f);

    // mullions (cross bars)
    col(WOOD_DARK);
    box(0.0f, y, z0 + 0.05f, 0.30f, 0.03f, 0.02f);
    box(0.0f, y, z0 + 0.05f, 0.03f, 0.46f, 0.02f);

    // shutters with slat lines
    for (int s = -1; s <= 1; s += 2) {
        float sx = s * 0.28f;
        col(SHUTTER);
        box(sx, y, z0 + 0.01f, 0.13f, 0.52f, 0.04f);
        col(SHUTTER_LN);
        for (int k = 0; k < 6; ++k)
            box(sx, y - 0.21f + k * 0.084f, z0 + 0.035f,
                0.11f, 0.018f, 0.012f);
    }
}

void drawWindows()
{
    const float angs[3] = {0.0f, 90.0f, -90.0f};
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        glRotatef(angs[i], 0.0f, 1.0f, 0.0f);
        drawWindow(3.50f);
        drawWindow(4.80f);
        if (i == 0) drawWindow(1.95f);     // extra low window on front
        glPopMatrix();
    }
}

// ---- Front door with stone arch and steps -------------------
void drawDoor()
{
    // stone surround (rect + arch)
    col(STONE_LITE);
    box(0.0f, 0.36f, 0.74f, 0.54f, 0.52f, 0.10f);
    archFan(0.0f, 0.62f, 0.792f, 0.27f, STONE_LITE);

    // door leaf (rect + arch)
    col(WOOD);
    box(0.0f, 0.35f, 0.75f, 0.42f, 0.50f, 0.10f);
    archFan(0.0f, 0.60f, 0.802f, 0.21f, WOOD);

    // planks
    col(WOOD_DARK);
    for (int i = -1; i <= 1; ++i)
        box(i * 0.10f, 0.35f, 0.805f, 0.015f, 0.50f, 0.02f);
    // iron straps + handle
    col(METAL);
    box(0.0f, 0.22f, 0.812f, 0.42f, 0.03f, 0.015f);
    box(0.0f, 0.48f, 0.812f, 0.42f, 0.03f, 0.015f);
    box(0.12f, 0.36f, 0.82f, 0.03f, 0.03f, 0.03f);

    // steps
    col(STONE_LITE);
    box(0.0f, 0.04f, 0.88f, 0.72f, 0.08f, 0.20f);
    col(STONE);
    box(0.0f, 0.02f, 1.00f, 0.86f, 0.04f, 0.14f);
}

// ---- Wooden gallery ring around the tower -------------------
void drawGallery()
{
    const float gy = 2.60f;

    // floor + fascia board
    frustum(gy - 0.02f, 0.10f, 1.00f, 1.00f, WOOD);
    frustum(gy - 0.12f, 0.10f, 0.92f, 0.92f, WOOD_DARK);

    // support brackets under the floor
    col(WOOD_DARK);
    for (int k = 0; k < 4; ++k) {
        glPushMatrix();
        glRotatef(k * 90.0f, 0.0f, 1.0f, 0.0f);
        for (int i = -1; i <= 1; ++i) {
            box(i * 0.55f, gy - 0.20f, 0.72f, 0.06f, 0.10f, 0.40f);
            box(i * 0.55f, gy - 0.30f, 0.60f, 0.06f, 0.10f, 0.16f);
        }
        glPopMatrix();
    }

    // railing: posts + two rails on each side
    for (int k = 0; k < 4; ++k) {
        glPushMatrix();
        glRotatef(k * 90.0f, 0.0f, 1.0f, 0.0f);

        col(WOOD_LITE);
        for (int i = 0; i <= 6; ++i)
            box(-0.90f + i * 0.30f, gy + 0.25f, 0.95f,
                0.05f, 0.50f, 0.05f);

        col(WOOD);
        box(0.0f, gy + 0.50f, 0.95f, 1.90f, 0.05f, 0.06f);
        box(0.0f, gy + 0.28f, 0.95f, 1.90f, 0.03f, 0.03f);

        glPopMatrix();
    }
}

// ---- Cap: overhanging base + curved dome + finial + vane ----
void drawCap()
{
    // wooden collar under the cap
    frustum(5.78f, 0.12f, towerHalf(5.85f) + 0.04f,
                          towerHalf(5.90f) + 0.04f, WOOD_DARK);

    // overhanging trim
    frustum(TOWER_TOP, 0.08f, 0.62f, 0.62f, WOOD_DARK);

    // cap body (slightly tapered)
    frustum(5.98f, 0.55f, 0.56f, 0.52f, PLASTER_LN);

    // round vent on the front of the cap
    discFan(0.0f, 6.34f, 0.545f, 0.09f, WOOD_DARK);

    // dome roof: stacked shrinking frustums following a sqrt profile
    const int   n  = 7;
    const float R  = 0.62f;
    const float H  = 0.78f;
    const float y0 = 6.53f;
    for (int i = 0; i < n; ++i) {
        float t0 = (float)i / n;
        float t1 = (float)(i + 1) / n;
        float h0 = R * sqrtf(1.0f - t0 * t0);
        float h1 = R * sqrtf(1.0f - t1 * t1);
        C rc = ROOF;
        float k = (i % 2) ? 0.90f : 1.0f;     // tile-course banding
        rc.r *= k; rc.g *= k; rc.b *= k;
        frustum(y0 + H * t0, H * (t1 - t0), h0, h1, rc);
    }

    // eave trim around the roof base
    frustum(6.50f, 0.05f, 0.64f, 0.64f, WOOD_DARK);

    // finial + weather vane
    float top = y0 + H;
    col(WOOD_DARK);
    box(0.0f, top + 0.25f, 0.0f, 0.03f, 0.50f, 0.03f);
    col(METAL);
    box(0.0f, top + 0.52f, 0.0f, 0.09f, 0.09f, 0.09f);           // ball
    box(0.0f, top + 0.40f, 0.0f, 0.34f, 0.03f, 0.03f);           // arm X
    box(0.0f, top + 0.40f, 0.0f, 0.03f, 0.03f, 0.34f);           // arm Z
    box(0.06f, top + 0.62f, 0.0f, 0.34f, 0.04f, 0.02f);          // vane
    box(0.24f, top + 0.62f, 0.0f, 0.08f, 0.14f, 0.02f);          // arrow tail
    box(-0.12f, top + 0.62f, 0.0f, 0.06f, 0.10f, 0.02f);         // arrow head
}

// ---- A single blade: stock + lattice frame + patchwork sail -
// Drawn along +Y, from (0,0) to (0,length).
void drawBlade(float length, float width)
{
    const float yS   = 0.55f;             // where the sail starts
    const float yE   = length * 0.97f;    // where the sail ends
    const int   bays = 8;
    const float bayH = (yE - yS) / bays;
    const float xIn  = 0.10f;             // inner edge of sail frame

    // ---- Sail cloth: alternating canvas panels (flat quads) ----
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.0f, 1.0f);
    for (int j = 0; j < bays; ++j) {
        col((j % 2) ? CANVAS_B : CANVAS_A);
        float y0 = yS + j * bayH + 0.01f;
        float y1 = yS + (j + 1) * bayH - 0.01f;
        glVertex3f(xIn + 0.02f,  y0, 0.055f);
        glVertex3f(width - 0.03f, y0, 0.055f);
        glVertex3f(width - 0.03f, y1, 0.055f);
        glVertex3f(xIn + 0.02f,  y1, 0.055f);
    }
    glEnd();

    // ---- Main stock (long spar through the hub) ----
    col(WOOD_DARK);
    box(0.0f, length * 0.5f, 0.0f, 0.10f, length, 0.10f);

    // stock reinforcement near the hub
    col(WOOD);
    box(0.0f, 0.30f, 0.0f, 0.17f, 0.50f, 0.14f);

    // ---- Outer rail + root / tip cross bars ----
    col(WOOD);
    box(width, (yS + yE) * 0.5f, 0.04f,
        0.05f, (yE - yS) + 0.05f, 0.08f);
    box((width + 0.05f) * 0.5f, yS, 0.04f,
        width - 0.05f, 0.05f, 0.08f);
    box((width + 0.05f) * 0.5f, yE, 0.04f,
        width - 0.05f, 0.05f, 0.08f);

    // ---- Horizontal battens (lattice, in front of the cloth) ----
    col(WOOD_DARK);
    for (int j = 1; j < bays; ++j) {
        float y = yS + j * bayH;
        box((xIn + width) * 0.5f, y, 0.075f,
            width - xIn, 0.03f, 0.03f);
    }

    // ---- Vertical battens ----
    col(WOOD_LITE);
    for (int v = 1; v <= 2; ++v) {
        float x = xIn + (width - xIn) * v / 3.0f;
        box(x, (yS + yE) * 0.5f, 0.078f,
            0.025f, yE - yS, 0.03f);
    }

    // ---- Diagonal brace near the root ----
    col(WOOD);
    rotBox(0.30f, 0.34f, 0.04f, 0.03f, 0.50f, 0.05f, -38.0f);

    // ---- Tip cap ----
    col(WOOD_DARK);
    box(0.0f, length + 0.03f, 0.0f, 0.14f, 0.08f, 0.12f);
    col(METAL);
    box(0.0f, length + 0.09f, 0.0f, 0.05f, 0.05f, 0.05f);
}

} // anonymous namespace


// ============================================================
// WINDMILL
// ============================================================
//
//  Local origin is at the base of the tower (y = 0).
//  The whole windmill sits at world position (x, z).
//  The hub/shaft comes out of the cap and rotates around Z.
//  The tower, gallery, cap and vane never move — only the hub
//  and the four blades rotate.
// ============================================================

void drawWindmill(float x, float z, float bladeAngle, float scale)
{
    glPushMatrix();

    // -- World placement + uniform scale --
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);

    // ----------------------------------------------------------
    // 1. STONE FOUNDATION  (plinth + tapered base + courses)
    // ----------------------------------------------------------
    frustum(0.00f, 0.16f, 0.88f, 0.88f, STONE_DARK);      // plinth
    frustum(0.16f, BASE_TOP - 0.16f,
            baseHalf(0.16f), BASE_HALF_T, STONE);         // tapered base

    // horizontal stone courses (thin darker lines)
    for (int i = 0; i < 4; ++i) {
        float y = 0.30f + i * 0.17f;
        float h = baseHalf(y) + 0.008f;
        frustum(y, 0.02f, h, h, STONE_DARK);
    }

    // top trim of the foundation
    frustum(0.87f, 0.09f, 0.72f, 0.72f, STONE_LITE);

    // ----------------------------------------------------------
    // 2. TAPERED PLASTER TOWER (one smooth frustum)
    // ----------------------------------------------------------
    frustum(TOWER_BOT, TOWER_TOP - TOWER_BOT,
            TOWER_HALF_B, TOWER_HALF_T, PLASTER);

    // subtle horizontal plaster lines
    for (float y = 1.30f; y < 5.70f; y += 0.50f) {
        float h = towerHalf(y) + 0.008f;
        frustum(y, 0.02f, h, h, PLASTER_LN);
    }

    // wooden tie bands (lower, middle, upper)
    const float bands[3] = {1.10f, 4.15f, 5.55f};
    for (int i = 0; i < 3; ++i) {
        float h = towerHalf(bands[i] + 0.04f) + 0.03f;
        frustum(bands[i], 0.08f, h, h, WOOD_DARK);
    }

    // ----------------------------------------------------------
    // 3. DOOR, WINDOWS, GALLERY
    // ----------------------------------------------------------
    drawDoor();
    drawWindows();
    drawGallery();

    // ----------------------------------------------------------
    // 4. CAP  (collar, trim, body, dome roof, vane)
    // ----------------------------------------------------------
    drawCap();

    // ----------------------------------------------------------
    // 5. SHAFT  (fixed, sticks out of the cap front)
    // ----------------------------------------------------------
    const float hubY = 6.25f;
    const float hubZ = 1.18f;

    // bearing plate on the cap face
    col(METAL, 0.9f);
    box(0.0f, hubY, 0.55f, 0.34f, 0.34f, 0.06f);

    // iron shaft
    cylinderZ(0.0f, hubY, 0.45f, 0.10f, 0.62f, METAL);

    // ----------------------------------------------------------
    // 6. ROTATING PART: hub + four blades
    // ----------------------------------------------------------
    glPushMatrix();

        glTranslatef(0.0f, hubY, hubZ);
        glRotatef(bladeAngle, 0.0f, 0.0f, 1.0f);

        // wooden hub drum
        cylinderZ(0.0f, 0.0f, -0.12f, 0.24f, 0.26f, WOOD_DARK, 16);

        // cross block where the stocks meet
        col(WOOD);
        box(0.0f, 0.0f, 0.0f, 0.46f, 0.46f, 0.16f);

        // metal ring + front cap
        cylinderZ(0.0f, 0.0f, 0.14f, 0.17f, 0.05f, METAL, 14);
        cylinderZ(0.0f, 0.0f, 0.19f, 0.09f, 0.09f, WOOD_LITE, 12);

        // four blades
        const float bladeLen   = 2.60f;
        const float bladeWidth = 0.60f;

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
