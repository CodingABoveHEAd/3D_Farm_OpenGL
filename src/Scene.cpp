#include "Scene.h"

#include "Input.h"
#include "graphics/Primitives.h"
#include "objects/Vegetation.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <initializer_list>
#include <random>

namespace
{
float randomTrafficTime(std::mt19937& rng, float minimum, float maximum)
{
    std::uniform_real_distribution<float> distribution(minimum, maximum);
    return distribution(rng);
}
}

Scene::Scene()
    : farmers_{
          Farmer(-2.0f, -55.0f, FarmerRoute::Road, 3.0f, 0.0f),
          Farmer(2.0f, -45.0f, FarmerRoute::Road, 2.4f, 1.2f),
          Farmer(-2.0f, -32.0f, FarmerRoute::Road, 2.7f, 2.1f),
          Farmer(2.0f, -18.0f, FarmerRoute::Road, 2.2f, 2.8f),
          Farmer(-2.0f, -4.0f, FarmerRoute::Road, 2.8f, 3.5f),
          Farmer(2.0f, 24.0f, FarmerRoute::Road, 2.5f, 4.2f)},
      farmLayouts_{
          FarmLayout{2, 1, true, true, -1.2f, 0.4f, 1.0f, 0.5f, -2.0f},
          FarmLayout{5, 2, false, true, 1.1f, -0.6f, -0.8f, 0.3f, 3.0f},
          FarmLayout{3, 0, true, false, -0.5f, 1.0f, 0.7f, -0.5f, -4.0f},
          FarmLayout{4, 2, false, true, 0.8f, 0.7f, -1.0f, -0.2f, 2.0f},
          FarmLayout{2, 1, false, false, -1.0f, -0.8f, 0.5f, 0.6f, -3.0f},
          FarmLayout{5, 0, true, true, 0.6f, 0.3f, -0.6f, -0.4f, 4.0f},
          FarmLayout{3, 2, false, true, -0.8f, 0.8f, 1.1f, 0.2f, -2.5f},
          FarmLayout{4, 1, true, false, 1.0f, -0.4f, -0.5f, 0.5f, 3.5f},
          FarmLayout{2, 2, false, true, -0.7f, -0.5f, 0.8f, -0.3f, -3.5f},
          FarmLayout{5, 0, true, true, 0.5f, 0.9f, -0.9f, 0.4f, 2.5f},
          FarmLayout{3, 1, false, false, -1.1f, 0.2f, 0.6f, -0.6f, -1.5f},
          FarmLayout{4, 2, true, true, 0.9f, -0.9f, -0.7f, 0.1f, 1.5f}},
      roadTractorZ_{-60.0f, -20.0f, 22.0f, 60.0f},
      trafficRng_(std::random_device{}())
{
    for (std::size_t index = 0; index < farmers_.size(); ++index)
    {
        farmerTrafficActive_[index] = index % 3 != 0;
        farmerTrafficTimers_[index] = randomTrafficTime(
            trafficRng_,
            farmerTrafficActive_[index] ? 18.0f : 2.0f,
            farmerTrafficActive_[index] ? 42.0f : 10.0f);
        farmers_[index].setVisible(farmerTrafficActive_[index]);
    }

    for (std::size_t index = 0; index < roadTractorZ_.size(); ++index)
    {
        tractorTrafficActive_[index] = index % 2 == 0;
        tractorTrafficTimers_[index] = randomTrafficTime(
            trafficRng_,
            tractorTrafficActive_[index] ? 14.0f : 3.0f,
            tractorTrafficActive_[index] ? 32.0f : 12.0f);
    }

    Cloud::initField();
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
    if (Input::wasPressed(GLFW_KEY_EQUAL) || Input::wasPressed(GLFW_KEY_KP_ADD))
    {
        animation_.changeWindmillSpeed(15.0f);
    }
    if (Input::wasPressed(GLFW_KEY_MINUS) || Input::wasPressed(GLFW_KEY_KP_SUBTRACT))
    {
        animation_.changeWindmillSpeed(-15.0f);
    }
}

