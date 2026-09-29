#include "TetrahedralMesher.hpp"

#include <mutex>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace nglib {
#include <nglib.h>
}

namespace sura::organ {

namespace {
std::once_flag gNgInitFlag;
}

void TetrahedralMesher::ensureInitialized() {
    std::call_once(gNgInitFlag, []() {
        nglib::Ng_Init();
    });
}

bool TetrahedralMesher::generate(const SurfaceMesh& surfaceMesh,
                                TetrahedralMesh& outTetMesh,
                                const MesherOptions& options) {
    if (surfaceMesh.empty()) {
        std::cerr << "[TetrahedralMesher] SurfaceMesh is empty!" << std::endl;
        return false;
    }

    ensureInitialized();
    outTetMesh.clear();

    // Determine effective maxH
    AABB bbox = surfaceMesh.computeAABB();
    Vec3 extent = bbox.size();
    float diag = extent.norm();
    double effectiveMaxH = options.maxH > 0.0f ? static_cast<double>(options.maxH) : (diag > 0.0f ? diag * 0.25 : 1.0);

    bool success = false;
    if (options.useSTLMode) {
        success = meshSTL(surfaceMesh, outTetMesh, options, effectiveMaxH);
        if (!success || outTetMesh.empty()) {
            // Fallback to direct
            success = meshDirect(surfaceMesh, outTetMesh, options, effectiveMaxH);
        }
    } else {
        success = meshDirect(surfaceMesh, outTetMesh, options, effectiveMaxH);
        if (!success || outTetMesh.empty()) {
            // Fallback to STL
            success = meshSTL(surfaceMesh, outTetMesh, options, effectiveMaxH);
        }
    }

    if (success && !outTetMesh.empty()) {
        outTetMesh.buildSpatialIndex();
    }

    return success && !outTetMesh.empty();
}

bool TetrahedralMesher::meshDirect(const SurfaceMesh& surfaceMesh,
                                  TetrahedralMesh& outTetMesh,
                                  const MesherOptions& options,
                                  double effectiveMaxH) {
    nglib::Ng_Mesh* mesh = nglib::Ng_NewMesh();
    if (!mesh) {
        std::cerr << "[TetrahedralMesher] Failed to create Netgen mesh structure." << std::endl;
        return false;
    }

    // Add points
    for (const auto& v : surfaceMesh.vertices) {
        double pt[3] = { static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z) };
        nglib::Ng_AddPoint(mesh, pt);
    }

    // Add surface triangles (1-based indices in nglib)
    for (const auto& tri : surfaceMesh.triangles) {
        int trig[3] = {
            static_cast<int>(tri[0] + 1),
            static_cast<int>(tri[1] + 1),
            static_cast<int>(tri[2] + 1)
        };
        nglib::Ng_AddSurfaceElement(mesh, nglib::NG_TRIG, trig);
    }

    // Configure meshing parameters
    nglib::Ng_Meshing_Parameters mp;
    mp.maxh = effectiveMaxH;
    mp.minh = options.minH > 0.0f ? static_cast<double>(options.minH) : 0.0;
    mp.fineness = static_cast<double>(std::clamp(options.fineness, 0.0f, 1.0f));
    mp.grading = static_cast<double>(std::clamp(options.grading, 0.0f, 1.0f));
    mp.second_order = options.secondOrder ? 1 : 0;
    mp.optsteps_3d = options.optSteps3D;

    nglib::Ng_Result res = nglib::Ng_GenerateVolumeMesh(mesh, &mp);
    if (res != nglib::NG_OK) {
        nglib::Ng_DeleteMesh(mesh);
        return false;
    }

    int np = nglib::Ng_GetNP(mesh);
    int ne = nglib::Ng_GetNE(mesh);
    if (ne <= 0 || np <= 0) {
        nglib::Ng_DeleteMesh(mesh);
        return false;
    }

    // Extract vertices
    outTetMesh.vertices.reserve(np);
    for (int i = 1; i <= np; ++i) {
        double pt[3];
        nglib::Ng_GetPoint(mesh, i, pt);
        outTetMesh.vertices.push_back(Vec3(
            static_cast<float>(pt[0]),
            static_cast<float>(pt[1]),
            static_cast<float>(pt[2])
        ));
    }

