#pragma once

namespace Village {

// Static, deterministic grass for one farm. Intended to be recorded in the
// farm's existing display list.
void drawFarmGrass(int seed, float cropX, float cropZ,
                   float houseX, float houseZ, bool hasWindmill);

// World-space village details. Animation time is the shared, pausable scene
// clock; nightAmount is 0 for day and 1 for full night.
void drawPondSeating(float animationTime);
void drawRoadsideAmenities(float animationTime, float electricLightAmount,
                           unsigned int benchOccupancy);

// Shared articulated character poses used by vehicles and gathering sites.
void drawDriver(float animationTime, float phase, bool detailed);
void drawBonfireSite(int siteIndex, float animationTime, float nightAmount);

// Deterministic world vegetation, cached by the implementation.
using VisibilityTest = bool (*)(float x, float y, float z, float radius);
void drawWorldVegetation(VisibilityTest visibility, float viewerX, float viewerZ);

} // namespace Village
