#pragma once

namespace Bridge {

// Draw a wooden bridge at (x, z) with rotation angle and span length
// Arched railing uses Quadratic Bézier curves: B(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2
void drawBridge(float x, float z, float angle = 0.0f, float length = 12.0f, float width = 3.6f);

} // namespace Bridge