    // Extract tetrahedra
    outTetMesh.tetrahedra.reserve(ne);
    for (int i = 1; i <= ne; ++i) {
        int tet[4];
        nglib::Ng_GetVolumeElement(mesh, i, tet);
        // Convert from 1-based to 0-based
        outTetMesh.tetrahedra.push_back(Tetrahedron(
            static_cast<uint32_t>(tet[0] - 1),
            static_cast<uint32_t>(tet[1] - 1),
            static_cast<uint32_t>(tet[2] - 1),
            static_cast<uint32_t>(tet[3] - 1)
        ));
    }

    nglib::Ng_DeleteMesh(mesh);
    return true;
}

bool TetrahedralMesher::meshSTL(const SurfaceMesh& surfaceMesh,
                               TetrahedralMesh& outTetMesh,
                               const MesherOptions& options,
                               double effectiveMaxH) {
    nglib::Ng_STL_Geometry* stl = nglib::Ng_STL_NewGeometry();
    if (!stl) return false;

    for (const auto& tri : surfaceMesh.triangles) {
        if (tri[0] >= surfaceMesh.vertices.size() ||
            tri[1] >= surfaceMesh.vertices.size() ||
            tri[2] >= surfaceMesh.vertices.size()) continue;

        const Vec3& p0 = surfaceMesh.vertices[tri[0]];
        const Vec3& p1 = surfaceMesh.vertices[tri[1]];
        const Vec3& p2 = surfaceMesh.vertices[tri[2]];
        Vec3 fn = (p1 - p0).cross(p2 - p0).normalized();

        double d0[3] = { p0.x, p0.y, p0.z };
        double d1[3] = { p1.x, p1.y, p1.z };
        double d2[3] = { p2.x, p2.y, p2.z };
        double dn[3] = { fn.x, fn.y, fn.z };

        nglib::Ng_STL_AddTriangle(stl, d0, d1, d2, dn);
    }

    if (nglib::Ng_STL_InitSTLGeometry(stl) != nglib::NG_OK) {
        return false;
    }

    nglib::Ng_Mesh* mesh = nglib::Ng_NewMesh();
    if (!mesh) return false;

    nglib::Ng_Meshing_Parameters mp;
    mp.maxh = effectiveMaxH;
    mp.minh = options.minH > 0.0f ? static_cast<double>(options.minH) : 0.0;
    mp.fineness = static_cast<double>(std::clamp(options.fineness, 0.0f, 1.0f));
    mp.grading = static_cast<double>(std::clamp(options.grading, 0.0f, 1.0f));
    mp.second_order = options.secondOrder ? 1 : 0;
    mp.optsteps_3d = options.optSteps3D;

    if (nglib::Ng_STL_MakeEdges(stl, mesh, &mp) != nglib::NG_OK) {
        nglib::Ng_DeleteMesh(mesh);
        return false;
    }

    if (nglib::Ng_STL_GenerateSurfaceMesh(stl, mesh, &mp) != nglib::NG_OK) {
        nglib::Ng_DeleteMesh(mesh);
        return false;
    }

    if (nglib::Ng_GenerateVolumeMesh(mesh, &mp) != nglib::NG_OK) {
        nglib::Ng_DeleteMesh(mesh);
        return false;
    }

    int np = nglib::Ng_GetNP(mesh);
    int ne = nglib::Ng_GetNE(mesh);
    if (ne <= 0 || np <= 0) {
        nglib::Ng_DeleteMesh(mesh);
        return false;
    }

    outTetMesh.vertices.reserve(np);
    for (int i = 1; i <= np; ++i) {
        double pt[3];
        nglib::Ng_GetPoint(mesh, i, pt);
        outTetMesh.vertices.push_back(Vec3(
            static_cast<float>(pt[0]),
            static_cast<float>(pt[1]),
            static_cast<float>(pt[2])
        ));
    }

    outTetMesh.tetrahedra.reserve(ne);
    for (int i = 1; i <= ne; ++i) {
        int tet[4];
        nglib::Ng_GetVolumeElement(mesh, i, tet);
        outTetMesh.tetrahedra.push_back(Tetrahedron(
            static_cast<uint32_t>(tet[0] - 1),
            static_cast<uint32_t>(tet[1] - 1),
            static_cast<uint32_t>(tet[2] - 1),
            static_cast<uint32_t>(tet[3] - 1)
        ));
    }

    nglib::Ng_DeleteMesh(mesh);
    return true;
}

} // namespace sura::organ
