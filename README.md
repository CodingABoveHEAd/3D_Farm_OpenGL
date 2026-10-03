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
- Fast smooth camera movement with WASD and Q/E vertical controls
- Ambient, diffuse, and specular lighting for the 3D scene
- Extended terrain, road, and utility lines for a fake infinite-world horizon
- Phase 2 farm ground, crop region, and boundary fence
- Phase 3 low-polygon farmhouse built from OpenGL primitives
- Phase 4 reusable trees and repeated crop instances
- Phase 5 entrance gate, farmhouse pathway, and simple environmental rocks
- Phase 6 low-polygon manually controlled tractor
- Phase 8 sun and smoothly moving cloud groups
- Static grazing cows built from low-polygon primitives
- Twelve transformed copies of the complete farm arranged along a central road
- Animated farmers walking only along the central road
- A dense sky field of enlarged drifting clouds distributed across the horizon
- A rural electrical substation with fenced transformers and sagging power lines
- Utility poles and sagging power lines running along both sides of the main road
- Per-farm layout variation with different tree, animal, tractor, and windmill arrangements
- Very dense, enlarged forest belts and clusters throughout the complete open world while keeping the road clear
- Very dense world-wide tree scattering includes roadside belts while landmark and farm footprints remain clear
- Detailed red, green, blue, and brown tractors circulating on the central road
- Probabilistic, independently staggered road traffic arrivals
- Detailed rural barns with roofs, doors, beams, hay, fences, and equipment
- Enlarged world objects and landmarks for a fuller visual composition
- Farmers tending the crop rows with repeated walking and working motions
- Ponds with animated water, natural edges, reeds, rocks, grass, and a wooden bridge

## Phase 2 Environment

The farm uses a simple world-coordinate layout:

- Grass: 60 x 60 ground plane centered at `(0, 0, 0)`
- Crop field: translated toward negative Z, rotated by `-3` degrees, and scaled from a reusable plane
- Boundary: cube-based fence around approximately `x = -24..24`, `z = -19..19`
- Transformation marker: a red cube at `(-14, 1, 8)` with visible translation, rotation, and non-uniform scaling

The crop field contains only farmland rows for visual organization. No crops, tractor, windmill, trees, or animation have been added yet.

## Phase 3 Farmhouse

The farmhouse is implemented in [Farmhouse.cpp](src/objects/Farmhouse.cpp) and uses separate functions for its body, two-part roof, door, windows, chimney, and porch. It is placed near `(10, 0, 7)` using world coordinates. Each component demonstrates translation, rotation, and scaling through the legacy OpenGL model matrix stack.

## Phase 4 Trees and Crops

Vegetation is implemented in [Vegetation.cpp](src/objects/Vegetation.cpp). `drawTree(x, z, scale)` builds each tree hierarchically from one trunk and three crown cubes. `drawCrop(x, z, scale)` builds each crop from a stem and two rotated leaves. The scene reuses these functions for six trees and 42 crops, with translated positions and small scale variations instead of duplicating geometry definitions.

## Phase 5 Fence Details

The existing perimeter fence now has an entrance opening on the front side at approximately `x = 10`. Two gate panels use translated, scaled, and rotated fence rails. A simple brown pathway begins at the gate and leads toward the farmhouse, while four small cube-based rocks add environmental detail without introducing textures or external models.

## Phase 6 Tractor

The tractor is implemented in [Tractor.cpp](src/objects/Tractor.cpp). Its complete model is drawn under one root translation stored in `position_[3]`. The body, engine, cabin, roof, axles, wheels, and exhaust use local transforms relative to that root. Manual movement uses `I`, `J`, `K`, and `L`; wheel rotation changes only while the tractor is being moved.

## Phase 8 Sky and Clouds

The sky background uses the existing clear color set by the application. A low-polygon sphere is rendered as the sun in [sky.cpp](src/objects/sky.cpp). Each cloud in [cloud.cpp](src/objects/cloud.cpp) is a small hierarchy of three overlapping spheres.

Cloud translation is controlled by `Animation::cloudOffset()`. Every frame, `Animation::update(deltaTime)` increases the offset by `0.8 * deltaTime`, so movement remains smooth and frame-rate independent. When the offset passes `28`, it wraps to `-28`, keeping clouds inside a repeating world-space range without accumulating an unbounded position.

## Animals

[Animals.cpp](src/objects/Animals.cpp) provides the reusable `Animals::drawCow(x, z, scale, rotation)` function. Each cow is built hierarchically from a body, patches, lowered neck and head, muzzle, ears, horns, eyes, four legs with hooves, and a tail. Three cows are placed in the open pasture with different world positions, scales, and rotations. They are static grazing poses; no new animal animation was added.

## Multiple Farms and Road

The complete existing farm layout is rendered twelve times without changing the
farmhouse, tractor, windmill, vegetation, animal, fence, or rock definitions.
Each copy uses the same geometry and colors under a uniform world transform.
The copies are arranged in six rows on both sides of a central road, and the
ground is expanded to provide room for the full layout. The farms use enlarged
uniform transforms and wider row spacing so neighboring farms have more room
around their fences, crops, and equipment.

## Farmers

Farmers are reusable low-polygon characters animated by the scene update loop.
They travel only along the central road in a larger group with different
positions, speeds, and walking phases. Their walking limbs swing while moving,
and the existing `P` pause control pauses their movement along with the other
scene animations.

## Farm Variation

The twelve farms use deterministic scene-level layouts instead of identical
copy-pasted arrangements. Each layout can have two to five trees, zero to two
cows, an optional tractor, an optional windmill, and small independent offsets
for the crop field and farmhouse. The reusable object implementations and
their colors remain unchanged.

## Road Tractors

The existing detailed tractor model is reused for road traffic rather than
duplicated. Four scaled road tractors circulate along the central road in red,
green, blue, and brown body variants. The original manually controlled farm
tractor remains available with `I`, `J`, `K`, and `L`.

Road farmers and tractors no longer follow a fixed synchronized pattern. Each
traffic unit independently receives a randomized active travel duration and
waiting interval, then re-enters from the road edge. The initial active states
are staggered as well, so every application run produces a different traffic
arrival sequence.

## Barns

Four rural barns are placed around the outer farm areas. Each barn is built
hierarchically from a large body, two sloped roof slabs, large front doors, a
side door, windows, structural beams, hay bales, and small nearby equipment.
Several barns also have their own surrounding fence.

Each farm also has a crop worker placed inside its field. Crop workers move
between nearby crop rows, pause their walking, and continue with slower
working motions so they appear to tend the plants rather than travel on the
road.

## Ponds and Wooden Bridge

Three ponds are placed around the farms. Each pond has a sandy soil edge,
rocks, reeds, grass, translucent water, and animated brightness variation.
The larger pond includes a rural wooden bridge with a deck, supports, posts,
side railings, and a curved top rail.

The bridge railing is sampled from a cubic Bezier curve. The implementation
keeps the full curve equation in `src/objects/Pond.cpp` as a course-project
comment and uses the sampled points to create connected wooden rail segments.

## Controls

- `W`, `A`, `S`, `D`: move the camera
- `Q`, `E`: move vertically
- `Left Shift`: move faster
- `Left Ctrl`: move slowly for precise positioning
- Mouse: rotate the camera
- Arrow keys: rotate the camera as an alternative
- `R`: reset the camera
- `I`, `K`: move tractor forward/backward
- `J`, `L`: move tractor left/right
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
