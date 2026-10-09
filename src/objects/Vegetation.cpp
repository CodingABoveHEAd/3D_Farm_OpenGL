#include "objects/Vegetation.h"

#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace Vegetation {

// ------------------------------------------------------------
// Internal helpers  (NO OpenGL lighting is used anywhere)
//
// Depth is faked with colour only:
//   * per-vertex colours (smooth gradients dark -> light)
//   * a fixed "front is brighter, back is darker" multiplier
//   * random-looking per-facet variation on foliage
// ------------------------------------------------------------
namespace {

const float PI = 3.14159265f;

struct C { float r, g, b; };

// ---- Palette ------------------------------------------------
const C BARK       = {0.30f, 0.13f, 0.045f};
const C BARK_DARK  = {0.19f, 0.08f, 0.028f};
const C BARK_LITE  = {0.44f, 0.22f, 0.09f};
const C ROOT       = {0.26f, 0.11f, 0.04f};

const C LEAF_1     = {0.07f, 0.26f, 0.05f};   // lowest / darkest layer
const C LEAF_2     = {0.11f, 0.40f, 0.06f};
const C LEAF_3     = {0.15f, 0.48f, 0.08f};
const C LEAF_4     = {0.22f, 0.58f, 0.12f};   // top layer
const C LEAF_TUFT  = {0.30f, 0.68f, 0.18f};

const C APPLE_RED  = {0.88f, 0.22f, 0.12f};
const C APPLE_GOLD = {0.95f, 0.72f, 0.18f};

const C GRASS_D    = {0.09f, 0.30f, 0.05f};
const C GRASS_L    = {0.32f, 0.64f, 0.15f};

const C SOIL       = {0.28f, 0.18f, 0.08f};
const C SOIL_DARK  = {0.20f, 0.12f, 0.05f};

const C STEM_A     = {0.13f, 0.42f, 0.06f};
const C STEM_B     = {0.17f, 0.49f, 0.08f};
const C NODE       = {0.20f, 0.36f, 0.08f};
const C SHEATH     = {0.16f, 0.46f, 0.07f};
const C BLADE_BASE = {0.10f, 0.36f, 0.05f};
const C BLADE_TIP  = {0.34f, 0.66f, 0.14f};

const C GRAIN_A    = {0.92f, 0.74f, 0.20f};
const C GRAIN_B    = {0.80f, 0.60f, 0.12f};
const C AWN        = {0.88f, 0.72f, 0.24f};

// ---- Small utilities ----------------------------------------
void col(const C& c, float k = 1.0f)
{
    float r = c.r * k, g = c.g * k, b = c.b * k;
    glColor3f(r > 1.f ? 1.f : r, g > 1.f ? 1.f : g, b > 1.f ? 1.f : b);
}

C lerpC(const C& a, const C& b, float t)
{
    return { a.r + (b.r - a.r) * t,
             a.g + (b.g - a.g) * t,
             a.b + (b.b - a.b) * t };
}

// Deterministic pseudo-random 0..1 (so shapes never flicker)
float hash1(int a, int b = 0, int c = 0)
{
    float s = sinf(a * 12.9898f + b * 78.233f + c * 37.719f) * 43758.5453f;
    return s - floorf(s);
}

// Baked "side brightness" around a vertical axis.
// a = 0 is the front (+Z), a = 90deg is the right (+X).
float sideShade(float a)
{
    return 0.80f + 0.20f * cosf(a) + 0.06f * sinf(a);
}

// Tapered cylinder standing on y = y0, along +Y.
// Vertex colours vary around the circumference.
void cylinderY(float r0, float r1, float h, int segs, const C& c,
               bool capTop = false)
{
    const float slope = h > 1e-5f ? (r0 - r1) / h : 0.0f;
    glBegin(GL_QUADS);
    for (int i = 0; i < segs; ++i) {
        float a0 = 2.0f * PI * i / segs;
        float a1 = 2.0f * PI * (i + 1) / segs;
        float k0 = sideShade(a0), k1 = sideShade(a1);

        col(c, k0 * 0.92f);
        glNormal3f(sinf(a0), slope, cosf(a0));
        glVertex3f(r0 * sinf(a0), 0.0f, r0 * cosf(a0));
        col(c, k1 * 0.92f);
        glNormal3f(sinf(a1), slope, cosf(a1));
        glVertex3f(r0 * sinf(a1), 0.0f, r0 * cosf(a1));
        col(c, k1 * 1.05f);
        glNormal3f(sinf(a1), slope, cosf(a1));
        glVertex3f(r1 * sinf(a1), h,    r1 * cosf(a1));
        col(c, k0 * 1.05f);
        glNormal3f(sinf(a0), slope, cosf(a0));
        glVertex3f(r1 * sinf(a0), h,    r1 * cosf(a0));
    }
    glEnd();

    if (capTop) {
        col(c, 1.10f);
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(0.0f, h, 0.0f);
        for (int i = segs; i >= 0; --i) {
            float a = 2.0f * PI * i / segs;
            glVertex3f(r1 * sinf(a), h, r1 * cosf(a));
        }
        glEnd();
    }
}

// Tapered cylinder between two arbitrary points
void cylinderBetween(float x0, float y0, float z0,
                     float x1, float y1, float z1,
                     float r0, float r1, int segs, const C& c)
{
    float dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    float L = sqrtf(dx * dx + dy * dy + dz * dz);
    if (L < 1e-5f) return;

    glPushMatrix();
    glTranslatef(x0, y0, z0);

    float ax = dz, az = -dx;                 // axis = Y x d
    float alen = sqrtf(ax * ax + az * az);
    float ang = acosf(dy / L) * 180.0f / PI;
    if (alen > 1e-5f)      glRotatef(ang, ax, 0.0f, az);
    else if (dy < 0.0f)    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);

