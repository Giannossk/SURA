<div align="center">

# SURA

[![OGRE](https://img.shields.io/badge/OGRE-3D-red.svg)](https://github.com/OGRECave/ogre)
[![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Status](https://img.shields.io/badge/status-active-brightgreen.svg)]()

</div>

A simulation application in C++23, built on [OGRE (Object-Oriented Graphics Rendering Engine)](https://github.com/OGRECave/ogre)
for rendering and presentation.

**OGRE owns everything visual**: the window, the GPU device, the scene graph, materials,
lighting, and the frame loop. The simulation side is a separate, swappable layer behind a
narrow bridge — see [AGENTS.md](AGENTS.md) section 2 for the boundary rules.

> **Status:** The rendering front end is powered by OGRE. **The physics engine has not been chosen yet** — the seam where it plugs in is marked in `src/app/main.cpp`.

## Requirements

- CMake 3.22+
- A C++23 compiler (MSVC 14.4x / GCC 13+ / Clang 17+)
- Git

On Windows, CMake ships inside Visual Studio.

## Build

Dependencies are **automatically fetched and vendored** into `third_party/` at configure time.
The latest release from [OGRECave/ogre releases](https://github.com/OGRECave/ogre/releases) is resolved and checked out automatically.

```bash
cmake -B build
cmake --build build --config Release
```

Run it:

```bash
cmake --build build --config Release --target run
```

### Dependency Options

- `-DSURA_OGRE_TAG=latest`: (default) Resolves and grabs the latest release from https://github.com/OGRECave/ogre/releases
- `-DSURA_OGRE_TAG=<tag>`: Pin to a specific version (e.g. `v14.6.0`)
- `-DSURA_BULLET_TAG=<ref>`: Bullet Physics branch/tag/commit (default: `master`)
- `-DSURA_PBD_TAG=<ref>`: Position Based Dynamics branch/tag/commit (default: `main`)
- `-DSURA_NETGEN_TAG=<ref>`: Netgen mesh generator branch/tag/commit (default: `master`)
- `-DSURA_UPDATE_DEPS=ON`: Force re-checking and updating vendored dependencies

## Repository Layout

```text
├─ CMakeLists.txt          # minimal project and application target
├─ cmake/
│  ├─ SuraDependencies.cmake    # centralized dependency management & vendoring
│  ├─ DependencyOptions.cmake   # dependency options, set before add_subdirectory
│  ├─ FindBullet.cmake          # find module for Bullet targets
│  └─ VendorDeps.cmake          # clone/update helper querying latest release
├─ src/
│  ├─ CMakeLists.txt
│  └─ app/
│     └─ main.cpp          # entry point, frame loop, physics seam
├─ third_party/            # vendored dependencies (gitignored)
└─ AGENTS.md               # architecture rules and conventions
```

## License

Apache-2.0 (see [LICENSE](LICENSE)). Dependency licenses are their own — OGRE is MIT.

-----------------------------------------------------------------------------

HEAL Simulations, Inc.  
(c) 2026 IOANNIS SIOKOS

