#include "Lighting.h"
#include "graphics/Shadow.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace {
constexpr float Pi = 3.14159265358979323846f;
}

void Lighting::toggleNight()
{
    night_ = !night_;
}

void Lighting::setNight(bool night)
{
    night_ = night;
}

bool Lighting::isNight() const
{
    return night_;
}

void Lighting::enableLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    // FIX: OpenGL's default global ambient is (0.2,0.2,0.2) and it is added on
    // top of every light's own ambient. That is the main cause of the washed
    // out / over-exposed look. We control ambient per light instead.
    const GLfloat globalAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
}

void Lighting::disableLighting()
{
    glDisable(GL_LIGHTING);
}

void Lighting::applyDirectional() const
{
    // Sun / Moon directional light (w = 0.0f specifies directional light!)
    const GLfloat sunDir[]  = {0.45f, 0.85f, 0.35f, 0.0f};
    const GLfloat moonDir[] = {-0.45f, 0.75f, -0.35f, 0.0f};

    const GLfloat* dir = night_ ? moonDir : sunDir;

    // Update shadow system light direction to match
    Shadow::setLightDirection(dir[0], dir[1], dir[2]);

    const GLfloat ambient[] = {
        night_ ? 0.12f : 0.34f,
        night_ ? 0.15f : 0.34f,
        night_ ? 0.24f : 0.34f,
        1.0f
    };

    const GLfloat diffuse[] = {
        night_ ? 0.20f : 0.72f,
        night_ ? 0.25f : 0.70f,
        night_ ? 0.45f : 0.64f,
        1.0f
    };

    const GLfloat specular[] = {
        night_ ? 0.15f : 0.60f,
        night_ ? 0.20f : 0.60f,
        night_ ? 0.35f : 0.50f,
        1.0f
    };

    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, dir);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
}

void Lighting::applyFarmLights(float farmX, float farmZ, float farmYaw) const
{
    // Farmhouse Porch Point Light (GL_LIGHT1)
    // Farmhouse porch is located in farm local space around (x = -4.0, y = 2.2, z = 0.5)
    const float rad = farmYaw * Pi / 180.0f;
    const float localX = -4.0f;
    const float localZ = 0.5f;
    const float porchWorldX = farmX + (localX * std::cos(rad) - localZ * std::sin(rad));
    const float porchWorldZ = farmZ + (localX * std::sin(rad) + localZ * std::cos(rad));
    const float porchWorldY = 2.2f;

    const GLfloat porchPos[] = {porchWorldX, porchWorldY, porchWorldZ, 1.0f}; // w=1 is point light

    // Amber lantern glow
    const float intensity = night_ ? 1.0f : 0.0f;
    const GLfloat porchDiff[] = {1.0f * intensity, 0.72f * intensity, 0.35f * intensity, 1.0f};
    const GLfloat porchAmb[]  = {0.15f * intensity, 0.10f * intensity, 0.05f * intensity, 1.0f};

    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, porchPos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, porchAmb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, porchDiff);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.035f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.008f);

    // Farm Gate Entrance Warm Lamp (GL_LIGHT5)
    const float gateLocalX = 8.0f;
    const float gateLocalZ = -19.0f;
    const float gateWorldX = farmX + (gateLocalX * std::cos(rad) - gateLocalZ * std::sin(rad));
    const float gateWorldZ = farmZ + (gateLocalX * std::sin(rad) + gateLocalZ * std::cos(rad));
    const GLfloat gatePos[] = {gateWorldX, 2.5f, gateWorldZ, 1.0f};

    const GLfloat gateDiff[] = {0.95f * intensity, 0.80f * intensity, 0.40f * intensity, 1.0f};
    const GLfloat gateAmb[]  = {0.10f * intensity, 0.08f * intensity, 0.04f * intensity, 1.0f};

    glEnable(GL_LIGHT5);
    glLightfv(GL_LIGHT5, GL_POSITION, gatePos);
    glLightfv(GL_LIGHT5, GL_AMBIENT, gateAmb);
    glLightfv(GL_LIGHT5, GL_DIFFUSE, gateDiff);
    glLightf(GL_LIGHT5, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT5, GL_LINEAR_ATTENUATION, 0.04f);
    glLightf(GL_LIGHT5, GL_QUADRATIC_ATTENUATION, 0.01f);
}

