#include "objects/Birds.h"

#include "DayNightSettings.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace Birds {
namespace {
constexpr float Pi = 3.14159265358979323846f;

float fract(float value) { return value - std::floor(value); }

void drawBird(float x, float y, float z, float scale, float heading,
              float wingAngle, float r, float g, float b, float alpha)
{
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(heading, 0.0f, 1.0f, 0.0f);
    glScalef(scale, scale, scale);

    glColor4f(r, g, b, alpha);
    glBegin(GL_TRIANGLES);
    // Compact body and tail, facing +X in local space.
    glVertex3f(-0.50f, 0.0f, 0.0f);
    glVertex3f( 0.48f, 0.0f, 0.0f);
    glVertex3f(-0.35f, 0.15f, 0.0f);
    glVertex3f(-0.40f, 0.0f, 0.0f);
    glVertex3f(-0.72f, 0.10f, 0.18f);
    glVertex3f(-0.68f, 0.06f,-0.16f);
    glEnd();

    // Each wing pivots independently at the body. Triangles keep distant
    // silhouettes recognizable with very little geometry.
    for (float side : {-1.0f, 1.0f})
    {
        glPushMatrix();
        glRotatef(wingAngle * side, 1.0f, 0.0f, 0.0f);
        glBegin(GL_TRIANGLES);
        glVertex3f(0.05f, 0.04f, 0.0f);
        glVertex3f(-0.20f, 0.02f, 1.05f * side);
        glVertex3f( 0.42f, 0.01f, 0.34f * side);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();
}
}

void draw(float animationTime, float nightAmount)
{
    nightAmount = std::clamp(nightAmount, 0.0f, 1.0f);
    const float dayAlpha = 1.0f - nightAmount;

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT |
                 GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    // Three loose flocks with different periods, altitude and phase. Positions
    // wrap far outside the central view, avoiding abrupt edge spawning.
    if (dayAlpha > 0.001f)
    {
        for (int flock = 0; flock < 3; ++flock)
        {
            const float period = 25.0f + flock * 7.0f;
            const float travel = fract(animationTime / period + flock * 0.31f);
            const float baseX = -62.0f + travel * 124.0f;
            const float baseY = 14.0f + flock * 4.0f
                + std::sin(animationTime * 0.23f + flock) * 1.2f;
            const float baseZ = -42.0f - flock * 14.0f;
            const int count = 4 + flock;
            for (int bird = 0; bird < count; ++bird)
            {
                const float side = bird % 2 ? 1.0f : -1.0f;
                const float rank = static_cast<float>((bird + 1) / 2);
                const float wing = 32.0f * std::sin(
                    animationTime * (5.0f + 0.35f * flock) + bird * 1.73f);
                drawBird(baseX - rank * 2.0f, baseY - rank * 0.35f,
                         baseZ + side * rank * 1.55f,
                         0.62f + flock * 0.07f, 90.0f, wing,
                         0.10f, 0.12f, 0.14f, dayAlpha * 0.92f);
            }
        }
    }

    // A sparse night pair crosses the moon only during part of a long cycle.
    // Alpha also follows the day/night blend, so neither population pops.
    const float crossing = fract(animationTime / 22.0f);
    if (nightAmount > 0.001f && crossing < 0.38f)
    {
        const float progress = crossing / 0.38f;
        const float fade = std::min(1.0f,
            std::min(progress * 8.0f, (1.0f - progress) * 8.0f));
        for (int bird = 0; bird < 2; ++bird)
        {
            const float x = DayNightSettings::MoonPosition[0]
                - 5.5f + progress * 11.0f - bird * 1.4f;
            const float y = DayNightSettings::MoonPosition[1]
                + (bird == 0 ? 0.20f : -0.55f);
            const float wing = 24.0f * std::sin(animationTime * 4.2f + bird * 2.1f);
            drawBird(x, y, DayNightSettings::MoonPosition[2] + 0.8f,
                     0.42f, 90.0f, wing, 0.015f, 0.018f, 0.025f,
                     nightAmount * fade);
        }
    }

    glDepthMask(GL_TRUE);
    glPopAttrib();
}

} // namespace Birds
