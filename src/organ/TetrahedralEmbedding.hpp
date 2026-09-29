#pragma once

#include "Types.hpp"
#include "SurfaceMesh.hpp"
#include "TetrahedralMesh.hpp"
#include <vector>

namespace sura::organ {

/**
 * @brief Represents the barycentric embedding of a surface rendering mesh vertex
 * inside a volumetric tetrahedron.
 */
struct EmbeddedVertex {
    int32_t tetIndex{-1};
    float weights[4]{0.0f, 0.0f, 0.0f, 0.0f};

    bool valid() const noexcept { return tetIndex >= 0; }
};

/**
 * @brief Manages the mapping from a surface rendering mesh to a volumetric tetrahedral mesh.
 * Given simulated deformations of the tetrahedral nodes, rapidly evaluates the deformed
 * rendering mesh positions (and normals) for high-performance visual display.
 */
class TetrahedralEmbedding {
public:
    TetrahedralEmbedding() = default;

    /**
     * @brief Builds the embedding map by locating each rendering vertex in the tetrahedral mesh
     * and computing barycentric coordinates.
     * @param renderingMesh Visual surface mesh to embed.
     * @param tetMesh Volumetric simulation mesh.
     * @return true if all rendering vertices were successfully mapped.
     */
    bool build(const SurfaceMesh& renderingMesh, const TetrahedralMesh& tetMesh);

    /**
     * @brief Deforms the surface rendering mesh based on current tetrahedral vertex positions.
     * @param tetVertices Current positions of the tetrahedral mesh nodes.
     * @param outRenderingMesh The rendering mesh whose vertices and normals will be updated.
     * @param recomputeNormals Whether to recompute smooth surface normals after updating positions.
     */
    void applyDeformation(const std::vector<Vec3>& tetVertices,
                          SurfaceMesh& outRenderingMesh,
                          bool recomputeNormals = true) const;

    void applyDeformation(const TetrahedralMesh& tetMesh,
                          SurfaceMesh& outRenderingMesh,
                          bool recomputeNormals = true) const;

    const std::vector<EmbeddedVertex>& getEmbeddedVertices() const noexcept { return mEmbeddedVertices; }
    size_t size() const noexcept { return mEmbeddedVertices.size(); }
    bool empty() const noexcept { return mEmbeddedVertices.empty(); }
    void clear() noexcept { mEmbeddedVertices.clear(); }

    /**
     * @brief Projects arbitrary 4D weights onto the probability simplex (w_i >= 0, sum w_i = 1).
     */
    static void projectToSimplex(float w[4]) noexcept;

private:
    std::vector<EmbeddedVertex> mEmbeddedVertices;
    std::vector<Tetrahedron> mTetrahedraCache; // Cache tet indices for fast lookups
};

} // namespace sura::organ
