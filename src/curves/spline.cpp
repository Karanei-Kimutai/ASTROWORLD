/**
 * @file spline.cpp
 * @brief Catmull-Rom spline evaluation and sampling.
 */
#include "curves/spline.h"

#include <algorithm>

Vec2 evaluateCatmullRom(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t) {
    const double tSquared = t * t;
    const double tCubed = tSquared * t;
    return (p1 * 2.0 + (p2 - p0) * t + (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * tSquared +
            (p1 * 3.0 - p0 - p2 * 3.0 + p3) * tCubed) *
           0.5;
}

Vec2 evaluateCatmullRomTangent(const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, double t) {
    // Derivative of the polynomial above with respect to t.
    return ((p2 - p0) + (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * (2.0 * t) +
            (p1 * 3.0 - p0 - p2 * 3.0 + p3) * (3.0 * t * t)) *
           0.5;
}

std::vector<Vec2> sampleCatmullRomSpline(const std::vector<Vec2>& points, int samplesPerSegment, bool closed) {
    const int pointCount = static_cast<int>(points.size());
    if (pointCount < 2) return points;
    samplesPerSegment = std::max(samplesPerSegment, 1);

    // Neighbour lookup: closed curves wrap around, open curves clamp to the ends.
    auto pointAt = [&](int index) -> const Vec2& {
        if (closed) return points[((index % pointCount) + pointCount) % pointCount];
        return points[std::clamp(index, 0, pointCount - 1)];
    };

    const int segmentCount = closed ? pointCount : pointCount - 1;
    std::vector<Vec2> samples;
    samples.reserve(static_cast<std::size_t>(segmentCount * samplesPerSegment) + 1);
    for (int segment = 0; segment < segmentCount; ++segment) {
        const Vec2& p0 = pointAt(segment - 1);
        const Vec2& p1 = pointAt(segment);
        const Vec2& p2 = pointAt(segment + 1);
        const Vec2& p3 = pointAt(segment + 2);
        for (int step = 0; step < samplesPerSegment; ++step) {
            samples.push_back(evaluateCatmullRom(p0, p1, p2, p3, static_cast<double>(step) / samplesPerSegment));
        }
    }
    // Finish exactly on the last point (open) or back on the first (closed).
    samples.push_back(closed ? samples.front() : points.back());
    return samples;
}

std::vector<Vec3> sampleCatmullRomSpline3D(const std::vector<Vec3>& points, int samplesPerSegment, bool closed) {
    // Evaluate the 2D spline twice - once on (x, y) and once on (z, 0) - since
    // Catmull-Rom works on each coordinate independently.
    std::vector<Vec2> xyPoints;
    std::vector<Vec2> zPoints;
    for (const Vec3& point : points) {
        xyPoints.push_back({point.x, point.y});
        zPoints.push_back({point.z, 0.0});
    }
    const std::vector<Vec2> xySamples = sampleCatmullRomSpline(xyPoints, samplesPerSegment, closed);
    const std::vector<Vec2> zSamples = sampleCatmullRomSpline(zPoints, samplesPerSegment, closed);
    std::vector<Vec3> samples;
    samples.reserve(xySamples.size());
    for (std::size_t index = 0; index < xySamples.size(); ++index) {
        samples.push_back({xySamples[index].x, xySamples[index].y, zSamples[index].x});
    }
    return samples;
}
