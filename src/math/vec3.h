/**
 * @file vec3.h
 * @brief 3D vector for points and directions in world space.
 *
 * Convention: right-handed, x to the right, y UP, z toward the viewer
 * (the same convention OpenGL uses, so CP3 can reuse it unchanged).
 */
#pragma once

#include <cmath>

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vec3() = default;
    Vec3(double xValue, double yValue, double zValue) : x(xValue), y(yValue), z(zValue) {}

    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(double scalar) const { return {x * scalar, y * scalar, z * scalar}; }

    double length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        const double currentLength = length();
        return currentLength > 1e-12 ? *this * (1.0 / currentLength) : Vec3();
    }
};

inline double dotProduct(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

/** @brief Cross product: a vector perpendicular to both a and b (right-hand rule). */
inline Vec3 crossProduct(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
