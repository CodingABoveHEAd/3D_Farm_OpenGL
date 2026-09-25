#pragma once

namespace Windmill {

// Draw the windmill at world position (x, z).
// bladeAngle is the current rotation (in degrees) of the blades
// around the hub's rotation axis.
void drawWindmill(float x, float z, float bladeAngle, float scale);

} // namespace Windmill