#pragma once

namespace Farmer {

// Activity types for different animation states
enum Activity { Standing, Walking, Working };

// Draw a farmer at (x, z) facing `heading` degrees (around Y).
// `time` drives the animation; `activity` selects the motion style.
// `scale` defaults to 1.0.
void drawFarmer(float x, float z, float heading, float time,
                Activity activity = Standing, float scale = 1.0f);

} // namespace Farmer
