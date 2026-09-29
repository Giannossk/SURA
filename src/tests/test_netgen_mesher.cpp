#include "test_macros.hpp"
#include "organ/TetrahedralMesher.hpp"
#include "organ/SurfaceMesh.hpp"

using namespace sura::organ;

void test_netgen_mesher_cube() {
    SurfaceMesh cube = SurfaceMesh::createCube(Vec3(1.0f, 1.0f, 1.0f));
    TetrahedralMesh tetMesh;

    MesherOptions options;
    options.maxH = 0.5f;
    options.fineness = 0.5f;

    bool success = TetrahedralMesher::generate(cube, tetMesh, options);
    TEST_ASSERT(success);
    TEST_ASSERT(!tetMesh.empty());
    TEST_ASSERT(tetMesh.tetCount() > 0);
    TEST_ASSERT(tetMesh.vertexCount() >= 8);

    // Total volume of cube of side 1 should be ~1.0
    float totalVol = tetMesh.computeTotalVolume();
    TEST_NEAR(totalVol, 1.0f, 0.05f);

    // Check that all tets have valid indices and positive volume
    for (size_t i = 0; i < tetMesh.tetCount(); ++i) {
        const auto& tet = tetMesh.tetrahedra[i];
        for (int k = 0; k < 4; ++k) {
            TEST_ASSERT(tet[k] < tetMesh.vertexCount());
        }
        float v = tetMesh.computeTetVolume(i);
        TEST_ASSERT(v > 1e-7f);
    }
}

void test_netgen_mesher_sphere() {
    SurfaceMesh sphere = SurfaceMesh::createSphere(0.5f, 12, 16);
    TetrahedralMesh tetMesh;

    MesherOptions options;
    options.maxH = 0.3f;
    options.fineness = 0.5f;

    bool success = TetrahedralMesher::generate(sphere, tetMesh, options);
    TEST_ASSERT(success);
    TEST_ASSERT(tetMesh.tetCount() > 0);

    // Volume of sphere of radius 0.5: 4/3 * pi * r^3 = 4/3 * 3.14159 * 0.125 = ~0.5236
    float totalVol = tetMesh.computeTotalVolume();
    TEST_NEAR(totalVol, 0.5236f, 0.15f);
}
