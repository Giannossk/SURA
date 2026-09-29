#include "Organ.hpp"

#include <iostream>

namespace sura::organ {

bool Organ::loadFromFile(const std::string& filepath, const MesherOptions& options) {
    SurfaceMesh mesh;
    if (!mesh.loadFromFile(filepath)) {
        std::cerr << "[Organ] Failed to load surface mesh from: " << filepath << std::endl;
        return false;
    }
    return initialize(mesh, options);
}

bool Organ::initialize(const SurfaceMesh& renderingMesh, const MesherOptions& options) {
    if (renderingMesh.empty()) {
        std::cerr << "[Organ] Rendering mesh is empty." << std::endl;
        return false;
    }

    mOriginalRenderingMesh = renderingMesh;
    mCurrentRenderingMesh = renderingMesh;

    // Step 1: Build tetrahedral mesh using Netgen
    bool meshed = TetrahedralMesher::generate(mOriginalRenderingMesh, mTetMesh, options);
    if (!meshed || mTetMesh.empty()) {
        std::cerr << "[Organ] Netgen tetrahedral meshing failed." << std::endl;
        return false;
    }

    // Step 2: Build rendering-to-tetrahedral embedding map
    bool mapped = mEmbedding.build(mOriginalRenderingMesh, mTetMesh);
    if (!mapped) {
        std::cerr << "[Organ] Warning: Some rendering vertices were projected onto nearest tets." << std::endl;
    }

    // Step 3: Extract and export collision triangle mesh from tetrahedral boundary
    mCollisionMesh = CollisionMesh::fromTetrahedralMesh(mTetMesh);
    if (mCollisionMesh.empty()) {
        std::cerr << "[Organ] Warning: Extracted collision mesh is empty, falling back to surface." << std::endl;
        mCollisionMesh = CollisionMesh::fromSurfaceMesh(mOriginalRenderingMesh);
    }

    mInitialized = true;
    return true;
}

void Organ::update(const std::vector<Vec3>& newTetPositions, bool recomputeNormals) {
    if (!mInitialized) return;

    if (newTetPositions.size() != mTetMesh.vertices.size()) {
        std::cerr << "[Organ] update() received " << newTetPositions.size()
                  << " positions, expected " << mTetMesh.vertices.size() << std::endl;
        return;
    }

    // 1. Update tetrahedral mesh nodes
    mTetMesh.vertices = newTetPositions;

    // 2. Update collision mesh positions
    mCollisionMesh.updateFromTetVertices(newTetPositions);

    // 3. Update visual rendering mesh via tetrahedral embedding map
    mEmbedding.applyDeformation(newTetPositions, mCurrentRenderingMesh, recomputeNormals);
}

} // namespace sura::organ
