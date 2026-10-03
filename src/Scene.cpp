#include "Scene.h"

#include "Input.h"
#include "graphics/Primitives.h"
#include "objects/Vegetation.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <random>
#include <vector>

namespace
{
float randomTrafficTime(std::mt19937& rng, float minimum, float maximum)
{
    std::uniform_real_distribution<float> distribution(minimum, maximum);
    return distribution(rng);
}

float treeScatterHash(int x, int z)
{
    const float value = std::sin(
        static_cast<float>(x) * 12.9898f +
        static_cast<float>(z) * 78.233f) * 43758.5453f;
    return value - std::floor(value);
}

// ---------------------------------------------------------------------------
// Performance settings
// ---------------------------------------------------------------------------
// Static vegetation (trees and crops) is recorded into OpenGL display lists.
// If your Vegetation::drawTree / drawCrop animate on their own (for example
// they sway using glfwGetTime inside), set this to false: culling still
// works, but the vegetation is redrawn live every frame.
constexpr bool kCacheVegetation = true;

constexpr int   kFarmListCount     = 12;
constexpr float kFarmCullRadius    = 32.0f;
constexpr float kTreeChunkSize     = 60.0f;
constexpr float kTreeCullMargin    = 18.0f;

// ---------------------------------------------------------------------------
// View frustum culling
// ---------------------------------------------------------------------------
// The frustum is rebuilt from the current projection and modelview matrices
// at the start of Scene::render(), so anything outside the view is skipped
// without changing what is visible on screen.
struct Frustum
{
    float plane[6][4];
};

Frustum gFrustum{};
bool gFrustumValid = false;
bool gFarmVisible[kFarmListCount] = {};

void captureFrustum()
{
    GLfloat proj[16];
    GLfloat view[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);

    // clip = projection * modelview (both column-major)
    float clip[16];
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            float value = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                value += proj[k * 4 + row] * view[col * 4 + k];
            }
            clip[col * 4 + row] = value;
        }
    }

    const auto rowOf = [&](int r, float out[4])
    {
        out[0] = clip[0 * 4 + r];
        out[1] = clip[1 * 4 + r];
        out[2] = clip[2 * 4 + r];
        out[3] = clip[3 * 4 + r];
    };

    float r0[4], r1[4], r2[4], r3[4];
    rowOf(0, r0);
    rowOf(1, r1);
    rowOf(2, r2);
    rowOf(3, r3);

    const float* rows[3] = {r0, r1, r2};
    bool valid = true;

    for (int axis = 0; axis < 3; ++axis)
    {
        for (int side = 0; side < 2; ++side)
        {
            const float sign = side == 0 ? 1.0f : -1.0f;
            float* p = gFrustum.plane[axis * 2 + side];
            for (int i = 0; i < 4; ++i)
            {
                p[i] = r3[i] + sign * rows[axis][i];
            }

            const float length = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
            if (!(length > 1e-6f))
            {
                valid = false;
                continue;
            }
            for (int i = 0; i < 4; ++i)
            {
                p[i] /= length;
            }
        }
    }

    gFrustumValid = valid;
}