    cylinderY(r0, r1, L, segs, c);
    glPopMatrix();
}

// Low-poly ellipsoid "blob" with jittered vertices (organic look).
// Colour ramps from dark (bottom) to bright (top) per vertex, with
// slight per-facet variation, so no lighting is needed.
void blob(float cx, float cy, float cz,
          float rx, float ry, float rz,
          const C& base, int seed, float rotY = 0.0f,
          int rings = 6, int slices = 9, float jit = 0.14f)
{
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    auto pos = [&](int i, int j, float out[3]) {
        float th = PI * i / rings;
        float ph = 2.0f * PI * (j % slices) / slices;
        float jt = 1.0f + jit * (hash1(i, j % slices, seed) - 0.5f) * 2.0f;
        if (i == 0 || i == rings) jt = 1.0f;
        out[0] = rx * sinf(th) * sinf(ph) * jt;
        out[1] = ry * cosf(th) * jt;
        out[2] = rz * sinf(th) * cosf(ph) * jt;
    };
    auto shade = [&](int i, int j) -> float {
        float th = PI * i / rings;
        float ph = 2.0f * PI * (j % slices) / slices;
        float up = 0.5f + 0.5f * cosf(th);            // 1 top .. 0 bottom
        float front = cosf(ph) * sinf(th);            // +1 facing front
        return 0.66f + 0.50f * up + 0.10f * front;
    };

    glBegin(GL_QUADS);
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < slices; ++j) {
            float f = 0.90f + 0.20f * hash1(i, j, seed + 7);
            float p[4][3];
            pos(i,     j,     p[0]);
            pos(i + 1, j,     p[1]);
            pos(i + 1, j + 1, p[2]);
            pos(i,     j + 1, p[3]);
            int   ii[4] = {i, i + 1, i + 1, i};
            int   jj[4] = {j, j,     j + 1, j + 1};
            for (int v = 0; v < 4; ++v) {
                const float th = PI * ii[v] / rings;
                const float ph = 2.0f * PI * (jj[v] % slices) / slices;
                float nx = sinf(th) * sinf(ph) / std::max(rx, 1e-5f);
                float ny = cosf(th) / std::max(ry, 1e-5f);
                float nz = sinf(th) * cosf(ph) / std::max(rz, 1e-5f);
                const float nl = sqrtf(nx * nx + ny * ny + nz * nz);
                glNormal3f(nx / nl, ny / nl, nz / nl);
                col(base, shade(ii[v], jj[v]) * f);
                glVertex3f(p[v][0], p[v][1], p[v][2]);
            }
        }
    }
    glEnd();

    glPopMatrix();
}

