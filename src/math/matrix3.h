/**
 * @file matrix3.h
 * @brief 3x3 matrices for 2D transformations in HOMOGENEOUS COORDINATES.
 *
 * A 2D point (x, y) is written as the column vector (x, y, 1). The extra 1
 * lets TRANSLATION - which is not a linear map - be a matrix multiplication
 * too, so every 2D transformation (translate, rotate, scale, shear, reflect)
 * has the same form and they can be COMPOSED by multiplying matrices:
 *
 *     | a  b  tx |   | x |   | a x + b y + tx |
 *     | c  d  ty | * | y | = | c x + d y + ty |
 *     | 0  0  1  |   | 1 |   |       1        |
 *
 * Composition order: (A * B) * p applies B FIRST, then A. So
 * T(p) * R(theta) * T(-p) moves the pivot to the origin, rotates, and moves it back.
 *
 * Screen space has y pointing DOWN, so a positive rotation angle turns
 * shapes CLOCKWISE on screen.
 *
 * A direction (vector) is written (x, y, 0): translation has no effect on it.
 */
#pragma once

#include <vector>

#include "core/vec2.h"

class Matrix3 {
public:
    /** @brief The identity matrix (no transformation). */
    Matrix3();

    static Matrix3 identity() { return Matrix3(); }

    /** @brief Move by (translateX, translateY). */
    static Matrix3 translation(double translateX, double translateY);
    static Matrix3 translation(const Vec2& offset) { return translation(offset.x, offset.y); }

    /** @brief Rotate about the ORIGIN by 'radians': [cos -sin; sin cos]. */
    static Matrix3 rotation(double radians);

    /** @brief Scale about the ORIGIN by (scaleX, scaleY); negative values mirror. */
    static Matrix3 scaling(double scaleX, double scaleY);
    static Matrix3 scaling(double uniformScale) { return scaling(uniformScale, uniformScale); }

    /** @brief Shear: x' = x + shearX * y, y' = y + shearY * x. */
    static Matrix3 shearing(double shearX, double shearY);

    /** @brief Mirror across the vertical line x = 0 (flip left/right). */
    static Matrix3 reflectionAcrossYAxis() { return scaling(-1.0, 1.0); }

    /** @brief Mirror across the horizontal line y = 0 (flip up/down). */
    static Matrix3 reflectionAcrossXAxis() { return scaling(1.0, -1.0); }

    /** @brief Rotate about an arbitrary pivot: T(pivot) * R * T(-pivot). */
    static Matrix3 rotationAbout(const Vec2& pivot, double radians);

    /** @brief Scale about an arbitrary fixed point: T(pivot) * S * T(-pivot). */
    static Matrix3 scalingAbout(const Vec2& pivot, double scaleX, double scaleY);

    /** @brief Matrix product (this * other): 'other' is applied first. */
    Matrix3 operator*(const Matrix3& other) const;

    /** @brief Transform a POINT (w = 1): affected by translation. */
    Vec2 transformPoint(const Vec2& point) const;

    /** @brief Transform a DIRECTION (w = 0): translation is ignored. */
    Vec2 transformVector(const Vec2& vector) const;

    /** @brief Transform every point of a polygon / polyline. */
    std::vector<Vec2> transformPoints(const std::vector<Vec2>& points) const;

    /** @brief Element at (row, column), both 0..2. */
    double at(int row, int column) const { return elements[row][column]; }

private:
    double elements[3][3];
};
