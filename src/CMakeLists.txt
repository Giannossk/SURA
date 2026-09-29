# --- Organ Simulation & Meshing Library -------------------------------------
add_library(sura_organ STATIC
    organ/SurfaceMesh.cpp
    organ/TetrahedralMesh.cpp
    organ/TetrahedralMesher.cpp
    organ/TetrahedralEmbedding.cpp
    organ/CollisionMesh.cpp
    organ/Organ.cpp)

target_compile_features(sura_organ PUBLIC cxx_std_23)

set_target_properties(sura_organ PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF)

target_include_directories(sura_organ PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/organ)

target_link_libraries(sura_organ PUBLIC
    BulletCollision
    LinearMath
    nglib)

if(MSVC)
    target_compile_options(sura_organ PRIVATE /permissive- /W4 /utf-8 /wd4251 /wd4100 /wd4275)
else()
    target_compile_options(sura_organ PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Application.
#
# C++23 is set here, per-target, and deliberately not globally -- OGRE
# pins its internal requirements. See AGENTS.md section 4.3.

add_executable(SURA
    app/main.cpp)

target_compile_features(SURA PRIVATE cxx_std_23)

set_target_properties(SURA PROPERTIES
    CXX_STANDARD          23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS        OFF)

target_link_libraries(SURA PRIVATE
    OgreMain
    OgreBites
    OgreRTShaderSystem
    OgreBullet
    BulletDynamics
    BulletCollision
    LinearMath
    pbd
    nglib
    sura_organ)

# Ensure runtime plugins are built along with the application
add_dependencies(SURA
    RenderSystem_Direct3D11
    Codec_STBI
    Plugin_OctreeSceneManager)

# The physics engine gets linked here once it is vendored. Keep it PRIVATE and
# keep its headers out of our public surface -- see AGENTS.md section 2.

if(MSVC)
    target_compile_options(SURA PRIVATE /permissive- /W4 /utf-8 /wd4251 /wd4100 /wd4275)
else()
    target_compile_options(SURA PRIVATE -Wall -Wextra -Wpedantic)
endif()


# --- Assets -----------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../assets")
    add_custom_command(TARGET SURA POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/../assets"
                "$<TARGET_FILE_DIR:SURA>/assets"
        COMMENT "Copying assets next to the binary")
endif()

# --- Copy OGRE and dependency runtime binaries, resources, and write active plugins.cfg ---
add_custom_command(TARGET SURA POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_FILE_DIR:OgreMain>"
            "$<TARGET_FILE_DIR:SURA>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:nglib>"
            "$<TARGET_FILE_DIR:SURA>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:ngcore>"
            "$<TARGET_FILE_DIR:SURA>"
    COMMAND ${CMAKE_COMMAND} -E echo "# Defines plugins to load for SURA" > "$<TARGET_FILE_DIR:SURA>/plugins.cfg"
    COMMAND ${CMAKE_COMMAND} -E echo "PluginFolder=." >> "$<TARGET_FILE_DIR:SURA>/plugins.cfg"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin=RenderSystem_Direct3D11" >> "$<TARGET_FILE_DIR:SURA>/plugins.cfg"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin=Codec_STBI" >> "$<TARGET_FILE_DIR:SURA>/plugins.cfg"
    COMMAND ${CMAKE_COMMAND} -E echo "Plugin=Plugin_OctreeSceneManager" >> "$<TARGET_FILE_DIR:SURA>/plugins.cfg"
    COMMENT "Copying runtime binaries and writing active plugins.cfg next to SURA"
)

# --- Run target -------------------------------------------------------------
# `cmake --build build --target run` builds and launches.
add_custom_target(run
    DEPENDS SURA
    COMMAND $<TARGET_FILE:SURA>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:SURA>
    USES_TERMINAL)

# --- Test Target ------------------------------------------------------------
if(SURA_BUILD_TESTS)
    add_executable(sura_tests
        tests/test_main.cpp
        tests/test_surface_mesh.cpp
        tests/test_tetrahedral_mesh.cpp
        tests/test_netgen_mesher.cpp
        tests/test_embedding.cpp
        tests/test_collision_mesh.cpp
        tests/test_organ_pipeline.cpp)

    target_compile_features(sura_tests PRIVATE cxx_std_23)

    set_target_properties(sura_tests PROPERTIES
        CXX_STANDARD          23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS        OFF)

    target_include_directories(sura_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/organ
        ${CMAKE_CURRENT_SOURCE_DIR}/tests)

    target_link_libraries(sura_tests PRIVATE
        sura_organ)

    if(MSVC)
        target_compile_options(sura_tests PRIVATE /permissive- /W4 /utf-8 /wd4251 /wd4100 /wd4275)
    else()
        target_compile_options(sura_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_custom_command(TARGET sura_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:nglib>"
                "$<TARGET_FILE_DIR:sura_tests>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:ngcore>"
                "$<TARGET_FILE_DIR:sura_tests>"
        COMMENT "Copying Netgen runtime binaries next to sura_tests"
    )

    add_test(NAME sura_unit_tests COMMAND sura_tests)

    add_custom_target(run_tests
        DEPENDS sura_tests
        COMMAND $<TARGET_FILE:sura_tests>
        WORKING_DIRECTORY $<TARGET_FILE_DIR:sura_tests>
        USES_TERMINAL)
endif()

