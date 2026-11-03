/**
 * @file rollerCoaster.cpp
 * @brief Roller coaster drawing: supports, rails, ties and an arc-length-placed train.
 */
#include "scene/rollerCoaster.h"

#include <algorithm>
#include <cmath>

#include "animation/easing.h"
#include "curves/spline.h"
#include "fill/scanLineFill.h"
#include "math/matrix3.h"
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
        // Cars trailing behind the start of the track have not entered the ride yet.
        const double carDistance = trainDistance * cumulativeLength.back() - car * carSpacing;
        if (carDistance < 0.0) continue;
        Vec2 trackPoint;
        Vec2 tangent;
        findTrackFrame(track, cumulativeLength, carDistance, trackPoint, tangent);

        // Car model matrix: rotate the car (built level, in local coordinates) to the
        // track's slope, then translate it onto the track: model = T(trackPoint) * R(slope).
        const double slope = std::atan2(tangent.y, tangent.x);
        const Matrix3 carModel = Matrix3::translation(trackPoint) * Matrix3::rotation(slope);

        // Local car: x along the track, -y is "up" off the rail.
        static const std::vector<Vec2> localBody = {{-10.0, -14.0}, {10.0, -14.0}, {10.0, -2.0}, {-10.0, -2.0}};
        const std::vector<Vec2> body = carModel.transformPoints(localBody);
        fillPolygon(frameBuffer, body, hsvToRgb(190.0 + car * 40.0, 0.8, 1.0));
        drawPolylineBresenham(frameBuffer, body, astroPalette::ink, true);

        // Rider's head poking up, and two wheels on the rail.
        const Vec2 riderHead = carModel.transformPoint({0.0, -17.0});
        fillCircle(frameBuffer, roundToInt(riderHead.x), roundToInt(riderHead.y), 3, astroPalette::crowd);
        for (double wheelOffset : {-6.0, 6.0}) {
            const Vec2 wheel = carModel.transformPoint({wheelOffset, -2.0});
            fillCircle(frameBuffer, roundToInt(wheel.x), roundToInt(wheel.y), 2, astroPalette::ink);
        }
    }
}

double RollerCoaster::trainFractionAtTime(double timeSeconds) const {
    const std::vector<Vec2> track = sampleCatmullRomSpline(controlPoints, 20, false);
    const std::size_t sampleCount = track.size();
    if (sampleCount < 2) return 0.0;

    // The chain lift runs from the start up to the first summit (highest point
    // before the track first turns downward).
    std::size_t summitIndex = 0;
    while (summitIndex + 1 < sampleCount && track[summitIndex + 1].y <= track[summitIndex].y) ++summitIndex;
    const double summitHeight = groundY - track[summitIndex].y;

    // Time to travel each segment: dt = ds / v. After the summit the speed comes from
    // ENERGY CONSERVATION: 1/2 v^2 + g h = constant  =>  v = sqrt(v0^2 + 2 g (hSummit - h)).
    const double liftSpeed = 70.0;       // px/s, chain lift
    const double summitSpeed = 45.0;     // px/s as the train tips over the top
    const double gravity = 320.0;        // px/s^2
    std::vector<double> cumulativeTime(sampleCount, 0.0);
    std::vector<double> cumulativeLength(sampleCount, 0.0);
    for (std::size_t index = 1; index < sampleCount; ++index) {
        const double segmentLength = (track[index] - track[index - 1]).length();
        double speed = liftSpeed;
        if (index > summitIndex) {
            const double height = groundY - (track[index].y + track[index - 1].y) * 0.5;
            speed = std::sqrt(std::max(summitSpeed * summitSpeed + 2.0 * gravity * (summitHeight - height), 1.0));
        }
        cumulativeTime[index] = cumulativeTime[index - 1] + segmentLength / speed;
        cumulativeLength[index] = cumulativeLength[index - 1] + segmentLength;
    }

    // Loop: after the end of the track the train re-enters at the start. Look up the
    // distance travelled at this time by interpolating the time -> distance table.
    const double loopTime = wrapTime(timeSeconds, cumulativeTime.back());
    const auto upper = std::upper_bound(cumulativeTime.begin(), cumulativeTime.end(), loopTime);
    const std::size_t index = std::clamp<std::size_t>(static_cast<std::size_t>(upper - cumulativeTime.begin()), 1, sampleCount - 1);
    const double segmentTime = cumulativeTime[index] - cumulativeTime[index - 1];
    const double t = segmentTime > 1e-9 ? (loopTime - cumulativeTime[index - 1]) / segmentTime : 0.0;
    const double distance = cumulativeLength[index - 1] + t * (cumulativeLength[index] - cumulativeLength[index - 1]);
    return distance / cumulativeLength.back();
}
