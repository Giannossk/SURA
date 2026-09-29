#include "test_macros.hpp"
#include "organ/SurfaceMesh.hpp"
#include <cstdio>

using namespace sura::organ;

void test_surface_mesh_primitives() {
    // 1. Cube
    auto cube = SurfaceMesh::createCube(Vec3(2.0f, 2.0f, 2.0f));
    TEST_ASSERT(cube.vertexCount() == 8);
    TEST_ASSERT(cube.triangleCount() == 12);
    TEST_ASSERT(cube.normals.size() == 8);

    AABB cubeBox = cube.computeAABB();
    TEST_NEAR(cubeBox.min.x, -1.0f, 1e-4f);
    TEST_NEAR(cubeBox.max.x, 1.0f, 1e-4f);

    // 2. Sphere
    auto sphere = SurfaceMesh::createSphere(1.0f, 16, 16);
    TEST_ASSERT(!sphere.empty());
    TEST_ASSERT(sphere.vertexCount() > 0);
    TEST_ASSERT(sphere.triangleCount() > 0);

    AABB sphereBox = sphere.computeAABB();
    TEST_NEAR(sphereBox.min.x, -1.0f, 0.1f);
    TEST_NEAR(sphereBox.max.x, 1.0f, 0.1f);

    // 3. Ellipsoid
    auto ellipsoid = SurfaceMesh::createEllipsoid(Vec3(0.5f, 0.3f, 0.2f), 12, 12);
    TEST_ASSERT(!ellipsoid.empty());
    AABB ellBox = ellipsoid.computeAABB();
    TEST_NEAR(ellBox.max.x, 0.5f, 0.05f);
    TEST_NEAR(ellBox.max.y, 0.3f, 0.05f);
    TEST_NEAR(ellBox.max.z, 0.2f, 0.05f);
}

void test_surface_mesh_obj_io() {
    auto original = SurfaceMesh::createCube(Vec3(1.0f, 1.0f, 1.0f));
    std::string path = "test_cube_roundtrip.obj";

    TEST_ASSERT(original.saveToOBJ(path));

    SurfaceMesh loaded;
    TEST_ASSERT(loaded.loadFromOBJ(path));
    TEST_ASSERT(loaded.vertexCount() == original.vertexCount());
    TEST_ASSERT(loaded.triangleCount() == original.triangleCount());

    AABB origBox = original.computeAABB();
    AABB loadedBox = loaded.computeAABB();
    TEST_VEC3_NEAR(origBox.min, loadedBox.min, 1e-4f);
    TEST_VEC3_NEAR(origBox.max, loadedBox.max, 1e-4f);

    std::remove(path.c_str());
}

void test_surface_mesh_stl_io() {
    auto original = SurfaceMesh::createCube(Vec3(1.0f, 1.0f, 1.0f));
    std::string path = "test_cube_roundtrip.stl";

    // Binary STL
    TEST_ASSERT(original.saveToSTL(path, true));

    SurfaceMesh loaded;
    TEST_ASSERT(loaded.loadFromSTL(path));
    TEST_ASSERT(loaded.triangleCount() == original.triangleCount());

    std::remove(path.c_str());

    // ASCII STL
    TEST_ASSERT(original.saveToSTL(path, false));
    SurfaceMesh loadedAscii;
    TEST_ASSERT(loadedAscii.loadFromSTL(path));
    TEST_ASSERT(loadedAscii.triangleCount() == original.triangleCount());

    std::remove(path.c_str());
}