void Scene::update(float deltaTime)
{
    animation_.update(deltaTime);
    tractor_.update(deltaTime);
    for (std::size_t index = 0; index < farmers_.size(); ++index)
    {
        farmerTrafficTimers_[index] -= deltaTime;
        if (farmerTrafficTimers_[index] <= 0.0f)
        {
            farmerTrafficActive_[index] = !farmerTrafficActive_[index];
            farmers_[index].setVisible(farmerTrafficActive_[index]);
            farmerTrafficTimers_[index] = randomTrafficTime(
                trafficRng_,
                farmerTrafficActive_[index] ? 18.0f : 2.0f,
                farmerTrafficActive_[index] ? 42.0f : 10.0f);
            if (farmerTrafficActive_[index])
            {
                farmers_[index].setRoadPosition(-78.0f);
            }
        }

        if (farmerTrafficActive_[index])
        {
            farmers_[index].update(deltaTime);
        }
    }

    for (std::size_t index = 0; index < roadTractorZ_.size(); ++index)
    {
        tractorTrafficTimers_[index] -= deltaTime;
        if (tractorTrafficTimers_[index] <= 0.0f)
        {
            tractorTrafficActive_[index] = !tractorTrafficActive_[index];
            tractorTrafficTimers_[index] = randomTrafficTime(
                trafficRng_,
                tractorTrafficActive_[index] ? 14.0f : 3.0f,
                tractorTrafficActive_[index] ? 32.0f : 12.0f);
            if (tractorTrafficActive_[index])
            {
                roadTractorZ_[index] = -78.0f;
            }
        }

        if (tractorTrafficActive_[index])
        {
            tractor_.updateRoad(deltaTime, roadTractorZ_[index]);
        }
    }
    Cloud::updateField(deltaTime);
}

void Scene::render() const
{
    renderGround();
    renderRoad();
    renderPonds();
    renderFarmers();
    renderRoadTractors();
    renderBarns();

    constexpr float farmScale = 0.50f;
    constexpr float farmSpacing = 24.0f;
    constexpr float farmSideOffset = 25.0f;

    for (int row = -2; row <= 3; ++row)
    {
        const float z = static_cast<float>(row) * farmSpacing;
        const int layoutIndex = (row + 2) * 2;
        renderFarm(-farmSideOffset, z, farmScale, farmLayouts_[layoutIndex]);
        renderFarm(farmSideOffset, z, farmScale, farmLayouts_[layoutIndex + 1]);
    }

    Sky::drawSun();
    Cloud::drawField();
    // renderTransformationMarker();
}

void Scene::renderBarns() const
{
    Barn::draw(-42.0f, -52.0f, 0.72f, -4.0f, true);
    Barn::draw(42.0f, -28.0f, 0.66f, 5.0f, false);
    Barn::draw(-42.0f, 20.0f, 0.78f, -2.0f, true);
    Barn::draw(42.0f, 52.0f, 0.70f, 7.0f, true);
}

void Scene::renderPonds() const
{
    // Farms occupy roughly x = 13..37 on each side (farm centre 25, half-width 24 * 0.5 scale),
    // so ponds sit just outside that edge, close to the farms.
    Pond::draw(-48.0f, -30.0f, 16.0f, 11.0f, animation_.waterTime(),        false);
    Pond::draw( 49.0f,  18.0f, 18.0f, 12.0f, animation_.waterTime() + 1.4f, true);
    Pond::draw(-47.0f,  40.0f, 14.0f, 10.0f, animation_.waterTime() + 2.8f, false);
}

void Scene::renderFarmers() const
{
    for (const Farmer& farmer : farmers_)
    {
        farmer.render();
    }
}

void Scene::renderRoadTractors() const
{
    constexpr Tractor::Color colors[] = {
        Tractor::Color::Red,
        Tractor::Color::Green,
        Tractor::Color::Blue,
        Tractor::Color::Brown};

    for (std::size_t index = 0; index < roadTractorZ_.size(); ++index)
    {
        if (tractorTrafficActive_[index])
        {
            tractor_.drawRoadTractor(roadTractorZ_[index], colors[index]);
        }
    }
}

