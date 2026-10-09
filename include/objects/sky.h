#pragma once

namespace Sky {

// Existing sun-drawing API (unchanged signature).
void drawSun(float animationTime);

// NEW — set the clear color each frame based on day/night.
void setNightAmount(float nightAmount);

} // namespace Sky
