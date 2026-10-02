#pragma once

#include <functional>

class Camera;

// ------------------------------------------------------------
// Sizes shared by FarmWorld and the farm drawing code.
// ------------------------------------------------------------
namespace FarmLayout
{
constexpr int   FarmCount = 9;

// Half size of one farm's fence rectangle, in the farm's own units
// (the fence in Scene.cpp runs from -24..24 in X and -19..19 in Z).
constexpr float FarmHalfX = 24.0f;
constexpr float FarmHalfZ = 19.0f;

// Width of the main roads
constexpr float RoadWidth = 6.5f;
}

// ------------------------------------------------------------
// FarmWorld
//   * decides where every farm stands (position, turn, size)
//   * builds the countryside around them ONCE (roads, fields,
//     ponds, forests, hedges, power lines, hills) into OpenGL
//     display lists
//   * draws the endless-looking ground that follows the camera
//   * draws each farm through a callback (so the farm itself is
//     still drawn by Scene.cpp, exactly like before)
//   * flies the camera between farms and keeps it inside the
//     "playable" area
// ------------------------------------------------------------
class FarmWorld
{
public:
    // drawFarm(farmIndex, detailed)
    //   detailed == true  -> the farm is close: draw everything
    //   detailed == false -> the farm is far away: draw a cheap version
    using DrawFarmFn = std::function<void(int farmIndex, bool detailed)>;

    FarmWorld() = default;
    ~FarmWorld();

    FarmWorld(const FarmWorld&) = delete;
    FarmWorld& operator=(const FarmWorld&) = delete;

    // Fog + clear colour. Call once after the GL context exists.
    static void setupAtmosphere();

    // Call every frame: makes the fog match day / night so the far
    // ground melts into the sky colour.
    static void updateAtmosphere(bool night);

    // Where farm "index" stands, how much it is turned (degrees,
    // around Y) and how much it is scaled.
    static void  farmCenter(int index, float& x, float& z);
    static float farmYaw(int index);
    static float farmScale(int index);

    static int nearestFarm(float x, float z);

    void travelTo(int farmIndex, const Camera& camera);
    void update(float deltaTime, Camera& camera);
    void draw(const Camera& camera, const DrawFarmFn& drawFarm);
    void release();

private:
    void buildStatic();

    // Display lists (0 = not built yet)
    unsigned int groundList_  = 0;   // flat things: roads, fields, ponds, dirt
    unsigned int sceneryList_ = 0;   // 3D things: trees, hedges, poles, hills

    // Camera flight between farms
    bool  traveling_ = false;
    int   target_ = 0;
    float time_ = 0.0f;
    float duration_ = 1.0f;
    float arcHeight_ = 0.0f;
    float fromPos_[3] = {0.0f, 0.0f, 0.0f};
    float toPos_[3]   = {0.0f, 0.0f, 0.0f};
    float fromYaw_ = 0.0f, toYaw_ = 0.0f;
    float fromPitch_ = 0.0f, toPitch_ = 0.0f;
};