// Octahedron (used as a small sun-lit leaf tuft / knot)
void octa(float cx, float cy, float cz,
          float sx, float sy, float sz,
          const C& c, float rotY = 0.0f, float rotZ = 0.0f)
{
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotZ, 0.0f, 0.0f, 1.0f);
    glScalef(sx, sy, sz);

    const float T[3]  = {0, 1, 0},  B[3]  = {0, -1, 0};
    const float XP[3] = {1, 0, 0},  XN[3] = {-1, 0, 0};
    const float ZP[3] = {0, 0, 1},  ZN[3] = {0, 0, -1};

    struct Tri { const float *a, *b, *c; float k; };
    const Tri tris[8] = {
        {T, ZP, XP, 1.20f}, {T, XP, ZN, 1.00f},
        {T, ZN, XN, 0.85f}, {T, XN, ZP, 1.05f},
        {B, XP, ZP, 0.85f}, {B, ZP, XN, 0.75f},
        {B, XN, ZN, 0.60f}, {B, ZN, XP, 0.70f},
    };

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 8; ++i) {
        const float ux = tris[i].b[0] - tris[i].a[0];
        const float uy = tris[i].b[1] - tris[i].a[1];
        const float uz = tris[i].b[2] - tris[i].a[2];
        const float vx = tris[i].c[0] - tris[i].a[0];
        const float vy = tris[i].c[1] - tris[i].a[1];
        const float vz = tris[i].c[2] - tris[i].a[2];
        glNormal3f(uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx);
        col(c, tris[i].k);
        glVertex3fv(tris[i].a);
        glVertex3fv(tris[i].b);
        glVertex3fv(tris[i].c);
    }
    glEnd();

    glPopMatrix();
}

// A tuft of grass blades (double-sided triangles, dark base -> light tip)
void grassTuft(float x, float z, float h, int seed)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    for (int b = 0; b < 4; ++b) {
        float yaw   = hash1(seed, b, 1) * 360.0f;
        float lean  = (0.05f + 0.12f * hash1(seed, b, 2)) * h * 4.0f;
        float bh    = h * (0.65f + 0.45f * hash1(seed, b, 3));
        float w     = 0.030f * (0.8f + 0.5f * hash1(seed, b, 4)) * (h * 3.0f);

        glPushMatrix();
        glRotatef(yaw, 0.0f, 1.0f, 0.0f);

        glBegin(GL_TRIANGLES);
        // front face
        glNormal3f(0.0f, 0.0f, 1.0f);
        col(GRASS_D);
        glVertex3f(-w, 0.0f, 0.0f);
        glVertex3f( w, 0.0f, 0.0f);
        col(GRASS_L);
        glVertex3f(lean, bh, 0.0f);
        // back face (so it shows from both sides)
        glNormal3f(0.0f, 0.0f, -1.0f);
        col(GRASS_D, 0.85f);
        glVertex3f( w, 0.0f, 0.0f);
        glVertex3f(-w, 0.0f, 0.0f);
        col(GRASS_L, 0.85f);
        glVertex3f(lean, bh, 0.0f);
        glEnd();

        glPopMatrix();
    }
    glPopMatrix();
}

// ---- Tree-specific helpers ----------------------------------

// Trunk radius at height y (piecewise linear taper)
float trunkR(float y)
{
    if (y < 1.2f) return 0.52f + (0.42f - 0.52f) * (y / 1.2f);
    if (y < 2.4f) return 0.42f + (0.35f - 0.42f) * ((y - 1.2f) / 1.2f);
    float t = (y - 2.4f) / 1.0f;
    if (t > 1.0f) t = 1.0f;
    return 0.35f + (0.30f - 0.35f) * t;
}

