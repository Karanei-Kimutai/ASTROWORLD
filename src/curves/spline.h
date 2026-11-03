/**
 * @file spline.h
 * @brief Catmull-Rom splines - piecewise cubic curves that pass THROUGH their points.
 *
 * A spline joins many low-degree polynomial pieces end to end. Each Catmull-Rom
 * piece between P1 and P2 is shaped only by its four neighbours P0..P3, so
 * moving one point only changes the nearby pieces (LOCAL control). That makes
 * it ideal for long paths like the roller-coaster track, the head silhouette
 * and the hairline.
 *
 * The curve is C1-continuous: the tangent at each interior point is
 * (P[i+1] - P[i-1]) / 2, shared by both pieces that meet there.
 */
#pragma once

#include <vector>

#include "core/vec2.h"
#include "math/vec3.h"

/**
 * @brief Evaluate one Catmull-Rom segment between p1 (t = 0) and p2 (t = 1).
 *
 * Matrix form: 0.5 * [1 t t^2 t^3] * M * [P0 P1 P2 P3]^T with
 *   M = [ 0  2  0  0 ; -1  0  1  0 ; 2 -5  4 -1 ; -1  3 -3  1 ].
 */
Vec2 evaluateCatmullRom(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t);

/** @brief Tangent (first derivative) of a Catmull-Rom segment at t. */
Vec2 evaluateCatmullRomTangent(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t);

/**
 * @brief Sample a whole Catmull-Rom spline through the given points.
 *
 * @param closed If true the curve loops back to the first point (used for
 *        closed outlines such as the head); otherwise the end points are
 *        duplicated so the curve starts and ends exactly on them.
 */
std::vector<Vec2> sampleCatmullRomSpline(const std::vector<Vec2>& points, int samplesPerSegment, bool closed);

/**
 * @brief The same Catmull-Rom spline through 3D points (each axis is an
 *        independent cubic), used for the 3D coaster track and the dreads in OpenGL.
 */
std::vector<Vec3> sampleCatmullRomSpline3D(const std::vector<Vec3>& points, int samplesPerSegment, bool closed);
