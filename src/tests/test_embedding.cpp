#include "test_macros.hpp"
#include "organ/TetrahedralEmbedding.hpp"
#include "organ/TetrahedralMesh.hpp"
#include "organ/SurfaceMesh.hpp"

using namespace sura::organ;

void test_simplex_projection() {
    float w[4] = { -0.5f, 0.8f, 0.4f, 0.3f };
    TetrahedralEmbedding::projectToSimplex(w);

    float sum = w[0] + w[1] + w[2] + w[3];
    TEST_NEAR(sum, 1.0f, 1e-5f);
    for (int i = 0; i < 4; ++i) {
        TEST_ASSERT(w[i] >= 0.0f);
    }
}

void test_embedding_and_deformation() {
    // Single tetrahedron
    TetrahedralMesh tetMesh;
    tetMesh.vertices = {
        Vec3(0.0f, 0.0f, 0.0f),
        Vec3(1.0f, 0.0f, 0.0f),
        Vec3(0.0f, 1.0f, 0.0f),
        Vec3(0.0f, 0.0f, 1.0f)
    };
    tetMesh.tetrahedra = {
        Tetrahedron(0, 1, 2, 3)
    };
    tetMesh.buildSpatialIndex();

    // Rendering mesh with points inside the tet
    SurfaceMesh renderMesh;
    renderMesh.vertices = {
        Vec3(0.1f, 0.1f, 0.1f),
        Vec3(0.2f, 0.2f, 0.2f),
        Vec3(0.25f, 0.25f, 0.25f) // centroid
    };
    renderMesh.triangles = {
        Triangle(0, 1, 2)
    };

    TetrahedralEmbedding embedding;
    bool success = embedding.build(renderMesh, tetMesh);
    TEST_ASSERT(success);
    TEST_ASSERT(embedding.size() == 3);

    // Centroid vertex (index 2) should have weights ~ (0.25, 0.25, 0.25, 0.25)
    const auto& evCentroid = embedding.getEmbeddedVertices()[2];
    TEST_ASSERT(evCentroid.tetIndex == 0);
    for (int k = 0; k < 4; ++k) {
        TEST_NEAR(evCentroid.weights[k], 0.25f, 1e-4f);
    }

    // Now deform the tetrahedral mesh:
    // Translate by (5, 5, 5) and scale by 2.0
    std::vector<Vec3> deformedTetNodes = tetMesh.vertices;
    for (auto& v : deformedTetNodes) {
        v = (v * 2.0f) + Vec3(5.0f, 5.0f, 5.0f);
    }

    SurfaceMesh deformedRenderMesh = renderMesh;
    embedding.applyDeformation(deformedTetNodes, deformedRenderMesh);

    // Verify deformed vertex 2: original was (0.25, 0.25, 0.25) -> should now be (0.25 * 2 + 5) = 5.5
    TEST_VEC3_NEAR(deformedRenderMesh.vertices[2], Vec3(5.5f, 5.5f, 5.5f), 1e-4f);

    // Verify vertex 0: original was (0.1, 0.1, 0.1) -> should now be (0.1 * 2 + 5) = 5.2
    TEST_VEC3_NEAR(deformedRenderMesh.vertices[0], Vec3(5.2f, 5.2f, 5.2f), 1e-4f);
}
