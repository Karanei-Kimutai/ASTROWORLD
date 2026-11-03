/**
 * @file parkModels.cpp
 * @brief Procedural 3D models of the rides and props.
 */
#include "glScene/parkModels.h"

#include <algorithm>
#include <cmath>

#include "animation/astroworldTimeline.h"
#include "curves/bezier.h"
#include "curves/spline.h"
#include "mesh/meshBuilder.h"

namespace {

/** @brief Stripe colouriser alternating through a list of colours. */
SegmentColorizer stripes(std::vector<Color> colors) {
    return [colors](int segment) { return colors[static_cast<std::size_t>(segment) % colors.size()]; };
}

const Color tentGreen{30, 140, 112};
const Color tentBlue{36, 74, 164};
const Color tentYellow{232, 200, 64};
const Color towerLilac{150, 120, 190};

}  // namespace

// ---------------------------------------------------------------------------
// Ferris wheel
// ---------------------------------------------------------------------------

MeshData buildFerrisStand() {
    MeshData stand;
    const Vec3 hub{0.0, ferrisHubHeight, 0.0};
    for (double side : {-1.0, 1.0}) {
        for (double depth : {-1.0, 1.0}) {
            stand.append(makeCylinderBetween(hub + Vec3(0, 0, depth * 0.75), {side * 3.4, 0.0, depth * 1.2}, 0.13, 10, astroPalette::steel));
        }
        stand.append(makeCylinderBetween({side * 1.4, 4.4, -0.95}, {side * 1.4, 4.4, 0.95}, 0.08, 8, astroPalette::steel));
    }
    stand.append(makeCylinderBetween({-1.4, 4.4, 0.95}, {1.4, 4.4, 0.95}, 0.08, 8, astroPalette::steel));
    stand.append(makeCylinderBetween({-1.4, 4.4, -0.95}, {1.4, 4.4, -0.95}, 0.08, 8, astroPalette::steel));
    return stand;
}

MeshData buildFerrisRotor() {
    MeshData rotor;
    for (double depth : {-0.5, 0.5}) {
        // Outer and inner rims (tori lie in the xy-plane, so the wheel turns about z).
        rotor.append(makeTorus(ferrisRadius, 0.08, 96, 8, astroPalette::neonPink), Matrix4::translation({0, 0, depth}));
        rotor.append(makeTorus(ferrisRadius * 0.62, 0.05, 72, 6, astroPalette::neonCyan), Matrix4::translation({0, 0, depth}));
        for (int spoke = 0; spoke < ferrisGondolaCount; ++spoke) {
            const double angle = 2.0 * pi * spoke / ferrisGondolaCount;
            rotor.append(makeCylinderBetween({0, 0, depth}, Vec3(std::cos(angle), std::sin(angle), 0) * ferrisRadius + Vec3(0, 0, depth),
                                             0.035, 6, astroPalette::steelLight));
        }
    }
    // Axles joining the two rims at each gondola mount, and the hub.
    for (int spoke = 0; spoke < ferrisGondolaCount; ++spoke) {
        const double angle = 2.0 * pi * spoke / ferrisGondolaCount;
        const Vec3 mount = Vec3(std::cos(angle), std::sin(angle), 0) * ferrisRadius;
        rotor.append(makeCylinderBetween(mount + Vec3(0, 0, -0.55), mount + Vec3(0, 0, 0.55), 0.05, 6, astroPalette::steelLight));
    }
    rotor.append(makeCylinderBetween({0, 0, -0.8}, {0, 0, 0.8}, 0.35, 20, astroPalette::gold));
    return rotor;
}

MeshData buildFerrisBulbs() {
    MeshData bulbs;
    const int bulbCount = ferrisGondolaCount * 3;
    for (int bulb = 0; bulb < bulbCount; ++bulb) {
        const double angle = 2.0 * pi * bulb / bulbCount;
        const Color color = bulb % 2 == 0 ? astroPalette::goldLight : astroPalette::neonCyan;
        bulbs.append(makeEllipsoid({0.09, 0.09, 0.09}, 8, 6, color),
                     Matrix4::translation(Vec3(std::cos(angle), std::sin(angle), 0) * ferrisRadius + Vec3(0, 0, 0.6)));
    }
    return bulbs;
}

