#include "SurfaceMesh.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cstring>

namespace sura::organ {

void SurfaceMesh::clear() noexcept {
    vertices.clear();
    triangles.clear();
    normals.clear();
    uvs.clear();
}

AABB SurfaceMesh::computeAABB() const noexcept {
    AABB box;
    for (const auto& v : vertices) {
        box.expand(v);
    }
    return box;
}

void SurfaceMesh::computeFaceNormals(std::vector<Vec3>& outFaceNormals) const {
    outFaceNormals.resize(triangles.size());
    for (size_t i = 0; i < triangles.size(); ++i) {
        const auto& tri = triangles[i];
        if (tri[0] < vertices.size() && tri[1] < vertices.size() && tri[2] < vertices.size()) {
            const Vec3& p0 = vertices[tri[0]];
            const Vec3& p1 = vertices[tri[1]];
            const Vec3& p2 = vertices[tri[2]];
            outFaceNormals[i] = (p1 - p0).cross(p2 - p0).normalized();
        } else {
            outFaceNormals[i] = Vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

void SurfaceMesh::computeSmoothVertexNormals() {
    normals.assign(vertices.size(), Vec3(0.0f, 0.0f, 0.0f));

    for (const auto& tri : triangles) {
        if (tri[0] < vertices.size() && tri[1] < vertices.size() && tri[2] < vertices.size()) {
            const Vec3& p0 = vertices[tri[0]];
            const Vec3& p1 = vertices[tri[1]];
            const Vec3& p2 = vertices[tri[2]];
            Vec3 fn = (p1 - p0).cross(p2 - p0);
            normals[tri[0]] += fn;
            normals[tri[1]] += fn;
            normals[tri[2]] += fn;
        }
    }

    for (auto& n : normals) {
        n = n.normalized();
    }
}

SurfaceMesh SurfaceMesh::createCube(const Vec3& size, const Vec3& center) {
    SurfaceMesh mesh;
    const Vec3 h = size * 0.5f;

    // 8 cube corners
    mesh.vertices = {
        center + Vec3(-h.x, -h.y, -h.z), // 0
        center + Vec3( h.x, -h.y, -h.z), // 1
        center + Vec3( h.x,  h.y, -h.z), // 2
        center + Vec3(-h.x,  h.y, -h.z), // 3
        center + Vec3(-h.x, -h.y,  h.z), // 4
        center + Vec3( h.x, -h.y,  h.z), // 5
        center + Vec3( h.x,  h.y,  h.z), // 6
        center + Vec3(-h.x,  h.y,  h.z)  // 7
    };

    // 12 triangles (2 per face), outward CCW winding
    mesh.triangles = {
        // Front (z = +h)
        Triangle(4, 5, 6), Triangle(4, 6, 7),
        // Back (z = -h)
        Triangle(1, 0, 3), Triangle(1, 3, 2),
        // Left (x = -h)
        Triangle(0, 4, 7), Triangle(0, 7, 3),
        // Right (x = +h)
        Triangle(5, 1, 2), Triangle(5, 2, 6),
        // Top (y = +h)
        Triangle(3, 7, 6), Triangle(3, 6, 2),
        // Bottom (y = -h)
        Triangle(0, 1, 5), Triangle(0, 5, 4)
    };

    mesh.computeSmoothVertexNormals();
    return mesh;
}

SurfaceMesh SurfaceMesh::createSphere(float radius, uint32_t rings, uint32_t sectors, const Vec3& center) {
    return createEllipsoid(Vec3(radius, radius, radius), rings, sectors, center);
}

SurfaceMesh SurfaceMesh::createEllipsoid(const Vec3& radii, uint32_t rings, uint32_t sectors, const Vec3& center) {
    SurfaceMesh mesh;
    if (rings < 4) rings = 4;
    if (sectors < 3) sectors = 3;

    constexpr float kPi = 3.14159265358979323846f;

    // 1. South Pole
    uint32_t southPoleIdx = 0;
    mesh.vertices.push_back(center + Vec3(0.0f, -radii.y, 0.0f));
    mesh.normals.push_back(Vec3(0.0f, -1.0f, 0.0f));
    mesh.uvs.push_back(Vec2(0.5f, 0.0f));

    // 2. Intermediate rings (1 to rings - 2)
    for (uint32_t r = 1; r < rings - 1; ++r) {
        float phi = -kPi * 0.5f + kPi * (static_cast<float>(r) / static_cast<float>(rings - 1));
        float y = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (uint32_t s = 0; s < sectors; ++s) {
            float theta = 2.0f * kPi * (static_cast<float>(s) / static_cast<float>(sectors));
            float x = cosPhi * std::cos(theta);
            float z = cosPhi * std::sin(theta);

            mesh.vertices.push_back(center + Vec3(x * radii.x, y * radii.y, z * radii.z));
            Vec3 norm(x / (radii.x * radii.x), y / (radii.y * radii.y), z / (radii.z * radii.z));
            mesh.normals.push_back(norm.normalized());
            mesh.uvs.push_back(Vec2(static_cast<float>(s) / static_cast<float>(sectors),
                                    static_cast<float>(r) / static_cast<float>(rings - 1)));
        }
    }

    // 3. North Pole
    uint32_t northPoleIdx = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back(center + Vec3(0.0f, radii.y, 0.0f));
    mesh.normals.push_back(Vec3(0.0f, 1.0f, 0.0f));
    mesh.uvs.push_back(Vec2(0.5f, 1.0f));

    // Triangles
    // Bottom cap: south pole to ring 1
    for (uint32_t s = 0; s < sectors; ++s) {
        uint32_t nextS = (s + 1) % sectors;
        uint32_t v1 = 1 + s;
        uint32_t v2 = 1 + nextS;
        mesh.triangles.push_back(Triangle(southPoleIdx, v2, v1));
    }

    // Middle rings
    for (uint32_t r = 1; r < rings - 2; ++r) {
        uint32_t curRing = 1 + (r - 1) * sectors;
        uint32_t nextRing = 1 + r * sectors;

        for (uint32_t s = 0; s < sectors; ++s) {
            uint32_t nextS = (s + 1) % sectors;
            uint32_t c0 = curRing + s;
            uint32_t c1 = curRing + nextS;
            uint32_t n0 = nextRing + s;
            uint32_t n1 = nextRing + nextS;

            mesh.triangles.push_back(Triangle(c0, n0, n1));
            mesh.triangles.push_back(Triangle(c0, n1, c1));
        }
    }

    // Top cap: last intermediate ring to north pole
    uint32_t topRing = 1 + (rings - 3) * sectors;
    for (uint32_t s = 0; s < sectors; ++s) {
        uint32_t nextS = (s + 1) % sectors;
        uint32_t v0 = topRing + s;
        uint32_t v1 = topRing + nextS;
        mesh.triangles.push_back(Triangle(v0, northPoleIdx, v1));
    }

    // Ensure all triangles have strictly outward-pointing normals
    for (auto& tri : mesh.triangles) {
        const Vec3& p0 = mesh.vertices[tri[0]];
        const Vec3& p1 = mesh.vertices[tri[1]];
        const Vec3& p2 = mesh.vertices[tri[2]];
        Vec3 fn = (p1 - p0).cross(p2 - p0);
        Vec3 centroid = (p0 + p1 + p2) / 3.0f - center;
        if (fn.dot(centroid) < 0.0f) {
            std::swap(tri[1], tri[2]);
        }
    }

    mesh.computeSmoothVertexNormals();
    return mesh;
}


bool SurfaceMesh::loadFromFile(const std::string& filepath) {
    if (filepath.size() >= 4) {
        std::string ext = filepath.substr(filepath.size() - 4);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".obj") {
            return loadFromOBJ(filepath);
        } else if (ext == ".stl") {
            return loadFromSTL(filepath);
        }
    }
    // Try OBJ first, then STL
    if (loadFromOBJ(filepath)) return true;
    return loadFromSTL(filepath);
}

bool SurfaceMesh::loadFromOBJ(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[SurfaceMesh] Cannot open OBJ file: " << filepath << std::endl;
        return false;
    }

    clear();
    std::string line;
    std::vector<Vec3> tempNormals;
    std::vector<Vec2> tempUVs;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            Vec3 v;
            ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } else if (prefix == "vn") {
            Vec3 n;
            ss >> n.x >> n.y >> n.z;
            tempNormals.push_back(n);
        } else if (prefix == "vt") {
            Vec2 uv;
            ss >> uv.u >> uv.v;
            tempUVs.push_back(uv);
        } else if (prefix == "f") {
            std::vector<uint32_t> faceIndices;
            std::string token;
            while (ss >> token) {
                // Token format: v, v/vt, v//vn, v/vt/vn
                size_t slash1 = token.find('/');
                int vIdx = 0;
                if (slash1 == std::string::npos) {
                    vIdx = std::stoi(token);
                } else {
                    vIdx = std::stoi(token.substr(0, slash1));
                }

                if (vIdx > 0) {
                    faceIndices.push_back(static_cast<uint32_t>(vIdx - 1));
                } else if (vIdx < 0) {
                    faceIndices.push_back(static_cast<uint32_t>(vertices.size() + vIdx));
                }
            }

            // Fan triangulation for polygons
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                triangles.push_back(Triangle(faceIndices[0], faceIndices[i], faceIndices[i + 1]));
            }
        }
    }

    if (normals.empty() || normals.size() != vertices.size()) {
        computeSmoothVertexNormals();
    }

    return !vertices.empty() && !triangles.empty();
}

