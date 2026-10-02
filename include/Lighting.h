#pragma once

class Lighting {
public:
    void toggleNight();
    void setNight(bool night);
    bool isNight() const;

    // Apply global directional light (Sun/Moon, GL_LIGHT0 with w=0)
    void applyDirectional() const;

    // Apply farmhouse porch point light (GL_LIGHT1) and entrance light (GL_LIGHT5)
    void applyFarmLights(float farmX, float farmZ, float farmYaw) const;

    // Apply power plant industrial floodlight (GL_LIGHT2)
    void applyPowerPlantLight(float x, float y, float z) const;

    // Apply tractor dual spotlights (GL_LIGHT3 & GL_LIGHT4)
    void applyTractorHeadlights(float tractorX, float tractorY, float tractorZ,
                               float heading, bool headlightsOn) const;

    // Global lighting state helpers
    static void enableLighting();
    static void disableLighting();

private:
    bool night_ = false;
};