MeshData buildGondola() {
    // Built light grey; the renderer tints each gondola a different hue.
    MeshData gondola;
    gondola.append(makeCylinderBetween({0, 0, 0}, {0, -0.55, 0}, 0.04, 6, Color{200, 200, 200}));
    gondola.append(makeBox({0.95, 0.65, 0.85}, Color{235, 235, 235}), Matrix4::translation({0, -0.95, 0}));
    gondola.append(makeBox({0.8, 0.25, 0.86}, Color{70, 70, 80}), Matrix4::translation({0, -0.85, 0}));  // window band
    gondola.append(makeEllipsoid({0.55, 0.18, 0.5}, 16, 8, Color{150, 150, 150}), Matrix4::translation({0, -0.6, 0}));
    return gondola;
}

// ---------------------------------------------------------------------------
// Roller coaster
// ---------------------------------------------------------------------------

std::vector<Vec3> buildCoasterTrackPath() {
    // The SAME side profile the 2D renderer uses, mapped into the world:
    // pixel x -> world x, pixel height above the ground line -> world y, and the
    // track recedes in z as it runs right.
    std::vector<Vec3> controlPoints;
    for (const Vec2& point : coasterProfilePoints()) {
        const double x = (point.x - 640.0) / 32.0 + 1.0;
        const double height = (coasterProfileGroundY - point.y) / 32.0 * 1.25 + 0.3;
        const double z = -12.0 - (point.x - 830.0) / 470.0 * 6.0;
        controlPoints.push_back({x, height, z});
    }
    return sampleCatmullRomSpline3D(controlPoints, 24, false);
}

namespace {

/** @brief Tangent, lateral and up axes of the track at sample index. */
void trackFrame(const std::vector<Vec3>& path, std::size_t index, Vec3& tangent, Vec3& lateral, Vec3& up) {
    const Vec3& previous = path[index == 0 ? 0 : index - 1];
    const Vec3& next = path[std::min(index + 1, path.size() - 1)];
    tangent = (next - previous).normalized();
    lateral = crossProduct(tangent, {0.0, 1.0, 0.0}).normalized();
    up = crossProduct(lateral, tangent);
}

}  // namespace

MeshData buildCoasterTrack(const std::vector<Vec3>& path) {
    std::vector<Vec3> leftRail, rightRail, spine;
    for (std::size_t index = 0; index < path.size(); ++index) {
        Vec3 tangent, lateral, up;
        trackFrame(path, index, tangent, lateral, up);
        leftRail.push_back(path[index] - lateral * 0.38);
        rightRail.push_back(path[index] + lateral * 0.38);
        spine.push_back(path[index] - up * 0.6);
    }
    MeshData track;
    track.append(makeTube(leftRail, 0.07, 0.07, 8, astroPalette::coasterTeal));
    track.append(makeTube(rightRail, 0.07, 0.07, 8, astroPalette::coasterTeal));
    track.append(makeTube(spine, 0.11, 0.11, 8, astroPalette::coasterTealDark));
    // Support columns down to the ground.
    for (std::size_t index = 4; index < spine.size(); index += 9) {
        if (spine[index].y < 0.8) continue;
        track.append(makeCylinderBetween(spine[index], {spine[index].x, 0.0, spine[index].z}, 0.1, 8, astroPalette::coasterTealDark));
    }
    return track;
}

