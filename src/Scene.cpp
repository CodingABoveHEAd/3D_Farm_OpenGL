#include "Scene.h"

#include "Camera.h"
#include "Input.h"
#include "graphics/Primitives.h"
#include "graphics/Shadow.h"
#include "objects/Bridge.h"
#include "objects/PowerPlant.h"
#include "objects/Vegetation.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>

namespace {
constexpr float Pi = 3.14159265358979323846f;
}

Scene::Scene()
{
    Cloud::initField();
}

void Scene::toggleGate()
{
    gateTarget_ = (gateTarget_ > 45.0f) ? 0.0f : 90.0f;
}

void Scene::handleInput()
{
    if (Input::wasPressed(GLFW_KEY_N))
    {
        lighting_.toggleNight();
    }
    if (Input::wasPressed(GLFW_KEY_P))
    {
        animation_.togglePaused();
    }
    if (Input::wasPressed(GLFW_KEY_G))
    {
        toggleGate();
    }
    if (Input::wasPressed(GLFW_KEY_R))
    {
        animation_.changeWindmillSpeed(30.0f);
    }
    if (Input::wasPressed(GLFW_KEY_EQUAL) || Input::wasPressed(GLFW_KEY_KP_ADD))
    {
        animation_.changeWindmillSpeed(15.0f);
    }
    if (Input::wasPressed(GLFW_KEY_MINUS) || Input::wasPressed(GLFW_KEY_KP_SUBTRACT))
    {
        animation_.changeWindmillSpeed(-15.0f);
    }
}

void Scene::update(float deltaTime, Camera& camera)
{
    totalTime_ += deltaTime;
    animation_.update(deltaTime);
    tractor_.update(deltaTime);
    Cloud::updateField(deltaTime);

    // Smooth gate opening / closing animation
    const float gateBlend = 1.0f - std::exp(-5.0f * deltaTime);
    gateAngle_ += (gateTarget_ - gateAngle_) * gateBlend;

    // Nearest farm tracking for camera focus
    const int nearest = FarmWorld::nearestFarm(camera.posX(), camera.posZ());
    float nfx = 0.0f, nfz = 0.0f;
    FarmWorld::farmCenter(nearest, nfx, nfz);
    camera.setFocusFarm(nfx, nfz);

    // Tractor world space tracking for chase camera & lighting
    float f0x = 0.0f, f0z = 0.0f;
    FarmWorld::farmCenter(0, f0x, f0z);
    const float farm0YawRad = FarmWorld::farmYaw(0) * Pi / 180.0f;
    const float twX = f0x + (tractor_.posX() * std::cos(farm0YawRad) - tractor_.posZ() * std::sin(farm0YawRad));
    const float twZ = f0z + (tractor_.posX() * std::sin(farm0YawRad) + tractor_.posZ() * std::cos(farm0YawRad));
    camera.setTractorPose(twX, tractor_.posY(), twZ, tractor_.heading() + FarmWorld::farmYaw(0));

    world_.update(deltaTime, camera);
}

void Scene::render(const Camera& camera)
{
    // 1. Setup multi-light environment
    lighting_.applyDirectional();

    // 2. Setup power plant floodlight (GL_LIGHT2)
    float ppx = 0.0f, ppy = 0.0f, ppz = 0.0f;
    PowerPlant::getLightPosition(ppx, ppy, ppz);
    lighting_.applyPowerPlantLight(ppx, ppy, ppz);

    // 3. Setup nearest farm porch point light (GL_LIGHT1) and gate lamp (GL_LIGHT5)
    const int nearest = FarmWorld::nearestFarm(camera.posX(), camera.posZ());
    float nfx = 0.0f, nfz = 0.0f;
    FarmWorld::farmCenter(nearest, nfx, nfz);
    lighting_.applyFarmLights(nfx, nfz, FarmWorld::farmYaw(nearest));

    // 4. Setup tractor spotlights (GL_LIGHT3 & GL_LIGHT4)
    float f0x = 0.0f, f0z = 0.0f;
    FarmWorld::farmCenter(0, f0x, f0z);
    const float farm0YawRad = FarmWorld::farmYaw(0) * Pi / 180.0f;
    const float twX = f0x + (tractor_.posX() * std::cos(farm0YawRad) - tractor_.posZ() * std::sin(farm0YawRad));
    const float twZ = f0z + (tractor_.posX() * std::sin(farm0YawRad) + tractor_.posZ() * std::cos(farm0YawRad));
    lighting_.applyTractorHeadlights(twX, tractor_.posY() + 0.8f, twZ,
                                    tractor_.heading() + FarmWorld::farmYaw(0),
                                    tractor_.isHeadlightsOn());

    // 5. Draw world and farms
    Lighting::enableLighting();
    world_.draw(camera, [this](int farmIndex, bool isNearest) {
        renderFarm(farmIndex, isNearest);
    });
    Lighting::disableLighting();

    // 6. Atmosphere & celestial bodies
    Sky::drawSun();
    Cloud::drawField();

    // 7. Fireflies at night
    if (lighting_.isNight())
    {
        renderFireflies(totalTime_);
    }
}

