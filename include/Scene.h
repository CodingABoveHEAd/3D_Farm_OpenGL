#pragma once

#include "Animation.h"
#include "Lighting.h"

class Scene {
public:
    void handleInput();
    void update(float deltaTime);
    void render() const;
    bool isNight() const;

private:
    void renderGround() const;

    Animation animation_;
    Lighting lighting_;
};
