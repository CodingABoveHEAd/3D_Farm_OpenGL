#include "objects/Vegetation.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>

namespace Vegetation {

// ------------------------------------------------------------
// Internal helpers
// ------------------------------------------------------------
namespace {

// Quick box with position
void drawBox(
    float x, float y, float z,
    float w, float h, float d)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCube(w, h, d);
    glPopMatrix();
}

// Rotated + scaled box (used for leaves and clusters)
void drawLeaf(
    float x, float y, float z,
    float sx, float sy, float sz,
    float rotZ)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotZ, 0.0f, 0.0f, 1.0f);
    glScalef(sx, sy, sz);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

// Rounded foliage blob: main cube + 4 chamfer cubes to soften silhouette
void drawBlob(
    float x, float y, float z,
    float sx, float sy, float sz,
    float rotY = 0.0f)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glScalef(sx, sy, sz);

    // Core
    Primitives::drawCube(2.0f, 1.8f, 2.0f);

    // Softening corners (smaller cubes on each side)
    drawBox(-0.75f, 0.0f, 0.0f, 1.2f, 1.2f, 1.2f);
    drawBox( 0.75f, 0.0f, 0.0f, 1.2f, 1.2f, 1.2f);
    drawBox( 0.0f, 0.0f,-0.75f, 1.2f, 1.2f, 1.2f);
    drawBox( 0.0f, 0.0f, 0.75f, 1.2f, 1.2f, 1.2f);

    // Top cap
    drawBox(0.0f, 0.75f, 0.0f, 1.4f, 0.9f, 1.4f);

    glPopMatrix();
}

} // anonymous namespace


// ============================================================
// TREE  — layered, organic, 4-branch, rounded canopy
// ============================================================