// A thin vertical bark ridge that wobbles as it climbs the trunk
void barkRidge(float aC, float yA, float yB, const C& c, float width)
{
    const float step = 0.5f;
    glBegin(GL_QUADS);
    for (float y = yA; y < yB; y += step) {
        float a0 = aC + 0.12f * sinf(y * 3.0f + aC * 7.0f);
        float a1 = aC + 0.12f * sinf((y + step) * 3.0f + aC * 7.0f);
        float r0 = trunkR(y) + 0.012f;
        float r1 = trunkR(y + step) + 0.012f;

        col(c, sideShade(a0));
        glNormal3f(sinf(a0), 0.0f, cosf(a0));
        glVertex3f(r0 * sinf(a0 - width), y,        r0 * cosf(a0 - width));
        glVertex3f(r0 * sinf(a0 + width), y,        r0 * cosf(a0 + width));
        glVertex3f(r1 * sinf(a1 + width), y + step, r1 * cosf(a1 + width));
        glVertex3f(r1 * sinf(a1 - width), y + step, r1 * cosf(a1 - width));
    }
    glEnd();
}

struct BlobDef { float x, y, z, sx, sy, sz, rot; };

void foliageLayer(const BlobDef* defs, int n, const C& c, int seedBase)
{
    for (int i = 0; i < n; ++i) {
        const BlobDef& b = defs[i];
        // radii slightly larger than the old cube half-sizes
        blob(b.x, b.y, b.z,
             b.sx * 1.15f, b.sy * 1.00f, b.sz * 1.15f,
             c, seedBase + i * 11, b.rot);
    }
}

// ---- Crop-specific helpers ----------------------------------

// Stem bends gently forward as it rises
float stemX(float y) { return 0.06f * y * y; }

// One arched, V-folded leaf blade lying along local +X.
// Both faces are drawn so it's visible from any side.
void leafBlade(float len, float wid, float curl)
{
    const int N = 5;

    auto widthAt = [&](float s) {
        float taper = powf(1.0f - s * 0.97f, 0.6f);
        float root  = fminf(1.0f, 0.55f + s * 3.0f);
        return wid * taper * root;
    };
    auto P = [&](float s, int side, float& x, float& y, float& z) {
        x = len * s;
        y = len * (0.50f * s - curl * 0.55f * s * s);
        if (side == 0) y += wid * 0.20f * (1.0f - s);   // raised mid-rib
        z = side * widthAt(s);
    };

    for (int pass = 0; pass < 2; ++pass) {
        glBegin(GL_QUADS);
        for (int i = 0; i < N; ++i) {
            float s0 = (float)i / N, s1 = (float)(i + 1) / N;
            for (int half = 0; half < 2; ++half) {
                int sa = (half == 0) ? -1 : 0;
                int sb = (half == 0) ?  0 : 1;

                float v[4][3]; float k[4]; float s[4];
                P(s0, sa, v[0][0], v[0][1], v[0][2]); s[0] = s0; k[0] = (sa == 0) ? 1.12f : 1.0f;
                P(s0, sb, v[1][0], v[1][1], v[1][2]); s[1] = s0; k[1] = (sb == 0) ? 1.12f : 1.0f;
                P(s1, sb, v[2][0], v[2][1], v[2][2]); s[2] = s1; k[2] = (sb == 0) ? 1.12f : 1.0f;
                P(s1, sa, v[3][0], v[3][1], v[3][2]); s[3] = s1; k[3] = (sa == 0) ? 1.12f : 1.0f;

                for (int q = 0; q < 4; ++q) {
                    int idx = (pass == 0) ? q : 3 - q;
                    glNormal3f(0.0f, pass == 0 ? 1.0f : -1.0f, 0.0f);
                    col(lerpC(BLADE_BASE, BLADE_TIP, s[idx]),
                        k[idx] * (pass == 0 ? 1.0f : 0.85f));
                    glVertex3f(v[idx][0], v[idx][1], v[idx][2]);
                }
            }
        }
        glEnd();
    }
}

