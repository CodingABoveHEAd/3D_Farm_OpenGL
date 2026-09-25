#pragma once

#include "Animation.h"
#include "Lighting.h"
#include "objects/Farmhouse.h"

class Scene {
public:
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
    void renderTransformationMarker() const;
    void drawFencePost(float x, float z) const;
    void drawFenceRail(float x, float z, float length, bool rotate) const;

    Animation animation_;
    Lighting lighting_;
    Farmhouse farmhouse_;
};