void Scene::renderGround() const
{
    glDisable(GL_LIGHTING);
    glColor3f(0.20f, 0.50f, 0.20f);

    glPushMatrix();
    glTranslatef(0.0f, -0.02f, 0.0f);
    glScalef(1.0f, 1.0f, 0.85f);
    Primitives::drawPlane(150.0f, 150.0f);
    glPopMatrix();

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    // Keep a light reference grid without spending a draw call on every
    // single world unit across the entire expanded map.
    for (int coordinate = -75; coordinate <= 75; coordinate += 5)
    {
        const float value = static_cast<float>(coordinate);
        glColor3f(0.24f, 0.56f, 0.24f);
        glVertex3f(value, 0.01f, -75.0f);
        glVertex3f(value, 0.01f, 75.0f);
        glVertex3f(-75.0f, 0.01f, value);
        glVertex3f(75.0f, 0.01f, value);
    }
    glEnd();
}

void Scene::renderRoad() const
{
    glDisable(GL_LIGHTING);

    glColor3f(0.22f, 0.22f, 0.20f);
    glPushMatrix();
    glTranslatef(0.0f, 0.045f, 0.0f);
    Primitives::drawPlane(9.0f, 150.0f);
    glPopMatrix();

    glColor3f(0.86f, 0.75f, 0.24f);
    glPushMatrix();
    glTranslatef(0.0f, 0.06f, 0.0f);
    glScalef(0.12f, 1.0f, 150.0f);
    Primitives::drawCube(1.0f, 0.02f, 0.035f);
    glPopMatrix();
}

void Scene::renderFarm(
    float x, float z, float scale, const FarmLayout& layout) const
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);
    glRotatef(layout.rotation, 0.0f, 1.0f, 0.0f);

    glPushMatrix();
    glTranslatef(layout.cropOffsetX, 0.0f, layout.cropOffsetZ);
    renderCropField();
    renderCrops();
    glPopMatrix();

    const float treePositions[5][2] = {
        {-18.0f, -13.0f}, {-12.0f, -16.0f}, {18.0f, -13.0f},
        {19.0f, 1.0f}, {-18.0f, 9.0f}};
    const float treeScales[5] = {1.45f, 1.05f, 1.25f, 0.90f, 1.30f};
    for (int tree = 0; tree < layout.treeCount; ++tree)
    {
        Vegetation::drawTree(
            treePositions[tree][0], treePositions[tree][1], treeScales[tree]);
    }

    const float animalPositions[2][4] = {
        {-16.0f, -2.0f, 1.05f, 8.0f},
        {16.0f, -7.0f, 0.90f, -18.0f}};
    for (int animal = 0; animal < layout.animalCount; ++animal)
    {
        Animals::drawCow(
            animalPositions[animal][0], animalPositions[animal][1],
            animalPositions[animal][2], animalPositions[animal][3]);
    }

    renderPath();
    renderBoundary();

    glPushMatrix();
    glTranslatef(layout.houseOffsetX, 0.0f, layout.houseOffsetZ);
    farmhouse_.render();
    glPopMatrix();

    if (layout.hasTractor)
    {
        tractor_.drawTractor();
    }
    if (layout.hasWindmill)
    {
        Windmill::drawWindmill(
            -14.0f, 1.0f, animation_.windmillAngle(), 1.15f);
    }
    renderRocks();

    glPopMatrix();
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
    for (int row = 0; row < 6; ++row)
    {
        const float z = -10.0f + static_cast<float>(row) * 1.35f;
        for (int column = 0; column < 7; ++column)
        {
            const float x = -7.5f + static_cast<float>(column) * 2.5f;
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

void Scene::renderAnimals() const
{
    // Place cows in open pasture areas, away from the crop rows and buildings.
    Animals::drawCow(-16.0f, -2.0f, 1.05f, 8.0f);
    Animals::drawCow(16.0f, -7.0f, 0.90f, -18.0f);
    Animals::drawCow(-15.0f, 14.0f, 0.82f, 28.0f);
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

    drawGatePanel(9.0f, -5.0f);
    drawGatePanel(11.0f, 5.0f);
}

void Scene::drawGatePanel(float x, float angle) const
{
    for (float y : {0.70f, 1.45f})
    {
        glPushMatrix();
        glTranslatef(x, y, -19.0f);
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
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