// The wheat ear: rachis + spiral spikelets + long awns
void drawGrainHead(float hy)
{
    glPushMatrix();
    glTranslatef(stemX(hy), 0.0f, 0.0f);

    // central axis
    cylinderBetween(0.0f, hy - 0.02f, 0.0f, 0.0f, hy + 0.42f, 0.0f,
                    0.022f, 0.010f, 5, NODE);

    // spikelets in a spiral (golden-angle-ish), with awns
    for (int i = 0; i < 9; ++i) {
        float y   = hy + 0.04f + i * 0.04f;
        float a   = i * 2.4f;
        float off = 0.045f * (1.0f - i * 0.04f);
        float sx  = sinf(a) * off;
        float sz  = cosf(a) * off;

        C g = lerpC(GRAIN_A, GRAIN_B, hash1(i, 3, 9));
        blob(sx, y + 0.03f, sz, 0.030f, 0.058f, 0.026f,
             g, 100 + i, a * 180.0f / PI, 4, 6, 0.08f);

        // awn (long thin bristle, fanning outward & up)
        float ex = sx + sinf(a) * 0.12f;
        float ey = y + 0.07f + 0.30f - 0.015f * i;
        float ez = sz + cosf(a) * 0.12f;
        cylinderBetween(sx, y + 0.07f, sz, ex, ey, ez,
                        0.008f, 0.003f, 4, AWN);
    }

    // terminal grain + splayed top awns
    blob(0.0f, hy + 0.44f, 0.0f, 0.028f, 0.060f, 0.026f,
         GRAIN_A, 140, 0.0f, 4, 6, 0.08f);
    cylinderBetween(0.0f, hy + 0.49f, 0.0f, -0.05f, hy + 0.80f,  0.00f,
                    0.008f, 0.003f, 4, AWN);
    cylinderBetween(0.0f, hy + 0.49f, 0.0f,  0.05f, hy + 0.78f,  0.02f,
                    0.008f, 0.003f, 4, AWN);
    cylinderBetween(0.0f, hy + 0.49f, 0.0f,  0.00f, hy + 0.82f, -0.05f,
                    0.008f, 0.003f, 4, AWN);

    glPopMatrix();
}

// One complete wheat stalk standing at the local origin
void drawStalk()
{
    const float ys[5] = {0.00f, 0.32f, 0.66f, 0.98f, 1.28f};
    const float rs[5] = {0.055f, 0.048f, 0.040f, 0.033f, 0.027f};

    // segmented, gently curved stem
    for (int i = 0; i < 4; ++i) {
        cylinderBetween(stemX(ys[i]),     ys[i],     0.0f,
                        stemX(ys[i + 1]), ys[i + 1], 0.0f,
                        rs[i], rs[i + 1], 6, (i % 2) ? STEM_B : STEM_A);
    }

    // nodes (little swellings at the joints)
    for (int i = 1; i <= 3; ++i) {
        glPushMatrix();
        glTranslatef(stemX(ys[i]), ys[i] - 0.02f, 0.0f);
        cylinderY(rs[i] + 0.014f, rs[i] + 0.014f, 0.04f, 6, NODE, true);
        glPopMatrix();
    }

    // leaf sheath hugging the lower stem
    glPushMatrix();
    glTranslatef(stemX(0.05f), 0.05f, 0.0f);
    cylinderY(0.075f, 0.055f, 0.42f, 7, SHEATH, true);
    glPopMatrix();

    // arched leaves, spiralling up the stem
    struct Lf { float y, yaw, len, wid, curl; };
    const Lf leaves[] = {
        {0.18f,  20.0f, 0.85f, 0.110f, 0.90f},
        {0.30f, 200.0f, 0.80f, 0.100f, 0.90f},
        {0.55f, 110.0f, 0.68f, 0.090f, 0.80f},
        {0.72f, 290.0f, 0.62f, 0.085f, 0.80f},
        {0.95f,   0.0f, 0.45f, 0.070f, 0.70f},
        {1.05f, 180.0f, 0.36f, 0.060f, 0.60f},
    };
    for (int i = 0; i < 6; ++i) {
        glPushMatrix();
        glTranslatef(stemX(leaves[i].y), leaves[i].y, 0.0f);
        glRotatef(leaves[i].yaw, 0.0f, 1.0f, 0.0f);
        leafBlade(leaves[i].len, leaves[i].wid, leaves[i].curl);
        glPopMatrix();
    }

    // grain head on top
    drawGrainHead(ys[4]);
}

} // anonymous namespace


