/**
 * @file vec2.h
 * @brief Minimal 2D vector type used by every drawing routine in the project.
 *
 * All geometry in the software renderer is expressed in *screen space*:
 *   - x grows to the right
 *   - y grows DOWNWARD (row 0 is the top of the image)
 * This matches how the frame buffer is laid out in memory, so no flip is
 * needed when converting a point to a pixel address.
 */
#pragma once

#include <cmath>

/** @brief The constant pi (M_PI is not guaranteed by standard C++). */
inline constexpr double pi = 3.14159265358979323846;

/**
 * @brief A 2D point / direction with double precision coordinates.
 *
 * Doubles are used (instead of ints) so curves, splines and transformations
 * can be evaluated exactly before the rasterizer snaps them to pixels.
 */
struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    Vec2() = default;
    Vec2(double xValue, double yValue) : x(xValue), y(yValue) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator-() const { return {-x, -y}; }
    Vec2 operator*(double scalar) const { return {x * scalar, y * scalar}; }
    Vec2 operator/(double scalar) const { return {x / scalar, y / scalar}; }
    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    /** @brief Euclidean length sqrt(x^2 + y^2). */
    double length() const { return std::sqrt(x * x + y * y); }

    /** @brief Unit-length copy of this vector (zero vector stays zero). */
    Vec2 normalized() const {
        const double currentLength = length();
        return currentLength > 1e-12 ? *this / currentLength : Vec2();
    }

    /**
     * @brief Vector rotated 90 degrees: (x, y) -> (-y, x).
     *
     * With y pointing down, the perpendicular of a left-to-right direction
     * points DOWN the screen. Negate it to get the "upward" side.
     */
    Vec2 perpendicular() const { return {-y, x}; }
};

/** @brief Linear interpolation between two points: a + (b - a) * t. */
inline Vec2 lerpVec2(const Vec2& a, const Vec2& b, double t) { return a + (b - a) * t; }

/** @brief Dot product a.x*b.x + a.y*b.y. */
inline double dotProduct(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

/** @brief Round a real coordinate to the nearest pixel index. */
inline int roundToInt(double value) { return static_cast<int>(std::lround(value)); }
