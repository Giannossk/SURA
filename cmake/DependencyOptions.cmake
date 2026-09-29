# DependencyOptions.cmake
#
# Sets every option that must be in the cache *before* vendored dependencies
# are add_subdirectory()'d.
#
# Ordering is load-bearing: the projects we vendor declare their options with
# `set(<X> ... CACHE BOOL ...)` or `option()`, and both respect a pre-existing
# cache entry. Setting these afterwards has no effect.
#
# All assignments use plain CACHE (never FORCE) so a -D on the command line
# still overrides them.

function(sura_apply_dependency_options)

    # -----------------------------------------------------------------------
    # OGRE (Object-Oriented Graphics Rendering Engine)
    # -----------------------------------------------------------------------
    set(OGRE_BUILD_SAMPLES            OFF CACHE BOOL "Build OGRE demo samples")
    set(OGRE_BUILD_TESTS              OFF CACHE BOOL "Build OGRE unit tests")
    set(OGRE_BUILD_TOOLS              OFF CACHE BOOL "Build OGRE command-line tools")
    set(OGRE_BUILD_COMPONENT_PYTHON   OFF CACHE BOOL "Build OGRE Python bindings")
    set(OGRE_BUILD_COMPONENT_JAVA     OFF CACHE BOOL "Build OGRE Java bindings")
    set(OGRE_BUILD_COMPONENT_CSHARP   OFF CACHE BOOL "Build OGRE C# bindings")
    set(OGRE_INSTALL_SAMPLES          OFF CACHE BOOL "")
    set(OGRE_INSTALL_DOCS             OFF CACHE BOOL "")

    set(OGRE_BUILD_DEPENDENCIES       ON  CACHE BOOL "Automatically build OGRE dependencies (SDL2, pugixml)")
    # If cached dependencies exist in third_party/ogre_dependencies, point to them
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/ogre_dependencies")
        set(OGRE_DEPENDENCIES_DIR "${CMAKE_SOURCE_DIR}/third_party/ogre_dependencies" CACHE PATH "Path to prebuilt OGRE dependencies")
    endif()

    set(OGRE_BUILD_RENDERSYSTEM_D3D11 ON  CACHE BOOL "Build Direct3D11 RenderSystem")
    set(OGRE_BUILD_RENDERSYSTEM_GL    OFF CACHE BOOL "Disable legacy OpenGL")
    set(OGRE_BUILD_RENDERSYSTEM_GL3PLUS OFF CACHE BOOL "Disable OpenGL 3+ on Windows")
    set(OGRE_BUILD_RENDERSYSTEM_GLES2 OFF CACHE BOOL "Disable OpenGL ES")

    set(OGRE_BUILD_COMPONENT_BITES    ON  CACHE BOOL "Build OgreBites component (windowing, input, app context)")
    set(OGRE_BUILD_COMPONENT_RTSHADERSYSTEM ON CACHE BOOL "Build RTShader System component")
    set(OGRE_BUILD_COMPONENT_OVERLAY  ON  CACHE BOOL "Build Overlay component")
    set(OGRE_BUILD_COMPONENT_BULLET   ON  CACHE BOOL "Build OGRE Bullet physics component")

    set(OGRE_BUILD_PLUGIN_OCTREE      ON  CACHE BOOL "Build Octree SceneManager plugin")
    set(OGRE_BUILD_PLUGIN_STBI        ON  CACHE BOOL "Build STBI generic image codec")
    set(OGRE_BUILD_PLUGIN_PFX         OFF CACHE BOOL "Disable ParticleFX plugin")
    set(OGRE_BUILD_PLUGIN_BSP         OFF CACHE BOOL "Disable BSP SceneManager plugin")
    set(OGRE_BUILD_PLUGIN_PCZ         OFF CACHE BOOL "Disable PCZ SceneManager plugin")
    set(OGRE_BUILD_PLUGIN_ASSIMP      OFF CACHE BOOL "Disable Assimp plugin")
    set(OGRE_BUILD_PLUGIN_DOT_SCENE   OFF CACHE BOOL "Disable DotScene plugin")

    # -----------------------------------------------------------------------
    # Bullet Physics (bullet3)
    # -----------------------------------------------------------------------
    set(USE_MSVC_RUNTIME_LIBRARY_DLL  ON  CACHE BOOL "Use MSVC Runtime Library DLL (/MD or /MDd)")
    set(USE_MSVC_DISABLE_RTTI         OFF CACHE BOOL "Do not disable RTTI")
    set(BUILD_CPU_DEMOS               OFF CACHE BOOL "Build original Bullet CPU examples")
    set(BUILD_BULLET2_DEMOS           OFF CACHE BOOL "Build Bullet 2 demos")
    set(BUILD_OPENGL3_DEMOS           OFF CACHE BOOL "Build Bullet 3 OpenGL3+ demos")
    set(BUILD_EXTRAS                  OFF CACHE BOOL "Build Bullet extras")
    set(BUILD_UNIT_TESTS              OFF CACHE BOOL "Build Bullet unit tests")
    set(BUILD_CLSOCKET                OFF CACHE BOOL "Build clsocket")
    set(BUILD_ENET                    OFF CACHE BOOL "Build enet")
    set(BUILD_PYBULLET                OFF CACHE BOOL "Build pybullet")
    set(INSTALL_LIBS                  OFF CACHE BOOL "Install Bullet libraries")

    # -----------------------------------------------------------------------
    # PBD (Position Based Dynamics)
    # -----------------------------------------------------------------------
    set(PBD_BUILD_TESTING             OFF CACHE BOOL "Build PBD tests")
    set(BUILD_TESTING                 OFF CACHE BOOL "Build tests")

    # -----------------------------------------------------------------------
    # Netgen (Automatic 3D tetrahedral mesh generator)
    # -----------------------------------------------------------------------
    set(USE_SUPERBUILD                OFF CACHE BOOL "Build Netgen dependencies automatically")
    set(USE_GUI                       OFF CACHE BOOL "Build Netgen GUI")
    set(USE_PYTHON                    OFF CACHE BOOL "Build Netgen Python bindings")
    set(USE_OCC                       OFF CACHE BOOL "Build Netgen with OpenCascade")
    set(USE_MPI                       OFF CACHE BOOL "Enable Netgen MPI")
    set(USE_JPEG                      OFF CACHE BOOL "Enable Netgen JPEG")
    set(USE_MPEG                      OFF CACHE BOOL "Enable Netgen MPEG")
    set(USE_CGNS                      OFF CACHE BOOL "Enable Netgen CGNS")
    set(ENABLE_UNIT_TESTS             OFF CACHE BOOL "Enable Netgen Catch unit tests")
    set(BUILD_STUB_FILES              OFF CACHE BOOL "Build Netgen stub files")
    set(BUILD_FOR_CONDA               OFF CACHE BOOL "Link python libraries only to executables")
    set(INSTALL_PROFILES              OFF CACHE BOOL "Install Netgen profiles")

endfunction()
