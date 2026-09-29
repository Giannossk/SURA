#include "test_macros.hpp"
#include "organ/CollisionMesh.hpp"
#include "organ/TetrahedralMesh.hpp"

#include <btBulletCollisionCommon.h>

using namespace sura::organ;

void test_collision_mesh_extraction_and_bullet_export() {
    TetrahedralMesh tetMesh;
    // Cube decomposed into 5 tetrahedra
    tetMesh.vertices = {
        Vec3(-1.0f, -1.0f, -1.0f),
        Vec3( 1.0f, -1.0f, -1.0f),
        Vec3( 1.0f,  1.0f, -1.0f),
        Vec3(-1.0f,  1.0f, -1.0f),
        Vec3(-1.0f, -1.0f,  1.0f),
        Vec3( 1.0f, -1.0f,  1.0f),
        Vec3( 1.0f,  1.0f,  1.0f),
        Vec3(-1.0f,  1.0f,  1.0f)
    };
    tetMesh.tetrahedra = {
        Tetrahedron(0, 1, 3, 4),
        Tetrahedron(1, 2, 3, 6),
        Tetrahedron(1, 4, 5, 6),
        Tetrahedron(3, 4, 6, 7),
        Tetrahedron(1, 3, 4, 6)
    };

    CollisionMesh colMesh = CollisionMesh::fromTetrahedralMesh(tetMesh);
    TEST_ASSERT(!colMesh.empty());
    TEST_ASSERT(colMesh.vertexCount() == 8);
    TEST_ASSERT(colMesh.triangleCount() == 12);
    TEST_ASSERT(colMesh.tetVertexIndices.size() == 8);

    // Bullet export
    auto btMesh = colMesh.createBulletTriangleMesh();
    TEST_ASSERT(btMesh != nullptr);
    TEST_ASSERT(btMesh->getNumTriangles() == 12);

    auto bvhShape = CollisionMesh::createBulletBvhShape(btMesh.get());
    TEST_ASSERT(bvhShape != nullptr);

    btVector3 aabbMin, aabbMax;
    btTransform trans;
    trans.setIdentity();
    bvhShape->getAabb(trans, aabbMin, aabbMax);
    TEST_NEAR(aabbMin.x(), -1.0f, 0.05f);
    TEST_NEAR(aabbMax.x(), 1.0f, 0.05f);

    // Test dynamic deformation update
    std::vector<Vec3> deformedNodes = tetMesh.vertices;
    for (auto& v : deformedNodes) {
        v.x *= 2.0f; // Stretch along X
    }

    colMesh.updateFromTetVertices(deformedNodes);
    AABB newBox = colMesh.computeAABB();
    TEST_NEAR(newBox.min.x, -2.0f, 1e-4f);
    TEST_NEAR(newBox.max.x, 2.0f, 1e-4f);
}
