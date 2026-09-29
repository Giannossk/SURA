cmake_minimum_required(VERSION 3.22)

project(SURA
    VERSION      0.1.0
    DESCRIPTION  "XR simulation application built on OGRE"
    HOMEPAGE_URL "https://github.com/Giannossk/SURA"
    LANGUAGES    CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
# Deliberately NOT setting CMAKE_CXX_STANDARD globally. Dependencies pin their
# own requirements; our targets are C++23, set per-target in src/CMakeLists.txt.
# A global setting leaks into dependencies and can produce mixed-standard builds.

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
include(SuraDependencies)

# ---------------------------------------------------------------------------
# Application
# ---------------------------------------------------------------------------
option(SURA_BUILD_APP "Build the application" ON)
option(SURA_BUILD_TESTS "Build test suite" ON)
if(SURA_BUILD_TESTS)
    enable_testing()
endif()

if(SURA_BUILD_APP)
    add_subdirectory(src)
endif()

