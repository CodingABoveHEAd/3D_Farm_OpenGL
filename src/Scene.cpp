#include "Scene.h"

#include "DayNightSettings.h"
#include "Input.h"
#include "graphics/Primitives.h"
#include "objects/Vegetation.h"
#include "objects/Birds.h"
#include "objects/Village.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iostream>
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

// Master switch for every display list in this file (ground, road, trees,
// farm parts). If the window still comes up white or crashes, set this to
// false: the scene then draws exactly as before the optimization, only
// slower, and it tells us whether display lists are the problem.
constexpr bool kUseDisplayLists = true;

constexpr int   kFarmListCount     = 12;
constexpr float kFarmCullRadius    = 32.0f;
constexpr float kTreeChunkSize     = 60.0f;
constexpr float kTreeCullMargin    = 18.0f;

// Optional draw distances. 0 means unlimited, which keeps the scene exactly
// as designed. If the frame rate is still too low on a CPU-only machine, try
// something like 350 for trees and 400 for farms: far objects are skipped
// entirely. Leave both at 0 to change nothing visually.
constexpr float kTreeMaxDistance = 225.0f;
constexpr float kFarmMaxDistance = 360.0f;

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
float gEye[3] = {0.0f, 0.0f, 0.0f};

void captureFrustum()
{
    GLfloat proj[16];
    GLfloat view[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetFloatv(GL_MODELVIEW_MATRIX, view);

    // Camera position from the view matrix (eye = -R^T * t); only used by the
    // optional draw-distance limits above.
    gEye[0] = -(view[0] * view[12] + view[1] * view[13] + view[2] * view[14]);
    gEye[1] = -(view[4] * view[12] + view[5] * view[13] + view[6] * view[14]);
    gEye[2] = -(view[8] * view[12] + view[9] * view[13] + view[10] * view[14]);

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

// True when the optional distance limit is off or the object is inside it.
bool withinDistance(float x, float z, float maxDistance, float radius)
{
    if (maxDistance <= 0.0f)
    {
        return true;
    }

    const float dx = x - gEye[0];
    const float dz = z - gEye[2];
    const float limit = maxDistance + radius;
    return dx * dx + dz * dz <= limit * limit;
}

// ---------------------------------------------------------------------------
// Display-list helper
// ---------------------------------------------------------------------------
// Records `body` once and replays it afterwards. If a list cannot be created
// the body is simply drawn directly.
template <typename Body>
void cachedList(GLuint& list, Body&& body)
{
    // Master switch: false draws everything live (no display lists at all).
    if (!kUseDisplayLists)
    {
        body();
        return;
    }

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
    GLuint detailedList = 0;
    GLuint simpleList = 0;
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
            {-82.0f, -42.0f, 30.0f},
            { 73.0f,  24.0f, 29.0f},
            {-71.0f,  58.0f, 27.0f},
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

        for (int site = 0; site < VillageSimulationSettings::BonfireSiteCount; ++site)
        {
            const float dx = x - VillageSimulationSettings::BonfireSiteX[site];
            const float dz = z - VillageSimulationSettings::BonfireSiteZ[site];
            if (dx * dx + dz * dz
                < VillageSimulationSettings::BonfireTreeClearance
                    * VillageSimulationSettings::BonfireTreeClearance)
                return false;
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

void drawSimpleTree(const TreeInstance& tree)
{
    glPushMatrix();
    glTranslatef(tree.x, 0.0f, tree.z);
    glScalef(tree.size, tree.size, tree.size);

    glColor3f(0.30f, 0.16f, 0.07f);
    glPushMatrix();
    glTranslatef(0.0f, 1.6f, 0.0f);
    Primitives::drawCube(0.55f, 3.2f, 0.55f);
    glPopMatrix();

    // Three low-poly crowns retain the silhouette and layered greens of the
    // detailed tree while using a small fraction of its vertices.
    const float crowns[][5] = {
        {-0.72f, 3.75f, 0.0f, 1.25f, 0.31f},
        { 0.72f, 3.82f, 0.0f, 1.20f, 0.38f},
        { 0.00f, 4.65f, 0.0f, 1.45f, 0.44f}};
    for (const auto& crown : crowns)
    {
        glColor3f(0.12f, crown[4], 0.10f);
        glPushMatrix();
        glTranslatef(crown[0], crown[1], crown[2]);
        glScalef(1.0f, 0.82f, 1.0f);
        Primitives::drawSphere(crown[3], 5, 3);
        glPopMatrix();
    }
    glPopMatrix();
}

void drawTreeChunk(const TreeChunk& chunk, bool simplified)
{
    for (const TreeInstance& tree : chunk.trees)
    {
        if (simplified)
        {
            drawSimpleTree(tree);
        }
        else
        {
            Vegetation::drawTree(tree.x, tree.z, tree.size);
        }
    }
}

}

Scene::Scene()
    : farmers_{
          Farmer(-8.5f, -55.0f, FarmerRoute::Road, 3.0f, 0.0f),
          Farmer(8.5f, -45.0f, FarmerRoute::Road, 2.4f, 1.2f),
          Farmer(-8.5f, -32.0f, FarmerRoute::Road, 2.7f, 2.1f),
          Farmer(8.5f, -10.0f, FarmerRoute::Road, 2.2f, 2.8f)},
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
      roadTractorZ_{},
      trafficRng_(std::random_device{}())
{
    constexpr float routeLength = VillageSimulationSettings::TrafficRouteMaxZ
        - VillageSimulationSettings::TrafficRouteMinZ;
    constexpr float trafficGap = routeLength
        / static_cast<float>(VillageSimulationSettings::TrafficVehicleCount);
    static_assert(trafficGap >= VillageSimulationSettings::TrafficMinimumSpacing,
                  "Traffic route is too short for the configured safe spacing");

    std::uniform_int_distribution<int> chickenCountDistribution(0, 3);
    for (FarmLayout& layout : farmLayouts_)
    {
        layout.chickenCount = chickenCountDistribution(trafficRng_);
    }

    for (std::size_t index = 0; index < farmers_.size(); ++index)
    {
        farmerTrafficActive_[index] = index % 2 == 0;
        farmerTrafficTimers_[index] = randomTrafficTime(
            trafficRng_,
            farmerTrafficActive_[index]
                ? VillageSimulationSettings::PedestrianActiveMinSeconds
                : VillageSimulationSettings::PedestrianWaitMinSeconds,
            farmerTrafficActive_[index]
                ? VillageSimulationSettings::PedestrianActiveMaxSeconds
                : VillageSimulationSettings::PedestrianWaitMaxSeconds);
        farmers_[index].setVisible(farmerTrafficActive_[index]);
    }

    for (std::size_t index = 0; index < roadTractorZ_.size(); ++index)
    {
        roadTractorZ_[index] = VillageSimulationSettings::TrafficRouteMinZ
            + trafficGap * (static_cast<float>(index) + 0.5f);
        tractorTrafficActive_[index] = true;
    }

    // Randomize populations once, then retain those choices for the complete
    // run. Sampling here (rather than while drawing) prevents visible flicker.
    std::uniform_real_distribution<float> workerJitter(-0.35f, 0.35f);
    constexpr float workerX[] = {-5.4f, 0.0f, 5.4f};
    constexpr float workerZ[] = {-9.0f, -6.8f, -8.0f};
    std::array<int, VillageSimulationSettings::FarmCount> workerCounts{};
    static_assert(
        VillageSimulationSettings::FarmCount
            % (VillageSimulationSettings::MaxWorkersPerFarm + 1) == 0,
        "Balanced worker population requires complete 0..max groups");
    for (int farm = 0; farm < VillageSimulationSettings::FarmCount; ++farm)
        workerCounts[farm] = farm % (VillageSimulationSettings::MaxWorkersPerFarm + 1);
    std::shuffle(workerCounts.begin(), workerCounts.end(), trafficRng_);
    cropWorkers_.reserve(
        VillageSimulationSettings::FarmCount
        * VillageSimulationSettings::MaxWorkersPerFarm);
    for (int farm = 0; farm < VillageSimulationSettings::FarmCount; ++farm)
    {
        cropWorkerStarts_[farm] = static_cast<unsigned char>(cropWorkers_.size());
        const int count = workerCounts[farm];
        cropWorkerCounts_[farm] = static_cast<unsigned char>(count);
        for (int worker = 0; worker < count; ++worker)
        {
            const float phase = 0.37f + static_cast<float>(farm) * 0.83f
                + static_cast<float>(worker) * 2.11f;
            const float baseSpeed = 0.82f
                + 0.12f * static_cast<float>((farm + worker) % 3);
            cropWorkers_.emplace_back(
                workerX[worker] + workerJitter(trafficRng_),
                workerZ[worker] + workerJitter(trafficRng_),
                FarmerRoute::CropWork, baseSpeed, phase);
        }
    }

    std::bernoulli_distribution occupied(
        VillageSimulationSettings::BenchOccupancyChance);
    std::bernoulli_distribution secondVisitor(0.42);
    int occupiedBenches = 0;
    for (int bench = 0; bench < VillageSimulationSettings::RoadsideBenchCount; ++bench)
    {
        const unsigned int count = occupied(trafficRng_)
            ? (secondVisitor(trafficRng_) ? 2u : 1u) : 0u;
        roadsideBenchOccupancy_ |= count << (bench * 2);
        occupiedBenches += count > 0 ? 1 : 0;
    }
    if (occupiedBenches == 0)
        roadsideBenchOccupancy_ |= 1u;
    if (occupiedBenches == VillageSimulationSettings::RoadsideBenchCount)
        roadsideBenchOccupancy_ &= ~(3u << 6);

    std::cout << "SCENE_INIT crop_workers=" << cropWorkers_.size()
              << " worker_counts=";
    for (unsigned char count : cropWorkerCounts_)
        std::cout << static_cast<int>(count);
    std::cout << " bench_occupancy=" << roadsideBenchOccupancy_ << '\n';

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
    if (Input::wasPressed(GLFW_KEY_T))
    {
        trafficRunning_ = !trafficRunning_;
    }
    if (Input::wasPressed(GLFW_KEY_L))
    {
        powerOn_ = !powerOn_;
        animation_.setPowerOn(powerOn_);
    }
    if (Input::wasPressed(GLFW_KEY_O))
    {
        workersPaused_ = !workersPaused_;
    }
    if (Input::wasPressed(GLFW_KEY_B))
    {
        bonfiresEnabled_ = !bonfiresEnabled_;
    }
    if (Input::wasPressed(GLFW_KEY_LEFT_BRACKET))
    {
        workerSpeedTarget_ = std::max(
            VillageSimulationSettings::WorkerSpeedMin,
            workerSpeedTarget_ - VillageSimulationSettings::WorkerSpeedStep);
    }
    if (Input::wasPressed(GLFW_KEY_RIGHT_BRACKET))
    {
        workerSpeedTarget_ = std::min(
            VillageSimulationSettings::WorkerSpeedMax,
            workerSpeedTarget_ + VillageSimulationSettings::WorkerSpeedStep);
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
    lighting_.update(deltaTime);
    const float powerBlend = 1.0f - std::exp(
        -VillageSimulationSettings::PowerFadeResponse * deltaTime);
    powerAmount_ += ((powerOn_ ? 1.0f : 0.0f) - powerAmount_) * powerBlend;
    const float workerBlend = 1.0f - std::exp(
        -VillageSimulationSettings::WorkerSpeedResponse * deltaTime);
    workerSpeedScale_ += (workerSpeedTarget_ - workerSpeedScale_) * workerBlend;
    farmhouse_.setPowerAmount(powerAmount_);
    PowerSubstation::setPowerAmount(powerAmount_);
    Cloud::setNightAmount(lighting_.nightAmount());
    tractor_.setNightAmount(lighting_.nightAmount());
    animation_.update(deltaTime);
    if (animation_.isPaused())
    {
        return;
    }

    tractor_.update(deltaTime);
    if (trafficRunning_)
    {
        for (std::size_t index = 0; index < farmers_.size(); ++index)
        {
            farmerTrafficTimers_[index] -= deltaTime;
            if (farmerTrafficTimers_[index] <= 0.0f)
            {
                if (farmerTrafficActive_[index])
                {
                    farmerTrafficActive_[index] = false;
                    farmers_[index].setVisible(false);
                    farmerTrafficTimers_[index] = randomTrafficTime(
                        trafficRng_,
                        VillageSimulationSettings::PedestrianWaitMinSeconds,
                        VillageSimulationSettings::PedestrianWaitMaxSeconds);
                }
                else
                {
                    bool spawnClear = true;
                    for (std::size_t other = 0; other < farmers_.size(); ++other)
                    {
                        if (other == index || !farmerTrafficActive_[other]
                            || farmers_[other].roadX() * farmers_[index].roadX() < 0.0f)
                            continue;
                        const float dz = std::fabs(
                            farmers_[other].roadZ()
                            - VillageSimulationSettings::TrafficRouteMinZ);
                        if (dz < VillageSimulationSettings::PedestrianMinimumSpacing)
                        {
                            spawnClear = false;
                            break;
                        }
                    }
                    if (spawnClear)
                    {
                        farmerTrafficActive_[index] = true;
                        farmers_[index].setVisible(true);
                        farmers_[index].setRoadPosition(
                            VillageSimulationSettings::TrafficRouteMinZ);
                        farmerTrafficTimers_[index] = randomTrafficTime(
                            trafficRng_,
                            VillageSimulationSettings::PedestrianActiveMinSeconds,
                            VillageSimulationSettings::PedestrianActiveMaxSeconds);
                    }
                    else
                    {
                        farmerTrafficTimers_[index] = randomTrafficTime(
                            trafficRng_, 3.0f, 6.0f);
                    }
                }
            }

            if (farmerTrafficActive_[index])
                farmers_[index].update(deltaTime);
        }
    }

    if (!workersPaused_)
    {
        for (Farmer& worker : cropWorkers_)
            worker.update(deltaTime * workerSpeedScale_);
    }

    if (trafficRunning_)
    {
        for (std::size_t index = 0; index < roadTractorZ_.size(); ++index)
        {
            tractor_.updateRoad(
                deltaTime, roadTractorZ_[index], roadTractorWheelRotation_[index]);
        }
    }
    Cloud::updateField(deltaTime);
}

void Scene::render() const
{
    warmUpDrawing();

    // Work out what is on screen once per frame.
    captureFrustum();

    constexpr float farmScale = 0.78f;
    constexpr float farmSpacing = 34.0f;
    constexpr float farmSideOffset = 38.0f;

    for (int row = -2; row <= 3; ++row)
    {
        const float z = static_cast<float>(row) * farmSpacing;
        const int layoutIndex = (row + 2) * 2;
        gFarmVisible[layoutIndex] =
            sphereVisible(-farmSideOffset, 5.0f, z, kFarmCullRadius)
            && withinDistance(-farmSideOffset, z, kFarmMaxDistance, kFarmCullRadius);
        gFarmVisible[layoutIndex + 1] =
            sphereVisible(farmSideOffset, 5.0f, z, kFarmCullRadius)
            && withinDistance(farmSideOffset, z, kFarmMaxDistance, kFarmCullRadius);
    }

    renderGround();
    renderRoad();
    // Ground and road use their original flat colors; apply material lighting
    // to the three-dimensional world objects that follow.
    lighting_.apply();
    applyNightLights();

    // Establish a predictable fixed-function state before drawing lit
    // geometry. Lighting::apply owns material defaults; clouds and sky now
    // preserve their state instead of leaking it into the next frame.
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_NORMALIZE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    PowerSubstation::drawRoadUtilities(animation_.waterTime());
    Village::drawWorldVegetation(sphereVisible, gEye[0], gEye[2]);
    renderScatteredTrees();
    renderPonds();
    Village::drawPondSeating(animation_.waterTime());
    Village::drawRoadsideAmenities(
        animation_.waterTime(), lighting_.nightAmount() * powerAmount_,
        roadsideBenchOccupancy_);
    renderFarmers();
    renderRoadTractors();
    renderBonfires();
    renderBarns();
    renderNightFixtures();
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

    // The terrain and road deliberately use large unlit display-list quads.
    // Lightweight projected pools make local lamps visible there without
    // tessellating the whole world or consuming more hardware light slots.
    renderNightLightPools();
    Cloud::drawField();
    // renderTransformationMarker();
}

void Scene::applyNightLights() const
{
    std::array<Lighting::LocalLight, 32> lights{};
    std::size_t count = 0;
    const auto addPoint = [&](float x, float y, float z,
                              const float color[3], float linear, float quadratic)
    {
        if (count >= lights.size() || powerAmount_ < 0.01f) return;
        Lighting::LocalLight& light = lights[count++];
        light.position[0] = x; light.position[1] = y; light.position[2] = z;
        for (int component = 0; component < 3; ++component)
            light.color[component] = color[component] * powerAmount_;
        light.linearAttenuation = linear;
        light.quadraticAttenuation = quadratic;
    };
    const auto addIndependentPoint = [&](float x, float y, float z,
                                         const float color[3], float linear,
                                         float quadratic)
    {
        if (count >= lights.size()) return;
        Lighting::LocalLight& light = lights[count++];
        light.position[0] = x; light.position[1] = y; light.position[2] = z;
        for (int component = 0; component < 3; ++component)
            light.color[component] = color[component];
        light.linearAttenuation = linear;
        light.quadraticAttenuation = quadratic;
    };

    const float nearestPoleZ = std::round(gEye[2] / 24.0f) * 24.0f;
    for (int row = -1; row <= 1; ++row)
    {
        const float z = nearestPoleZ + static_cast<float>(row) * 24.0f;
        addPoint(-5.2f, 8.37f, z, DayNightSettings::WarmLamp, 0.035f, 0.006f);
        addPoint( 5.2f, 8.37f, z, DayNightSettings::WarmLamp, 0.035f, 0.006f);
    }

    // Tea-shop bulbs. These join the same nearest-light ranking as houses and
    // street lamps, so the fixed-function hardware limit is never exceeded.
    addPoint(-12.35f, 2.75f, -32.0f, DayNightSettings::WarmLamp, 0.050f, 0.012f);
    addPoint( 12.35f, 2.75f,  18.0f, DayNightSettings::WarmLamp, 0.050f, 0.012f);
    addPoint(-12.35f, 2.75f,  66.0f, DayNightSettings::WarmLamp, 0.050f, 0.012f);

    struct BarnLight { float x, z, scale, rotation; };
    constexpr BarnLight barnLights[] = {
        {-65.0f, -68.0f, 1.02f, -4.0f}, {65.0f, -34.0f, 0.96f, 5.0f},
        {-65.0f,  34.0f, 1.08f, -2.0f}, {65.0f,  68.0f, 1.00f, 7.0f}};
    for (const BarnLight& barn : barnLights)
    {
        const float angle = barn.rotation * 3.14159265358979323846f / 180.0f;
        const float localZ = -4.35f * barn.scale;
        addPoint(barn.x + std::sin(angle) * localZ, 4.55f * barn.scale,
                 barn.z + std::cos(angle) * localZ,
                 DayNightSettings::WarmLamp, 0.055f, 0.012f);
    }

    constexpr float farmScale = 0.78f;
    for (int row = -2; row <= 3; ++row)
    {
        for (int side = 0; side < 2; ++side)
        {
            const int index = (row + 2) * 2 + side;
            const FarmLayout& layout = farmLayouts_[index];
            const float angle = layout.rotation * 3.14159265358979323846f / 180.0f;
            const float lx = (layout.houseOffsetX + 10.0f) * farmScale;
            const float lz = (layout.houseOffsetZ + 12.72f) * farmScale;
            const float centerX = side == 0 ? -38.0f : 38.0f;
            const float centerZ = static_cast<float>(row) * 34.0f;
            addPoint(centerX + std::cos(angle) * lx + std::sin(angle) * lz,
                     4.45f * farmScale,
                     centerZ - std::sin(angle) * lx + std::cos(angle) * lz,
                     DayNightSettings::WarmLamp, 0.060f, 0.014f);
        }
    }

    if (bonfiresEnabled_)
    {
        constexpr float fireLight[] = {1.0f, 0.30f, 0.055f};
        for (int site = 0; site < VillageSimulationSettings::BonfireSiteCount; ++site)
        {
            addIndependentPoint(
                VillageSimulationSettings::BonfireSiteX[site], 1.2f,
                VillageSimulationSettings::BonfireSiteZ[site],
                fireLight, 0.075f, 0.022f);
        }
    }

    std::size_t nearestTractor = roadTractorZ_.size();
    float nearestDistance = 1e30f;
    for (std::size_t index = 0; index < roadTractorZ_.size(); ++index)
    {
        if (!tractorTrafficActive_[index]) continue;
        const float dz = roadTractorZ_[index] - gEye[2];
        const float distance = dz * dz + gEye[0] * gEye[0];
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestTractor = index;
        }
    }
    if (nearestTractor < roadTractorZ_.size())
    {
        for (float x : {-0.95f, 0.95f})
        {
            Lighting::LocalLight& light = lights[count++];
            light.position[0] = x; light.position[1] = 2.80f;
            light.position[2] = roadTractorZ_[nearestTractor] + 2.45f;
            for (int component = 0; component < 3; ++component)
                light.color[component] = DayNightSettings::Headlight[component];
            light.direction[0] = 0.0f; light.direction[1] = -0.08f;
            light.direction[2] = 1.0f;
            light.cutoff = 28.0f;
            light.exponent = 14.0f;
            light.linearAttenuation = 0.025f;
            light.quadraticAttenuation = 0.004f;
            light.priority = true;
        }
    }

    lighting_.applyLocalLights(lights.data(), count, gEye[0], gEye[1], gEye[2]);
}

void Scene::drawNightBulb(float x, float y, float z, float scale) const
{
    const float night = lighting_.nightAmount() * powerAmount_;
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);
    glColor3f(0.12f, 0.10f, 0.07f);
    glPushMatrix();
    glTranslatef(0.0f, 0.18f, 0.0f);
    Primitives::drawCube(0.42f, 0.18f, 0.42f);
    glPopMatrix();

    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT);
    const GLfloat emission[] = {
        DayNightSettings::WarmLamp[0] * night,
        DayNightSettings::WarmLamp[1] * night,
        DayNightSettings::WarmLamp[2] * night, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
    glColor3f(0.45f + 0.55f * night, 0.38f + 0.42f * night,
              0.22f + 0.18f * night);
    Primitives::drawSphere(0.18f, 10, 7);
    glPopAttrib();
    glPopMatrix();
}

void Scene::renderNightFixtures() const
{
    struct BarnLight { float x, z, scale, rotation; };
    constexpr BarnLight barnLights[] = {
        {-65.0f, -68.0f, 1.02f, -4.0f}, {65.0f, -34.0f, 0.96f, 5.0f},
        {-65.0f,  34.0f, 1.08f, -2.0f}, {65.0f,  68.0f, 1.00f, 7.0f}};
    for (const BarnLight& barn : barnLights)
    {
        const float angle = barn.rotation * 3.14159265358979323846f / 180.0f;
        const float localZ = -4.35f * barn.scale;
        drawNightBulb(barn.x + std::sin(angle) * localZ, 4.55f * barn.scale,
                      barn.z + std::cos(angle) * localZ, 1.0f);
    }
}

void Scene::renderNightLightPools() const
{
    const float night = lighting_.nightAmount();
    if (night < 0.01f)
    {
        return;
    }

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT |
                 GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    const float electricLight = night * powerAmount_;
    const auto drawPool = [electricLight](float x, float z, float radius, float alpha)
    {
        constexpr int segments = 16;
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(DayNightSettings::WarmLamp[0],
                  DayNightSettings::WarmLamp[1] * 0.86f,
                  DayNightSettings::WarmLamp[2] * 0.55f, alpha * electricLight);
        glVertex3f(x, 0.10f, z);
        glColor4f(DayNightSettings::WarmLamp[0],
                  DayNightSettings::WarmLamp[1],
                  DayNightSettings::WarmLamp[2], 0.0f);
        for (int segment = 0; segment <= segments; ++segment)
        {
            const float angle = static_cast<float>(segment) *
                2.0f * 3.14159265358979323846f / static_cast<float>(segments);
            glVertex3f(x + std::cos(angle) * radius, 0.10f,
                       z + std::sin(angle) * radius);
        }
        glEnd();
    };
    const auto drawFirePool = [night](float x, float z)
    {
        constexpr int segments = 18;
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 0.30f, 0.045f, 0.24f * night);
        glVertex3f(x, 0.105f, z);
        glColor4f(1.0f, 0.62f, 0.14f, 0.0f);
        for (int segment = 0; segment <= segments; ++segment)
        {
            const float angle = static_cast<float>(segment)
                * 2.0f * 3.14159265358979323846f / static_cast<float>(segments);
            glVertex3f(x + std::cos(angle) * 7.5f, 0.105f,
                       z + std::sin(angle) * 7.5f);
        }
        glEnd();
    };

    if (electricLight > 0.001f)
    {
        const float nearestPoleZ = std::round(gEye[2] / 24.0f) * 24.0f;
        for (int row = -2; row <= 2; ++row)
        {
            const float z = nearestPoleZ + static_cast<float>(row) * 24.0f;
            drawPool(-5.2f, z, 7.0f, 0.14f);
            drawPool( 5.2f, z, 7.0f, 0.14f);
        }
        drawPool(-12.35f, -32.0f, 5.0f, 0.12f);
        drawPool( 12.35f,  18.0f, 5.0f, 0.12f);
        drawPool(-12.35f,  66.0f, 5.0f, 0.12f);
    }

    if (bonfiresEnabled_)
    {
        for (int site = 0; site < VillageSimulationSettings::BonfireSiteCount; ++site)
        {
            const float x = VillageSimulationSettings::BonfireSiteX[site];
            const float z = VillageSimulationSettings::BonfireSiteZ[site];
            if (withinDistance(
                    x, z, VillageSimulationSettings::BonfireDrawDistance, 8.0f))
                drawFirePool(x, z);
        }
    }

    for (std::size_t index = 0; index < roadTractorZ_.size(); ++index)
    {
        if (!tractorTrafficActive_[index]) continue;
        const float z = roadTractorZ_[index];
        const float dz = z - gEye[2];
        if (dz * dz + gEye[0] * gEye[0] > 180.0f * 180.0f) continue;

        const float nearZ = z + 2.55f;
        const float farZ = z + 22.0f;
        glBegin(GL_QUADS);
        glColor4f(DayNightSettings::Headlight[0],
                  DayNightSettings::Headlight[1],
                  DayNightSettings::Headlight[2], 0.20f * night);
        glVertex3f(-1.45f, 0.12f, nearZ);
        glVertex3f( 1.45f, 0.12f, nearZ);
        glColor4f(DayNightSettings::Headlight[0],
                  DayNightSettings::Headlight[1],
                  DayNightSettings::Headlight[2], 0.0f);
        glVertex3f( 4.2f, 0.12f, farZ);
        glVertex3f(-4.2f, 0.12f, farZ);
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
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
            const int farmIndex = layoutIndex + side;
            if (!gFarmVisible[farmIndex])
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
                farmLayouts_[farmIndex].rotation,
                0.0f, 1.0f, 0.0f);
            glTranslatef(
                farmLayouts_[farmIndex].cropOffsetX,
                0.0f,
                farmLayouts_[farmIndex].cropOffsetZ);
            const std::size_t start = cropWorkerStarts_[farmIndex];
            const std::size_t count = cropWorkerCounts_[farmIndex];
            for (std::size_t worker = 0; worker < count; ++worker)
                cropWorkers_[start + worker].render();
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
            || !sphereVisible(chunk.centerX, chunk.centerY, chunk.centerZ, chunk.radius)
            || !withinDistance(
                chunk.centerX, chunk.centerZ, kTreeMaxDistance, chunk.radius))
        {
            continue;
        }

        if (kCacheVegetation)
        {
            const float dx = chunk.centerX - gEye[0];
            const float dz = chunk.centerZ - gEye[2];
            const bool simplified = dx * dx + dz * dz > 70.0f * 70.0f;
            GLuint& list = simplified ? chunk.simpleList : chunk.detailedList;
            cachedList(list, [&chunk, simplified]() {
                drawTreeChunk(chunk, simplified);
            });
        }
        else
        {
            drawTreeChunk(chunk, false);
        }
    }
}