void Scene::shutdown()
{
    world_.release();
}

void Scene::renderFarm(int farmIndex, bool isNearest) const
{
    static const float rotations[] = {0.0f, 7.0f, -5.0f, 12.0f, -9.0f,
                                      4.0f, -13.0f, 8.0f, -6.0f};
    static const float scales[] = {1.00f, 0.96f, 1.04f, 0.98f, 1.03f,
                                   0.95f, 1.06f, 1.01f, 0.97f};

    glPushMatrix();
    glRotatef(rotations[farmIndex], 0.0f, 1.0f, 0.0f);
    glScalef(scales[farmIndex], scales[farmIndex], scales[farmIndex]);

    // Shadows rendered first so objects draw on top
    if (isNearest)
    {
        renderFarmShadows(farmIndex);
    }

    renderCropField();
    renderCrops();
    renderTrees();
    renderAnimals();
    renderFarmers();
    renderPath();
    renderBoundary();
    farmhouse_.render();
    // Barn on selected farms for variety
    if (farmIndex % 3 == 1)
        Barn::drawBarn(-10.5f, 12.5f, 0.52f, 18.0f);
    else if (farmIndex % 3 == 2)
        Barn::drawBarn(13.5f, 10.0f, 0.44f, -12.0f);
    tractor_.drawTractor();
    Windmill::drawWindmill(-14.0f, 1.0f, animation_.windmillAngle(), 1.15f);
    renderRocks();
    renderChimneySmoke(totalTime_);

    glPopMatrix();
}

void Scene::renderFarmShadows(int farmIndex) const
{
    Shadow::begin(0.03f);

    farmhouse_.render();
    if (farmIndex % 3 == 1)
        Barn::drawBarn(-10.5f, 12.5f, 0.52f, 18.0f);
    else if (farmIndex % 3 == 2)
        Barn::drawBarn(13.5f, 10.0f, 0.44f, -12.0f);
    tractor_.drawTractor();
    Windmill::drawWindmill(-14.0f, 1.0f, animation_.windmillAngle(), 1.15f);

    Shadow::end();
}

void Scene::renderCropField() const
{
    glColor3f(0.48f, 0.28f, 0.10f);

    glPushMatrix();
    glTranslatef(0.0f, 0.04f, -7.0f);
    glRotatef(-3.0f, 0.0f, 1.0f, 0.0f);
    glScalef(1.5f, 1.0f, 1.25f);
    Primitives::drawPlane(12.0f, 8.0f);
    glPopMatrix();

    glColor3f(0.34f, 0.18f, 0.06f);
    for (int row = -4; row <= 4; row += 2)
    {
        glPushMatrix();
        glTranslatef(static_cast<float>(row), 0.07f, -7.0f);
        glRotatef(-3.0f, 0.0f, 1.0f, 0.0f);
        glScalef(0.08f, 1.0f, 1.0f);
        Primitives::drawCube(1.0f, 0.02f, 7.0f);
        glPopMatrix();
    }
}

void Scene::renderCrops() const
{
    // Small wind sway animation on crops
    const float windSway = std::sin(totalTime_ * 2.2f) * 0.05f;

    for (int row = 0; row < 6; ++row)
    {
        const float z = -10.0f + static_cast<float>(row) * 1.35f;
        for (int column = 0; column < 7; ++column)
        {
            const float x = -7.5f + static_cast<float>(column) * 2.5f + windSway;
            const float size = 0.82f + static_cast<float>((row + column) % 3) * 0.08f;
            Vegetation::drawCrop(x, z, size);
        }
    }
}

