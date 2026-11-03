/**
 * @file matrix4.h
 * @brief 4x4 matrices for 3D transformations and projections in homogeneous coordinates.
 *
 * A 3D point (x, y, z) becomes (x, y, z, 1). As in 2D, the extra coordinate
 * makes translation a matrix product, so a whole chain such as
 *
 *     projection * view * model
 *
 * collapses into ONE 4x4 matrix that is applied to every vertex - exactly what
 * the vertex-processing stage of the pipeline (and an OpenGL vertex shader) does.
 *
 * After a PERSPECTIVE projection w is no longer 1: dividing x, y, z by w (the
 * "perspective divide") makes distant points shrink toward the vanishing point.
 */
#pragma once

#include "math/vec3.h"

class Matrix4 {
public:
    Matrix4();  // identity

    static Matrix4 identity() { return Matrix4(); }
    static Matrix4 translation(const Vec3& offset);
    static Matrix4 scaling(const Vec3& factors);

    /** @brief Rotations about the coordinate axes (right-handed, counter-clockwise looking down the axis). */
    static Matrix4 rotationX(double radians);
    static Matrix4 rotationY(double radians);
    static Matrix4 rotationZ(double radians);

    /**
     * @brief Orthographic (parallel) projection of the box [left,right]x[bottom,top]x[-near,-far]
     *        onto normalised device coordinates [-1, 1]^3. Parallel lines stay parallel.
     */
    static Matrix4 orthographic(double left, double right, double bottom, double top, double nearPlane, double farPlane);

    /**
     * @brief Perspective projection with vertical field of view fovYRadians (OpenGL convention).
     *        The camera looks down -z; the centre of projection is the origin.
     */
    static Matrix4 perspective(double fovYRadians, double aspectRatio, double nearPlane, double farPlane);

    /**
     * @brief The VIEW matrix of a camera at 'eye' looking at 'target'.
     *
     * Builds the camera's own axes - forward f = normalize(target - eye),
     * right r = normalize(f x up), true up u = r x f - and returns the matrix
     * that moves the world so the camera sits at the origin looking down -z.
     */
    static Matrix4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);

    /**
     * @brief A rotation whose columns are the given axes (plus a translation).
     *        Maps local x/y/z onto xAxis/yAxis/zAxis - used to orient an object
     *        along a direction, e.g. a coaster car along the track tangent.
     */
    static Matrix4 fromBasis(const Vec3& xAxis, const Vec3& yAxis, const Vec3& zAxis, const Vec3& origin);

    Matrix4 operator*(const Matrix4& other) const;

    /** @brief General inverse by Gauss-Jordan elimination (identity if the matrix is singular). */
    Matrix4 inverse() const;

    /** @brief Rows and columns swapped. inverse().transposed() transforms NORMALS correctly. */
    Matrix4 transposed() const;

    /**
     * @brief Copy into 16 floats in COLUMN-MAJOR order, the layout OpenGL expects
     *        (our elements are stored row-major, so this is also a transpose).
     */
    void toColumnMajor(float output[16]) const;

    /** @brief Transform a point (w = 1) and divide by the resulting w. */
    Vec3 transformPoint(const Vec3& point) const;

    /** @brief Transform a direction (w = 0): ignores translation, no divide. */
    Vec3 transformDirection(const Vec3& direction) const;

    double at(int row, int column) const { return elements[row][column]; }

private:
    double elements[4][4];
};
