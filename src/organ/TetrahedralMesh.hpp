#pragma once

#include "Types.hpp"
#include "SurfaceMesh.hpp"
#include <vector>
#include <string>

namespace sura::organ {

/**
 * @brief Represents a volumetric 3D tetrahedral mesh for physics simulation.
 * Computes volumes, barycentric coordinates, spatial containment queries,
 * and boundary surface extraction.
 */
class TetrahedralMesh {
public:
    std::vector<Vec3> vertices;
    std::vector<Tetrahedron> tetrahedra;

    TetrahedralMesh() = default;

    size_t vertexCount() const noexcept { return vertices.size(); }
    size_t tetCount() const noexcept { return tetrahedra.size(); }
    bool empty() const noexcept { return vertices.empty() || tetrahedra.empty(); }
    void clear() noexcept;

    // Bounding volume
    AABB computeAABB() const noexcept;

    // Volume computation
    float computeSignedTetVolume(size_t tetIndex) const noexcept;
    float computeTetVolume(size_t tetIndex) const noexcept;
    float computeTotalVolume() const noexcept;

    // Barycentric coordinates
    bool computeBarycentricCoordinates(size_t tetIndex, const Vec3& p, float weights[4]) const noexcept;
    bool containsPoint(size_t tetIndex, const Vec3& p, float eps = 1e-4f) const noexcept;

    // Boundary extraction (extracts watertight surface triangles with outward normals)
    void extractBoundarySurface(SurfaceMesh& outSurfaceMesh,
                                std::vector<uint32_t>* outBoundaryToTetVertexMap = nullptr) const;

    // Spatial indexing & queries
    void buildSpatialIndex();
    int32_t findEnclosingTetrahedron(const Vec3& p, float weights[4], float eps = 1e-4f) const noexcept;
    int32_t findClosestTetrahedron(const Vec3& p, float weights[4]) const noexcept;

    // Export for inspection
    bool saveToVTK(const std::string& filepath) const;

private:
    std::vector<AABB> mTetAABBs;
    bool mIndexBuilt{false};
};

} // namespace sura::organ
