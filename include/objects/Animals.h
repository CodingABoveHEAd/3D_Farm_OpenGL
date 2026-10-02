#pragma once

namespace Animals {

// Draw static/grazing cow
void drawCow(float x, float z, float scale, float rotation);

// Draw walking cow with swinging legs and optional grazing
void drawWalkingCow(float x, float z, float scale, float rotation, float legSwing = 0.0f, bool isGrazing = false);

// Draw animated chicken with walking legs, pecking head, and flapping wings
void drawChicken(float x, float z, float scale, float rotation, float time);

} // namespace Animals