void Scene::renderTrees() const
{
    Vegetation::drawTree(-18.0f, -13.0f, 1.45f);
    Vegetation::drawTree(-12.0f, -16.0f, 1.05f);
    Vegetation::drawTree(18.0f, -13.0f, 1.25f);
    Vegetation::drawTree(19.0f, 1.0f, 0.90f);
    Vegetation::drawTree(-18.0f, 9.0f, 1.30f);
    Vegetation::drawTree(18.0f, 14.0f, 1.55f);
}

void Scene::renderFarmers() const
{
    // 1. Farmer standing on farmhouse porch
    Farmer::drawFarmer(-4.0f, 0.5f, 90.0f, totalTime_, Farmer::Standing, 1.05f);

    // 2. Farmer working in the crop field with hoe
    Farmer::drawFarmer(-2.5f, -6.5f, -15.0f, totalTime_, Farmer::Working, 1.0f);

    // 3. Farmer walking along the farm driveway
    const float walkCycle = std::sin(totalTime_ * 0.5f);
    const float walkerZ = -5.0f + walkCycle * 8.0f;
    const float walkerHeading = (walkCycle >= 0.0f) ? 0.0f : 180.0f;
    Farmer::drawFarmer(10.0f, walkerZ, walkerHeading, totalTime_, Farmer::Walking, 1.0f);
}

void Scene::renderAnimals() const
{
    // 1. Walking cow following elliptical grazing path with swinging legs
    const float cowAngle = totalTime_ * 0.30f;
    const float cowX = -16.0f + std::sin(cowAngle) * 3.2f;
    const float cowZ = -2.0f + std::cos(cowAngle) * 4.2f;
    const float cowHeading = std::atan2(std::cos(cowAngle) * 3.2f, -std::sin(cowAngle) * 4.2f) * 180.0f / Pi;
    const float legSwing = std::sin(totalTime_ * 4.5f) * 22.0f;
    Animals::drawWalkingCow(cowX, cowZ, 1.05f, cowHeading, legSwing, false);

    // 2. Grazing cow with head down
    Animals::drawWalkingCow(16.0f, -7.0f, 0.90f, -18.0f, 0.0f, true);

    // 3. Resting cow in back pasture
    Animals::drawCow(-15.0f, 14.0f, 0.82f, 28.0f);

    // 4. Animated chickens pecking and flapping around farmyard
    Animals::drawChicken(-13.0f, 12.0f, 0.85f, totalTime_ * 12.0f, totalTime_);
    Animals::drawChicken(-14.5f, 14.5f, 0.80f, -40.0f + std::sin(totalTime_ * 1.5f) * 25.0f, totalTime_ + 1.2f);
    Animals::drawChicken(-12.0f, 13.8f, 0.75f, 65.0f, totalTime_ + 2.4f);
    Animals::drawChicken( 14.0f, -5.0f, 0.82f, 110.0f, totalTime_ + 0.8f);
    Animals::drawChicken( 15.2f, -3.8f, 0.78f, -75.0f, totalTime_ + 1.8f);
}

void Scene::renderBoundary() const
{
    constexpr float halfWidth = 24.0f;
    constexpr float halfDepth = 19.0f;

    glColor3f(0.35f, 0.18f, 0.06f);

    for (int x = -24; x <= 24; x += 8)
    {
        if (x < 8 || x > 12)
        {
            drawFencePost(static_cast<float>(x), -halfDepth);
        }
        drawFencePost(static_cast<float>(x), halfDepth);
    }
    for (int z = -16; z <= 16; z += 8)
    {
        drawFencePost(-halfWidth, static_cast<float>(z));
        drawFencePost(halfWidth, static_cast<float>(z));
    }

    drawFenceRail(-8.0f, -halfDepth, 32.0f, false);
    drawFenceRail(18.0f, -halfDepth, 12.0f, false);
    drawFenceRail(0.0f, halfDepth, 48.0f, false);
    drawFenceRail(-halfWidth, 0.0f, 38.0f, true);
    drawFenceRail(halfWidth, 0.0f, 38.0f, true);
    renderGate();
}

void Scene::renderPath() const
{
    glColor3f(0.58f, 0.42f, 0.23f);

    glPushMatrix();
    glTranslatef(10.0f, 0.035f, -5.5f);
    glScalef(1.0f, 1.0f, 1.0f);
    Primitives::drawPlane(3.5f, 27.0f);
    glPopMatrix();
}

