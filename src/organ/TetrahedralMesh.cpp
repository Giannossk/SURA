#include "TetrahedralMesh.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>

namespace sura::organ {

namespace {

// Key for unordered face hashing
struct FaceKey {
    uint32_t v[3];

    FaceKey(uint32_t a, uint32_t b, uint32_t c) noexcept {
        v[0] = a; v[1] = b; v[2] = c;
        if (v[0] > v[1]) std::swap(v[0], v[1]);
        if (v[1] > v[2]) std::swap(v[1], v[2]);
        if (v[0] > v[1]) std::swap(v[0], v[1]);
    }

    bool operator==(const FaceKey& o) const noexcept {
        return v[0] == o.v[0] && v[1] == o.v[1] && v[2] == o.v[2];
    }
};

struct FaceKeyHash {
    size_t operator()(const FaceKey& k) const noexcept {
        size_t h1 = std::hash<uint32_t>{}(k.v[0]);
        size_t h2 = std::hash<uint32_t>{}(k.v[1]);
        size_t h3 = std::hash<uint32_t>{}(k.v[2]);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

} // namespace

void TetrahedralMesh::clear() noexcept {
    vertices.clear();
    tetrahedra.clear();
    mTetAABBs.clear();
    mIndexBuilt = false;
}

AABB TetrahedralMesh::computeAABB() const noexcept {
    AABB box;
    for (const auto& v : vertices) {
        box.expand(v);
    }
    return box;
}

float TetrahedralMesh::computeSignedTetVolume(size_t tetIndex) const noexcept {
    if (tetIndex >= tetrahedra.size()) return 0.0f;
    const auto& tet = tetrahedra[tetIndex];
    const Vec3& p0 = vertices[tet[0]];
    const Vec3& p1 = vertices[tet[1]];
    const Vec3& p2 = vertices[tet[2]];
    const Vec3& p3 = vertices[tet[3]];

    Vec3 v0 = p1 - p0;
    Vec3 v1 = p2 - p0;
    Vec3 v2 = p3 - p0;

    return (v0.cross(v1).dot(v2)) / 6.0f;
}

float TetrahedralMesh::computeTetVolume(size_t tetIndex) const noexcept {
    return std::abs(computeSignedTetVolume(tetIndex));
}

float TetrahedralMesh::computeTotalVolume() const noexcept {
    float total = 0.0f;
    for (size_t i = 0; i < tetrahedra.size(); ++i) {
        total += computeTetVolume(i);
    }
    return total;
}

bool TetrahedralMesh::computeBarycentricCoordinates(size_t tetIndex, const Vec3& p, float weights[4]) const noexcept {
    if (tetIndex >= tetrahedra.size()) return false;
    const auto& tet = tetrahedra[tetIndex];
    const Vec3& p0 = vertices[tet[0]];
    const Vec3& p1 = vertices[tet[1]];
    const Vec3& p2 = vertices[tet[2]];
    const Vec3& p3 = vertices[tet[3]];

    Vec3 v0 = p1 - p0;
    Vec3 v1 = p2 - p0;
    Vec3 v2 = p3 - p0;
    Vec3 r  = p - p0;

    float det = v0.cross(v1).dot(v2);
    if (std::abs(det) < 1e-12f) {
        return false;
    }

    float invDet = 1.0f / det;
    weights[1] = r.cross(v1).dot(v2) * invDet;
    weights[2] = v0.cross(r).dot(v2) * invDet;
    weights[3] = v0.cross(v1).dot(r) * invDet;
    weights[0] = 1.0f - weights[1] - weights[2] - weights[3];

    return true;
}

bool TetrahedralMesh::containsPoint(size_t tetIndex, const Vec3& p, float eps) const noexcept {
    float w[4];
    if (!computeBarycentricCoordinates(tetIndex, p, w)) return false;
    return (w[0] >= -eps && w[1] >= -eps && w[2] >= -eps && w[3] >= -eps);
}

void TetrahedralMesh::buildSpatialIndex() {
    mTetAABBs.resize(tetrahedra.size());
    for (size_t i = 0; i < tetrahedra.size(); ++i) {
        const auto& tet = tetrahedra[i];
        AABB box;
        box.expand(vertices[tet[0]]);
        box.expand(vertices[tet[1]]);
        box.expand(vertices[tet[2]]);
        box.expand(vertices[tet[3]]);
        mTetAABBs[i] = box;
    }
    mIndexBuilt = true;
}

int32_t TetrahedralMesh::findEnclosingTetrahedron(const Vec3& p, float weights[4], float eps) const noexcept {
    for (size_t i = 0; i < tetrahedra.size(); ++i) {
        if (mIndexBuilt && !mTetAABBs[i].contains(p, eps)) {
            continue;
        }

        float w[4];
        if (computeBarycentricCoordinates(i, p, w)) {
            if (w[0] >= -eps && w[1] >= -eps && w[2] >= -eps && w[3] >= -eps) {
                weights[0] = w[0];
                weights[1] = w[1];
                weights[2] = w[2];
                weights[3] = w[3];
                return static_cast<int32_t>(i);
            }
        }
    }
    return -1;
}

int32_t TetrahedralMesh::findClosestTetrahedron(const Vec3& p, float weights[4]) const noexcept {
    int32_t bestTet = -1;
    float minViolation = std::numeric_limits<float>::max();
    float bestW[4]{0.25f, 0.25f, 0.25f, 0.25f};

    for (size_t i = 0; i < tetrahedra.size(); ++i) {
        float w[4];
        if (computeBarycentricCoordinates(i, p, w)) {
            float violation = 0.0f;
            for (int k = 0; k < 4; ++k) {
                if (w[k] < 0.0f) {
                    violation += (-w[k]);
                }
            }
            if (violation < minViolation) {
                minViolation = violation;
                bestTet = static_cast<int32_t>(i);
                for (int k = 0; k < 4; ++k) bestW[k] = w[k];
                if (violation == 0.0f) break; // Inside
            }
        }
    }

    if (bestTet >= 0) {
        for (int k = 0; k < 4; ++k) weights[k] = bestW[k];
    }
    return bestTet;
}

void TetrahedralMesh::extractBoundarySurface(SurfaceMesh& outSurfaceMesh,
                                             std::vector<uint32_t>* outBoundaryToTetVertexMap) const {
    outSurfaceMesh.clear();

    struct FaceOccurrence {
        uint32_t count{0};
        uint32_t tetIndex{0};
        uint32_t v[3]{0, 0, 0};
        uint32_t oppVertex{0};
    };

    std::unordered_map<FaceKey, FaceOccurrence, FaceKeyHash> faceMap;

    for (size_t t = 0; t < tetrahedra.size(); ++t) {
        const auto& tet = tetrahedra[t];

        // 4 faces of tetrahedron
        const uint32_t faces[4][4] = {
            {tet[1], tet[2], tet[3], tet[0]}, // face opposite v0
            {tet[0], tet[3], tet[2], tet[1]}, // face opposite v1
            {tet[0], tet[1], tet[3], tet[2]}, // face opposite v2
            {tet[0], tet[2], tet[1], tet[3]}  // face opposite v3
        };

        for (int f = 0; f < 4; ++f) {
            FaceKey key(faces[f][0], faces[f][1], faces[f][2]);
            auto& occ = faceMap[key];
            occ.count++;
            occ.tetIndex = static_cast<uint32_t>(t);
            occ.v[0] = faces[f][0];
            occ.v[1] = faces[f][1];
            occ.v[2] = faces[f][2];
            occ.oppVertex = faces[f][3];
        }
    }

    // Only faces with count == 1 are boundary faces
    std::unordered_map<uint32_t, uint32_t> tetToSurfaceVertMap;
    std::vector<uint32_t> surfaceToTetVertMap;

    auto getSurfaceVert = [&](uint32_t tetV) -> uint32_t {
        auto it = tetToSurfaceVertMap.find(tetV);
        if (it != tetToSurfaceVertMap.end()) {
            return it->second;
        }
        uint32_t sIdx = static_cast<uint32_t>(outSurfaceMesh.vertices.size());
        outSurfaceMesh.vertices.push_back(vertices[tetV]);
        tetToSurfaceVertMap[tetV] = sIdx;
        surfaceToTetVertMap.push_back(tetV);
        return sIdx;
    };

    for (const auto& [key, occ] : faceMap) {
        if (occ.count == 1) {
            uint32_t a = occ.v[0];
            uint32_t b = occ.v[1];
            uint32_t c = occ.v[2];
            uint32_t d = occ.oppVertex;

            // Ensure outward normal pointing away from opposite vertex d
            const Vec3& pa = vertices[a];
            const Vec3& pb = vertices[b];
            const Vec3& pc = vertices[c];
            const Vec3& pd = vertices[d];

            Vec3 fn = (pb - pa).cross(pc - pa);
            Vec3 away = pa - pd;
            if (fn.dot(away) < 0.0f) {
                std::swap(b, c);
            }

            uint32_t sa = getSurfaceVert(a);
            uint32_t sb = getSurfaceVert(b);
            uint32_t sc = getSurfaceVert(c);

            outSurfaceMesh.triangles.push_back(Triangle(sa, sb, sc));
        }
    }

    outSurfaceMesh.computeSmoothVertexNormals();

    if (outBoundaryToTetVertexMap) {
        *outBoundaryToTetVertexMap = std::move(surfaceToTetVertMap);
    }
}

bool TetrahedralMesh::saveToVTK(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# vtk DataFile Version 3.0\n";
    file << "SURA Volumetric Tetrahedral Mesh\n";
    file << "ASCII\n";
    file << "DATASET UNSTRUCTURED_GRID\n";

    file << "POINTS " << vertices.size() << " float\n";
    for (const auto& v : vertices) {
        file << v.x << " " << v.y << " " << v.z << "\n";
    }

    file << "CELLS " << tetrahedra.size() << " " << (tetrahedra.size() * 5) << "\n";
    for (const auto& tet : tetrahedra) {
        file << "4 " << tet[0] << " " << tet[1] << " " << tet[2] << " " << tet[3] << "\n";
    }

    file << "CELL_TYPES " << tetrahedra.size() << "\n";
    for (size_t i = 0; i < tetrahedra.size(); ++i) {
        file << "10\n"; // VTK_TETRA
    }

    return true;
}

} // namespace sura::organ
