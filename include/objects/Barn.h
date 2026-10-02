#pragma once

namespace Barn {

// Draw a classic American barn at farm-local position (x, z).
// scale controls size, yaw rotates around Y (degrees).
// Barn faces +Z (doors on south face by default).
void drawBarn(float x, float z, float scale, float yaw = 0.0f);

} // namespace Barn