MeshData buildCoasterTruss(const std::vector<Vec3>& path) {
    // Lattice web between the two rails and the spine (as on the night album cover).
    std::vector<Vec3> endpoints;
    for (std::size_t index = 0; index + 2 < path.size(); index += 2) {
        Vec3 tangent, lateral, up;
        trackFrame(path, index, tangent, lateral, up);
        const Vec3 left = path[index] - lateral * 0.38;
        const Vec3 right = path[index] + lateral * 0.38;
        const Vec3 spine = path[index] - up * 0.6;
        Vec3 nextTangent, nextLateral, nextUp;
        trackFrame(path, index + 2, nextTangent, nextLateral, nextUp);
        const Vec3 nextLeft = path[index + 2] - nextLateral * 0.38;
        const Vec3 nextRight = path[index + 2] + nextLateral * 0.38;
        endpoints.insert(endpoints.end(), {left, right, left, spine, right, spine, spine, nextLeft, spine, nextRight});
    }
    return makeLines(endpoints, astroPalette::coasterTeal);
}

MeshData buildCoasterCar() {
    MeshData car;
    car.append(makeBox({1.15, 0.45, 0.8}, Color{240, 240, 245}), Matrix4::translation({0, 0.4, 0}));
    car.append(makeBox({0.3, 0.55, 0.82}, Color{200, 200, 210}), Matrix4::translation({0.45, 0.5, 0}));  // front fairing
    for (double seat : {-0.25, 0.2}) {
        car.append(makeEllipsoid({0.14, 0.16, 0.14}, 12, 8, Color{90, 60, 45}), Matrix4::translation({seat, 0.85, 0.18}));
        car.append(makeEllipsoid({0.14, 0.16, 0.14}, 12, 8, Color{60, 40, 30}), Matrix4::translation({seat, 0.85, -0.18}));
    }
    return car;
}

// ---------------------------------------------------------------------------
// Drop tower
// ---------------------------------------------------------------------------

MeshData buildDropTowerColumn() {
    MeshData tower;
    tower.append(makeCylinder(0.45, dropTowerHeight, 20, towerLilac));
    tower.append(makeCone(0.95, 1.5, 20, astroPalette::gold), Matrix4::translation({0, dropTowerHeight, 0}));
    tower.append(makeCylinderBetween({0, dropTowerHeight + 1.4, 0}, {0, dropTowerHeight + 2.3, 0}, 0.04, 6, astroPalette::steelLight));
    tower.append(makeCylinder(1.3, 0.25, 24, astroPalette::steelDark));  // footing
    return tower;
}

MeshData buildDropTowerLights() {
    MeshData lights;
    int index = 0;
    for (double height = 0.8; height < dropTowerHeight - 0.4; height += 0.75, ++index) {
        lights.append(makeEllipsoid({0.09, 0.09, 0.09}, 8, 6, index % 2 == 0 ? astroPalette::neonPink : astroPalette::goldLight),
                      Matrix4::translation({0, height, 0.47}));
    }
    lights.append(makeEllipsoid({0.18, 0.18, 0.18}, 12, 8, Color{255, 60, 70}), Matrix4::translation({0, dropTowerHeight + 2.4, 0}));
    return lights;
}

MeshData buildDropTowerSeats() {
    MeshData seats;
    seats.append(makeTorus(1.25, 0.13, 40, 8, astroPalette::gold), Matrix4::rotationX(pi / 2.0));
    seats.append(makeTorus(1.25, 0.1, 40, 8, astroPalette::goldDark), Matrix4::translation({0, -0.55, 0}) * Matrix4::rotationX(pi / 2.0));
    const int seatCount = 12;
    for (int seat = 0; seat < seatCount; ++seat) {
        const double angle = 2.0 * pi * seat / seatCount;
        const Color color = hsvToRgb(280.0 + seat * 12.0, 0.75, 0.95);
        seats.append(makeBox({0.35, 0.55, 0.3}, color),
                     Matrix4::rotationY(angle) * Matrix4::translation({1.25, -0.28, 0}));
    }
    return seats;
}

// ---------------------------------------------------------------------------
// Carousel
// ---------------------------------------------------------------------------

