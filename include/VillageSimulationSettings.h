#pragma once

namespace VillageSimulationSettings
{
constexpr int FarmCount = 12;
constexpr int MaxWorkersPerFarm = 3;
constexpr float WorkerSpeedMin = 0.35f;
constexpr float WorkerSpeedMax = 2.50f;
constexpr float WorkerSpeedStep = 0.25f;
constexpr float WorkerSpeedResponse = 5.0f;

constexpr int TrafficVehicleCount = 3;
constexpr float TrafficRouteMinZ = -108.0f;
constexpr float TrafficRouteMaxZ = 108.0f;
constexpr float TrafficSpeed = 5.5f;
constexpr float TrafficMinimumSpacing = 50.0f;

constexpr int RoadPedestrianCount = 4;
constexpr float PedestrianMinimumSpacing = 18.0f;
constexpr float PedestrianActiveMinSeconds = 24.0f;
constexpr float PedestrianActiveMaxSeconds = 46.0f;
constexpr float PedestrianWaitMinSeconds = 12.0f;
constexpr float PedestrianWaitMaxSeconds = 25.0f;

constexpr int RoadsideBenchCount = 4;
constexpr float BenchOccupancyChance = 0.58f;

constexpr float PowerFadeResponse = 5.0f;
constexpr float WindmillPowerResponse = 2.8f;

constexpr float MainPondWidth = 36.0f;
constexpr float MainPondDepth = 26.0f;
constexpr float SecondaryPondWidth = 32.0f;
constexpr float SecondaryPondDepth = 23.0f;

constexpr int BonfireSiteCount = 5;
constexpr float BonfireSiteX[BonfireSiteCount] = {
    -126.0f, 132.0f, -120.0f, -82.0f, 102.0f};
constexpr float BonfireSiteZ[BonfireSiteCount] = {
    -108.0f, -52.0f, 94.0f, -19.0f, 42.0f};
constexpr float BonfireTreeClearance = 14.0f;
constexpr float BonfireDrawDistance = 155.0f;
}
