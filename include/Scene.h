#pragma once

#include "Animation.h"
#include "FarmWorld.h"
#include "Lighting.h"
#include "objects/Farmhouse.h"
#include "objects/Animals.h"
#include "objects/Farmer.h"
#include "objects/Tractor.h"
#include "objects/Windmill.h"
#include "objects/cloud.h"
#include "objects/sky.h"

class Scene {
public:
    Scene();
    void handleInput();
    void update(float deltaTime, Camera& camera);
    void render(const Camera& camera);
    void shutdown();
    bool isNight() const;

    void toggleGate();

private:
    void renderFarm(int farmIndex, bool isNearest) const;
    void renderCropField() const;
    void renderCrops() const;
    void renderTrees() const;
    void renderAnimals() const;
    void renderFarmers() const;
    void renderBoundary() const;
    void renderPath() const;
    void renderGate() const;
    void renderRocks() const;
    void renderChimneySmoke(float time) const;
    void renderFireflies(float time) const;
    void renderFarmShadows(int farmIndex) const;
    void renderTransformationMarker() const;
    void drawFencePost(float x, float z) const;
    void drawFenceRail(float x, float z, float length, bool rotate) const;
    void drawGatePanel(float hingeX, float panelDir, float angle) const;
    void drawRock(float x, float z, float scale, float rotation) const;

    Animation animation_;
    Lighting lighting_;
    FarmWorld world_;
    Farmhouse farmhouse_;
    Tractor tractor_;

    float gateAngle_ = 0.0f;
    float gateTarget_ = 0.0f;
    float totalTime_ = 0.0f;
};
