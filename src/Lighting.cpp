#include "Lighting.h"
#include "DayNightSettings.h"
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
float mix(float day, float night, float amount)
{
    return day + (night - day) * amount;
}

void mixColor(const float day[4], const float night[4], float amount,
              GLfloat output[4])
{
    for (int component = 0; component < 4; ++component)
    {
        output[component] = mix(day[component], night[component], amount);
    }
}
}

void Lighting::toggleNight()
{
    nightTarget_ = !nightTarget_;
}

void Lighting::update(float deltaTime)
{
    const float direction = nightTarget_ ? 1.0f : -1.0f;
    nightAmount_ = std::clamp(
        nightAmount_ + direction * deltaTime / DayNightSettings::TransitionSeconds,
        0.0f, 1.0f);
}

void Lighting::apply() const
{
    // Shadow side: a cool, fairly bright sky fill so it is only a little
    // darker than the sun side, not black.
    GLfloat ambient[4];
    mixColor(DayNightSettings::DayDirectionalAmbient,
             DayNightSettings::NightDirectionalAmbient, nightAmount_, ambient);
    // Sun side: warm, strong sunlight.
    GLfloat diffuse[4];
    mixColor(DayNightSettings::DayDirectionalDiffuse,
             DayNightSettings::NightDirectionalDiffuse, nightAmount_, diffuse);
    GLfloat lightSpecular[4];
    mixColor(DayNightSettings::DayDirectionalSpecular,
             DayNightSettings::NightDirectionalSpecular, nightAmount_, lightSpecular);
    const GLfloat materialSpecular[] = {0.06f, 0.06f, 0.06f, 1.0f};
    GLfloat globalAmbient[4];
    mixColor(DayNightSettings::DayGlobalAmbient,
             DayNightSettings::NightGlobalAmbient, nightAmount_, globalAmbient);

    // w = 0 makes this a directional light. Interpolating the direction keeps
    // highlights consistent with the visible sun and moon during transition.
    GLfloat direction[3];
    float directionLengthSquared = 0.0f;
    for (int component = 0; component < 3; ++component)
    {
        direction[component] = mix(DayNightSettings::SunPosition[component],
                                   DayNightSettings::MoonPosition[component],
                                   nightAmount_);
        directionLengthSquared += direction[component] * direction[component];
    }
    const float inverseLength = 1.0f / std::sqrt(directionLengthSquared);
    const GLfloat position[] = {
        direction[0] * inverseLength, direction[1] * inverseLength,
        direction[2] * inverseLength, 0.0f};

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // The scene scales geometry with glScalef, so normals must be renormalised.
    glEnable(GL_NORMALIZE);

    glShadeModel(GL_SMOOTH);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    // The direction is transformed by the current modelview matrix, so
    // apply() must be called AFTER the camera/view matrix is set each frame.
    glLightfv(GL_LIGHT0, GL_POSITION, position);

    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, materialSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, mix(16.0f, 12.0f, nightAmount_));
    const GLfloat noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);
}

void Lighting::applyLocalLights(const LocalLight* lights, std::size_t count,
                                float viewerX, float viewerY, float viewerZ) const
{
    GLint hardwareLights = 0;
    glGetIntegerv(GL_MAX_LIGHTS, &hardwareLights);
    const int available = std::max(0, std::min(
        hardwareLights - 1, DayNightSettings::MaxLocalLights));

    for (int slot = 0; slot < available; ++slot)
    {
        glDisable(GL_LIGHT1 + slot);
    }
    if (nightAmount_ < DayNightSettings::LocalLightsStart || !lights || count == 0)
    {
        return;
    }

    struct RankedLight
    {
        std::size_t index;
        float distanceSquared;
    };
    std::array<RankedLight, 64> ranked{};
    const std::size_t rankedCount = std::min(count, ranked.size());
    for (std::size_t index = 0; index < rankedCount; ++index)
    {
        const float dx = lights[index].position[0] - viewerX;
        const float dy = lights[index].position[1] - viewerY;
        const float dz = lights[index].position[2] - viewerZ;
        ranked[index] = {index, dx * dx + dy * dy + dz * dz};
    }
    std::stable_sort(ranked.begin(), ranked.begin() + rankedCount,
        [lights](const RankedLight& a, const RankedLight& b)
        {
            if (lights[a.index].priority != lights[b.index].priority)
                return lights[a.index].priority;
            return a.distanceSquared < b.distanceSquared;
        });

    const float intensity = nightAmount_ * nightAmount_ * (3.0f - 2.0f * nightAmount_);
    const int enabledCount = std::min(available, static_cast<int>(rankedCount));
    for (int slot = 0; slot < enabledCount; ++slot)
    {
        const LocalLight& source = lights[ranked[slot].index];
        const GLenum light = GL_LIGHT1 + slot;
        const GLfloat position[] = {
            source.position[0], source.position[1], source.position[2], 1.0f};
        const GLfloat diffuse[] = {
            source.color[0] * intensity,
            source.color[1] * intensity,
            source.color[2] * intensity, 1.0f};
        const GLfloat specular[] = {
            diffuse[0] * 0.35f, diffuse[1] * 0.35f, diffuse[2] * 0.35f, 1.0f};
        const GLfloat noAmbient[] = {0.0f, 0.0f, 0.0f, 1.0f};

        glEnable(light);
        glLightfv(light, GL_POSITION, position);
        glLightfv(light, GL_AMBIENT, noAmbient);
        glLightfv(light, GL_DIFFUSE, diffuse);
        glLightfv(light, GL_SPECULAR, specular);
        glLightf(light, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(light, GL_LINEAR_ATTENUATION, source.linearAttenuation);
        glLightf(light, GL_QUADRATIC_ATTENUATION, source.quadraticAttenuation);
        glLightf(light, GL_SPOT_CUTOFF, source.cutoff);
        if (source.cutoff < 180.0f)
        {
            glLightfv(light, GL_SPOT_DIRECTION, source.direction);
            glLightf(light, GL_SPOT_EXPONENT, source.exponent);
        }
        else
        {
            glLightf(light, GL_SPOT_EXPONENT, 0.0f);
        }
    }
}

bool Lighting::isNight() const
{
    return nightTarget_;
}