bool sphereVisible(float x, float y, float z, float radius)
{
    if (!gFrustumValid)
    {
        return true;
    }

    for (int i = 0; i < 6; ++i)
    {
        const float* p = gFrustum.plane[i];
        if (p[0] * x + p[1] * y + p[2] * z + p[3] < -radius)
        {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Display-list helper
// ---------------------------------------------------------------------------
// Records `body` once and replays it afterwards. If a list cannot be created
// the body is simply drawn directly.
template <typename Body>
void cachedList(GLuint& list, Body&& body)
{
    if (list == 0)
    {
        list = glGenLists(1);
        if (list == 0)
        {
            body();
            return;
        }

        glNewList(list, GL_COMPILE);
        body();
        glEndList();
    }

    glCallList(list);
}

// Some primitives create their own helper data on first use, which is not
// allowed while a display list is being recorded. Touch them once, scaled to
// nothing, so nothing visible is drawn.
void warmUpDrawing()
{
    static bool done = false;
    if (done)
    {
        return;
    }
    done = true;

    glPushMatrix();
    glScalef(0.0f, 0.0f, 0.0f);
    Primitives::drawPlane(1.0f, 1.0f);
    Primitives::drawCube(1.0f, 1.0f, 1.0f);
    Vegetation::drawTree(0.0f, 0.0f, 1.0f);
    Vegetation::drawCrop(0.0f, 0.0f, 1.0f);
    glPopMatrix();
}

// ---------------------------------------------------------------------------
// Scattered world trees: placed once, grouped into chunks
// ---------------------------------------------------------------------------
struct TreeInstance
{
    float x;
    float z;
    float size;
};

struct TreeChunk
{
    std::vector<TreeInstance> trees;
    GLuint list = 0;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float centerZ = 0.0f;
    float radius = 0.0f;
};

std::vector<TreeChunk> gTreeChunks;
bool gTreeChunksBuilt = false;

void buildTreeChunks()
{
    gTreeChunksBuilt = true;

    // Reuse the original tree model while filling the enlarged terrain.
    // The central exclusion corridor keeps trees away from the road and poles.
    constexpr int minCoordinate = -450;
    constexpr int maxCoordinate = 450;
    constexpr int xSpacing = 14;
    constexpr int zSpacing = 18;
    constexpr float roadClearance = 14.0f;
    constexpr float farmSideOffset = 38.0f;
    constexpr float farmSpacing = 34.0f;

    const auto clearForTree = [&](float x, float z)
    {
        if (std::fabs(x) < roadClearance)
        {
            return false;
        }

        // Keep the enlarged farm footprints, including their crop fields,
        // houses, fences, and equipment, free of world-scattered trees.
        for (int row = -2; row <= 3; ++row)
        {
            const float farmZ = static_cast<float>(row) * farmSpacing;
            if (std::fabs(z - farmZ) < 19.0f &&
                std::fabs(std::fabs(x) - farmSideOffset) < 23.0f)
            {
                return false;
            }
        }

        // Preserve open space around ponds, barns, the substation, and the sun.
        const float landmarks[][3] = {
            {-72.0f, -42.0f, 25.0f},
            { 73.0f,  24.0f, 27.0f},
            {-71.0f,  58.0f, 23.0f},
            {-65.0f, -68.0f, 14.0f},
            { 65.0f, -34.0f, 14.0f},
            {-65.0f,  34.0f, 14.0f},
            { 65.0f,  68.0f, 14.0f},
            { 70.0f,  92.0f, 17.0f},
            {-10.0f, -18.0f, 10.0f}};
        for (const auto& landmark : landmarks)
        {
            const float dx = x - landmark[0];
            const float dz = z - landmark[1];
            if (dx * dx + dz * dz < landmark[2] * landmark[2])
            {
                return false;
            }
        }

        return true;
    };

    std::vector<TreeInstance> all;
    all.reserve(4096);

    for (int x = minCoordinate; x <= maxCoordinate; x += xSpacing)
    {
        for (int z = minCoordinate; z <= maxCoordinate; z += zSpacing)
        {
            const float placement = treeScatterHash(x, z);
            const bool outerWorld = std::fabs(static_cast<float>(x)) > 105.0f;
            const bool guaranteedForest = outerWorld
                && (std::abs(x / xSpacing + z / zSpacing) % 5 != 0);
            if (!guaranteedForest && placement < 0.16f)
            {
                continue;
            }

            const float offsetX = (treeScatterHash(x - 11, z + 7) - 0.5f) * 8.0f;
            const float offsetZ = (treeScatterHash(x + 5, z + 19) - 0.5f) * 10.0f;
            const float worldX = static_cast<float>(x) + offsetX;
            const float worldZ = static_cast<float>(z) + offsetZ;
            if (!clearForTree(worldX, worldZ))
            {
                continue;
            }

            const float size = 1.75f + treeScatterHash(x + 17, z - 31) * 2.45f;
            all.push_back({worldX, worldZ, size});
        }
    }

    // Dense, clearly visible forest clusters in the open world between the
    // farms and the distant terrain.
    for (int side : {-1, 1})
    {
        for (int cluster = 0; cluster < 12; ++cluster)
        {
            const float centerX = static_cast<float>(side) *
                (125.0f + static_cast<float>(cluster % 3) * 55.0f);
            const float centerZ = -420.0f + static_cast<float>(cluster) * 120.0f;

            for (int row = -3; row <= 3; ++row)
            {
                for (int column = -3; column <= 3; ++column)
                {
                    const float worldX = centerX
                        + static_cast<float>(column) * 10.0f;
                    const float worldZ = centerZ
                        + static_cast<float>(row) * 11.0f;
                    const float size = 1.80f + treeScatterHash(
                        cluster * 17 + row, column - side * 13) * 2.20f;
                    if (clearForTree(worldX, worldZ))
                    {
                        all.push_back({worldX, worldZ, size});
                    }
                }
            }
        }
    }

    if (all.empty())
    {
        return;
    }

    // Group the trees into square chunks so whole groups can be skipped.
    float minX = all[0].x;
    float maxX = all[0].x;
    float minZ = all[0].z;
    float maxZ = all[0].z;
    for (const TreeInstance& tree : all)
    {
        minX = std::min(minX, tree.x);
        maxX = std::max(maxX, tree.x);
        minZ = std::min(minZ, tree.z);
        maxZ = std::max(maxZ, tree.z);
    }

    const int columns = static_cast<int>((maxX - minX) / kTreeChunkSize) + 1;
    const int rows = static_cast<int>((maxZ - minZ) / kTreeChunkSize) + 1;
    gTreeChunks.assign(static_cast<std::size_t>(columns) * rows, TreeChunk{});

    for (const TreeInstance& tree : all)
    {
        const int column = static_cast<int>((tree.x - minX) / kTreeChunkSize);
        const int row = static_cast<int>((tree.z - minZ) / kTreeChunkSize);
        gTreeChunks[static_cast<std::size_t>(row) * columns + column].trees.push_back(tree);
    }

    for (TreeChunk& chunk : gTreeChunks)
    {
        if (chunk.trees.empty())
        {
            continue;
        }

        float cMinX = chunk.trees[0].x;
        float cMaxX = cMinX;
        float cMinZ = chunk.trees[0].z;
        float cMaxZ = cMinZ;
        for (const TreeInstance& tree : chunk.trees)
        {
            cMinX = std::min(cMinX, tree.x);
            cMaxX = std::max(cMaxX, tree.x);
            cMinZ = std::min(cMinZ, tree.z);
            cMaxZ = std::max(cMaxZ, tree.z);
        }

        const float halfX = (cMaxX - cMinX) * 0.5f;
        const float halfZ = (cMaxZ - cMinZ) * 0.5f;
        chunk.centerX = (cMinX + cMaxX) * 0.5f;
        chunk.centerZ = (cMinZ + cMaxZ) * 0.5f;
        chunk.centerY = kTreeCullMargin * 0.4f;
        chunk.radius = std::sqrt(halfX * halfX + halfZ * halfZ) + kTreeCullMargin;
    }
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
      cropWorkers_{
          Farmer(-5.5f, -9.0f, FarmerRoute::CropWork, 1.0f, 0.3f),
          Farmer(4.5f, -8.0f, FarmerRoute::CropWork, 0.9f, 1.1f),
          Farmer(-3.0f, -7.0f, FarmerRoute::CropWork, 1.1f, 1.9f),
          Farmer(5.0f, -6.0f, FarmerRoute::CropWork, 0.8f, 2.7f),
          Farmer(-6.0f, -8.0f, FarmerRoute::CropWork, 1.0f, 3.5f),
          Farmer(3.5f, -9.0f, FarmerRoute::CropWork, 0.9f, 4.3f),
          Farmer(-4.5f, -6.5f, FarmerRoute::CropWork, 1.1f, 5.1f),
          Farmer(5.5f, -8.5f, FarmerRoute::CropWork, 0.8f, 5.9f),
          Farmer(-5.0f, -7.5f, FarmerRoute::CropWork, 1.0f, 6.7f),
          Farmer(4.0f, -6.0f, FarmerRoute::CropWork, 0.9f, 7.5f),
          Farmer(-3.5f, -8.5f, FarmerRoute::CropWork, 1.1f, 8.3f),
          Farmer(5.0f, -7.0f, FarmerRoute::CropWork, 0.8f, 9.1f)},
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
    std::uniform_int_distribution<int> chickenCountDistribution(0, 3);
    for (FarmLayout& layout : farmLayouts_)
    {
        layout.chickenCount = chickenCountDistribution(trafficRng_);
    }

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
                farmers_[index].setRoadPosition(-108.0f);
            }
        }

        if (farmerTrafficActive_[index])
        {
            farmers_[index].update(deltaTime);
        }
    }

    for (Farmer& worker : cropWorkers_)
    {
        worker.update(deltaTime);
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
    // Work out what is on screen once per frame.
    warmUpDrawing();
    captureFrustum();

    constexpr float farmScale = 0.78f;
    constexpr float farmSpacing = 34.0f;
    constexpr float farmSideOffset = 38.0f;

    for (int row = -2; row <= 3; ++row)
    {
        const float z = static_cast<float>(row) * farmSpacing;
        const int layoutIndex = (row + 2) * 2;
        gFarmVisible[layoutIndex] =
            sphereVisible(-farmSideOffset, 5.0f, z, kFarmCullRadius);
        gFarmVisible[layoutIndex + 1] =
            sphereVisible(farmSideOffset, 5.0f, z, kFarmCullRadius);
    }

    renderGround();
    renderRoad();
    PowerSubstation::drawRoadUtilities(animation_.waterTime());
    renderScatteredTrees();
    renderPonds();
    renderFarmers();
    renderRoadTractors();
    renderBarns();
    renderCropWorkers();
    // Keep the full substation visible beside the central road, just beyond
    // the roadside utility poles and outside the traffic lane.
    if (sphereVisible(16.0f, 4.0f, 0.0f, 28.0f))
    {
        PowerSubstation::draw(16.0f, 0.0f, 0.90f, animation_.waterTime());
    }

    // Enlarge each complete farm while retaining a wide road corridor.
    for (int row = -2; row <= 3; ++row)
    {
        const float z = static_cast<float>(row) * farmSpacing;
        const int layoutIndex = (row + 2) * 2;
        if (gFarmVisible[layoutIndex])
        {
            renderFarm(-farmSideOffset, z, farmScale, farmLayouts_[layoutIndex]);
        }
        if (gFarmVisible[layoutIndex + 1])
        {
            renderFarm(farmSideOffset, z, farmScale, farmLayouts_[layoutIndex + 1]);
        }
    }

    Sky::drawSun();
    Cloud::drawField();
    // renderTransformationMarker();
}

void Scene::renderBarns() const
{
    if (sphereVisible(-65.0f, 6.0f, -68.0f, 26.0f))
    {
        Barn::draw(-65.0f, -68.0f, 1.02f, -4.0f, true);
    }
    if (sphereVisible(65.0f, 6.0f, -34.0f, 26.0f))
    {
        Barn::draw(65.0f, -34.0f, 0.96f, 5.0f, false);
    }
    if (sphereVisible(-65.0f, 6.0f, 34.0f, 26.0f))
    {
        Barn::draw(-65.0f, 34.0f, 1.08f, -2.0f, true);
    }
    if (sphereVisible(65.0f, 6.0f, 68.0f, 26.0f))
    {
        Barn::draw(65.0f, 68.0f, 1.00f, 7.0f, true);
    }
}

void Scene::renderCropWorkers() const
{
    constexpr float farmScale = 0.78f;
    constexpr float farmSpacing = 34.0f;
    constexpr float farmSideOffset = 38.0f;

    for (int row = -2; row <= 3; ++row)
    {
        const float z = static_cast<float>(row) * farmSpacing;
        const int layoutIndex = (row + 2) * 2;
        for (int side = 0; side < 2; ++side)
        {
            const int workerIndex = layoutIndex + side;
            if (!gFarmVisible[workerIndex])
            {
                continue;
            }

            glPushMatrix();
            glTranslatef(
                side == 0 ? -farmSideOffset : farmSideOffset,
                0.0f,
                z);
            glScalef(farmScale, farmScale, farmScale);
            glRotatef(
                farmLayouts_[workerIndex].rotation,
                0.0f, 1.0f, 0.0f);
            glTranslatef(
                farmLayouts_[workerIndex].cropOffsetX,
                0.0f,
                farmLayouts_[workerIndex].cropOffsetZ);
            cropWorkers_[workerIndex].render();
            glPopMatrix();
        }

    }
}

void Scene::renderScatteredTrees() const
{
    // Tree placement is computed once; each frame only the chunks that are in
    // view are drawn.
    if (!gTreeChunksBuilt)
    {
        buildTreeChunks();
    }

    for (TreeChunk& chunk : gTreeChunks)
    {
        if (chunk.trees.empty()
            || !sphereVisible(chunk.centerX, chunk.centerY, chunk.centerZ, chunk.radius))
        {
            continue;
        }

        const auto drawChunk = [&chunk]()
        {
            for (const TreeInstance& tree : chunk.trees)
            {
                Vegetation::drawTree(tree.x, tree.z, tree.size);
            }
        };

        if (kCacheVegetation)
        {
            cachedList(chunk.list, drawChunk);
        }
        else
        {
            drawChunk();
        }
    }
}

void Scene::renderPonds() const
{
    // Keep the enlarged farms and ponds separated from the central road.
    if (sphereVisible(-72.0f, 0.0f, -42.0f, 30.0f))
    {
        Pond::draw(-72.0f, -42.0f, 23.0f, 16.0f, animation_.waterTime(),        false);
    }
    if (sphereVisible(73.0f, 0.0f, 24.0f, 32.0f))
    {
        Pond::draw( 73.0f,  24.0f, 25.0f, 17.0f, animation_.waterTime() + 1.4f, true);
    }
    if (sphereVisible(-71.0f, 0.0f, 58.0f, 28.0f))
    {
        Pond::draw(-71.0f,  58.0f, 21.0f, 15.0f, animation_.waterTime() + 2.8f, false);
    }
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
        if (tractorTrafficActive_[index]
            && sphereVisible(0.0f, 2.0f, roadTractorZ_[index], 14.0f))
        {
            tractor_.drawRoadTractor(roadTractorZ_[index], colors[index]);
        }
    }
}

