#pragma once

namespace PowerPlant {

// Draw the rural electric substation at (x, z) with orientation yaw
void drawSubstation(float x, float z, float yaw = 0.0f, float time = 0.0f);

// Get the position of the substation's inspection light for GL_LIGHT2
void getLightPosition(float& x, float& y, float& z);

} // namespace PowerPlant
