/**
 * @file matrix4.cpp
 * @brief 3D homogeneous transformation and projection matrices.
 */
#include "math/matrix4.h"

#include <cmath>

Matrix4::Matrix4()
    : elements{{1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0}} {}

Matrix4 Matrix4::translation(const Vec3& offset) {
    Matrix4 result;
    result.elements[0][3] = offset.x;
    result.elements[1][3] = offset.y;
    result.elements[2][3] = offset.z;
    return result;
}

Matrix4 Matrix4::scaling(const Vec3& factors) {
    Matrix4 result;
    result.elements[0][0] = factors.x;
    result.elements[1][1] = factors.y;
    result.elements[2][2] = factors.z;
    return result;
}

Matrix4 Matrix4::rotationX(double radians) {
    // y and z rotate; x is the axis and stays fixed.
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    Matrix4 result;
    result.elements[1][1] = cosine;
    result.elements[1][2] = -sine;
    result.elements[2][1] = sine;
    result.elements[2][2] = cosine;
    return result;
}

Matrix4 Matrix4::rotationY(double radians) {
    // z and x rotate; note the sign placement differs because of the cyclic order (z, x).
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    Matrix4 result;
    result.elements[0][0] = cosine;
    result.elements[0][2] = sine;
    result.elements[2][0] = -sine;
    result.elements[2][2] = cosine;
    return result;
}

Matrix4 Matrix4::rotationZ(double radians) {
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    Matrix4 result;
    result.elements[0][0] = cosine;
    result.elements[0][1] = -sine;
    result.elements[1][0] = sine;
    result.elements[1][1] = cosine;
    return result;
}

Matrix4 Matrix4::orthographic(double left, double right, double bottom, double top, double nearPlane, double farPlane) {
    // Scale the box to size 2 and translate its centre to the origin.
    Matrix4 result;
    result.elements[0][0] = 2.0 / (right - left);
    result.elements[1][1] = 2.0 / (top - bottom);
    result.elements[2][2] = -2.0 / (farPlane - nearPlane);
    result.elements[0][3] = -(right + left) / (right - left);
    result.elements[1][3] = -(top + bottom) / (top - bottom);
    result.elements[2][3] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    return result;
}

Matrix4 Matrix4::perspective(double fovYRadians, double aspectRatio, double nearPlane, double farPlane) {
    // focal = cot(fov / 2): how strongly x and y are scaled at depth 1.
    const double focal = 1.0 / std::tan(fovYRadians * 0.5);
    Matrix4 result;
    result.elements[0][0] = focal / aspectRatio;
    result.elements[1][1] = focal;
    result.elements[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result.elements[2][3] = 2.0 * farPlane * nearPlane / (nearPlane - farPlane);
    // Bottom row (0, 0, -1, 0) copies -z into w, so the later divide by w scales by 1/distance.
    result.elements[3][2] = -1.0;
    result.elements[3][3] = 0.0;
    return result;
}

Matrix4 Matrix4::operator*(const Matrix4& other) const {
    Matrix4 result;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            double sum = 0.0;
            for (int index = 0; index < 4; ++index) sum += elements[row][index] * other.elements[index][column];
            result.elements[row][column] = sum;
        }
    }
    return result;
}

Vec3 Matrix4::transformPoint(const Vec3& point) const {
    const double x = elements[0][0] * point.x + elements[0][1] * point.y + elements[0][2] * point.z + elements[0][3];
    const double y = elements[1][0] * point.x + elements[1][1] * point.y + elements[1][2] * point.z + elements[1][3];
    const double z = elements[2][0] * point.x + elements[2][1] * point.y + elements[2][2] * point.z + elements[2][3];
    const double w = elements[3][0] * point.x + elements[3][1] * point.y + elements[3][2] * point.z + elements[3][3];
    // Perspective divide (w is 1 for affine matrices, so this is free for them).
    const double inverseW = std::fabs(w) > 1e-12 ? 1.0 / w : 1.0;
    return {x * inverseW, y * inverseW, z * inverseW};
}

Vec3 Matrix4::transformDirection(const Vec3& direction) const {
    return {elements[0][0] * direction.x + elements[0][1] * direction.y + elements[0][2] * direction.z,
            elements[1][0] * direction.x + elements[1][1] * direction.y + elements[1][2] * direction.z,
            elements[2][0] * direction.x + elements[2][1] * direction.y + elements[2][2] * direction.z};
}
