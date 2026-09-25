#pragma once

namespace Cloud {

// --- Single cloud (existing API) ---
// Draws one cloud centered at (x, y, z) with uniform scale.
void drawCloud(float x, float y, float z, float scale);

// --- NEW: sky full of drifting clouds ---

// Initialize the cloud field (call once at startup).
void initField();

// Advance all clouds using delta time and wrap them when they exit.
void updateField(float dt);

// Draw every cloud in the field.
void drawField();

} // namespace Cloud