void Scene::renderGround() const
{
    // The ground never changes, so it is recorded once and replayed.
    static GLuint groundList = 0;

    cachedList(groundList, [this]()
    {
        glDisable(GL_LIGHTING);
        glColor3f(0.20f, 0.50f, 0.20f);

        glPushMatrix();
        glTranslatef(0.0f, -0.02f, 0.0f);
        glScalef(1.0f, 1.0f, 0.85f);
        // Oversized terrain gives the camera a continuous horizon beyond the
        // designed farm area.
        Primitives::drawPlane(1000.0f, 1000.0f);
        glPopMatrix();

 
        // Keep a light reference grid without spending a draw call on every
        // single world unit across the entire expanded map.
        for (int coordinate = -110; coordinate <= 110; coordinate += 5)
        {
            const float value = static_cast<float>(coordinate);
            glColor3f(0.24f, 0.56f, 0.24f);
            glVertex3f(value, 0.01f, -110.0f);
            glVertex3f(value, 0.01f, 110.0f);
            glVertex3f(-110.0f, 0.01f, value);
            glVertex3f(110.0f, 0.01f, value);
        }
        glEnd();
    });
}

void Scene::renderRoad() const
{
    static GLuint roadList = 0;

    cachedList(roadList, [this]()
    {
        glDisable(GL_LIGHTING);

        glColor3f(0.22f, 0.22f, 0.20f);
        glPushMatrix();
        glTranslatef(0.0f, 0.045f, 0.0f);
        Primitives::drawPlane(9.0f, 1000.0f);
        glPopMatrix();

        glColor3f(0.86f, 0.75f, 0.24f);
        glPushMatrix();
        glTranslatef(0.0f, 0.06f, 0.0f);
        glScalef(0.12f, 1.0f, 1000.0f);
        Primitives::drawCube(1.0f, 0.02f, 0.035f);
        glPopMatrix();
    });
}

