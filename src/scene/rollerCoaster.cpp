/**
 * @file rollerCoaster.cpp
 * @brief Roller coaster drawing: supports, rails, ties and an arc-length-placed train.
 */
#include "scene/rollerCoaster.h"

#include <algorithm>
#include <cmath>

#include "curves/spline.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"

namespace {

/**
 * @brief Find the point and unit tangent at a given distance along a polyline.
 *
 * Placing cars by ARC LENGTH (not by curve parameter t) keeps them evenly
 * spaced even where the spline samples bunch up on tight curves.
 */
void findTrackFrame(const std::vector<Vec2>& track, const std::vector<double>& cumulativeLength, double distance,
                    Vec2& pointOut, Vec2& tangentOut) {
    distance = std::clamp(distance, 0.0, cumulativeLength.back());
    // First sample whose cumulative length reaches the requested distance.
    const auto upper = std::lower_bound(cumulativeLength.begin(), cumulativeLength.end(), distance);
    std::size_t index = static_cast<std::size_t>(std::max<std::ptrdiff_t>(1, upper - cumulativeLength.begin()));
    index = std::min(index, track.size() - 1);

    const double segmentLength = cumulativeLength[index] - cumulativeLength[index - 1];
    const double t = segmentLength > 1e-9 ? (distance - cumulativeLength[index - 1]) / segmentLength : 0.0;
    pointOut = lerpVec2(track[index - 1], track[index], t);
    tangentOut = (track[index] - track[index - 1]).normalized();
}

}  // namespace

void RollerCoaster::draw(FrameBuffer& frameBuffer) const {
    const std::vector<Vec2> track = sampleCatmullRomSpline(controlPoints, 20, false);
    const std::size_t sampleCount = track.size();
    if (sampleCount < 2) return;

    // Per-sample normals from central differences. perpendicular() of a
    // left-to-right tangent points DOWN on screen, so "up" is -normal.
    std::vector<Vec2> normals(sampleCount);
    for (std::size_t index = 0; index < sampleCount; ++index) {
        const Vec2& previous = track[index == 0 ? 0 : index - 1];
        const Vec2& next = track[std::min(index + 1, sampleCount - 1)];
        normals[index] = (next - previous).normalized().perpendicular();
    }

    // ---- Supports: slim teal posts with X-bracing between neighbours ------------
    const std::size_t supportStep = 6;
    for (std::size_t index = 0; index < sampleCount; index += supportStep) {
        const Vec2 bottomChordPoint = track[index] + normals[index] * 12.0;
        if (bottomChordPoint.y > groundY - 8.0) continue;  // track is already at ground level here
        drawThickLine(frameBuffer, bottomChordPoint, {bottomChordPoint.x, groundY}, 1.2, astroPalette::coasterTealDark);

        const std::size_t nextIndex = index + supportStep;
        if (nextIndex < sampleCount) {
            const Vec2 nextPoint = track[nextIndex] + normals[nextIndex] * 12.0;
            if (nextPoint.y > groundY - 8.0) continue;
            const double midY = (bottomChordPoint.y + groundY) * 0.5;
            const double nextMidY = (nextPoint.y + groundY) * 0.5;
            drawLineBresenham(frameBuffer, {bottomChordPoint.x, midY}, {nextPoint.x, groundY}, astroPalette::coasterTealDark);
            drawLineBresenham(frameBuffer, {bottomChordPoint.x, groundY}, {nextPoint.x, nextMidY}, astroPalette::coasterTealDark);
        }
    }

    // ---- Lattice truss track (as on the night album cover) ----------------------
    // Top chord = the spline itself, bottom chord = the spline offset 12 px along
    // its normal. Bresenham zig-zags between the chords form the truss web.
    std::vector<Vec2> bottomChord(sampleCount);
    for (std::size_t index = 0; index < sampleCount; ++index) bottomChord[index] = track[index] + normals[index] * 12.0;
    for (std::size_t index = 0; index + 2 < sampleCount; index += 2) {
        drawLineBresenham(frameBuffer, track[index], bottomChord[index + 2], astroPalette::coasterTeal);  // diagonal
        drawLineBresenham(frameBuffer, track[index], bottomChord[index], astroPalette::coasterTeal);      // vertical
    }
    drawThickPolyline(frameBuffer, bottomChord, 1.4, astroPalette::coasterTeal);
    drawThickPolyline(frameBuffer, track, 2.2, astroPalette::coasterTeal);
    // Specular glint running along the top rail.
    std::vector<Vec2> glint(sampleCount);
    for (std::size_t index = 0; index < sampleCount; ++index) glint[index] = track[index] - normals[index] * 1.0;
    drawPolylineBresenham(frameBuffer, glint, makeTint(astroPalette::coasterTeal, 0.5));

    // ---- Train: each car is a rectangle aligned to the track tangent ---------
    std::vector<double> cumulativeLength(sampleCount, 0.0);
    for (std::size_t index = 1; index < sampleCount; ++index) {
        cumulativeLength[index] = cumulativeLength[index - 1] + (track[index] - track[index - 1]).length();
    }
    const double carSpacing = 24.0;
    for (int car = 0; car < carCount; ++car) {
        Vec2 trackPoint;
        Vec2 tangent;
        findTrackFrame(track, cumulativeLength, trainDistance * cumulativeLength.back() - car * carSpacing, trackPoint,
                       tangent);
        const Vec2 up = tangent.perpendicular() * -1.0;
        const Vec2 carCenter = trackPoint + up * 8.0;

        // Corners = centre +- half-length along the tangent +- half-height along "up".
        const std::vector<Vec2> body = {carCenter - tangent * 10.0 - up * 6.0, carCenter + tangent * 10.0 - up * 6.0,
                                        carCenter + tangent * 10.0 + up * 6.0, carCenter - tangent * 10.0 + up * 6.0};
        fillPolygon(frameBuffer, body, hsvToRgb(190.0 + car * 40.0, 0.8, 1.0));
        drawPolylineBresenham(frameBuffer, body, astroPalette::ink, true);

        // Riders' heads poking up, and two wheels on the rail.
        const Vec2 riderHead = carCenter + up * 9.0;
        fillCircle(frameBuffer, roundToInt(riderHead.x), roundToInt(riderHead.y), 3, astroPalette::crowd);
        for (double wheelOffset : {-6.0, 6.0}) {
            const Vec2 wheel = carCenter - up * 6.0 + tangent * wheelOffset;
            fillCircle(frameBuffer, roundToInt(wheel.x), roundToInt(wheel.y), 2, astroPalette::ink);
        }
    }
}