void Lighting::applyPowerPlantLight(float x, float y, float z) const
{
    // Industrial cool-white floodlight (GL_LIGHT2)
    const GLfloat plantPos[] = {x, y, z, 1.0f};
    const float intensity = night_ ? 1.0f : 0.0f;

    const GLfloat plantDiff[] = {0.85f * intensity, 0.90f * intensity, 1.0f * intensity, 1.0f};
    const GLfloat plantAmb[]  = {0.10f * intensity, 0.12f * intensity, 0.15f * intensity, 1.0f};

    glEnable(GL_LIGHT2);
    glLightfv(GL_LIGHT2, GL_POSITION, plantPos);
    glLightfv(GL_LIGHT2, GL_AMBIENT, plantAmb);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, plantDiff);
    glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.003f);
}

void Lighting::applyTractorHeadlights(float tractorX, float tractorY, float tractorZ,
                                     float heading, bool headlightsOn) const
{
    if (!headlightsOn)
    {
        glDisable(GL_LIGHT3);
        glDisable(GL_LIGHT4);
        return;
    }

    const float rad = heading * Pi / 180.0f;
    // Tractor forward direction vector in XZ (tractor faces -Z in local coords)
    const float dirX = -std::sin(rad);
    const float dirZ = -std::cos(rad);
    const float dirY = -0.15f; // slightly angled down at ground

    const GLfloat spotDir[] = {dirX, dirY, dirZ};

    // Right vector for headlight separation
    const float rightX = std::cos(rad);
    const float rightZ = -std::sin(rad);
    const float separation = 0.95f;

    // Left headlight position (GL_LIGHT3)
    const GLfloat leftPos[] = {
        tractorX - rightX * separation + dirX * 2.3f,
        tractorY + 1.2f,
        tractorZ - rightZ * separation + dirZ * 2.3f,
        1.0f
    };

    // Right headlight position (GL_LIGHT4)
    const GLfloat rightPos[] = {
        tractorX + rightX * separation + dirX * 2.3f,
        tractorY + 1.2f,
        tractorZ + rightZ * separation + dirZ * 2.3f,
        1.0f
    };

    const GLfloat lightDiff[] = {1.0f, 0.96f, 0.82f, 1.0f};
    const GLfloat lightSpec[] = {0.8f, 0.8f, 0.7f, 1.0f};
    const GLfloat lightAmb[]  = {0.05f, 0.05f, 0.04f, 1.0f};

    // Setup GL_LIGHT3 (Left Headlight Spot)
    glEnable(GL_LIGHT3);
    glLightfv(GL_LIGHT3, GL_POSITION, leftPos);
    glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, spotDir);
    glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 32.0f);     // 32 degree beam angle
    glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 12.0f);   // focus
    glLightfv(GL_LIGHT3, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT3, GL_SPECULAR, lightSpec);
    glLightfv(GL_LIGHT3, GL_AMBIENT, lightAmb);
    glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, 0.002f);

    // Setup GL_LIGHT4 (Right Headlight Spot)
    glEnable(GL_LIGHT4);
    glLightfv(GL_LIGHT4, GL_POSITION, rightPos);
    glLightfv(GL_LIGHT4, GL_SPOT_DIRECTION, spotDir);
    glLightf(GL_LIGHT4, GL_SPOT_CUTOFF, 32.0f);
    glLightf(GL_LIGHT4, GL_SPOT_EXPONENT, 12.0f);
    glLightfv(GL_LIGHT4, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT4, GL_SPECULAR, lightSpec);
    glLightfv(GL_LIGHT4, GL_AMBIENT, lightAmb);
    glLightf(GL_LIGHT4, GL_CONSTANT_ATTENUATION, 1.0f);
    glLightf(GL_LIGHT4, GL_LINEAR_ATTENUATION, 0.02f);
    glLightf(GL_LIGHT4, GL_QUADRATIC_ATTENUATION, 0.002f);
}