void Scene::renderGate() const
{
    glColor3f(0.28f, 0.12f, 0.04f);
    drawFencePost(8.0f, -19.0f);
    drawFencePost(12.0f, -19.0f);

    // Left gate panel hinges at x = 8.0, swings inward (negative angle)
    drawGatePanel(8.0f, 1.0f, -gateAngle_);

    // Right gate panel hinges at x = 12.0, swings inward (positive angle)
    drawGatePanel(12.0f, -1.0f, gateAngle_);
}

void Scene::drawGatePanel(float hingeX, float panelDir, float angle) const
{
    // Hierarchical transformation: hinge pivot -> rotate -> offset to panel center
    for (float y : {0.70f, 1.45f})
    {
        glPushMatrix();
        glTranslatef(hingeX, y, -19.0f);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        glTranslatef(panelDir * 1.0f, 0.0f, 0.0f);
        glScalef(2.0f, 0.16f, 0.16f);
        Primitives::drawCube(1.0f, 1.0f, 1.0f);
        glPopMatrix();
    }
}

void Scene::drawFencePost(float x, float z) const
{
    glPushMatrix();
    glTranslatef(x, 0.9f, z);
    glScalef(0.35f, 1.8f, 0.35f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void Scene::drawFenceRail(float x, float z, float length, bool rotate) const
{
    glPushMatrix();
    glTranslatef(x, 1.15f, z);
    if (rotate)
    {
        glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    }
    glScalef(length, 0.16f, 0.16f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void Scene::renderRocks() const
{
    glColor3f(0.30f, 0.32f, 0.28f);
    drawRock(-20.0f, -6.0f, 0.75f, 18.0f);
    drawRock(20.0f, -5.0f, 0.55f, 35.0f);
    drawRock(-19.0f, 5.0f, 0.45f, 12.0f);
    drawRock(20.0f, 9.0f, 0.70f, 52.0f);
}

void Scene::drawRock(float x, float z, float scale, float rotation) const
{
    glPushMatrix();
    glTranslatef(x, scale * 0.28f, z);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale * 0.55f, scale * 0.75f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void Scene::renderChimneySmoke(float time) const
{
    // Chimney position atop farmhouse
    const float chimneyX = 3.6f;
    const float chimneyY = 7.8f;
    const float chimneyZ = -1.2f;

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    constexpr int puffCount = 9;
    for (int i = 0; i < puffCount; ++i)
    {
        const float phase = std::fmod(time * 0.65f + static_cast<float>(i) / puffCount, 1.0f);
        const float py = chimneyY + phase * 4.8f;
        const float px = chimneyX + std::sin(phase * 4.0f + i) * 0.35f + phase * 0.8f;
        const float pz = chimneyZ + std::cos(phase * 3.5f + i) * 0.30f;
        const float size = 0.25f + phase * 0.95f;
        const float alpha = (1.0f - phase) * 0.28f;

        glColor4f(0.85f, 0.85f, 0.88f, alpha);
        glPushMatrix();
        glTranslatef(px, py, pz);
        glScalef(size, size, size);
        Primitives::drawCube(1.0f, 1.0f, 1.0f);
        glPopMatrix();
    }

    glPopAttrib();
}

void Scene::renderFireflies(float time) const
{
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive glow
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glPointSize(5.0f);

    glBegin(GL_POINTS);
    for (int i = 0; i < 30; ++i)
    {
        const float angle = time * 0.5f + static_cast<float>(i) * 0.85f;
        const float fx = std::sin(angle * 1.3f + i * 2.1f) * 22.0f;
        const float fz = std::cos(angle * 1.1f + i * 1.7f) * 18.0f;
        const float fy = 0.6f + std::abs(std::sin(angle * 2.5f + i)) * 1.8f;
        const float pulse = 0.4f + 0.6f * std::abs(std::sin(time * 3.2f + i * 1.3f));

        glColor4f(0.85f * pulse, 0.98f * pulse, 0.25f * pulse, pulse);
        glVertex3f(fx, fy, fz);
    }
    glEnd();

    glPopAttrib();
}

void Scene::renderTransformationMarker() const
{
    glColor3f(0.88f, 0.22f, 0.10f);

    glPushMatrix();
    glTranslatef(-14.0f, 1.0f, 8.0f);
    glRotatef(25.0f, 0.0f, 1.0f, 0.0f);
    glScalef(2.0f, 1.0f, 0.75f);
    Primitives::drawCube(2.0f, 2.0f, 2.0f);
    glPopMatrix();
}

bool Scene::isNight() const
{
    return lighting_.isNight();
}
