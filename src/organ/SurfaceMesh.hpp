#pragma once

#include "Types.hpp"
#include <string>
#include <vector>

namespace sura::organ {

/**
 * @brief Represents a 3D surface triangle mesh (e.g. organ visual/rendering geometry).
 * Handles geometry data, normal generation, procedural shapes, and file I/O (OBJ, STL).
 */
class SurfaceMesh {
public:
    std::vector<Vec3> vertices;
    std::vector<Triangle> triangles;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    SurfaceMesh() = default;

    size_t vertexCount() const noexcept { return vertices.size(); }
    size_t triangleCount() const noexcept { return triangles.size(); }
    bool empty() const noexcept { return vertices.empty() || triangles.empty(); }
    void clear() noexcept;

    // Bounding volume
    AABB computeAABB() const noexcept;

    // Normals
    void computeFaceNormals(std::vector<Vec3>& outFaceNormals) const;
    void computeSmoothVertexNormals();

    // Procedural shape generators for organs and testing
    static SurfaceMesh createCube(const Vec3& size = {1.0f, 1.0f, 1.0f}, const Vec3& center = {0.0f, 0.0f, 0.0f});
    static SurfaceMesh createSphere(float radius = 0.5f, uint32_t rings = 16, uint32_t sectors = 32, const Vec3& center = {0.0f, 0.0f, 0.0f});
    static SurfaceMesh createEllipsoid(const Vec3& radii = {0.6f, 0.4f, 0.3f}, uint32_t rings = 16, uint32_t sectors = 32, const Vec3& center = {0.0f, 0.0f, 0.0f});

    // File I/O
    bool loadFromFile(const std::string& filepath);
    bool loadFromOBJ(const std::string& filepath);
    bool saveToOBJ(const std::string& filepath) const;
    bool loadFromSTL(const std::string& filepath);
    bool saveToSTL(const std::string& filepath, bool binary = true) const;
};

} // namespace sura::organ
