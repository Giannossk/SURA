# AGENTS.md

Guidance for AI coding agents (and humans) working in this repository.

---

## 1. What this project is

A **simulation application in C++23**, combining a real-time physics
simulation with an OGRE-based 3D front end.

| Layer | Role | Where |
| --- | --- | --- |
| [OGRE](https://github.com/OGRECave/ogre) | **Everything visual.** 3D scene graph, GPU device, frame loop, materials, lighting, cameras, viewports. | `third_party/ogre`, vendored dynamically from the latest release |
| [Bullet Physics](https://github.com/bulletphysics/bullet3) | **Everything simulated.** Dynamics, constraints, collision, solvers. | `third_party/bullet3`, vendored from master branch |
| [Netgen](https://github.com/NGSolve/netgen) | **Mesh generation.** Automatic 3D tetrahedral mesh generation and optimization. | `third_party/netgen`, vendored from master branch |
| Bridge / Integration | Translation and synchronization between the two (using `OgreBullet` component). | `OgreBullet` / `src/app/main.cpp` |

This is a *bridge and an application*, not a renderer and not a physics engine.
Its whole job is to move state from the simulation into OGRE's scene and assets
fast enough to hold frame rate.

---

## 2. The one architectural rule

> **The simulation layer must never contain, reference, or link rendering code.
> OGRE owns everything visual.**

Every change must preserve this. It is not stylistic — it is what keeps the
simulation deterministic, testable without a GPU, and swappable.

### 2.1 Where the line falls

| Simulation concern | Presentation concern |
| --- | --- |
| Mass, stiffness, damping, forces | Colour, material, shader |
| Constraints and joints | Scale, artistic placement |
| Collision shape and response | How a collision *looks* |
| Integrators and solvers | Camera, lighting, viewports |
| Topology (which vertices connect to which) | Which GPU buffers hold them |
| What moves, and why | Scene nodes, visual orientation |

If a decision has no physical meaning, it does not belong in the simulation
layer. If it has no visual meaning, it does not belong in OGRE code.

### 2.2 Do not build a second scene graph

OGRE has a scene graph (`SceneNode`, `Entity`, `ManualObject`) and most physics
engines have their own. Given any task, decide which side owns the concept and
use only that one:

- **Simulation meaning** → physics engine, always.
- **Visual meaning** → OGRE, always.

Never mirror the simulation's object graph into OGRE's hierarchy. Extract
*geometry* into a snapshot and hand it to a dynamic mesh or `ManualObject` (§6).
Never let an OGRE transform feed back into a simulated body.

### 2.3 The physics engine: Bullet Physics (bullet3)
 
Bullet Physics is vendored from the latest commit on `master` branch:
`https://github.com/bulletphysics/bullet3` into `third_party/bullet3`.
 
- Configurable via `SURA_BULLET_TAG` (defaults to `master`).
- Configured with `USE_MSVC_RUNTIME_LIBRARY_DLL=ON` to match OGRE's `/MD` CRT.
- OGRE's built-in Bullet component (`OgreBullet`) is enabled (`OGRE_BUILD_COMPONENT_BULLET=ON`).
- `cmake/FindBullet.cmake` resolves the vendored `third_party/bullet3` targets (`BulletDynamics`, `BulletCollision`, `LinearMath`).
- `src/app/main.cpp` executes the simulation step via `btDiscreteDynamicsWorld` and updates scene nodes via `Ogre::Bullet::RigidBodyState`.
 
### 2.4 Position Based Dynamics (PBD)

PBD is vendored from `https://github.com/Giannossk/pbd` into `third_party/pbd`.

- Configurable via `SURA_PBD_TAG` (defaults to `main`).
- Uses `LinearMath` from `https://github.com/Giannossk/LinearMath` as a dependency.
- Targets `pbd` (alias `pbd::pbd`) providing position-based dynamics solvers, constraints, and numerical integrators.

### 2.5 Netgen Mesh Generation

Netgen is vendored from `https://github.com/NGSolve/netgen` into `third_party/netgen`.

- Configurable via `SURA_NETGEN_TAG` (defaults to `master`).
- Targets `nglib` and `ngcore` providing 3D tetrahedral mesh generation and optimization.
- Configured headless without GUI or Python (`USE_GUI=OFF`, `USE_PYTHON=OFF`, `USE_SUPERBUILD=OFF`).

---

## 3. Repository layout

```text
.
├─ AGENTS.md                 # this file
├─ CMakeLists.txt            # minimal root: project, module path, app target
├─ cmake/
│  ├─ SuraDependencies.cmake   # centralized dependency management & vendoring
│  ├─ DependencyOptions.cmake  # dependency options, set before add_subdirectory
│  ├─ FindBullet.cmake         # find module for Bullet targets
│  └─ VendorDeps.cmake         # clone-at-configure-time helper (fetches latest release)
├─ src/
│  ├─ CMakeLists.txt
│  ├─ app/                   # OGRE application: entry point, frame loop, setup
│  │  └─ main.cpp
│  ├─ scene/                 # simulation ownership: build, step, query   (to create)
│  └─ bridge/                # THE translation layer                     (to create)
│     ├─ MeshBridge.*        # simulated geometry -> OGRE mesh / ManualObject
│     ├─ Snapshot.hpp        # POD frame snapshot handed to the render side
│     └─ Mapping.hpp         # engine vectors/quaternions <-> OGRE types
├─ assets/                   # assets folder (textures, materials, models)
└─ third_party/              # cloned dependencies, gitignored
```

`src/bridge/` is the only place allowed to include both the physics engine's
headers and OGRE headers. Keep it that way — it makes the boundary greppable.

---

## 4. Build

### 4.1 Dependencies are vendored, not installed

CMake clones each dependency into `third_party/` at configure time.
For OGRE, CMake automatically checks and retrieves the latest release from
`https://github.com/OGRECave/ogre/releases` (configurable via `SURA_OGRE_TAG`).
Nothing is installed by hand.

```bash
cmake -B build
cmake --build build --config Release
```

| When | What |
| --- | --- |
| First configure | Queries latest release from GitHub and clones into `third_party/ogre` |
| Later configures | Reuses existing checkout, checks if newer release is published |
| `-DSURA_UPDATE_DEPS=ON` | Forces re-fetching the release tag |
| `-DSURA_OGRE_TAG=<tag>` | Pins a specific release/tag instead of 'latest' |
| `-DSURA_BULLET_TAG=<tag>` | Pins Bullet ref (branch/tag/commit) |
| `-DSURA_PBD_TAG=<tag>` | Pins PBD ref (branch/tag/commit) |
| `-DSURA_NETGEN_TAG=<tag>` | Pins Netgen ref (branch/tag/commit) |

### 4.2 Centralized dependency management & option ordering

All dependency configuration is encapsulated in `cmake/SuraDependencies.cmake`
to keep `CMakeLists.txt` clean and minimal.

`cmake/DependencyOptions.cmake` sets dependency options *before*
`sura_vendor_dependency()` runs. **This ordering is load-bearing.** The
vendored projects declare their options with `set(<X> ... CACHE BOOL ...)` or
`option()`, and both respect a pre-existing cache entry — setting them
afterwards does nothing at all.

### 4.3 C++ standard: per-target, never global

- OGRE specifies its required standard internally.
- This project targets C++23 per-target in `src/CMakeLists.txt`.

**Do not set `CMAKE_CXX_STANDARD` globally.**

---

## 5. Threading model

`startRendering()` / `renderOneFrame()` owns the main thread: it owns the
window, the GPU device and the frame loop. An unbounded physics step on that
same thread couples solver cost directly to render frame time.

```text
main thread                                 simulation thread (if needed)
-----------                                 ------------------------------
startRendering()
  frameRenderingQueued():  <- every frame
    ├─ acquire latest snapshot (short lock)
    ├─ update ManualObject / vertex buffers
    └─ process visual nodes
```

Rules:

1. **All OGRE calls happen on the main thread.**
2. **All physics calls happen on the simulation thread.** Never dereference a
   simulation object from render callbacks.
3. **Never hold a lock across a solver step**, and never across a GPU upload.
4. **Use a fixed timestep.** Solvers are tuned for it.
5. **Double- or triple-buffer the payload.**
6. **Stop order matters**: signal the sim thread to stop → `join()` → *then*
   shut down OGRE.

---

## 6. The bridge: simulation → OGRE

Dynamic geometry updates flatten simulation data into `Ogre::ManualObject` or
hardware vertex buffers (`Ogre::HardwareVertexBufferSharedPtr`).

---

## 7. Commands

```bash
# Configure (resolves latest OGRE release)
cmake -B build

# Build (Release)
cmake --build build --config Release

# Run
cmake --build build --config Release --target run
```

On Windows, CMake and Ninja ship inside Visual Studio. If `cmake` is not on `PATH`:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -B build
```
