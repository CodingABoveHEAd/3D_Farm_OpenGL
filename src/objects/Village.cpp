#include "objects/Village.h"

#include "DayNightSettings.h"
#include "graphics/Primitives.h"

#include <GLFW/glfw3.h>
#include <cmath>
#include <initializer_list>

namespace Village {
namespace {
constexpr float Pi = 3.14159265358979323846f;

struct Color { float r, g, b; };
constexpr Color Wood{0.43f, 0.24f, 0.09f};
constexpr Color WoodDark{0.24f, 0.12f, 0.04f};
constexpr Color WoodLight{0.63f, 0.39f, 0.16f};
constexpr Color Skin{0.76f, 0.51f, 0.32f};
constexpr Color Metal{0.28f, 0.30f, 0.31f};

float hash01(int value)
{
    const float s = std::sin(value * 12.9898f) * 43758.5453f;
    return s - std::floor(s);
}

void box(const Color& color, float x, float y, float z,
         float width, float height, float depth)
{
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCube(width, height, depth);
    glPopMatrix();
}

void sphere(const Color& color, float x, float y, float z,
            float sx, float sy, float sz, int slices = 10, int stacks = 7)
{
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    Primitives::drawSphere(1.0f, slices, stacks);
    glPopMatrix();
}

void cylinder(const Color& color, float x, float y, float z,
              float radius, float height, int slices = 10)
{
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    Primitives::drawCylinder(radius, height, slices);
    glPopMatrix();
}

void drawBench(float length = 4.4f)
{
    for (float x : {-length * 0.38f, length * 0.38f})
    {
        box(WoodDark, x, 0.42f, 0.0f, 0.22f, 0.82f, 0.72f);
        box(WoodDark, x, 1.26f, -0.36f, 0.18f, 1.25f, 0.18f);
    }
    box(Wood, 0.0f, 0.86f, 0.02f, length, 0.20f, 0.78f);
    for (float y : {1.22f, 1.58f})
        box(WoodLight, 0.0f, y, -0.43f, length, 0.23f, 0.16f);
    box(Metal, 0.0f, 0.72f, 0.02f, length + 0.12f, 0.08f, 0.08f);
}

void limb(const Color& color, float x, float y, float z,
          float length, float angleX, float angleZ, float radius)
{
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(angleZ, 0.0f, 0.0f, 1.0f);
    glRotatef(angleX, 1.0f, 0.0f, 0.0f);
    Primitives::drawCylinder(radius, length, 9);
    glPopMatrix();
}

void drawCup(float x, float y, float z)
{
    cylinder({0.88f, 0.82f, 0.65f}, x, y, z, 0.10f, 0.18f, 10);
    sphere({0.34f, 0.18f, 0.07f}, x, y + 0.19f, z, 0.075f, 0.018f, 0.075f, 8, 4);
}

void drawVillager(bool seated, float phase, float time, bool holdingCup)
{
    const Color shirts[] = {
        {0.68f, 0.18f, 0.14f}, {0.16f, 0.42f, 0.26f},
        {0.20f, 0.38f, 0.68f}, {0.78f, 0.56f, 0.15f}};
    const Color shirt = shirts[static_cast<int>(phase * 3.0f) & 3];
    const float gesture = std::sin(time * (0.75f + phase * 0.08f) + phase) * 10.0f;
    const float sip = holdingCup
        ? 0.5f + 0.5f * std::sin(time * 0.48f + phase * 2.3f) : 0.0f;
    const float headTurn = std::sin(time * 0.31f + phase) * 14.0f;

    if (seated)
    {
        // Thighs project from the seat; shins drop so both feet meet ground.
        for (float side : {-1.0f, 1.0f})
        {
            limb({0.16f, 0.22f, 0.34f}, side * 0.18f, 1.05f, 0.0f,
                 0.58f, -67.0f, 0.0f, 0.105f);
            limb({0.18f, 0.14f, 0.09f}, side * 0.18f, 0.82f, 0.52f,
                 0.72f, 8.0f, 0.0f, 0.09f);
            box({0.12f, 0.07f, 0.035f}, side * 0.18f, 0.08f, 0.63f,
                0.24f, 0.13f, 0.40f);
        }
    }
    else
    {
        for (float side : {-1.0f, 1.0f})
        {
            limb({0.16f, 0.22f, 0.34f}, side * 0.17f, 1.10f, 0.0f,
                 0.93f, 0.0f, 0.0f, 0.105f);
            box({0.12f, 0.07f, 0.035f}, side * 0.17f, 0.07f, 0.10f,
                0.24f, 0.13f, 0.38f);
        }
    }

    const float hipY = seated ? 1.12f : 1.45f;
    sphere(shirt, 0.0f, hipY + 0.53f, 0.0f, 0.38f, 0.58f, 0.25f);
    box({0.16f, 0.22f, 0.34f}, 0.0f, hipY + 0.10f, 0.0f, 0.68f, 0.26f, 0.46f);

    for (float side : {-1.0f, 1.0f})
    {
        const bool cupArm = holdingCup && side > 0.0f;
        const float armAngle = cupArm ? -58.0f * sip : gesture * side;
        limb(shirt, side * 0.45f, hipY + 0.82f, 0.0f,
             0.62f, armAngle, side * 8.0f, 0.09f);
        sphere(Skin, side * 0.45f, hipY + 0.25f + (cupArm ? 0.45f * sip : 0.0f),
               cupArm ? 0.30f * sip : 0.05f,
               0.11f, 0.11f, 0.11f, 8, 6);
        if (cupArm) drawCup(side * 0.45f, hipY + 0.30f + 0.43f * sip, 0.28f * sip);
    }

    glPushMatrix();
    glTranslatef(0.0f, hipY + 1.25f, 0.0f);
    glRotatef(headTurn, 0.0f, 1.0f, 0.0f);
    sphere(Skin, 0.0f, 0.0f, 0.0f, 0.25f, 0.30f, 0.25f);
    sphere({0.16f, 0.09f, 0.04f}, 0.0f, 0.19f, -0.04f, 0.26f, 0.14f, 0.24f);
    sphere({0.03f, 0.03f, 0.025f}, -0.08f, 0.04f, 0.235f, 0.025f, 0.025f, 0.018f, 6, 4);
    sphere({0.03f, 0.03f, 0.025f},  0.08f, 0.04f, 0.235f, 0.025f, 0.025f, 0.018f, 6, 4);
    glPopMatrix();
}

void drawGrassTuft(float x, float z, float height, float rotation, int shade)
{
    static const Color greens[] = {
        {0.10f, 0.38f, 0.08f}, {0.16f, 0.47f, 0.10f},
        {0.22f, 0.55f, 0.13f}, {0.12f, 0.43f, 0.16f}};
    const Color color = greens[shade & 3];
    glColor3f(color.r, color.g, color.b);
    glPushMatrix();
    glTranslatef(x, 0.04f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glBegin(GL_TRIANGLES);
    for (int blade = 0; blade < 3; ++blade)
    {
        const float offset = (blade - 1) * 0.12f;
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(offset - 0.055f, 0.0f, 0.0f);
        glVertex3f(offset + 0.055f, 0.0f, 0.0f);
        glVertex3f(offset + (blade - 1) * 0.05f, height, 0.0f);
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, offset - 0.055f);
        glVertex3f(0.0f, 0.0f, offset + 0.055f);
        glVertex3f(0.0f, height * 0.92f, offset);
    }
    glEnd();
    glPopMatrix();
}

void drawKettle()
{
    sphere(Metal, 0.0f, 1.78f, 2.08f, 0.35f, 0.28f, 0.35f, 12, 8);
    cylinder({0.12f, 0.12f, 0.11f}, 0.0f, 2.03f, 2.08f, 0.16f, 0.06f, 10);
    glColor3f(Metal.r, Metal.g, Metal.b);
    glPushMatrix();
    glTranslatef(0.32f, 1.85f, 2.08f);
    glRotatef(68.0f, 0.0f, 0.0f, 1.0f);
    Primitives::drawCylinder(0.07f, 0.48f, 9);
    glPopMatrix();
}

void drawShop(int variant, float time, float nightAmount)
{
    const Color wall[] = {{0.70f,0.42f,0.18f},{0.26f,0.48f,0.36f},{0.62f,0.30f,0.18f}};
    const Color trim[] = {{0.90f,0.76f,0.46f},{0.82f,0.78f,0.61f},{0.88f,0.68f,0.34f}};
    const Color roof[] = {{0.35f,0.08f,0.045f},{0.28f,0.16f,0.07f},{0.20f,0.11f,0.06f}};

    box(wall[variant], 0.0f, 1.55f, 0.0f, 6.4f, 3.1f, 3.8f);
    box(WoodDark, 0.0f, 0.15f, 0.0f, 6.8f, 0.30f, 4.2f);
    box(roof[variant], 0.0f, 3.40f, 0.0f, 7.4f, 0.32f, 4.8f);
    box(trim[variant], 0.0f, 2.35f, 1.96f, 5.2f, 1.35f, 0.14f);
    box(Wood, 0.0f, 1.24f, 2.25f, 5.8f, 0.90f, 0.65f);
    box(WoodLight, 0.0f, 1.72f, 2.25f, 6.0f, 0.10f, 0.78f);
    // Awning and posts.
    box(trim[variant], 0.0f, 3.02f, 2.72f, 6.5f, 0.15f, 1.75f);
    for (float x : {-2.85f, 2.85f}) box(Wood, x, 1.55f, 3.25f, 0.16f, 2.95f, 0.16f);

    // Shelves, jars, cups and a clearly recognizable kettle.
    for (float y : {1.70f, 2.20f}) box(WoodDark, 0.0f, y, -1.82f, 5.4f, 0.10f, 0.30f);
    for (int jar = 0; jar < 6; ++jar)
    {
        const Color contents = jar % 2 ? Color{0.72f,0.28f,0.10f} : Color{0.72f,0.62f,0.18f};
        cylinder(contents, -2.0f + jar * 0.80f, 1.93f + (jar % 2) * 0.50f,
                 -1.66f, 0.17f, 0.34f, 9);
    }
    drawKettle();
    for (int cup = 0; cup < 4; ++cup) drawCup(-1.6f + cup * 0.48f, 1.80f, 2.22f);

    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    const GLfloat emission[] = {DayNightSettings::WarmLamp[0] * nightAmount,
        DayNightSettings::WarmLamp[1] * nightAmount,
        DayNightSettings::WarmLamp[2] * nightAmount, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    sphere({0.95f,0.72f,0.30f}, 0.0f, 2.75f, 2.15f, 0.16f, 0.20f, 0.16f, 9, 6);
    glPopAttrib();

    // Unsynchronised customers: two seated, one standing near the counter.
    glPushMatrix(); glTranslatef(-1.25f, 0.0f, 4.05f); glRotatef(180.0f,0,1,0);
    drawVillager(true, variant + 0.3f, time, true); glPopMatrix();
    glPushMatrix(); glTranslatef(1.20f, 0.0f, 4.10f); glRotatef(174.0f,0,1,0);
    drawVillager(true, variant + 1.7f, time, false); glPopMatrix();
    glPushMatrix(); glTranslatef(2.15f, 0.0f, 2.70f); glRotatef(205.0f,0,1,0);
    drawVillager(false, variant + 2.8f, time, true); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.0f, 4.30f); drawBench(4.0f); glPopMatrix();
}
}

void drawFarmGrass(int seed, float cropX, float cropZ,
                   float houseX, float houseZ, bool hasWindmill)
{
    const float actualHouseX = houseX + 10.0f;
    const float actualHouseZ = houseZ + 7.0f;
    for (int index = 0; index < 92; ++index)
    {
        const float x = -21.5f + hash01(seed * 211 + index * 17) * 43.0f;
        const float z = -16.5f + hash01(seed * 137 + index * 29) * 33.0f;
        if (std::fabs(x - cropX) < 10.0f && std::fabs(z - (cropZ - 7.0f)) < 6.2f) continue;
        if (std::fabs(x - actualHouseX) < 7.0f && std::fabs(z - actualHouseZ) < 6.0f) continue;
        if (std::fabs(x - 10.0f) < 2.4f && z < 8.0f) continue; // access path
        if (hasWindmill)
        {
            const float dx = x + 14.0f, dz = z - 1.0f;
            if (dx * dx + dz * dz < 16.0f) continue;
        }
        drawGrassTuft(x, z, 0.24f + hash01(index * 43 + seed) * 0.42f,
                      hash01(index * 71 + seed) * 180.0f, index + seed);
    }
}

void drawPondSeating(float animationTime)
{
    // East of the western pond, outside its shoreline. The bench faces west
    // toward the water; its occupants' feet land on the access-side grass.
    glPushMatrix();
    glTranslatef(-57.5f, 0.0f, -42.0f);
    glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    drawBench(4.8f);
    glPushMatrix(); glTranslatef(-1.05f, 0.0f, 0.0f);
    drawVillager(true, 0.6f, animationTime, false); glPopMatrix();
    glPushMatrix(); glTranslatef(1.05f, 0.0f, 0.0f);
    drawVillager(true, 2.2f, animationTime, true); glPopMatrix();
    glPopMatrix();

    // Compact dirt access path from the open field to the bench.
    glColor3f(0.52f, 0.38f, 0.21f);
    glPushMatrix(); glTranslatef(-54.2f, 0.025f, -42.0f);
    Primitives::drawPlane(6.2f, 2.2f); glPopMatrix();
}

void drawRoadsideAmenities(float animationTime, float nightAmount)
{
    struct Placement { float x, z, rotation; int variant; };
    constexpr Placement shops[] = {
        {-14.5f, -32.0f,  90.0f, 0},
        { 14.5f,  18.0f, -90.0f, 1},
        {-14.5f,  66.0f,  90.0f, 2}};
    for (const Placement& shop : shops)
    {
        glPushMatrix();
        glTranslatef(shop.x, 0.0f, shop.z);
        glRotatef(shop.rotation, 0.0f, 1.0f, 0.0f);
        drawShop(shop.variant, animationTime, nightAmount);
        glPopMatrix();
    }

    constexpr Placement benches[] = {
        {-10.5f, -58.0f,  90.0f, 0},
        { 10.5f, -10.0f, -90.0f, 0},
        {-10.5f,  38.0f,  90.0f, 0},
        { 10.5f,  78.0f, -90.0f, 0}};
    for (int index = 0; index < 4; ++index)
    {
        const Placement& bench = benches[index];
        glPushMatrix();
        glTranslatef(bench.x, 0.0f, bench.z);
        glRotatef(bench.rotation + (index % 2 ? 3.0f : -2.0f), 0.0f, 1.0f, 0.0f);
        drawBench(3.8f + (index % 2) * 0.3f);
        glPopMatrix();
    }
}

} // namespace Village
