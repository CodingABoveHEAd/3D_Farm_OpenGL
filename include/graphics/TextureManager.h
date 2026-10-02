#pragma once

namespace TextureManager {

enum TextureType {
    Grass = 0,
    DirtRoad,
    Wood,
    Stone,
    Count
};

// Initialize procedural textures in OpenGL memory (no external file needed)
void init();

// Bind a procedural texture for rendering
void bind(TextureType type);

// Unbind / disable 2D texturing
void unbind();

// Release textures on shutdown
void shutdown();

} // namespace TextureManager
