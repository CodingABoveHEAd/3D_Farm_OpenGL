#pragma once

namespace Sky {

// Existing sun-drawing API (unchanged signature).
void drawSun();

// NEW — set the clear color each frame based on day/night.
void setClearColor(bool isNight);

} // namespace Sky