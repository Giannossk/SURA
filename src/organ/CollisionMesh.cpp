#include "CollisionMesh.hpp"

#include <btBulletCollisionCommon.h>
#include <iostream>

namespace sura::organ {

void CollisionMesh::clear() noexcept {
    vertices.clear();
    triangles.clear();
    tetVertexIndices.clear();
}

AABB CollisionMesh::computeAABB() const noexcept {
    AABB box;
    for (const auto& v : vertices) {
        box.expand(v);
    }
    return box;
}

CollisionMesh CollisionMesh::fromTetrahedralMesh(const TetrahedralMesh& tetMesh) {
    CollisionMesh colMesh;
    SurfaceMesh boundary;
    tetMesh.extractBoundarySurface(boundary, &colMesh.tetVertexIndices);

    colMesh.vertices = std::move(boundary.vertices);
    colMesh.triangles = std::move(boundary.triangles);

    return colMesh;
}

CollisionMesh CollisionMesh::fromSurfaceMesh(const SurfaceMesh& surfaceMesh) {
    CollisionMesh colMesh;
    colMesh.vertices = surfaceMesh.vertices;
    colMesh.triangles = surfaceMesh.triangles;
    colMesh.tetVertexIndices.resize(colMesh.vertices.size());
    for (uint32_t i = 0; i < colMesh.vertices.size(); ++i) {
        colMesh.tetVertexIndices[i] = i;
    }
    return colMesh;
}

void CollisionMesh::updateFromTetVertices(const std::vector<Vec3>& tetVertices) {
    if (tetVertexIndices.empty()) {
        return;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        uint32_t tetNode = tetVertexIndices[i];
        if (tetNode < tetVertices.size()) {
            vertices[i] = tetVertices[tetNode];
        }
    }
}

void CollisionMesh::exportToBulletTriangleMesh(btTriangleMesh& outBtMesh) const {
    for (const auto& tri : triangles) {
        if (tri[0] < vertices.size() && tri[1] < vertices.size() && tri[2] < vertices.size()) {
            const Vec3& p0 = vertices[tri[0]];
            const Vec3& p1 = vertices[tri[1]];
            const Vec3& p2 = vertices[tri[2]];

            outBtMesh.addTriangle(
                btVector3(p0.x, p0.y, p0.z),
                btVector3(p1.x, p1.y, p1.z),
                btVector3(p2.x, p2.y, p2.z),
                false // do not remove duplicates
            );
        }
    }
}

std::unique_ptr<btTriangleMesh> CollisionMesh::createBulletTriangleMesh() const {
    auto btMesh = std::make_unique<btTriangleMesh>(true, false);
    exportToBulletTriangleMesh(*btMesh);
    return btMesh;
}

std::unique_ptr<btBvhTriangleMeshShape> CollisionMesh::createBulletBvhShape(
    btTriangleMesh* btMesh,
    bool useQuantizedAabbCompression) {
    if (!btMesh) {
        return nullptr;
    }
    return std::make_unique<btBvhTriangleMeshShape>(btMesh, useQuantizedAabbCompression);
}

} // namespace sura::organ
