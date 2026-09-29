#include "test_macros.hpp"
#include "organ/Organ.hpp"
#include <btBulletCollisionCommon.h>

using namespace sura::organ;


void test_organ_full_pipeline() {
    // 1. Create organ surface mesh (ellipsoid modeling an organ such as kidney or bladder)
    SurfaceMesh organSurface = SurfaceMesh::createEllipsoid(Vec3(0.6f, 0.4f, 0.3f), 12, 16);
    TEST_ASSERT(!organSurface.empty());
    size_t originalVertCount = organSurface.vertexCount();

    // 2. Initialize Organ: Meshes with Netgen, builds embedding, exports collision mesh
    Organ organ;
    MesherOptions options;
    options.maxH = 0.25f;
    options.fineness = 0.5f;

    bool initSuccess = organ.initialize(organSurface, options);
    TEST_ASSERT(initSuccess);
    TEST_ASSERT(organ.isInitialized());

    // 3. Verify volumetric tetrahedral mesh was created by Netgen
    const auto& tetMesh = organ.getTetrahedralMesh();
    TEST_ASSERT(!tetMesh.empty());
    TEST_ASSERT(tetMesh.tetCount() > 0);
    TEST_ASSERT(tetMesh.vertexCount() > 0);

    // 4. Verify rendering-to-tetrahedral embedding map was generated
    const auto& embedding = organ.getEmbedding();
    TEST_ASSERT(!embedding.empty());
    TEST_ASSERT(embedding.size() == originalVertCount);

    // 5. Verify triangle collision mesh was exported
    const auto& colMesh = organ.getCollisionMesh();
    TEST_ASSERT(!colMesh.empty());
    TEST_ASSERT(colMesh.triangleCount() > 0);

    // Verify Bullet collision export
    auto btMesh = colMesh.createBulletTriangleMesh();
    TEST_ASSERT(btMesh != nullptr);
    TEST_ASSERT(btMesh->getNumTriangles() == static_cast<int>(colMesh.triangleCount()));

    // 6. Simulate a physical deformation step:
    // Compress along Y by 30% and stretch along X by 20%
    std::vector<Vec3> deformedNodes = tetMesh.vertices;
    for (auto& node : deformedNodes) {
        node.x *= 1.2f;
        node.y *= 0.7f;
    }

    organ.update(deformedNodes, /*recomputeNormals=*/true);

    // Verify that the collision mesh was deformed accordingly
    AABB colAABB = organ.getCollisionMesh().computeAABB();
    AABB origAABB = organSurface.computeAABB();
    TEST_NEAR(colAABB.max.x, origAABB.max.x * 1.2f, 0.1f);
    TEST_NEAR(colAABB.max.y, origAABB.max.y * 0.7f, 0.1f);

    // Verify that the rendering mesh was deformed via the embedding map
    AABB renderAABB = organ.getRenderingMesh().computeAABB();
    TEST_NEAR(renderAABB.max.x, origAABB.max.x * 1.2f, 0.1f);
    TEST_NEAR(renderAABB.max.y, origAABB.max.y * 0.7f, 0.1f);
}
