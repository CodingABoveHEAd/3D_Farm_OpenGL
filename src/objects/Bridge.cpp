#include "objects/Bridge.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

namespace Bridge {
namespace {

void drawBox(float r, float g, float b,
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

void drawCylinderY(float r, float g, float b,
                   float x, float y, float z,
                   float radius, float height, int segments = 10)
{
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCylinder(radius, height, segments);
    glPopMatrix();
}

// ============================================================
// BÉZIER CURVE EVALUATION (Teacher Requirement)
// Evaluates Quadratic Bézier curve at parameter t:
// B(t) = (1-t)^2 * P0 + 2 * (1-t) * t * P1 + t^2 * P2
// ============================================================
struct Point2D { float z, y; };

Point2D evalQuadraticBezier(Point2D p0, Point2D p1, Point2D p2, float t)
{
    const float u = 1.0f - t;
    return {
        u * u * p0.z + 2.0f * u * t * p1.z + t * t * p2.z,
        u * u * p0.y + 2.0f * u * t * p1.y + t * t * p2.y
    };
}

} // namespace

void drawBridge(float x, float z, float angle, float length, float width)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(angle, 0.0f, 1.0f, 0.0f);

    const float halfLen = length * 0.5f;
    const float halfWid = width * 0.5f;
    const float archApex = 0.85f; // height of center arch above base

    // 1. Foundation Support Pilings / Piles under bridge into water bed
    for (float pz : {-halfLen * 0.65f, 0.0f, halfLen * 0.65f})
    {
        for (float px : {-halfWid + 0.25f, halfWid - 0.25f})
        {
            drawCylinderY(0.24f, 0.16f, 0.09f, px, -0.6f, pz, 0.22f, 1.8f);
        }
        // Cross-beam between pilings
        drawBox(0.32f, 0.20f, 0.11f, 0.0f, 0.45f, pz, width - 0.2f, 0.24f, 0.35f);
    }

    // 2. Arched Deck Planks
    // Discretize span into small wooden planks
    constexpr int plankCount = 28;
    const Point2D p0_deck{-halfLen, 0.40f};
    const Point2D p1_deck{0.0f,     0.40f + archApex};
    const Point2D p2_deck{ halfLen, 0.40f};

    const float plankSpacing = length / static_cast<float>(plankCount);

    for (int i = 0; i < plankCount; ++i)
    {
        const float t0 = static_cast<float>(i) / plankCount;
        const float t1 = static_cast<float>(i + 1) / plankCount;
        const Point2D ptA = evalQuadraticBezier(p0_deck, p1_deck, p2_deck, t0);
        const Point2D ptB = evalQuadraticBezier(p0_deck, p1_deck, p2_deck, t1);

        const float midZ = (ptA.z + ptB.z) * 0.5f;
        const float midY = (ptA.y + ptB.y) * 0.5f;
        const float dZ = ptB.z - ptA.z;
        const float dY = ptB.y - ptA.y;
        const float slopeAngle = std::atan2(dY, dZ) * 180.0f / 3.14159265f;

        // Alternate wood stain subtle tones
        const float stain = (i % 2 == 0) ? 0.46f : 0.42f;

        glPushMatrix();
        glTranslatef(0.0f, midY, midZ);
        glRotatef(slopeAngle, 1.0f, 0.0f, 0.0f);
        drawBox(stain, stain * 0.65f, stain * 0.38f, 0.0f, 0.0f, 0.0f, width, 0.12f, plankSpacing * 0.94f);
        glPopMatrix();
    }

    // 3. Arched Bézier Handrail on both sides
    const Point2D p0_rail{-halfLen, 0.40f + 0.90f};
    const Point2D p1_rail{0.0f,     0.40f + archApex + 0.90f};
    const Point2D p2_rail{ halfLen, 0.40f + 0.90f};

    for (float side : {-halfWid + 0.15f, halfWid - 0.15f})
    {
        // Vertical baluster posts
        constexpr int postCount = 9;
        for (int p = 0; p <= postCount; ++p)
        {
            const float t = static_cast<float>(p) / postCount;
            const Point2D deckPt = evalQuadraticBezier(p0_deck, p1_deck, p2_deck, t);
            const Point2D railPt = evalQuadraticBezier(p0_rail, p1_rail, p2_rail, t);

            const float postH = railPt.y - deckPt.y;
            drawBox(0.35f, 0.22f, 0.12f, side, deckPt.y + postH * 0.5f, deckPt.z, 0.14f, postH, 0.14f);
        }

        // Curved continuous top handrail segments using Bézier
        constexpr int railSegments = 24;
        for (int s = 0; s < railSegments; ++s)
        {
            const float tA = static_cast<float>(s) / railSegments;
            const float tB = static_cast<float>(s + 1) / railSegments;
            const Point2D rA = evalQuadraticBezier(p0_rail, p1_rail, p2_rail, tA);
            const Point2D rB = evalQuadraticBezier(p0_rail, p1_rail, p2_rail, tB);

            const float rMidZ = (rA.z + rB.z) * 0.5f;
            const float rMidY = (rA.y + rB.y) * 0.5f;
            const float rDZ = rB.z - rA.z;
            const float rDY = rB.y - rA.y;
            const float rSlope = std::atan2(rDY, rDZ) * 180.0f / 3.14159265f;
            const float segLen = std::sqrt(rDZ * rDZ + rDY * rDY);

            glPushMatrix();
            glTranslatef(side, rMidY, rMidZ);
            glRotatef(rSlope, 1.0f, 0.0f, 0.0f);
            drawBox(0.48f, 0.30f, 0.16f, 0.0f, 0.0f, 0.0f, 0.18f, 0.10f, segLen * 1.05f);
            glPopMatrix();
        }
    }

    glPopMatrix();
}

} // namespace Bridge