MeshData buildCarouselBase() { return makeCylinder(2.95, 0.3, 40, astroPalette::steelDark); }

MeshData buildCarouselRotor() {
    MeshData rotor;
    rotor.append(makeCylinder(2.8, 0.12, 40, astroPalette::goldDark), Matrix4::translation({0, 0.3, 0}));
    rotor.append(makeCylinder(0.3, 3.0, 16, astroPalette::gold), Matrix4::translation({0, 0.42, 0}));
    // Striped canopy: a cone on top of a striped rim (separate vertices per stripe keep edges crisp).
    rotor.append(makeCone(3.15, 1.35, 16, stripes({astroPalette::neonPink, astroPalette::cream})), Matrix4::translation({0, 3.05, 0}));
    rotor.append(makeCylinder(3.17, 0.38, 24, stripes({astroPalette::gold, astroPalette::neonPurple}), false),
                 Matrix4::translation({0, 2.7, 0}));
    for (int pole = 0; pole < 8; ++pole) {
        const double angle = 2.0 * pi * pole / 8.0;
        const Vec3 base{std::cos(angle) * carouselHorseRadius, 0.42, -std::sin(angle) * carouselHorseRadius};
        rotor.append(makeCylinderBetween(base, base + Vec3(0, 2.3, 0), 0.045, 8, astroPalette::goldLight));
    }
    rotor.append(makeCylinderBetween({0, 4.4, 0}, {0, 5.1, 0}, 0.03, 6, astroPalette::goldLight));
    return rotor;
}

MeshData buildCarouselBulbs() {
    MeshData bulbs;
    for (int bulb = 0; bulb < 32; ++bulb) {
        const double angle = 2.0 * pi * bulb / 32.0;
        bulbs.append(makeEllipsoid({0.07, 0.07, 0.07}, 8, 6, astroPalette::goldLight),
                     Matrix4::translation({std::cos(angle) * 3.2, 2.68, -std::sin(angle) * 3.2}));
    }
    return bulbs;
}

MeshData buildCarouselHorse() {
    // Built white; the renderer tints alternate horses cream and pink. Faces -z.
    const Color coat{250, 250, 250};
    const Color mane{140, 120, 130};
    MeshData horse;
    horse.append(makeEllipsoid({0.17, 0.2, 0.42}, 16, 10, coat));
    horse.append(makeCylinderBetween({0, 0.1, -0.3}, {0, 0.42, -0.48}, 0.09, 10, coat));                 // neck
    horse.append(makeEllipsoid({0.1, 0.11, 0.2}, 12, 8, coat), Matrix4::translation({0, 0.47, -0.6}));     // head
    horse.append(makeCylinderBetween({0, 0.25, -0.32}, {0, 0.55, -0.5}, 0.04, 6, mane));                  // mane
    for (double side : {-0.09, 0.09}) {
        for (double end : {-0.28, 0.28}) {
            horse.append(makeCylinderBetween({side, -0.1, end}, {side, -0.5, end + (end < 0 ? -0.1 : 0.1)}, 0.035, 6, coat));
        }
    }
    horse.append(makeCylinderBetween({0, 0.05, 0.4}, {0, -0.3, 0.55}, 0.04, 6, mane));  // tail
    return horse;
}

// ---------------------------------------------------------------------------
// Props
// ---------------------------------------------------------------------------

