/**
 * @file matrix3.cpp
 * @brief 2D homogeneous transformation matrices.
 */
#include "math/matrix3.h"

#include <cmath>

Matrix3::Matrix3() : elements{{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}} {}

Matrix3 Matrix3::translation(double translateX, double translateY) {
    Matrix3 result;
    result.elements[0][2] = translateX;
    result.elements[1][2] = translateY;
    return result;
}

Matrix3 Matrix3::rotation(double radians) {
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    Matrix3 result;
    result.elements[0][0] = cosine;
    result.elements[0][1] = -sine;
    result.elements[1][0] = sine;
    result.elements[1][1] = cosine;
    return result;
}

Matrix3 Matrix3::scaling(double scaleX, double scaleY) {
    Matrix3 result;
    result.elements[0][0] = scaleX;
    result.elements[1][1] = scaleY;
    return result;
}

Matrix3 Matrix3::shearing(double shearX, double shearY) {
    Matrix3 result;
    result.elements[0][1] = shearX;
    result.elements[1][0] = shearY;
    return result;
}

Matrix3 Matrix3::rotationAbout(const Vec2& pivot, double radians) {
    // Read right to left: move pivot to origin, rotate, move back.
    return translation(pivot) * rotation(radians) * translation(-pivot);
}

Matrix3 Matrix3::scalingAbout(const Vec2& pivot, double scaleX, double scaleY) {
    return translation(pivot) * scaling(scaleX, scaleY) * translation(-pivot);
}

Matrix3 Matrix3::operator*(const Matrix3& other) const {
    Matrix3 result;
    // result[row][column] = dot(row of this, column of other)
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            double sum = 0.0;
            for (int index = 0; index < 3; ++index) sum += elements[row][index] * other.elements[index][column];
            result.elements[row][column] = sum;
        }
    }
    return result;
}

Vec2 Matrix3::transformPoint(const Vec2& point) const {
    // Multiply (x, y, 1). For the affine matrices used here the bottom row is
    // (0, 0, 1), so w stays 1 and no division is needed.
    return {elements[0][0] * point.x + elements[0][1] * point.y + elements[0][2],
            elements[1][0] * point.x + elements[1][1] * point.y + elements[1][2]};
}

Vec2 Matrix3::transformVector(const Vec2& vector) const {
    // Multiply (x, y, 0): the translation column is multiplied by 0.
    return {elements[0][0] * vector.x + elements[0][1] * vector.y, elements[1][0] * vector.x + elements[1][1] * vector.y};
}

std::vector<Vec2> Matrix3::transformPoints(const std::vector<Vec2>& points) const {
    std::vector<Vec2> transformed;
    transformed.reserve(points.size());
    for (const Vec2& point : points) transformed.push_back(transformPoint(point));
    return transformed;
}