void Scene::renderPonds() const
{
    // Keep the enlarged farms and ponds separated from the central road.
    if (sphereVisible(-82.0f, 0.0f, -42.0f, 34.0f))
    {
        Pond::draw(-82.0f, -42.0f,
                   VillageSimulationSettings::MainPondWidth,
                   VillageSimulationSettings::MainPondDepth,
                   animation_.waterTime(), false);
    }
    if (sphereVisible(73.0f, 0.0f, 24.0f, 34.0f))
    {
        Pond::draw(73.0f, 24.0f,
                   VillageSimulationSettings::SecondaryPondWidth,
                   VillageSimulationSettings::SecondaryPondDepth,
                   animation_.waterTime() + 1.4f, true);
    }
    if (sphereVisible(-71.0f, 0.0f, 58.0f, 32.0f))
    {
        Pond::draw(-71.0f, 58.0f,
                   VillageSimulationSettings::SecondaryPondWidth,
                   VillageSimulationSettings::SecondaryPondDepth,
                   animation_.waterTime() + 2.8f, false);
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
            const float dx = gEye[0];
            const float dz = roadTractorZ_[index] - gEye[2];
            tractor_.drawRoadTractor(
                roadTractorZ_[index], roadTractorWheelRotation_[index], colors[index],
                animation_.waterTime(), dx * dx + dz * dz < 42.0f * 42.0f);
        }
    }
}

