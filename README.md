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
- A 3D ground plane and verification grid
- A reference cube rendered with a model transformation
- Mouse-look camera rotation
- Phase 2 farm ground, crop region, and boundary fence
- Phase 3 low-polygon farmhouse built from OpenGL primitives

## Phase 2 Environment

The farm uses a simple world-coordinate layout:

- Grass: 60 x 60 ground plane centered at `(0, 0, 0)`
- Crop field: translated toward negative Z, rotated by `-3` degrees, and scaled from a reusable plane
- Boundary: cube-based fence around approximately `x = -24..24`, `z = -19..19`
- Transformation marker: a red cube at `(-14, 1, 8)` with visible translation, rotation, and non-uniform scaling

The crop field contains only farmland rows for visual organization. No crops, tractor, windmill, trees, or animation have been added yet.

## Phase 3 Farmhouse

The farmhouse is implemented in [Farmhouse.cpp](src/objects/Farmhouse.cpp) and uses separate functions for its body, two-part roof, door, windows, chimney, and porch. It is placed near `(10, 0, 7)` using world coordinates. Each component demonstrates translation, rotation, and scaling through the legacy OpenGL model matrix stack.

## Controls

- `W`, `A`, `S`, `D`: move the camera
- `Q`, `E`: move vertically
- Mouse: rotate the camera
- Arrow keys: rotate the camera as an alternative
- `R`: reset the camera
- `Esc`: exit

The mouse cursor is captured while the application is running. Press `Esc` to close the window and release the cursor.

## Phase 1 Run Script

On Windows with the existing MSYS2 UCRT64 GLFW installation, run this from PowerShell:

```powershell
.\run_phase1.ps1
```

The script compiles all files under `src/` into `build/Phase1Farm.exe`, adds the local GLFW DLL directory to `PATH`, and launches the program.

For Git Bash, run:

```bash
cd /d/4-1/3D_Farm_OpenGl
chmod +x run_phase1.sh
./run_phase1.sh
```

The script compiles with the MSYS2 UCRT64 GLFW installation and launches the Phase 2 environment.

## Build

The included `CMakeLists.txt` expects GLFW to be available through a CMake package configuration and OpenGL to be available from the platform.

```text
cmake -S . -B build
cmake --build build
```

`test.cpp` is preserved as the original 2D reference implementation while the new application is developed under `src/` and `include/`.