MeshData buildRocket() {
    MeshData rocket;
    const Color silver = astroPalette::rocketSilver;
    const Color red = astroPalette::rocketRed;
    rocket.append(makeBox({1.7, 0.3, 1.7}, Color{90, 88, 95}), Matrix4::translation({0, 0.15, 0}));
    // Body as stacked cylinders: silver with two red bands.
    const double bands[][3] = {{0.3, 0.6, 0}, {0.9, 0.35, 1}, {1.25, 2.35, 0}, {3.6, 0.3, 1}, {3.9, 0.7, 0}};
    for (const auto& band : bands) {
        rocket.append(makeCylinder(0.55, band[1], 24, band[2] > 0 ? red : silver), Matrix4::translation({0, band[0], 0}));
    }
    rocket.append(makeCone(0.55, 1.35, 24, silver), Matrix4::translation({0, 4.6, 0}));
    rocket.append(makeCone(0.2, 0.5, 16, red), Matrix4::translation({0, 5.55, 0}));
    for (int fin = 0; fin < 3; ++fin) {
        rocket.append(makeBox({0.08, 1.3, 0.75}, red), Matrix4::rotationY(fin * 2.0 * pi / 3.0 + pi / 2.0) * Matrix4::translation({0, 0.95, 0.75}));
    }
    rocket.append(makeCylinderBetween({0, 3.05, 0.5}, {0, 3.05, 0.6}, 0.22, 16, Color{60, 110, 170}));  // porthole
    return rocket;
}

MeshData buildBigTop() {
    MeshData tent;
    tent.append(makeCylinder(3.4, 2.2, 18, stripes({tentGreen, tentBlue, tentYellow})));
    tent.append(makeCone(3.75, 3.0, 18, stripes({tentGreen, tentBlue, tentYellow})), Matrix4::translation({0, 2.2, 0}));
    tent.append(makeCylinderBetween({0, 5.1, 0}, {0, 6.0, 0}, 0.03, 6, Color{80, 80, 80}));
    MeshData flag;
    flag.vertices = {makeVertex({0, 6.0, 0}, {0, 0, 1}, astroPalette::rocketRed), makeVertex({0.6, 5.85, 0}, {0, 0, 1}, astroPalette::rocketRed),
                     makeVertex({0, 5.7, 0}, {0, 0, 1}, astroPalette::rocketRed)};
    flag.addTriangle(0, 2, 1);
    tent.append(flag);
    // Doorway.
    tent.append(makeBox({1.1, 1.6, 0.1}, Color{30, 20, 30}), Matrix4::translation({0, 0.8, 3.38}));
    return tent;
}

MeshData buildSignFrame() {
    MeshData frame;
    for (double side : {-2.4, 2.4}) frame.append(makeCylinderBetween({side, 0, 0}, {side, 5.4, 0}, 0.1, 8, Color{60, 60, 70}));
    frame.append(makeBox({6.2, 1.4, 0.2}, Color{44, 48, 70}), Matrix4::translation({0, 6.0, 0}));
    return frame;
}

MeshData buildSignFace() {
    MeshData face;
    face.append(makeTexturedQuad(5.9, 1.15, astroPalette::white), Matrix4::translation({0, 6.0, 0.11}));
    return face;
}

MeshData buildFence(double halfLength) {
    // A black palisade: thin pickets, thicker posts and two rails (all boxes merged into one mesh).
    MeshData fence;
    const Color black = astroPalette::fenceBlack;
    for (double x = -halfLength; x <= halfLength; x += 0.28) fence.append(makeBox({0.05, 1.6, 0.05}, black), Matrix4::translation({x, 0.8, 0}));
    for (double x = -halfLength; x <= halfLength; x += 6.0) fence.append(makeBox({0.14, 1.9, 0.14}, black), Matrix4::translation({x, 0.95, 0}));
    for (double y : {0.25, 1.4}) fence.append(makeBox({2.0 * halfLength, 0.07, 0.07}, black), Matrix4::translation({0, y, 0}));
    return fence;
}

