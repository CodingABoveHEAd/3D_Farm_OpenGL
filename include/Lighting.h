#pragma once

#include <cstddef>

class Lighting {
public:
    struct LocalLight
    {
        float position[3];
        float color[3];
        float direction[3]{0.0f, -1.0f, 0.0f};
        float cutoff = 180.0f;
        float exponent = 0.0f;
        float linearAttenuation = 0.045f;
        float quadraticAttenuation = 0.008f;
        bool priority = false;
    };

    void toggleNight();
    void update(float deltaTime);
    void apply() const;
    void applyLocalLights(const LocalLight* lights, std::size_t count,
                          float viewerX, float viewerY, float viewerZ) const;
    bool isNight() const;
    float nightAmount() const { return nightAmount_; }

private:
    bool nightTarget_ = false;
    float nightAmount_ = 0.0f;
};