void drawTree(float x, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);

    // --------------------------------------------------------
    // 1. Root flare (wide base)
    // --------------------------------------------------------
    glColor3f(0.24f, 0.10f, 0.03f);
    glPushMatrix();
    glTranslatef(0.0f, 0.15f, 0.0f);
    glScalef(1.1f, 0.30f, 1.1f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // --------------------------------------------------------
    // 2. Main trunk (3 stacked boxes, tapered)
    // --------------------------------------------------------
    glColor3f(0.30f, 0.13f, 0.045f);

    glPushMatrix();
    glTranslatef(0.0f, 0.90f, 0.0f);
    glScalef(0.90f, 1.8f, 0.90f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 2.20f, 0.0f);
    glScalef(0.80f, 1.0f, 0.80f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 3.00f, 0.0f);
    glScalef(0.70f, 0.8f, 0.70f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // --------------------------------------------------------
    // 3. Bark detail (thin dark strips on trunk surface)
    // --------------------------------------------------------
    glColor3f(0.20f, 0.085f, 0.03f);

    glPushMatrix();
    glTranslatef(-0.15f, 1.70f, 0.46f);
    glScalef(0.10f, 2.6f, 0.06f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.46f, 1.60f, 0.10f);
    glScalef(0.06f, 2.4f, 0.10f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.46f, 1.65f, -0.05f);
    glScalef(0.06f, 2.5f, 0.08f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // --------------------------------------------------------
    // 4. Light-side highlight strip
    // --------------------------------------------------------
    glColor3f(0.42f, 0.21f, 0.08f);
    glPushMatrix();
    glTranslatef(-0.22f, 1.30f, 0.47f);
    glScalef(0.28f, 1.9f, 0.05f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // --------------------------------------------------------
    // 5. Branches — 2 main + 2 sub
    // --------------------------------------------------------
    glColor3f(0.27f, 0.115f, 0.04f);

    glPushMatrix();
    glTranslatef(-0.55f, 2.55f, 0.0f);
    glRotatef(-42.0f, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(0.17f, 1.55f, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.55f, 2.65f, 0.0f);
    glRotatef(42.0f, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(0.17f, 1.50f, 8);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.40f, 3.05f, -0.30f);
    glRotatef(-25.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(-20.0f, 1.0f, 0.0f, 0.0f);
    Primitives::drawCylinder(0.13f, 1.20f, 7);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.40f, 3.15f, 0.30f);
    glRotatef(28.0f, 0.0f, 0.0f, 1.0f);
    glRotatef(18.0f, 1.0f, 0.0f, 0.0f);
    Primitives::drawCylinder(0.13f, 1.15f, 7);
    glPopMatrix();

    // --------------------------------------------------------
    // 6. Foliage — layered blobs, dark bottom → bright top
    // --------------------------------------------------------
    glColor3f(0.07f, 0.26f, 0.05f);
    drawBlob(-0.90f, 3.55f, 0.00f, 0.85f, 0.75f, 0.85f, -15.0f);
    drawBlob( 0.90f, 3.60f, 0.00f, 0.85f, 0.75f, 0.85f,  20.0f);
    drawBlob( 0.00f, 3.45f, -0.85f, 0.80f, 0.70f, 0.80f, 10.0f);

    glColor3f(0.11f, 0.40f, 0.06f);
    drawBlob( 0.00f, 4.05f, 0.00f, 1.35f, 1.00f, 1.35f,   0.0f);
    drawBlob(-1.00f, 3.95f, 0.20f, 0.95f, 0.85f, 0.95f,  10.0f);
    drawBlob( 1.00f, 4.00f,-0.20f, 0.95f, 0.85f, 0.95f, -10.0f);

    glColor3f(0.15f, 0.48f, 0.08f);
    drawBlob(-0.45f, 4.65f, 0.30f, 0.85f, 0.75f, 0.85f,  15.0f);
    drawBlob( 0.50f, 4.70f,-0.25f, 0.85f, 0.75f, 0.85f, -12.0f);
    drawBlob( 0.00f, 4.75f, 0.00f, 0.90f, 0.70f, 0.90f,   0.0f);

    glColor3f(0.22f, 0.58f, 0.12f);
    drawBlob( 0.10f, 5.30f, 0.00f, 0.70f, 0.55f, 0.70f,   8.0f);
    drawBlob(-0.30f, 5.15f, 0.25f, 0.55f, 0.45f, 0.55f, -20.0f);

    // --------------------------------------------------------
    // 7. Sun-lit highlights
    // --------------------------------------------------------
    glColor3f(0.30f, 0.68f, 0.18f);
    drawLeaf(-0.40f, 5.45f, 0.35f, 0.55f, 0.30f, 0.35f, -15.0f);
    drawLeaf( 0.55f, 5.20f, 0.30f, 0.50f, 0.28f, 0.32f,  12.0f);
    drawLeaf( 0.00f, 5.65f, 0.10f, 0.55f, 0.32f, 0.40f,   0.0f);
    drawLeaf(-0.70f, 4.40f, 0.90f, 0.60f, 0.35f, 0.35f, -25.0f);
    drawLeaf( 0.75f, 4.55f, 0.80f, 0.55f, 0.32f, 0.35f,  22.0f);

    // --------------------------------------------------------
    // 8. Fruits
    // --------------------------------------------------------
    glColor3f(0.88f, 0.22f, 0.12f);
    drawBox(-0.95f, 3.95f, 1.05f, 0.18f, 0.18f, 0.18f);
    drawBox( 0.85f, 4.10f, 1.05f, 0.16f, 0.16f, 0.16f);

    glColor3f(0.95f, 0.72f, 0.18f);
    drawBox(-0.20f, 4.85f, 0.95f, 0.15f, 0.15f, 0.15f);
    drawBox( 0.45f, 4.55f, 1.10f, 0.14f, 0.14f, 0.14f);

    glPopMatrix();
}


// ============================================================
// CROP  — wrapped stem, staggered leaves, detailed grain head
// ============================================================

void drawCrop(float x, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, 0.08f, z);
    glScalef(scale, scale, scale);

    // --------------------------------------------------------
    // 1. Soil mound at base
    // --------------------------------------------------------
    glColor3f(0.28f, 0.18f, 0.08f);
    glPushMatrix();
    glTranslatef(0.0f, 0.02f, 0.0f);
    glScalef(1.0f, 0.25f, 1.0f);
    Primitives::drawCylinder(0.34f, 1.0f, 8);
    glPopMatrix();

    // --------------------------------------------------------
    // 2. Main stem  (thicker at bottom, thinner at top)
    // --------------------------------------------------------
    glColor3f(0.10f, 0.36f, 0.05f);
    glPushMatrix();
    glTranslatef(0.0f, 0.45f, 0.0f);
    glScalef(0.16f, 0.90f, 0.16f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glColor3f(0.13f, 0.42f, 0.06f);
    glPushMatrix();
    glTranslatef(0.0f, 0.95f, 0.0f);
    glScalef(0.12f, 0.55f, 0.12f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // Stem highlight (light side)
    glColor3f(0.18f, 0.50f, 0.08f);
    glPushMatrix();
    glTranslatef(0.05f, 0.55f, 0.05f);
    glScalef(0.05f, 0.80f, 0.05f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // --------------------------------------------------------
    // 3. Bottom leaves (largest, drooping outward + down)
    // --------------------------------------------------------
    glColor3f(0.12f, 0.40f, 0.05f);

    glPushMatrix();
    glTranslatef(-0.30f, 0.42f, 0.0f);
    glRotatef(-55.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.70f, 0.14f, 0.20f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.30f, 0.42f, 0.0f);
    glRotatef(55.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.70f, 0.14f, 0.20f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // Back leaves
    glPushMatrix();
    glTranslatef(0.0f, 0.45f, -0.28f);
    glRotatef(-50.0f, 1.0f, 0.0f, 0.0f);
    glScalef(0.20f, 0.14f, 0.65f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // --------------------------------------------------------
    // 4. Middle leaves (medium, angled ~35°)
    // --------------------------------------------------------
    glColor3f(0.17f, 0.48f, 0.07f);

    drawLeaf(-0.24f, 0.65f, 0.0f, 0.55f, 0.12f, 0.17f, -38.0f);
    drawLeaf( 0.24f, 0.65f, 0.0f, 0.55f, 0.12f, 0.17f,  38.0f);
    drawLeaf( 0.0f, 0.70f, 0.22f, 0.17f, 0.12f, 0.55f,   0.0f);

    // --------------------------------------------------------
    // 5. Upper leaves (smaller, more upright)
    // --------------------------------------------------------
    glColor3f(0.22f, 0.55f, 0.09f);

    drawLeaf(-0.18f, 0.88f, 0.0f, 0.42f, 0.10f, 0.15f, -28.0f);
    drawLeaf( 0.18f, 0.88f, 0.0f, 0.42f, 0.10f, 0.15f,  28.0f);

    // Very top flag leaf (small, near-vertical)
    glColor3f(0.28f, 0.62f, 0.11f);
    drawLeaf(-0.10f, 1.05f, 0.0f, 0.30f, 0.08f, 0.12f, -15.0f);
    drawLeaf( 0.10f, 1.05f, 0.0f, 0.30f, 0.08f, 0.12f,  15.0f);

    // --------------------------------------------------------
    // 6. Grain head  (husk + kernels)
    // --------------------------------------------------------
    glColor3f(0.72f, 0.58f, 0.10f);   // darker husk
    glPushMatrix();
    glTranslatef(0.0f, 1.22f, 0.0f);
    glScalef(0.26f, 0.40f, 0.22f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();

    // Kernel row — stacked small bright cubes
    glColor3f(0.92f, 0.74f, 0.20f);
    drawBox( 0.00f, 1.16f, 0.14f, 0.10f, 0.12f, 0.08f);
    drawBox( 0.00f, 1.28f, 0.14f, 0.10f, 0.12f, 0.08f);
    drawBox( 0.00f, 1.40f, 0.12f, 0.09f, 0.11f, 0.08f);

    drawBox(-0.08f, 1.22f, 0.10f, 0.08f, 0.10f, 0.07f);
    drawBox( 0.08f, 1.22f, 0.10f, 0.08f, 0.10f, 0.07f);
    drawBox(-0.07f, 1.34f, 0.09f, 0.07f, 0.09f, 0.06f);
    drawBox( 0.07f, 1.34f, 0.09f, 0.07f, 0.09f, 0.06f);

    // Light speck on top (sun catch)
    glColor3f(0.98f, 0.85f, 0.35f);
    drawBox(0.0f, 1.48f, 0.05f, 0.06f, 0.08f, 0.06f);

    // --------------------------------------------------------
    // 7. Awns / bristles (thin spikes above grain head)
    // --------------------------------------------------------
    glColor3f(0.85f, 0.68f, 0.18f);
    drawBox(-0.05f, 1.58f, 0.0f, 0.03f, 0.20f, 0.03f);
    drawBox( 0.05f, 1.58f, 0.0f, 0.03f, 0.20f, 0.03f);
    drawBox( 0.00f, 1.62f, 0.05f, 0.03f, 0.18f, 0.03f);

    glPopMatrix();
}

} // namespace Vegetation