namespace {

/** @brief The four bunting strings: start and end points and how far each sags. */
struct BuntingString {
    Vec3 start;
    Vec3 end;
    double sag;
};
const BuntingString buntingStrings[] = {
    {{-3.4, 7.2, -1.2}, {-17.0, 5.6, -8.5}, 1.4}, {{-3.2, 4.8, 0.0}, {-12.5, 3.6, -8.0}, 0.9},
    {{3.4, 7.2, -1.2}, {17.0, 5.6, -8.5}, 1.4},   {{3.2, 4.8, 0.0}, {12.5, 3.6, -8.0}, 0.9},
};

Vec3 buntingPoint(const BuntingString& string, double t) {
    // Cubic Bezier sag, as in the 2D bunting: inner control points pulled down.
    const Vec3 control1 = lerpVec3(string.start, string.end, 0.33) + Vec3(0, -string.sag, 0);
    const Vec3 control2 = lerpVec3(string.start, string.end, 0.66) + Vec3(0, -string.sag, 0);
    const double u = 1.0 - t;
    return string.start * (u * u * u) + control1 * (3 * t * u * u) + control2 * (3 * t * t * u) + string.end * (t * t * t);
}

}  // namespace

MeshData buildBuntingFlags() {
    const Color flagColors[5] = {Color{220, 40, 50}, Color{250, 210, 40}, Color{40, 110, 210}, Color{245, 245, 245}, Color{240, 130, 40}};
    MeshData flags;
    int colorIndex = 0;
    for (const BuntingString& string : buntingStrings) {
        const int flagCount = 16;
        for (int flag = 0; flag < flagCount; ++flag) {
            const Vec3 left = buntingPoint(string, (flag + 0.15) / flagCount);
            const Vec3 right = buntingPoint(string, (flag + 0.85) / flagCount);
            const Vec3 tip = lerpVec3(left, right, 0.5) + Vec3(0, -(right - left).length() * 1.2, 0);
            const Vec3 normal = crossProduct(right - left, tip - left).normalized();
            const std::uint32_t first = flags.nextIndex();
            const Color color = flagColors[colorIndex++ % 5];
            flags.vertices.push_back(makeVertex(left, normal, color));
            flags.vertices.push_back(makeVertex(right, normal, color));
            flags.vertices.push_back(makeVertex(tip, normal, color));
            flags.addTriangle(first, first + 1, first + 2);
        }
        // A pole holds up the outer end of each string.
        flags.append(makeCylinderBetween({string.end.x, 0.0, string.end.z}, string.end + Vec3(0, 0.3, 0), 0.07, 8, Color{90, 80, 70}));
    }
    return flags;
}

MeshData buildBuntingStrings() {
    std::vector<Vec3> endpoints;
    for (const BuntingString& string : buntingStrings) {
        for (int step = 0; step < 32; ++step) {
            endpoints.push_back(buntingPoint(string, step / 32.0));
            endpoints.push_back(buntingPoint(string, (step + 1) / 32.0));
        }
    }
    return makeLines(endpoints, Color{60, 60, 60});
}

MeshData buildPersonTorso() {
    // Built white; the renderer tints clothes per visitor. Feet at origin, facing +z.
    MeshData person;
    person.append(makeCylinder(0.2, 0.62, 12, Color{240, 240, 240}), Matrix4::translation({0, 0.85, 0}));
    person.append(makeEllipsoid({0.2, 0.08, 0.2}, 12, 6, Color{240, 240, 240}), Matrix4::translation({0, 1.47, 0}));
    person.append(makeEllipsoid({0.13, 0.15, 0.13}, 12, 8, Color{150, 105, 80}), Matrix4::translation({0, 1.68, 0}));
    for (double side : {-1.0, 1.0}) {
        person.append(makeCylinderBetween({side * 0.24, 1.42, 0}, {side * 0.28, 0.9, 0.05}, 0.06, 8, Color{240, 240, 240}));
    }
    return person;
}

MeshData buildPersonLeg() { return makeCylinderBetween({0, 0, 0}, {0, -0.85, 0}, 0.08, 8, Color{60, 70, 110}); }

MeshData buildBalloon() {
    MeshData balloon = makeEllipsoid({0.3, 0.36, 0.3}, 18, 12, astroPalette::white);
    balloon.append(makeCone(0.06, 0.1, 8, astroPalette::white), Matrix4::translation({0, -0.44, 0}));
    return balloon;
}
