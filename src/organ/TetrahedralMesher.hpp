#pragma once

#include "Types.hpp"
#include "SurfaceMesh.hpp"
#include "TetrahedralMesh.hpp"

namespace sura::organ {

/**
 * @brief Options controlling Netgen tetrahedral mesh generation.
 */
struct MesherOptions {
    float maxH{0.0f};          //!< Max element size. 0 => auto-computed based on bounding box.
    float minH{0.0f};          //!< Min element size.
    float fineness{0.5f};      //!< Mesh fineness 0..1 (0 coarse, 1 fine).
    float grading{0.3f};       //!< Mesh grading 0..1 (0 uniform, 1 aggressive local grading).
    int optSteps3D{3};         //!< 3D volume optimization iterations.
    bool secondOrder{false};   //!< Second order tets (10 nodes) vs linear (4 nodes).
    bool useSTLMode{false};    //!< Use STL pipeline instead of direct surface element mode.
};

/**
 * @brief Integrates Netgen (nglib) to generate volumetric tetrahedral meshes
 * from 3D surface meshes (load -> build tetrahedral mesh pipeline).
 */
class TetrahedralMesher {
public:
    /**
     * @brief Generates a TetrahedralMesh from an input SurfaceMesh using Netgen.
     * @param surfaceMesh Input surface geometry (e.g. organ surface).
     * @param outTetMesh Generated volumetric tetrahedral mesh.
     * @param options Meshing parameters.
     * @return true if meshing succeeded with at least one tetrahedron generated.
     */
    static bool generate(const SurfaceMesh& surfaceMesh,
                         TetrahedralMesh& outTetMesh,
                         const MesherOptions& options = MesherOptions{});

private:
    static void ensureInitialized();
    static bool meshDirect(const SurfaceMesh& surfaceMesh,
                           TetrahedralMesh& outTetMesh,
                           const MesherOptions& options,
                           double effectiveMaxH);
    static bool meshSTL(const SurfaceMesh& surfaceMesh,
                        TetrahedralMesh& outTetMesh,
                        const MesherOptions& options,
                        double effectiveMaxH);
};

} // namespace sura::organ
