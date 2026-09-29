# SuraDependencies.cmake
#
# Centralized dependency management for SURA.
# Configures options, fetches, and sets up:
#   - Bullet Physics (bullet3)
#   - LinearMath
#   - PBD (Position Based Dynamics)
#   - Netgen (Mesh generator)
#   - OGRE (3D Graphics Engine)

include(DependencyOptions)
include(VendorDeps)
include(FetchContent)

# ---------------------------------------------------------------------------
# Dependency Version Tags & Options
# ---------------------------------------------------------------------------
option(SURA_UPDATE_DEPS "Re-fetch vendored dependencies on every configure" OFF)

set(SURA_OGRE_TAG "latest" CACHE STRING
    "OGRE git tag to vendor into third_party/ogre ('latest' queries https://github.com/OGRECave/ogre/releases)")

set(SURA_BULLET_TAG "master" CACHE STRING
    "Bullet git ref (branch/tag/commit) to vendor into third_party/bullet3")

set(SURA_PBD_TAG "main" CACHE STRING
    "PBD git ref (branch/tag/commit) to vendor into third_party/pbd")

set(SURA_NETGEN_TAG "master" CACHE STRING
    "Netgen git ref (branch/tag/commit) to vendor into third_party/netgen")

# ---------------------------------------------------------------------------
# Apply Options to Cache
# ---------------------------------------------------------------------------
# Options must be set in the cache *before* subprojects are added.
sura_apply_dependency_options()

# Ensure prebuilt ogre_dependencies are in prefix path if present
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/ogre_dependencies")
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/third_party/ogre_dependencies")
    set(ZLIB_ROOT "${CMAKE_SOURCE_DIR}/third_party/ogre_dependencies" CACHE PATH "Path to ZLIB")
endif()

# ---------------------------------------------------------------------------
# 1. Bullet Physics (bullet3)
# ---------------------------------------------------------------------------
sura_vendor_dependency(BULLET
    REPOSITORY https://github.com/bulletphysics/bullet3.git
    TAG        "${SURA_BULLET_TAG}"
    DIRECTORY  bullet3)

# Ensure Bullet targets have INTERFACE include directories for consumers
if(TARGET BulletDynamics)
    target_include_directories(BulletDynamics INTERFACE "${CMAKE_SOURCE_DIR}/third_party/bullet3/src")
    target_include_directories(BulletCollision INTERFACE "${CMAKE_SOURCE_DIR}/third_party/bullet3/src")
    target_include_directories(LinearMath INTERFACE "${CMAKE_SOURCE_DIR}/third_party/bullet3/src")
    if(TARGET BulletSoftBody)
        target_include_directories(BulletSoftBody INTERFACE "${CMAKE_SOURCE_DIR}/third_party/bullet3/src")
    endif()
endif()

# ---------------------------------------------------------------------------
# 2. LinearMath (Giannossk/LinearMath for PBD)
# ---------------------------------------------------------------------------
# Giannossk/LinearMath is required by PBD. Bullet already defines a target named 'LinearMath'.
# Populate Giannossk/LinearMath headers without invoking its add_subdirectory (which would collide
# on target name 'LinearMath'), and attach its include directory to the LinearMath target.
FetchContent_Declare(
    LinearMath
    GIT_REPOSITORY https://github.com/Giannossk/LinearMath.git
    GIT_TAG        main
    GIT_SHALLOW    FALSE
)
FetchContent_GetProperties(LinearMath)
if(NOT linearmath_POPULATED)
    FetchContent_Populate(LinearMath)
    target_include_directories(LinearMath INTERFACE "${linearmath_SOURCE_DIR}/src")
endif()

# ---------------------------------------------------------------------------
# 3. Position Based Dynamics (PBD)
# ---------------------------------------------------------------------------
sura_vendor_dependency(PBD
    REPOSITORY https://github.com/Giannossk/pbd.git
    TAG        "${SURA_PBD_TAG}"
    DIRECTORY  pbd)

# ---------------------------------------------------------------------------
# 4. Netgen (NGSolve/netgen)
# ---------------------------------------------------------------------------
sura_vendor_dependency(NETGEN
    REPOSITORY https://github.com/NGSolve/netgen.git
    TAG        "${SURA_NETGEN_TAG}"
    DIRECTORY  netgen)

# Ensure Netgen targets have INTERFACE include directories for consumers
if(TARGET nglib)
    target_include_directories(nglib INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/netgen/nglib>"
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/netgen/libsrc>"
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/netgen/libsrc/include>"
        "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/netgen-build>"
    )
endif()
if(TARGET ngcore)
    target_include_directories(ngcore INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/netgen/libsrc>"
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/netgen/libsrc/include>"
        "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/_deps/netgen-build>"
    )
endif()

# ---------------------------------------------------------------------------
# 5. OGRE (Object-Oriented Graphics Rendering Engine)
# ---------------------------------------------------------------------------
sura_vendor_dependency(OGRE
    REPOSITORY https://github.com/OGRECave/ogre.git
    TAG        "${SURA_OGRE_TAG}"
    DIRECTORY  ogre)

# ---------------------------------------------------------------------------
# Status summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "SURA ${PROJECT_VERSION} Dependencies:")
message(STATUS "  OGRE    ${SURA_OGRE_TAG}   -> third_party/ogre")
message(STATUS "  Bullet  ${SURA_BULLET_TAG} -> third_party/bullet3")
message(STATUS "  PBD     ${SURA_PBD_TAG}     -> third_party/pbd")
message(STATUS "  Netgen  ${SURA_NETGEN_TAG}  -> third_party/netgen")
message(STATUS "")
