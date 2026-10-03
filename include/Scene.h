#pragma once

#include "Animation.h"
#include "Lighting.h"
#include "objects/Farmhouse.h"
#include "objects/Animals.h"
#include "objects/Barn.h"
#include "objects/Farmer.h"
#include "objects/Pond.h"
#include "objects/PowerSubstation.h"
#include "objects/Tractor.h"
#include "objects/Windmill.h"
#include "objects/cloud.h"
#include "objects/sky.h"

#include <array>
#include <random>

class Scene {
public:
    Scene();
    void handleInput();
    void update(float deltaTime);
    void render() const;
    bool isNight() const;

private:
    struct FarmLayout
    {
        int treeCount;
        int animalCount;
        bool hasTractor;
        bool hasWindmill;
        float cropOffsetX;
        float cropOffsetZ;
        float houseOffsetX;
        float houseOffsetZ;
        float rotation;
        int chickenCount = 0;
    };

    void renderGround() const;
    void renderRoad() const;
    void renderPonds() const;
    void renderFarmers() const;
    void renderRoadTractors() const;
    void renderCropWorkers() const;
    void renderScatteredTrees() const;
    void renderBarns() const;
    void renderFarm(float x, float z, float scale, const FarmLayout& layout) const;
    void renderCropField() const;
    void renderCrops() const;
    void renderTrees() const;
    void renderAnimals() const;
    void renderBoundary() const;
    void renderPath() const;
    void renderGate() const;
    void renderRocks() const;
    void renderTransformationMarker() const;
    void drawFencePost(float x, float z) const;
    void drawFenceRail(float x, float z, float length, bool rotate) const;
    void drawGatePanel(float x, float angle) const;
    void drawRock(float x, float z, float scale, float rotation) const;

    Animation animation_;
    Lighting lighting_;
    Farmhouse farmhouse_;
    Tractor tractor_;
    std::array<Farmer, 6> farmers_;
    std::array<Farmer, 12> cropWorkers_;
    std::array<bool, 6> farmerTrafficActive_{};
    std::array<float, 6> farmerTrafficTimers_{};
    std::array<FarmLayout, 12> farmLayouts_;
    std::array<float, 4> roadTractorZ_;
    std::array<bool, 4> tractorTrafficActive_{};
    std::array<float, 4> tractorTrafficTimers_{};
    std::mt19937 trafficRng_;
};
