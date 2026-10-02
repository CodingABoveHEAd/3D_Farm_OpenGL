#pragma once

namespace Shadow {

// Configure light direction (normalized or unnormalized)
void setLightDirection(float lx, float ly, float lz);

// Begin shadow projection pass onto plane y = groundY
void begin(float groundY = 0.02f);

// End shadow projection pass and restore OpenGL state
void end();

} // namespace Shadow