void Scene::renderBonfires() const
{
    if (!bonfiresEnabled_)
        return;

    for (int site = 0; site < VillageSimulationSettings::BonfireSiteCount; ++site)
    {
        const float x = VillageSimulationSettings::BonfireSiteX[site];
        const float z = VillageSimulationSettings::BonfireSiteZ[site];
        if (sphereVisible(x, 2.0f, z, 9.0f)
            && withinDistance(
                x, z, VillageSimulationSettings::BonfireDrawDistance, 9.0f))
        {
            Village::drawBonfireSite(
                site, animation_.waterTime(), lighting_.nightAmount());
        }
    }
}

void Scene::renderGround() const
{
    // Geometry is static, but its unlit colour follows the day/night blend.
    // A blackout only removes a little artificial sky fill; it does not
    // incorrectly affect daylight or moonlight.
    static GLuint groundList = 0;
    const float night = lighting_.nightAmount();
    const float blackout = night * (1.0f - powerAmount_);
    const float r = (0.18f + (0.045f - 0.18f) * night)
        * (1.0f - 0.10f * blackout);
    const float g = (0.43f + (0.095f - 0.43f) * night)
        * (1.0f - 0.14f * blackout);
    const float b = (0.16f + (0.060f - 0.16f) * night)
        * (1.0f - 0.10f * blackout);
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    cachedList(groundList, []()
    {
        glPushMatrix();
        glTranslatef(0.0f, -0.02f, 0.0f);
        glScalef(1.0f, 1.0f, 0.85f);
        // Oversized terrain gives the camera a continuous horizon beyond the
        // designed farm area. The old reference grid is gone.
        Primitives::drawPlane(1000.0f, 1000.0f);
        glPopMatrix();
    });
}

