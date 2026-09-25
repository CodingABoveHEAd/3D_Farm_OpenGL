#pragma once

#include "Animation.h"
#include "Lighting.h"
#include "objects/Farmhouse.h"
#include "objects/Tractor.h"
#include "objects/Windmill.h"
#include "objects/cloud.h"
#include "objects/sky.h"

class Scene {
public:
    Scene();
    void handleInput();
    void update(float deltaTime);
    void render() const;
    bool isNight() const;

private:
    void renderGround() const;
    void renderCropField() const;
    void renderCrops() const;
    void renderTrees() const;
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
};
