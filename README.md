# Animated 3D Farm Scene with Lighting and Transformations

Student: Niloy Chowdhury  
Roll: 2107117

This project is being built incrementally with C++, GLFW, and legacy OpenGL compatibility rendering. The current scaffold provides:

- GLFW application lifecycle
- 3D perspective projection
- Depth testing
- Interactive camera movement
- Centralized keyboard state tracking
- Scene ownership and frame updates
- A 3D ground plane

## Controls

- `W`, `A`, `S`, `D`: move the camera
- `Q`, `E`: move vertically
- Arrow keys: rotate the camera
- `R`: reset the camera
- `Esc`: exit

## Build

The included `CMakeLists.txt` expects GLFW to be available through a CMake package configuration and OpenGL to be available from the platform.

```text
cmake -S . -B build
cmake --build build
```

`test.cpp` is preserved as the original 2D reference implementation while the new application is developed under `src/` and `include/`.
