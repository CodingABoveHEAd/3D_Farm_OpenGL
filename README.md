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
- Heading-relative first-person movement with W/S, A/D turning, and Q/E vertical controls
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
- Three detailed, driver-occupied tractors circulating on the central road
- Evenly spaced road traffic with a pause/resume control
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

The tractor is implemented in [Tractor.cpp](src/objects/Tractor.cpp). Its complete model is drawn under one root translation stored in `position_[3]`. The body, engine, cabin, roof, axles, wheels, and exhaust use local transforms relative to that root. Manual movement uses `I`/`K` to drive and `J`/`;` to steer; wheel rotation changes only while the tractor is being moved.

## Phase 8 Sky and Clouds

The sky is a camera-centred gradient dome. The sun in
[sky.cpp](src/objects/sky.cpp) is a viewer-facing distant disk with a soft,
restrained glow; it shares `DayNightSettings::SunPosition` with the daylight
direction, fades through the day/night transition, and never writes scene
depth. Each cloud in [cloud.cpp](src/objects/cloud.cpp) is a small hierarchy of
overlapping spheres.

Cloud translation is controlled by `Animation::cloudOffset()`. Every frame, `Animation::update(deltaTime)` increases the offset by `0.8 * deltaTime`, so movement remains smooth and frame-rate independent. When the offset passes `28`, it wraps to `-28`, keeping clouds inside a repeating world-space range without accumulating an unbounded position.

## Animals

[Animals.cpp](src/objects/Animals.cpp) provides the reusable animated cow.
Each cow has a proportioned barrel, chest and rump, individual hide markings,
articulated grazing neck, muzzle, ears, horns, blinking eyes, grounded legs,
cloven hooves, udder and fly-swatting tail. Cows use position-derived timing so
they lower their heads, chew, flick their ears and occasionally look up at
different times; all motion comes from the shared frame-rate-independent scene
clock.

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
duplicated. Three scaled road tractors circulate along the central road in red,
green, and blue body variants. Every road vehicle contains a seated driver
using the shared articulated villager model. The original manually controlled
farm tractor remains available with `I`/`K` and `J`/`;`.

Three road tractors form a continuously wrapped convoy. They share one speed
and start 72 world units apart, so wrapping cannot cause a vehicle to respawn
on top of another vehicle. `T` pauses or resumes traffic without changing its
spacing. The roadside population was reduced to four possible pedestrians,
only two of which start active. Longer waiting intervals and a guarded entry
point prevent repetitive, overlapping pedestrian arrivals.

## Barns

Four rural barns are placed around the outer farm areas. Each is now a real
wall shell rather than a solid box, with an accessible front opening, visible
two-sided roof, packed-earth timber floor, posts, tie beams, roof supports,
side stalls, feeding troughs, a clear centre aisle, hay, shelves and hanging
tools. The front doors slide along an overhead track and camera collision
follows their animated opening, so a closed door blocks entry and an open one
admits the camera. Exterior and pendant lights provide warm night illumination
and fade with load shedding.

Three manual interior interactions share the barn architecture: a bounded
wheelbarrow with matching wheel rotation, a hinged storage chest, and a hinged
feeding gate. Their animations reverse smoothly and remain usable without
grid power. Several barns also retain their surrounding fence.

Each farm receives a stable random population of zero to three crop workers at
startup. Crop workers move between nearby crop rows, pause their walking, and
continue with slower working motions so they appear to tend the plants rather
than travel on the road. `O` independently pauses field work, while `[` and
`]` decrease or increase work speed.

## Ponds and Wooden Bridge

Three ponds are placed around the farms. Each pond has a sandy soil edge,
rocks, reeds, grass, translucent water, and animated brightness variation.
The larger pond includes a rural wooden bridge with a deck, supports, posts,
side railings, and a curved top rail.

The bridge railing is sampled from a cubic Bezier curve. The implementation
keeps the full curve equation in `src/objects/Pond.cpp` as a course-project
comment and uses the sampled points to create connected wooden rail segments.

## Environment and Bonfires

Ground and road colours now respond to the day/night transition, with an
additional restrained darkening during a nighttime power outage. The full
road has segmented centre markings, entrance gaps, dirt shoulders, and subtle
flat weathering patches placed above the surface to avoid z-fighting.

Deterministic, chunk-culled grass fills suitable open terrain. Farm entrances,
roads, paths, structures, ponds, and fire clearings remain unobstructed.
Flowers around houses, flowering shrubs, and flowering trees add colour using
cached low-polygon geometry.

