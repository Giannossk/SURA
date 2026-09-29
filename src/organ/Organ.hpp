#pragma once

#include "Types.hpp"
#include "SurfaceMesh.hpp"
#include "TetrahedralMesh.hpp"
#include "TetrahedralMesher.hpp"
#include "TetrahedralEmbedding.hpp"
#include "CollisionMesh.hpp"

#include <string>
#include <vector>
#include <memory>

namespace sura::organ {

/**
 * @brief High-level Organ simulation entity.
 * Orchestrates the full pipeline:
 *   1. Load surface/rendering mesh
 *   2. Build volumetric tetrahedral mesh using Netgen
 *   3. Build rendering-to-tetrahedral embedding map
 *   4. Export triangle collision mesh
 *   5. Real-time deformation synchronization across simulation, collision, and rendering
 */
class Organ {
public:
    Organ() = default;

    /**
     * @brief Executes the pipeline from a surface file (.obj, .stl).
     * @param filepath Path to surface mesh file.
     * @param options Netgen meshing parameters.
     * @return true if loading, meshing, mapping, and collision export succeeded.
     */
    bool loadFromFile(const std::string& filepath, const MesherOptions& options = MesherOptions{});

    /**
     * @brief Executes the pipeline from an existing SurfaceMesh (e.g. procedurally generated).
     * @param renderingMesh Input surface geometry.
     * @param options Netgen meshing parameters.
     * @return true if meshing, mapping, and collision export succeeded.
     */
    bool initialize(const SurfaceMesh& renderingMesh, const MesherOptions& options = MesherOptions{});

    /**
     * @brief Synchronizes deformation when the tetrahedral simulation nodes move.
     * Updates:
     *   - Tetrahedral mesh positions
     *   - Collision mesh positions
     *   - Rendering mesh positions (via barycentric embedding map)
     *   - Smooth rendering normals
     * @param newTetPositions Current simulated positions of all tetrahedral nodes.
     * @param recomputeNormals Whether to recompute normals on the rendering mesh.
     */
    void update(const std::vector<Vec3>& newTetPositions, bool recomputeNormals = true);

    // Mesh accessors
    const SurfaceMesh& getOriginalRenderingMesh() const noexcept { return mOriginalRenderingMesh; }
    const SurfaceMesh& getRenderingMesh() const noexcept { return mCurrentRenderingMesh; }
    SurfaceMesh& getRenderingMesh() noexcept { return mCurrentRenderingMesh; }

    const TetrahedralMesh& getTetrahedralMesh() const noexcept { return mTetMesh; }
    TetrahedralMesh& getTetrahedralMesh() noexcept { return mTetMesh; }

    const TetrahedralEmbedding& getEmbedding() const noexcept { return mEmbedding; }

    const CollisionMesh& getCollisionMesh() const noexcept { return mCollisionMesh; }
    CollisionMesh& getCollisionMesh() noexcept { return mCollisionMesh; }

    bool isInitialized() const noexcept { return mInitialized; }

private:
    SurfaceMesh mOriginalRenderingMesh;
    SurfaceMesh mCurrentRenderingMesh;
    TetrahedralMesh mTetMesh;
    TetrahedralEmbedding mEmbedding;
    CollisionMesh mCollisionMesh;
    bool mInitialized{false};
};

} // namespace sura::organ
