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

## Build

The included `CMakeLists.txt` expects GLFW to be available through a CMake package configuration and OpenGL to be available from the platform.

```text
cmake -S . -B build
cmake --build build
```

`test.cpp` is preserved as the original 2D reference implementation while the new application is developed under `src/` and `include/`.