bool SurfaceMesh::saveToOBJ(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[SurfaceMesh] Cannot write to OBJ file: " << filepath << std::endl;
        return false;
    }

    file << "# Exported by SURA SurfaceMesh\n";
    for (const auto& v : vertices) {
        file << "v " << v.x << " " << v.y << " " << v.z << "\n";
    }

    if (!normals.empty()) {
        for (const auto& n : normals) {
            file << "vn " << n.x << " " << n.y << " " << n.z << "\n";
        }
    }

    if (!uvs.empty()) {
        for (const auto& uv : uvs) {
            file << "vt " << uv.u << " " << uv.v << "\n";
        }
    }

    for (const auto& tri : triangles) {
        uint32_t i0 = tri[0] + 1;
        uint32_t i1 = tri[1] + 1;
        uint32_t i2 = tri[2] + 1;
        if (!normals.empty() && normals.size() == vertices.size()) {
            file << "f " << i0 << "//" << i0 << " " << i1 << "//" << i1 << " " << i2 << "//" << i2 << "\n";
        } else {
            file << "f " << i0 << " " << i1 << " " << i2 << "\n";
        }
    }

    return true;
}

bool SurfaceMesh::loadFromSTL(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[SurfaceMesh] Cannot open STL file: " << filepath << std::endl;
        return false;
    }

    clear();

    // Check if ASCII or binary
    char header[80];
    file.read(header, 80);
    if (file.gcount() < 80) return false;

    // Check for "solid" keyword
    bool isAscii = (std::strncmp(header, "solid", 5) == 0);
    if (isAscii) {
        // Double check by reading a line
        file.seekg(0, std::ios::beg);
        std::string firstLine;
        std::getline(file, firstLine);
        std::string secondLine;
        std::getline(file, secondLine);
        if (secondLine.find("facet") != std::string::npos || firstLine.find("facet") != std::string::npos) {
            // Definitely ASCII
            file.seekg(0, std::ios::beg);
            std::string line;
            std::unordered_map<std::string, uint32_t> vertexMap;

            auto getVertexIndex = [&](float x, float y, float z) -> uint32_t {
                // 1e-4 quantizing for welding
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.4f,%.4f,%.4f", x, y, z);
                std::string key(buf);
                auto it = vertexMap.find(key);
                if (it != vertexMap.end()) return it->second;
                uint32_t idx = static_cast<uint32_t>(vertices.size());
                vertices.push_back(Vec3(x, y, z));
                vertexMap[key] = idx;
                return idx;
            };

            while (std::getline(file, line)) {
                if (line.find("facet normal") != std::string::npos) {
                    std::string outer;
                    std::getline(file, outer); // outer loop
                    uint32_t triIdx[3]{0, 0, 0};
                    for (int i = 0; i < 3; ++i) {
                        std::string vLine;
                        std::getline(file, vLine);
                        std::istringstream ss(vLine);
                        std::string kw;
                        float x, y, z;
                        ss >> kw >> x >> y >> z;
                        triIdx[i] = getVertexIndex(x, y, z);
                    }
                    std::string endloop, endfacet;
                    std::getline(file, endloop);
                    std::getline(file, endfacet);
                    triangles.push_back(Triangle(triIdx[0], triIdx[1], triIdx[2]));
                }
            }
            computeSmoothVertexNormals();
            return !vertices.empty() && !triangles.empty();
        }
    }

    // Binary STL
    file.seekg(80, std::ios::beg);
    uint32_t numTriangles = 0;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(uint32_t));

    struct STLTriangleRaw {
        float normal[3];
        float v0[3];
        float v1[3];
        float v2[3];
        uint16_t attrByteCount;
    };

    vertices.reserve(numTriangles * 3);
    triangles.reserve(numTriangles);

    for (uint32_t i = 0; i < numTriangles; ++i) {
        STLTriangleRaw raw;
        file.read(reinterpret_cast<char*>(&raw), 50);
        if (file.gcount() < 50) break;

        uint32_t base = static_cast<uint32_t>(vertices.size());
        vertices.push_back(Vec3(raw.v0[0], raw.v0[1], raw.v0[2]));
        vertices.push_back(Vec3(raw.v1[0], raw.v1[1], raw.v1[2]));
        vertices.push_back(Vec3(raw.v2[0], raw.v2[1], raw.v2[2]));

        triangles.push_back(Triangle(base, base + 1, base + 2));
    }

    computeSmoothVertexNormals();
    return !vertices.empty() && !triangles.empty();
}