// ============================================================
// TREE — flared roots, tapered bark trunk, forked branches,
//        organic low-poly canopy, sun tufts, apples, grass
// ============================================================

void drawTree(float x, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);

    // --------------------------------------------------------
    // 1. Grass tufts around the base
    // --------------------------------------------------------
    for (int i = 0; i < 9; ++i) {
        float a = i * 2.0f * PI / 9.0f + hash1(i, 1) * 0.5f;
        float r = 0.85f + 0.45f * hash1(i, 2);
        grassTuft(sinf(a) * r, cosf(a) * r, 0.22f + 0.14f * hash1(i, 3), i + 1);
    }

    // --------------------------------------------------------
    // 2. Root buttresses (radiate from the trunk into the soil)
    // --------------------------------------------------------
    for (int k = 0; k < 5; ++k) {
        glPushMatrix();
        glRotatef(k * 72.0f + 15.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.22f, 0.38f, 0.0f);
        glRotatef(-105.0f, 0.0f, 0.0f, 1.0f);       // outward & slightly down
        cylinderY(0.17f, 0.04f, 0.95f, 6, ROOT);
        glPopMatrix();
    }

    // --------------------------------------------------------
    // 3. Trunk: three tapered sections
    // --------------------------------------------------------
    cylinderY(0.52f, 0.42f, 1.2f, 10, BARK);

    glPushMatrix();
    glTranslatef(0.0f, 1.2f, 0.0f);
    cylinderY(0.42f, 0.35f, 1.2f, 10, BARK);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 2.4f, 0.0f);
    cylinderY(0.35f, 0.30f, 1.0f, 10, BARK, true);
    glPopMatrix();

    // --------------------------------------------------------
    // 4. Bark detail: dark ridges, one light ridge, rings, knot
    // --------------------------------------------------------
    const float ridgeAng[6] = {0.30f, 1.30f, 2.20f, 3.60f, 4.40f, 5.50f};
    for (int k = 0; k < 6; ++k)
        barkRidge(ridgeAng[k], 0.25f, 3.00f, BARK_DARK, 0.09f);

    barkRidge(-0.55f, 0.40f, 2.90f, BARK_LITE, 0.13f);   // light-side strip

    const float ringY[3] = {0.90f, 1.80f, 2.60f};
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
        glTranslatef(0.0f, ringY[i], 0.0f);
        cylinderY(trunkR(ringY[i]) + 0.015f, trunkR(ringY[i] + 0.05f) + 0.015f,
                  0.05f, 10, BARK_DARK);
        glPopMatrix();
    }

    octa(0.06f, 1.60f, trunkR(1.6f) + 0.005f, 0.09f, 0.12f, 0.05f, BARK_DARK);

    // --------------------------------------------------------
    // 5. Branches: forked, tapered, leaning in 3D
    // --------------------------------------------------------
    // left main
    glPushMatrix();
    glTranslatef(-0.15f, 2.60f, 0.0f);
    glRotatef(-15.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(48.0f, 0.0f, 0.0f, 1.0f);
    cylinderY(0.17f, 0.07f, 1.60f, 8, BARK);
    glTranslatef(0.0f, 1.00f, 0.0f);            // fork
    glRotatef(-32.0f, 0.0f, 0.0f, 1.0f);
    cylinderY(0.08f, 0.03f, 0.80f, 6, BARK);
    glPopMatrix();

    // right main
    glPushMatrix();
    glTranslatef(0.15f, 2.70f, 0.0f);
    glRotatef(15.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-48.0f, 0.0f, 0.0f, 1.0f);
    cylinderY(0.17f, 0.07f, 1.55f, 8, BARK);
    glTranslatef(0.0f, 0.95f, 0.0f);
    glRotatef(32.0f, 0.0f, 0.0f, 1.0f);
    cylinderY(0.08f, 0.03f, 0.80f, 6, BARK);
    glPopMatrix();

    // front + back sub-branches
    glPushMatrix();
    glTranslatef(0.05f, 2.95f, 0.10f);
    glRotatef(36.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(-10.0f, 0.0f, 0.0f, 1.0f);
    cylinderY(0.13f, 0.05f, 1.20f, 7, BARK);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.05f, 2.95f, -0.10f);
    glRotatef(-36.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(10.0f, 0.0f, 0.0f, 1.0f);
    cylinderY(0.13f, 0.05f, 1.20f, 7, BARK);
    glPopMatrix();

    // central leader (runs up into the canopy)
    glPushMatrix();
    glTranslatef(0.0f, 3.30f, 0.0f);
    glRotatef(4.0f, 0.0f, 0.0f, 1.0f);
    cylinderY(0.28f, 0.08f, 1.30f, 8, BARK);
    glPopMatrix();

    // --------------------------------------------------------
    // 6. Foliage — layered low-poly blobs, dark bottom -> bright top
    // --------------------------------------------------------
    const BlobDef L1[] = {
        {-0.90f, 3.55f,  0.00f, 0.85f, 0.75f, 0.85f, -15.0f},
        { 0.90f, 3.60f,  0.00f, 0.85f, 0.75f, 0.85f,  20.0f},
        { 0.00f, 3.45f, -0.85f, 0.80f, 0.70f, 0.80f,  10.0f},
        { 0.05f, 3.40f,  0.85f, 0.80f, 0.70f, 0.80f,  -5.0f},
    };
    const BlobDef L2[] = {
        { 0.00f, 4.05f,  0.00f, 1.35f, 1.00f, 1.35f,   0.0f},
        {-1.00f, 3.95f,  0.20f, 0.95f, 0.85f, 0.95f,  10.0f},
        { 1.00f, 4.00f, -0.20f, 0.95f, 0.85f, 0.95f, -10.0f},
        { 0.15f, 3.90f,  0.95f, 0.80f, 0.70f, 0.80f,  25.0f},
    };
    const BlobDef L3[] = {
        {-0.45f, 4.65f,  0.30f, 0.85f, 0.75f, 0.85f,  15.0f},
        { 0.50f, 4.70f, -0.25f, 0.85f, 0.75f, 0.85f, -12.0f},
        { 0.00f, 4.75f,  0.00f, 0.90f, 0.70f, 0.90f,   0.0f},
    };
    const BlobDef L4[] = {
        { 0.10f, 5.30f,  0.00f, 0.70f, 0.55f, 0.70f,   8.0f},
        {-0.30f, 5.15f,  0.25f, 0.55f, 0.45f, 0.55f, -20.0f},
    };
    foliageLayer(L1, 4, LEAF_1, 10);
    foliageLayer(L2, 4, LEAF_2, 50);
    foliageLayer(L3, 3, LEAF_3, 90);
    foliageLayer(L4, 2, LEAF_4, 130);

    // --------------------------------------------------------
    // 7. Sun-lit leaf tufts scattered over the canopy surface
    // --------------------------------------------------------
    struct Tuft { float x, y, z, s, rY, rZ; };
    const Tuft tufts[] = {
        {-0.40f, 5.55f,  0.35f, 0.32f, -15.0f, -12.0f},
        { 0.55f, 5.30f,  0.30f, 0.28f,  12.0f,  15.0f},
        { 0.00f, 5.75f,  0.10f, 0.34f,   0.0f,   0.0f},
        {-0.70f, 4.55f,  1.00f, 0.32f, -25.0f, -20.0f},
        { 0.75f, 4.70f,  0.95f, 0.30f,  22.0f,  20.0f},
        {-1.55f, 4.15f,  0.40f, 0.30f, -40.0f, -25.0f},
        { 1.55f, 4.20f,  0.25f, 0.30f,  35.0f,  25.0f},
        { 0.05f, 4.95f,  0.95f, 0.28f,   5.0f,   0.0f},
        {-1.20f, 3.75f,  0.85f, 0.26f, -30.0f, -15.0f},
        { 1.15f, 3.80f,  0.80f, 0.26f,  30.0f,  15.0f},
    };
    for (int i = 0; i < 10; ++i) {
        const Tuft& t = tufts[i];
        octa(t.x, t.y, t.z, t.s, t.s * 0.55f, t.s * 0.75f,
             LEAF_TUFT, t.rY, t.rZ);
    }

    // --------------------------------------------------------
    // 8. Apples (little low-poly spheres with stems)
    // --------------------------------------------------------
    struct Fruit { float x, y, z, r; bool red; };
    const Fruit fruits[] = {
        {-0.95f, 3.95f, 1.10f, 0.10f, true },
        { 0.85f, 4.10f, 1.10f, 0.09f, true },
        {-0.20f, 4.85f, 1.00f, 0.085f, false},
        { 0.45f, 4.55f, 1.15f, 0.08f, false},
    };
    for (int i = 0; i < 4; ++i) {
        const Fruit& f = fruits[i];
        blob(f.x, f.y, f.z, f.r, f.r * 0.95f, f.r,
             f.red ? APPLE_RED : APPLE_GOLD, 200 + i, 0.0f, 4, 7, 0.04f);
        glPushMatrix();
        glTranslatef(f.x, f.y + f.r * 0.85f, f.z);
        cylinderY(0.009f, 0.006f, 0.07f, 4, BARK_DARK);
        glPopMatrix();
    }

    glPopMatrix();
}


// ============================================================
// CROP — soil mound, three wheat stalks (main + 2 tillers),
//        bent stems with nodes, arched leaf blades, detailed ear
// ============================================================

void drawCrop(float x, float z, float scale)
{
    glPushMatrix();
    glTranslatef(x, 0.08f, z);
    glScalef(scale, scale, scale);

    // --------------------------------------------------------
    // 1. Soil mound + clumps + small grass
    // --------------------------------------------------------
    glPushMatrix();
    glTranslatef(0.0f, -0.07f, 0.0f);
    cylinderY(0.38f, 0.16f, 0.11f, 10, SOIL, true);
    glPopMatrix();

    for (int i = 0; i < 5; ++i) {
        float a = i * 2.0f * PI / 5.0f + 0.4f;
        blob(sinf(a) * 0.26f, -0.03f, cosf(a) * 0.26f,
             0.07f, 0.045f, 0.07f, SOIL_DARK, 300 + i, 0.0f, 4, 6, 0.10f);
    }

    grassTuft( 0.22f,  0.16f, 0.14f, 41);
    grassTuft(-0.24f, -0.12f, 0.12f, 42);
    grassTuft( 0.05f, -0.26f, 0.13f, 43);

    // --------------------------------------------------------
    // 2. Stalks
    // --------------------------------------------------------
    // main stalk
    drawStalk();

    // left tiller (shorter, leaning outward)
    glPushMatrix();
    glTranslatef(-0.14f, 0.0f, 0.06f);
    glRotatef(14.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.78f, 0.78f, 0.78f);
    drawStalk();
    glPopMatrix();

    // right tiller
    glPushMatrix();
    glTranslatef(0.13f, 0.0f, -0.05f);
    glRotatef(60.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(-12.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.68f, 0.68f, 0.68f);
    drawStalk();
    glPopMatrix();

    glPopMatrix();
}

} // namespace Vegetation
