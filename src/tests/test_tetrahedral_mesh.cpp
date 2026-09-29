#include "test_macros.hpp"
#include "organ/TetrahedralMesh.hpp"
#include <cstdio>

using namespace sura::organ;

void test_tetrahedral_mesh_volume_and_barycentric() {
    TetrahedralMesh tetMesh;
    // Unit orthogonal tetrahedron with vertices at (0,0,0), (1,0,0), (0,1,0), (0,0,1)
    tetMesh.vertices = {
        Vec3(0.0f, 0.0f, 0.0f), // 0
        Vec3(1.0f, 0.0f, 0.0f), // 1
        Vec3(0.0f, 1.0f, 0.0f), // 2
        Vec3(0.0f, 0.0f, 1.0f)  // 3
    };
    tetMesh.tetrahedra = {
        Tetrahedron(0, 1, 2, 3)
    };

    // Volume should be 1/6 = 0.166667
    float vol = tetMesh.computeTetVolume(0);
    TEST_NEAR(vol, 1.0f / 6.0f, 1e-5f);
    TEST_NEAR(tetMesh.computeTotalVolume(), 1.0f / 6.0f, 1e-5f);

    // Barycentric coordinates of vertices
    float w[4];
    TEST_ASSERT(tetMesh.computeBarycentricCoordinates(0, Vec3(0.0f, 0.0f, 0.0f), w));
    TEST_NEAR(w[0], 1.0f, 1e-4f);
    TEST_NEAR(w[1], 0.0f, 1e-4f);
    TEST_NEAR(w[2], 0.0f, 1e-4f);
    TEST_NEAR(w[3], 0.0f, 1e-4f);

    TEST_ASSERT(tetMesh.computeBarycentricCoordinates(0, Vec3(1.0f, 0.0f, 0.0f), w));
    TEST_NEAR(w[0], 0.0f, 1e-4f);
    TEST_NEAR(w[1], 1.0f, 1e-4f);
    TEST_NEAR(w[2], 0.0f, 1e-4f);
    TEST_NEAR(w[3], 0.0f, 1e-4f);

    // Barycentric coordinates of centroid (0.25, 0.25, 0.25)
    TEST_ASSERT(tetMesh.computeBarycentricCoordinates(0, Vec3(0.25f, 0.25f, 0.25f), w));
    TEST_NEAR(w[0], 0.25f, 1e-4f);
    TEST_NEAR(w[1], 0.25f, 1e-4f);
    TEST_NEAR(w[2], 0.25f, 1e-4f);
    TEST_NEAR(w[3], 0.25f, 1e-4f);

    // Containment tests
    TEST_ASSERT(tetMesh.containsPoint(0, Vec3(0.1f, 0.1f, 0.1f)));
    TEST_ASSERT(!tetMesh.containsPoint(0, Vec3(0.5f, 0.5f, 0.5f))); // outside (sum of coords > 1)
}

void test_tetrahedral_mesh_boundary_extraction() {
    TetrahedralMesh tetMesh;
    // Cube decomposed into 5 tetrahedra
    tetMesh.vertices = {
        Vec3(-1.0f, -1.0f, -1.0f), // 0
        Vec3( 1.0f, -1.0f, -1.0f), // 1
        Vec3( 1.0f,  1.0f, -1.0f), // 2
        Vec3(-1.0f,  1.0f, -1.0f), // 3
        Vec3(-1.0f, -1.0f,  1.0f), // 4
        Vec3( 1.0f, -1.0f,  1.0f), // 5
        Vec3( 1.0f,  1.0f,  1.0f), // 6
        Vec3(-1.0f,  1.0f,  1.0f)  // 7
    };
    tetMesh.tetrahedra = {
        Tetrahedron(0, 1, 3, 4),
        Tetrahedron(1, 2, 3, 6),
        Tetrahedron(1, 4, 5, 6),
        Tetrahedron(3, 4, 6, 7),
        Tetrahedron(1, 3, 4, 6)
    };

    // Total volume of cube of side 2 is 8.0
    TEST_NEAR(tetMesh.computeTotalVolume(), 8.0f, 1e-4f);

    SurfaceMesh boundary;
    std::vector<uint32_t> boundaryMap;
    tetMesh.extractBoundarySurface(boundary, &boundaryMap);

    // A cube has 6 square faces = 12 boundary triangles
    TEST_ASSERT(boundary.triangleCount() == 12);
    TEST_ASSERT(boundary.vertexCount() == 8);
    TEST_ASSERT(boundaryMap.size() == 8);

    // Verify all outward normals point away from center (0,0,0)
    for (const auto& tri : boundary.triangles) {
        const Vec3& p0 = boundary.vertices[tri[0]];
        const Vec3& p1 = boundary.vertices[tri[1]];
        const Vec3& p2 = boundary.vertices[tri[2]];
        Vec3 fn = (p1 - p0).cross(p2 - p0);
        Vec3 centroid = (p0 + p1 + p2) / 3.0f;
        TEST_ASSERT(fn.dot(centroid) > 0.0f); // points outwards from origin
    }

    // VTK export test
    std::string vtkPath = "test_tet.vtk";
    TEST_ASSERT(tetMesh.saveToVTK(vtkPath));
    std::remove(vtkPath.c_str());
}