bool SurfaceMesh::saveToSTL(const std::string& filepath, bool binary) const {
    if (!binary) {
        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        file << "solid SURA_Organ\n";
        for (const auto& tri : triangles) {
            Vec3 p0 = vertices[tri[0]];
            Vec3 p1 = vertices[tri[1]];
            Vec3 p2 = vertices[tri[2]];
            Vec3 n = (p1 - p0).cross(p2 - p0).normalized();
            file << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n";
            file << "    outer loop\n";
            file << "      vertex " << p0.x << " " << p0.y << " " << p0.z << "\n";
            file << "      vertex " << p1.x << " " << p1.y << " " << p1.z << "\n";
            file << "      vertex " << p2.x << " " << p2.y << " " << p2.z << "\n";
            file << "    endloop\n";
            file << "  endfacet\n";
        }
        file << "endsolid SURA_Organ\n";
        return true;
    }

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    char header[80] = {0};
    std::strncpy(header, "SURA Binary STL Export", sizeof(header) - 1);
    file.write(header, 80);

    uint32_t numTriangles = static_cast<uint32_t>(triangles.size());
    file.write(reinterpret_cast<const char*>(&numTriangles), sizeof(uint32_t));

    uint16_t attr = 0;
    for (const auto& tri : triangles) {
        Vec3 p0 = vertices[tri[0]];
        Vec3 p1 = vertices[tri[1]];
        Vec3 p2 = vertices[tri[2]];
        Vec3 n = (p1 - p0).cross(p2 - p0).normalized();

        float data[12] = {
            n.x, n.y, n.z,
            p0.x, p0.y, p0.z,
            p1.x, p1.y, p1.z,
            p2.x, p2.y, p2.z
        };
        file.write(reinterpret_cast<const char*>(data), 12 * sizeof(float));
        file.write(reinterpret_cast<const char*>(&attr), sizeof(uint16_t));
    }

    return true;
}

} // namespace sura::organ