void Scene::renderFarm(
    float x, float z, float scale, const FarmLayout& layout) const
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);
    glRotatef(layout.rotation, 0.0f, 1.0f, 0.0f);

    // Farms never change layout after construction, so the static parts are
    // recorded once per farm. The animated parts (animals, house, tractor,
    // windmill) are still drawn live, in the original order.
    const std::ptrdiff_t farmIndex = &layout - &farmLayouts_[0];
    const bool cacheable = farmIndex >= 0 && farmIndex < kFarmListCount;
    static GLuint farmLists[kFarmListCount][3] = {};

    // Part A: crop field, crops and trees.
    const auto drawFieldAndTrees = [&]()
    {
        glPushMatrix();
        glTranslatef(layout.cropOffsetX, 0.0f, layout.cropOffsetZ);
        renderCropField();
        renderCrops();
        glPopMatrix();

        const float treePositions[5][2] = {
            {-18.0f, -13.0f}, {-12.0f, -16.0f}, {18.0f, -13.0f},
            {19.0f, 1.0f}, {-18.0f, 9.0f}};
        const float treeScales[5] = {1.75f, 1.35f, 1.55f, 1.20f, 1.60f};
        for (int tree = 0; tree < layout.treeCount; ++tree)
        {
            Vegetation::drawTree(
                treePositions[tree][0], treePositions[tree][1], treeScales[tree]);
        }
    };

    // Part B: path and fence.
    const auto drawPathAndBoundary = [&]()
    {
        renderPath();
        renderBoundary();
    };

    // Part C: rocks.
    const auto drawRocks = [&]()
    {
        renderRocks();
    };

    if (cacheable && kCacheVegetation)
    {
        cachedList(farmLists[farmIndex][0], drawFieldAndTrees);
    }
    else
    {
        drawFieldAndTrees();
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

    const float chickenPositions[3][4] = {
        {-8.0f, 5.0f, 0.62f, 28.0f},
        {8.5f, 8.0f, 0.55f, -32.0f},
        {0.0f, 13.0f, 0.58f, 5.0f}};
    for (int chicken = 0; chicken < layout.chickenCount; ++chicken)
    {
        Animals::drawChicken(
            chickenPositions[chicken][0], chickenPositions[chicken][1],
            chickenPositions[chicken][2], chickenPositions[chicken][3]);
    }

    if (cacheable)
    {
        cachedList(farmLists[farmIndex][1], drawPathAndBoundary);
    }
    else
    {
        drawPathAndBoundary();
    }

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

    if (cacheable)
    {
        cachedList(farmLists[farmIndex][2], drawRocks);
    }
    else
    {
        drawRocks();
    }

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