#include "TetrahedralEmbedding.hpp"

#include <algorithm>
#include <numeric>
#include <iostream>

namespace sura::organ {

void TetrahedralEmbedding::projectToSimplex(float w[4]) noexcept {
    // Exact Euclidean projection onto the 3-simplex (sum u_i = 1, u_i >= 0)
    float u[4] = { w[0], w[1], w[2], w[3] };
    std::sort(u, u + 4, std::greater<float>());

    float cumsum = 0.0f;
    float theta = 0.0f;
    for (int i = 0; i < 4; ++i) {
        cumsum += u[i];
        float t = (cumsum - 1.0f) / static_cast<float>(i + 1);
        if (u[i] - t > 0.0f) {
            theta = t;
        }
    }

    float sum = 0.0f;
    for (int i = 0; i < 4; ++i) {
        w[i] = std::max(0.0f, w[i] - theta);
        sum += w[i];
    }

    if (sum > 1e-7f) {
        float inv = 1.0f / sum;
        for (int i = 0; i < 4; ++i) w[i] *= inv;
    } else {
        w[0] = 0.25f; w[1] = 0.25f; w[2] = 0.25f; w[3] = 0.25f;
    }
}

bool TetrahedralEmbedding::build(const SurfaceMesh& renderingMesh, const TetrahedralMesh& tetMesh) {
    if (renderingMesh.empty() || tetMesh.empty()) {
        std::cerr << "[TetrahedralEmbedding] Input mesh is empty." << std::endl;
        return false;
    }

    mEmbeddedVertices.resize(renderingMesh.vertices.size());
    mTetrahedraCache = tetMesh.tetrahedra;

    size_t mappedCount = 0;
    for (size_t i = 0; i < renderingMesh.vertices.size(); ++i) {
        const Vec3& p = renderingMesh.vertices[i];
        float w[4]{0.0f, 0.0f, 0.0f, 0.0f};

        // Try exact/inside search first
        int32_t tetIdx = tetMesh.findEnclosingTetrahedron(p, w, 1e-4f);

        if (tetIdx < 0) {
            // Find closest tet and project weights onto simplex
            tetIdx = tetMesh.findClosestTetrahedron(p, w);
            if (tetIdx >= 0) {
                projectToSimplex(w);
            }
        }

        if (tetIdx >= 0) {
            mEmbeddedVertices[i].tetIndex = tetIdx;
            mEmbeddedVertices[i].weights[0] = w[0];
            mEmbeddedVertices[i].weights[1] = w[1];
            mEmbeddedVertices[i].weights[2] = w[2];
            mEmbeddedVertices[i].weights[3] = w[3];
            mappedCount++;
        } else {
            // Fallback: assign to tet 0 centroid
            mEmbeddedVertices[i].tetIndex = 0;
            mEmbeddedVertices[i].weights[0] = 0.25f;
            mEmbeddedVertices[i].weights[1] = 0.25f;
            mEmbeddedVertices[i].weights[2] = 0.25f;
            mEmbeddedVertices[i].weights[3] = 0.25f;
        }
    }

    return mappedCount == renderingMesh.vertices.size();
}

void TetrahedralEmbedding::applyDeformation(const std::vector<Vec3>& tetVertices,
                                           SurfaceMesh& outRenderingMesh,
                                           bool recomputeNormals) const {
    if (mEmbeddedVertices.size() != outRenderingMesh.vertices.size()) {
        outRenderingMesh.vertices.resize(mEmbeddedVertices.size());
    }

    for (size_t i = 0; i < mEmbeddedVertices.size(); ++i) {
        const auto& ev = mEmbeddedVertices[i];
        if (ev.tetIndex >= 0 && static_cast<size_t>(ev.tetIndex) < mTetrahedraCache.size()) {
            const auto& tet = mTetrahedraCache[ev.tetIndex];
            const Vec3& p0 = tetVertices[tet[0]];
            const Vec3& p1 = tetVertices[tet[1]];
            const Vec3& p2 = tetVertices[tet[2]];
            const Vec3& p3 = tetVertices[tet[3]];

            outRenderingMesh.vertices[i] = p0 * ev.weights[0] +
                                          p1 * ev.weights[1] +
                                          p2 * ev.weights[2] +
                                          p3 * ev.weights[3];
        }
    }

    if (recomputeNormals) {
        outRenderingMesh.computeSmoothVertexNormals();
    }
}

void TetrahedralEmbedding::applyDeformation(const TetrahedralMesh& tetMesh,
                                           SurfaceMesh& outRenderingMesh,
                                           bool recomputeNormals) const {
    applyDeformation(tetMesh.vertices, outRenderingMesh, recomputeNormals);
}

} // namespace sura::organ
