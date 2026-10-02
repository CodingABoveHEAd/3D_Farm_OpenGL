#include "graphics/TextureManager.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <cmath>

namespace TextureManager {
namespace {

GLuint g_textures[Count] = {0};
bool g_initialized = false;

// Simple deterministic pseudo-random hash
float hash2D(int x, int y)
{
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f) * 0.5f + 0.5f;
}

GLuint generateGrassTexture(int size)
{
    std::vector<unsigned char> data(size * size * 3);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float h1 = hash2D(x, y);
            const float h2 = hash2D(x / 2, y / 2);
            const float noise = h1 * 0.35f + h2 * 0.65f;

            // Green pasture variations: base (0.32, 0.65, 0.22)
            const float r = (0.24f + noise * 0.16f) * 255.0f;
            const float g = (0.54f + noise * 0.25f) * 255.0f;
            const float b = (0.16f + noise * 0.14f) * 255.0f;

            const int idx = (y * size + x) * 3;
            data[idx + 0] = static_cast<unsigned char>(r > 255.0f ? 255 : r);
            data[idx + 1] = static_cast<unsigned char>(g > 255.0f ? 255 : g);
            data[idx + 2] = static_cast<unsigned char>(b > 255.0f ? 255 : b);
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return tex;
}

GLuint generateDirtTexture(int size)
{
    std::vector<unsigned char> data(size * size * 3);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float h1 = hash2D(x + 101, y + 203);
            const float h2 = hash2D(x / 3, y / 3);
            const float noise = h1 * 0.4f + h2 * 0.6f;

            // Warm earthy dirt tones (0.55, 0.40, 0.25)
            const float r = (0.46f + noise * 0.18f) * 255.0f;
            const float g = (0.34f + noise * 0.14f) * 255.0f;
            const float b = (0.20f + noise * 0.10f) * 255.0f;

            const int idx = (y * size + x) * 3;
            data[idx + 0] = static_cast<unsigned char>(r > 255.0f ? 255 : r);
            data[idx + 1] = static_cast<unsigned char>(g > 255.0f ? 255 : g);
            data[idx + 2] = static_cast<unsigned char>(b > 255.0f ? 255 : b);
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return tex;
}

GLuint generateWoodTexture(int size)
{
    std::vector<unsigned char> data(size * size * 3);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            // Wood grain rings / stripes along Y
            const float grain = std::sin(static_cast<float>(y) * 0.35f + hash2D(x, y) * 1.5f);
            const float base = 0.55f + grain * 0.12f;

            const float r = (base * 0.95f) * 255.0f;
            const float g = (base * 0.65f) * 255.0f;
            const float b = (base * 0.35f) * 255.0f;

            const int idx = (y * size + x) * 3;
            data[idx + 0] = static_cast<unsigned char>(r > 255.0f ? 255 : r);
            data[idx + 1] = static_cast<unsigned char>(g > 255.0f ? 255 : g);
            data[idx + 2] = static_cast<unsigned char>(b > 255.0f ? 255 : b);
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return tex;
}

GLuint generateStoneTexture(int size)
{
    std::vector<unsigned char> data(size * size * 3);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const float noise = hash2D(x * 2, y * 2) * 0.4f + hash2D(x, y) * 0.6f;
            const float lum = 0.40f + noise * 0.25f;

            const int idx = (y * size + x) * 3;
            data[idx + 0] = static_cast<unsigned char>(lum * 255.0f);
            data[idx + 1] = static_cast<unsigned char>((lum * 1.02f) * 255.0f);
            data[idx + 2] = static_cast<unsigned char>((lum * 0.98f) * 255.0f);
        }
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    return tex;
}

} // namespace

void init()
{
    if (g_initialized) return;

    g_textures[Grass]    = generateGrassTexture(64);
    g_textures[DirtRoad] = generateDirtTexture(64);
    g_textures[Wood]     = generateWoodTexture(64);
    g_textures[Stone]    = generateStoneTexture(64);

    g_initialized = true;
}

void bind(TextureType type)
{
    if (!g_initialized) init();

    if (type >= 0 && type < Count && g_textures[type] != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_textures[type]);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
}

void unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void shutdown()
{
    if (g_initialized)
    {
        for (int i = 0; i < Count; ++i)
        {
            if (g_textures[i] != 0)
            {
                glDeleteTextures(1, &g_textures[i]);
                g_textures[i] = 0;
            }
        }
        g_initialized = false;
    }
}

} // namespace TextureManager