Five bonfire clearings contain stone rings, crossed logs, embers, animated
flames, restrained smoke and sparks, and small groups of shared articulated
villagers. Their warm night lights remain independent of grid power. `B`
toggles all bonfire sites without affecting traffic or other animation.
All reusable benches, stools and split-log seats share one seat-surface height;
the seated pelvis, thighs, knees and grounded feet are authored against that
height so idle gestures do not push bodies through the furniture.

## Controls

- `W`, `S`: move forward/backward relative to the camera's current heading
- `A`, `D`: turn left/right in place (hold with `W` or `S` for curved movement)
- `Q`, `E`: move vertically
- `Left Shift`: move faster
- `Left Ctrl`: move slowly for precise positioning
- Left click or `Tab`: capture/release the mouse; move the mouse to look
- Arrow keys: rotate the camera as an alternative
- `R`: reset the camera
- `I`, `K`: drive the tractor forward/backward
- `J`, `;`: steer the tractor left/right while driving (`L` controls grid power)
- `N`: toggle day/night mode (one toggle per key press)
- `T`: pause/resume road traffic (one toggle per key press)
- `B`: show/hide the five bonfire gatherings (one toggle per key press)
- `G`: open/close all barn sliding doors (one toggle per key press; reversible)
- `Z`, `X`: roll the barn wheelbarrow toward the entrance/rear while held
- `C`: open/close the barn storage chest (one toggle per key press)
- `F`: open/close the barn feeding gate (one toggle per key press)
- `L`: toggle load shedding; grid lights switch and windmills coast to a stop
- `O`: pause/resume crop workers independently
- `[`, `]`: decrease/increase crop-worker animation speed
- `P`: pause/resume all scene animation
- `+`, `-`: adjust windmill speed
- `F11`: toggle fullscreen while preserving the windowed size and position
- `V`: toggle vertical synchronization
- `Esc`: release a captured mouse; press again to exit

The title bar reports current FPS, frame time, V-sync state, and window mode.
It also reports grid-power, traffic, bonfire, and worker-pause state.
W/S movement follows the camera heading and stays parallel to the ground even
while looking up or down. Opposing movement or turn keys cancel each other.

Night mode transitions smoothly over three seconds and adds a moon, stars,
darker clouds, moonlight, warm building/street lighting, and headlights that
follow the moving road tractors. The transition continues while scene
animation is paused. Visual and timing constants—including the transition
duration, sky/fog colors, global light levels, and lamp colors—are centralized
in `include/DayNightSettings.h`.

Load shedding is independent of day/night mode: moonlight, stars, and vehicle
headlights remain available, while farmhouse, shop, barn, substation, and road
lighting lose grid power. Windmills decelerate and restart smoothly. Village
simulation tuning (population limits, work speed, traffic spacing, power fade,
and pond sizes) is centralized in `include/VillageSimulationSettings.h`.

The village environment also includes enlarged traditional windmills, varied
farm grass, larger animated ponds, pond and roadside seating, three individual
tea shops with animated customers, and frame-rate-independent bird flocks.
During full night, daytime flocks fade out and an occasional distant pair
crosses the moon. Cows alternate between grazing and briefly raising their
heads; all ambient motion follows the shared pausable scene clock.

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

For the MSYS2 UCRT64 toolchain on Windows, a complete Release build is:

```powershell
cmake -S . -B build-cmake -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe `
  -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/mingw32-make.exe
cmake --build build-cmake -j 4
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
.\build-cmake\Animated3DFarmScene.exe
```

The CMake build also includes deterministic camera-control checks:

```powershell
ctest --test-dir build-cmake --output-on-failure
```

For an uncapped, automatically terminating performance run, set
`FARM_VSYNC=0` and `FARM_BENCHMARK_SECONDS` before launching. The first five
seconds are treated as display-list warm-up; override that with
`FARM_BENCHMARK_WARMUP` when needed. A machine-readable `BENCHMARK` line is
printed on completion. `FARM_WINDOW_SMOKE_TEST=1` runs an automatic windowed,
fullscreen, restored, and resized transition check and then exits.

## Runtime architecture

`Application` owns the GLFW window, fullscreen/V-sync state, timing, and frame
order. `Camera` owns view input and frame-rate-independent movement. `Scene`
owns gameplay objects, visibility/LOD decisions, and the single pausable
animation clock. Object modules build their models from the shared OpenGL
primitives. Static farms and vegetation use display lists, while distant trees
and clouds use cheaper models and frustum/distance culling. Linear fog hides
the LOD horizon and improves depth cues without changing nearby farm detail.
`Village` owns reusable benches, seated/standing villagers, tea shops, and
farm grass, while `Birds` owns sky-space flock and moon-crossing animation.

`test.cpp` is preserved as the original 2D reference implementation while the new application is developed under `src/` and `include/`.
