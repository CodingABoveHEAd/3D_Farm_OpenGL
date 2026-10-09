#pragma once

namespace Barn
{
struct State
{
    float doorOpen = 0.0f;
    float wheelbarrowOffset = 0.0f;
    float wheelRotation = 0.0f;
    float chestOpen = 0.0f;
    float feedingGateOpen = 0.0f;
    float electricLight = 0.0f;
};

void draw(float x, float z, float scale, float rotation, bool fenced,
          const State& state);
}
