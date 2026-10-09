#pragma once

// Central tuning for the complete day/night system. Colors use linear-ish
// OpenGL fixed-function values in RGBA order.
namespace DayNightSettings
{
constexpr float TransitionSeconds = 3.0f;
constexpr float LocalLightsStart = 0.08f;
constexpr int MaxLocalLights = 7; // GL_LIGHT1..GL_LIGHT7; GL_LIGHT0 is moon/sun.

constexpr float SunPosition[3]  = {-10.0f, 14.0f, -18.0f};
// Lower than the sun so it is visible from the default downward camera view.
constexpr float MoonPosition[3] = {0.0f, 2.0f, -30.0f};

constexpr float DayClear[4]   = {0.62f, 0.80f, 0.96f, 1.0f};
constexpr float NightClear[4] = {0.025f, 0.035f, 0.095f, 1.0f};
constexpr float DayFog[4]     = {0.62f, 0.80f, 0.96f, 1.0f};
constexpr float NightFog[4]   = {0.045f, 0.055f, 0.115f, 1.0f};

constexpr float DaySkyHorizon[3]   = {0.84f, 0.92f, 0.98f};
constexpr float DaySkyMid[3]       = {0.50f, 0.74f, 0.96f};
constexpr float DaySkyZenith[3]    = {0.20f, 0.46f, 0.86f};
constexpr float NightSkyHorizon[3] = {0.085f, 0.105f, 0.220f};
constexpr float NightSkyMid[3]     = {0.035f, 0.050f, 0.140f};
constexpr float NightSkyZenith[3]  = {0.008f, 0.014f, 0.045f};

constexpr float DayGlobalAmbient[4]   = {0.10f, 0.10f, 0.12f, 1.0f};
constexpr float NightGlobalAmbient[4] = {0.025f, 0.030f, 0.055f, 1.0f};
constexpr float DayDirectionalAmbient[4]   = {0.36f, 0.39f, 0.46f, 1.0f};
constexpr float NightDirectionalAmbient[4] = {0.055f, 0.070f, 0.130f, 1.0f};
constexpr float DayDirectionalDiffuse[4]   = {0.85f, 0.80f, 0.66f, 1.0f};
constexpr float NightDirectionalDiffuse[4] = {0.20f, 0.26f, 0.48f, 1.0f};
constexpr float DayDirectionalSpecular[4]   = {0.20f, 0.20f, 0.18f, 1.0f};
constexpr float NightDirectionalSpecular[4] = {0.08f, 0.11f, 0.20f, 1.0f};

constexpr float DayCloud[3]   = {0.94f, 0.96f, 0.99f};
constexpr float NightCloud[3] = {0.16f, 0.20f, 0.31f};
constexpr float WarmLamp[3]   = {1.00f, 0.68f, 0.30f};
constexpr float Headlight[3]  = {1.00f, 0.91f, 0.66f};
}
