#pragma once

#include "Types.hpp"
#include "SurfaceMesh.hpp"
#include "TetrahedralMesh.hpp"

#include <vector>
#include <memory>

class btTriangleMesh;
class btBvhTriangleMeshShape;

namespace sura::organ {

/**
 * @brief Triangle collision representation for organs.
 * Extracts surface collision triangles from the volumetric tetrahedral mesh,
 * maintains 1:1 mapping with simulation nodes for instant updates,
 * and exports to Bullet Physics triangle meshes (btTriangleMesh, btBvhTriangleMeshShape).
 */
class CollisionMesh {
public:
    std::vector<Vec3> vertices;
    std::vector<Triangle> triangles;
    std::vector<uint32_t> tetVertexIndices; // Maps each collision vertex to its tet node

    CollisionMesh() = default;

    size_t vertexCount() const noexcept { return vertices.size(); }
    size_t triangleCount() const noexcept { return triangles.size(); }
    size_t indexCount() const noexcept { return triangles.size() * 3; }
    bool empty() const noexcept { return vertices.empty() || triangles.empty(); }
    void clear() noexcept;

    const Vec3* getVertexBuffer() const noexcept { return vertices.data(); }
    const uint32_t* getIndexBuffer() const noexcept {
        return reinterpret_cast<const uint32_t*>(triangles.data());
    }

    AABB computeAABB() const noexcept;

    /**
     * @brief Extracts a watertight collision triangle mesh directly from a TetrahedralMesh's boundary.
     * Sets up tetVertexIndices for instant runtime updates during physics simulation.
     */
    static CollisionMesh fromTetrahedralMesh(const TetrahedralMesh& tetMesh);

    /**
     * @brief Creates a collision mesh from a surface mesh.
     */
    static CollisionMesh fromSurfaceMesh(const SurfaceMesh& surfaceMesh);

    /**
     * @brief Updates collision vertex positions in O(V) time from simulated tetrahedral nodes.
     * @param tetVertices Current positions of the tetrahedral nodes.
     */
    void updateFromTetVertices(const std::vector<Vec3>& tetVertices);

    /**
     * @brief Exports geometry to a Bullet btTriangleMesh.
     * @param outBtMesh The Bullet triangle mesh to populate.
     */
    void exportToBulletTriangleMesh(btTriangleMesh& outBtMesh) const;

    /**
     * @brief Creates a dynamically allocated btTriangleMesh.
     */
    std::unique_ptr<btTriangleMesh> createBulletTriangleMesh() const;

    /**
     * @brief Creates a Bullet btBvhTriangleMeshShape suitable for collision queries.
     * @param btMesh Owner of the triangle data (must outlive the shape).
     * @param useQuantizedAabbCompression Enable AABB quantization.
     */
    static std::unique_ptr<btBvhTriangleMeshShape> createBulletBvhShape(
        btTriangleMesh* btMesh,
        bool useQuantizedAabbCompression = true);
};

} // namespace sura::organ