void Scene::renderRoad() const
{
    static GLuint baseList = 0;
    static GLuint shoulderList = 0;
    static GLuint darkPatchList = 0;
    static GLuint lightPatchList = 0;
    static GLuint markingList = 0;
    const float night = lighting_.nightAmount();
    glDisable(GL_LIGHTING);

    glColor3f(0.23f - 0.13f * night,
              0.225f - 0.13f * night,
              0.21f - 0.12f * night);
    cachedList(baseList, []()
    {
        glPushMatrix();
        glTranslatef(0.0f, 0.045f, 0.0f);
        Primitives::drawPlane(9.0f, 1000.0f);
        glPopMatrix();

    });

    glColor3f(0.31f - 0.18f * night,
              0.235f - 0.14f * night,
              0.135f - 0.075f * night);
    cachedList(shoulderList, []()
    {
        // Worn soil shoulders soften the perfectly straight road boundary.
        for (float x : {-4.68f, 4.68f})
        {
            glPushMatrix();
            glTranslatef(x, 0.048f, 0.0f);
            Primitives::drawPlane(0.55f, 1000.0f);
            glPopMatrix();
        }
    });

    const auto drawPatchGeometry = [](int parity)
    {
        for (int index = 0; index < 64; ++index)
        {
            if ((index & 1) != parity) continue;
            const float x = -3.8f + treeScatterHash(index * 17, 9) * 7.6f;
            const float z = -480.0f + treeScatterHash(index * 31, 27) * 960.0f;
            const float width = 0.28f + treeScatterHash(index * 7, 51) * 0.62f;
            const float depth = 0.55f + treeScatterHash(index * 13, 73) * 1.25f;
            glPushMatrix();
            glTranslatef(x, 0.052f, z);
            glRotatef((treeScatterHash(index * 19, 91) - 0.5f) * 32.0f,
                      0.0f, 1.0f, 0.0f);
            Primitives::drawPlane(width, depth);
            glPopMatrix();
        }
    };

    glColor3f(0.205f - 0.112f * night,
              0.202f - 0.110f * night,
              0.190f - 0.102f * night);
    cachedList(darkPatchList, [&]() { drawPatchGeometry(0); });
    glColor3f(0.255f - 0.140f * night,
              0.250f - 0.136f * night,
              0.232f - 0.126f * night);
    cachedList(lightPatchList, [&]() { drawPatchGeometry(1); });

    glColor3f(0.82f - 0.35f * night,
              0.70f - 0.31f * night,
              0.22f - 0.08f * night);
    cachedList(markingList, []()
    {
        // Full-length dashed centre line. Farm-row entrances intentionally
        // get a wider break so markings never cross the access paths.
        constexpr float farmRows[] = {-68.0f, -34.0f, 0.0f, 34.0f, 68.0f, 102.0f};
        for (float z = -496.0f; z <= 496.0f; z += 12.0f)
        {
            bool entrance = false;
            for (float row : farmRows)
                entrance = entrance || std::fabs(z - row) < 6.0f;
            if (entrance) continue;
            glPushMatrix();
            glTranslatef(0.0f, 0.061f, z);
            Primitives::drawCube(0.13f, 0.012f, 7.0f);
            glPopMatrix();
        }
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
        Village::drawFarmGrass(
            static_cast<int>(farmIndex) + 1,
            layout.cropOffsetX, layout.cropOffsetZ,
            layout.houseOffsetX, layout.houseOffsetZ,
            layout.hasWindmill);
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
            animalPositions[animal][2], animalPositions[animal][3],
            animation_.waterTime());
    }

    const float chickenPositions[3][4] = {
        {-8.0f, 5.0f, 0.62f, 28.0f},
        {8.5f, 8.0f, 0.55f, -32.0f},
        {0.0f, 13.0f, 0.58f, 5.0f}};
    for (int chicken = 0; chicken < layout.chickenCount; ++chicken)
    {
        Animals::drawChicken(
            chickenPositions[chicken][0], chickenPositions[chicken][1],
            chickenPositions[chicken][2], chickenPositions[chicken][3],
            animation_.waterTime());
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
    drawNightBulb(
        layout.houseOffsetX + 10.0f,
        4.45f,
        layout.houseOffsetZ + 12.72f,
        0.80f);

    if (layout.hasTractor)
    {
        tractor_.drawTractor();
    }
    if (layout.hasWindmill)
    {
        Windmill::drawWindmill(
            -14.0f, 1.0f, animation_.windmillAngle(), 1.48f);
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
    Animals::drawCow(-16.0f, -2.0f, 1.05f, 8.0f, animation_.waterTime());
    Animals::drawCow(16.0f, -7.0f, 0.90f, -18.0f, animation_.waterTime());
    Animals::drawCow(-15.0f, 14.0f, 0.82f, 28.0f, animation_.waterTime());
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

float Scene::nightAmount() const
{
    return lighting_.nightAmount();
}

float Scene::minimumTrafficGap() const
{
    std::array<float, VillageSimulationSettings::TrafficVehicleCount> positions =
        roadTractorZ_;
    std::sort(positions.begin(), positions.end());
    float minimum = VillageSimulationSettings::TrafficRouteMaxZ
        - VillageSimulationSettings::TrafficRouteMinZ;
    for (std::size_t index = 1; index < positions.size(); ++index)
        minimum = std::min(minimum, positions[index] - positions[index - 1]);
    const float wrappedGap = positions.front()
        + (VillageSimulationSettings::TrafficRouteMaxZ
           - VillageSimulationSettings::TrafficRouteMinZ)
        - positions.back();
    return std::min(minimum, wrappedGap);
}

void Scene::renderSky() const
{
    Sky::drawSun(animation_.waterTime());
    Birds::draw(animation_.waterTime(), lighting_.nightAmount());
}
