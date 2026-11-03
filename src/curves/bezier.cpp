/**
 * @file bezier.cpp
 * @brief Bezier curve evaluation and sampling.
 */
#include "curves/bezier.h"

#include <algorithm>

Vec2 evaluateCubicBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t) {
    const double u = 1.0 - t;
    // Cubic Bernstein basis: (1-t)^3, 3t(1-t)^2, 3t^2(1-t), t^3 - they always sum to 1.
    const double basis0 = u * u * u;
    const double basis1 = 3.0 * t * u * u;
    const double basis2 = 3.0 * t * t * u;
    const double basis3 = t * t * t;
    return p0 * basis0 + p1 * basis1 + p2 * basis2 + p3 * basis3;
}

Vec2 evaluateCubicBezierTangent(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t) {
    const double u = 1.0 - t;
    // B'(t) = 3(1-t)^2 (P1-P0) + 6t(1-t) (P2-P1) + 3t^2 (P3-P2)
    return (p1 - p0) * (3.0 * u * u) + (p2 - p1) * (6.0 * t * u) + (p3 - p2) * (3.0 * t * t);
}

Vec2 evaluateBezierDeCasteljau(std::vector<Vec2> controlPoints, double t) {
    if (controlPoints.empty()) return Vec2();
    // Each pass replaces n points by n-1 interpolated points; after n-1 passes one remains.
    for (std::size_t level = controlPoints.size() - 1; level > 0; --level) {
        for (std::size_t index = 0; index < level; ++index) {
            controlPoints[index] = lerpVec2(controlPoints[index], controlPoints[index + 1], t);
        }
    }
    return controlPoints.front();
}

std::vector<Vec2> sampleCubicBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3,
                                    int segmentCount) {
    segmentCount = std::max(segmentCount, 1);
    std::vector<Vec2> samples;
    samples.reserve(static_cast<std::size_t>(segmentCount) + 1);
    for (int step = 0; step <= segmentCount; ++step) {
        samples.push_back(evaluateCubicBezier(p0, p1, p2, p3, static_cast<double>(step) / segmentCount));
    }
    return samples;
}

std::vector<Vec2> sampleBezierCurve(const std::vector<Vec2>& controlPoints, int segmentCount) {
    segmentCount = std::max(segmentCount, 1);
    std::vector<Vec2> samples;
    samples.reserve(static_cast<std::size_t>(segmentCount) + 1);
    for (int step = 0; step <= segmentCount; ++step) {
        samples.push_back(evaluateBezierDeCasteljau(controlPoints, static_cast<double>(step) / segmentCount));
    }
    return samples;
}
