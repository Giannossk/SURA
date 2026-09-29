#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <limits>
#include <string>
#include <iostream>

#include "LinearMath.h"
#include <LinearMath/btVector3.h>

namespace sura::organ {

/**
 * @brief 3D vector for geometry and simulation representation.
 * Compatible with LinearMath::Vector3r, btVector3, and raw float arrays.
 */
struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Vec3() noexcept = default;
    constexpr Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

    // LinearMath::Vector3r conversion
    Vec3(const LinearMath::Vector3r& v) noexcept
        : x(static_cast<float>(v.x())), y(static_cast<float>(v.y())), z(static_cast<float>(v.z())) {}

    operator LinearMath::Vector3r() const noexcept {
        return LinearMath::Vector3r(x, y, z);
    }

    // Bullet btVector3 conversion
    Vec3(const btVector3& v) noexcept
        : x(static_cast<float>(v.x())), y(static_cast<float>(v.y())), z(static_cast<float>(v.z())) {}

    operator btVector3() const noexcept {
        return btVector3(x, y, z);
    }

    // Accessors
    float& operator[](size_t i) noexcept {
        return (&x)[i];
    }

    const float& operator[](size_t i) const noexcept {
        return (&x)[i];
    }

    // Basic operators
    Vec3 operator+(const Vec3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const noexcept { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const noexcept { float inv = 1.0f / s; return {x * inv, y * inv, z * inv}; }
    Vec3 operator-() const noexcept { return {-x, -y, -z}; }

    Vec3& operator+=(const Vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) noexcept { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) noexcept { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(float s) noexcept { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }

    bool operator==(const Vec3& o) const noexcept {
        constexpr float eps = 1e-6f;
        return std::abs(x - o.x) < eps && std::abs(y - o.y) < eps && std::abs(z - o.z) < eps;
    }

    float dot(const Vec3& o) const noexcept {
        return x * o.x + y * o.y + z * o.z;
    }

    Vec3 cross(const Vec3& o) const noexcept {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    float squaredNorm() const noexcept {
        return x * x + y * y + z * z;
    }

    float norm() const noexcept {
        return std::sqrt(squaredNorm());
    }

    Vec3 normalized() const noexcept {
        float len = norm();
        if (len > 1e-8f) {
            return *this / len;
        }
        return {0.0f, 0.0f, 0.0f};
    }
};

inline Vec3 operator*(float s, const Vec3& v) noexcept {
    return v * s;
}

/**
 * @brief 2D vector for texture coordinates.
 */
struct Vec2 {
    float u{0.0f};
    float v{0.0f};

    constexpr Vec2() noexcept = default;
    constexpr Vec2(float u_, float v_) noexcept : u(u_), v(v_) {}
};

/**
 * @brief Triangle face referencing 3 vertex indices.
 */
struct Triangle {
    uint32_t v[3]{0, 0, 0};

    constexpr Triangle() noexcept = default;
    constexpr Triangle(uint32_t v0, uint32_t v1, uint32_t v2) noexcept : v{v0, v1, v2} {}

    uint32_t& operator[](size_t i) noexcept { return v[i]; }
    const uint32_t& operator[](size_t i) const noexcept { return v[i]; }
};

/**
 * @brief Tetrahedron referencing 4 vertex indices.
 */
struct Tetrahedron {
    uint32_t v[4]{0, 0, 0, 0};

    constexpr Tetrahedron() noexcept = default;
    constexpr Tetrahedron(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3) noexcept : v{v0, v1, v2, v3} {}

    uint32_t& operator[](size_t i) noexcept { return v[i]; }
    const uint32_t& operator[](size_t i) const noexcept { return v[i]; }
};

/**
 * @brief Axis-Aligned Bounding Box (AABB) for spatial queries.
 */
struct AABB {
    Vec3 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    Vec3 max{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

    constexpr AABB() noexcept = default;
    constexpr AABB(const Vec3& min_, const Vec3& max_) noexcept : min(min_), max(max_) {}

    void expand(const Vec3& p) noexcept {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        min.z = std::min(min.z, p.z);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
        max.z = std::max(max.z, p.z);
    }

    void expand(const AABB& other) noexcept {
        expand(other.min);
        expand(other.max);
    }

    bool contains(const Vec3& p, float eps = 1e-4f) const noexcept {
        return (p.x >= min.x - eps && p.x <= max.x + eps &&
                p.y >= min.y - eps && p.y <= max.y + eps &&
                p.z >= min.z - eps && p.z <= max.z + eps);
    }

    bool intersects(const AABB& other) const noexcept {
        return (min.x <= other.max.x && max.x >= other.min.x &&
                min.y <= other.max.y && max.y >= other.min.y &&
                min.z <= other.max.z && max.z >= other.min.z);
    }

    Vec3 center() const noexcept {
        return (min + max) * 0.5f;
    }

    Vec3 size() const noexcept {
        return max - min;
    }
};

} // namespace sura::organ
