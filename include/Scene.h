#pragma once

#include "Animation.h"
#include "Lighting.h"
#include "VillageSimulationSettings.h"
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
#include <vector>

class Scene {
public:
    Scene();
    void handleInput();
    void update(float deltaTime);
    void renderSky() const;
    void render() const;
    bool isNight() const;
    float nightAmount() const;
    bool isPowerOn() const { return powerOn_; }
    bool isTrafficRunning() const { return trafficRunning_; }
    bool areWorkersPaused() const { return workersPaused_; }
    bool areBonfiresEnabled() const { return bonfiresEnabled_; }
    float workerSpeed() const { return workerSpeedScale_; }
    std::size_t cropWorkerCount() const { return cropWorkers_.size(); }
    float minimumTrafficGap() const;

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
    void renderBonfires() const;
    void renderCropWorkers() const;
    void renderScatteredTrees() const;
    void renderBarns() const;
    void applyNightLights() const;
    void renderNightFixtures() const;
    void renderNightLightPools() const;
    void drawNightBulb(float x, float y, float z, float scale) const;
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
    std::array<Farmer, VillageSimulationSettings::RoadPedestrianCount> farmers_;
    std::vector<Farmer> cropWorkers_;
    std::array<unsigned char, VillageSimulationSettings::FarmCount> cropWorkerCounts_{};
    std::array<unsigned char, VillageSimulationSettings::FarmCount> cropWorkerStarts_{};
    std::array<bool, VillageSimulationSettings::RoadPedestrianCount> farmerTrafficActive_{};
    std::array<float, VillageSimulationSettings::RoadPedestrianCount> farmerTrafficTimers_{};
    std::array<FarmLayout, VillageSimulationSettings::FarmCount> farmLayouts_;
    std::array<float, VillageSimulationSettings::TrafficVehicleCount> roadTractorZ_;
    std::array<float, VillageSimulationSettings::TrafficVehicleCount> roadTractorWheelRotation_{};
    std::array<bool, VillageSimulationSettings::TrafficVehicleCount> tractorTrafficActive_{};
    unsigned int roadsideBenchOccupancy_ = 0;
    bool trafficRunning_ = true;
    bool powerOn_ = true;
    bool workersPaused_ = false;
    bool bonfiresEnabled_ = true;
    float powerAmount_ = 1.0f;
    float workerSpeedTarget_ = 1.0f;
    float workerSpeedScale_ = 1.0f;
    std::mt19937 trafficRng_;
};
