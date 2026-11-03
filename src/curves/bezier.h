/**
 * @file bezier.h
 * @brief Bezier curves - parametric curves shaped by control points.
 *
 * A Bezier curve of degree n is B(t) = sum_i C(n,i) (1-t)^(n-i) t^i P_i, t in [0, 1].
 * It passes through the first and last control points; the inner points
 * "pull" the curve toward them without being touched. Moving any control
 * point changes the WHOLE curve (global control) - contrast with splines.
 */
#pragma once

#include <vector>

#include "core/vec2.h"

/** @brief Cubic Bezier evaluated directly with the Bernstein polynomials. */
Vec2 evaluateCubicBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t);

/** @brief First derivative (tangent direction) of a cubic Bezier at t. */
Vec2 evaluateCubicBezierTangent(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t);

/**
 * @brief Evaluate a Bezier curve of ANY degree with de Casteljau's algorithm.
 *
 * Repeatedly linearly interpolates neighbouring control points at parameter t
 * until a single point remains. Numerically stable and the geometric
 * construction shown in lectures.
 */
Vec2 evaluateBezierDeCasteljau(std::vector<Vec2> controlPoints, double t);

/** @brief segmentCount + 1 evenly spaced (in t) points along a cubic Bezier. */
std::vector<Vec2> sampleCubicBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                                    int segmentCount);

/** @brief segmentCount + 1 points along a Bezier of any degree (de Casteljau). */
std::vector<Vec2> sampleBezierCurve(const std::vector<Vec2>& controlPoints, int segmentCount);
