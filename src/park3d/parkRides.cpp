/**
 * @file parkRides.cpp
 * @brief Procedural models of the expanded park.
 */
#include "park3d/parkRides.h"

#include <algorithm>
#include <cmath>

#include "core/simpleRandom.h"
#include "core/vec2.h"
#include "mesh/meshBuilder.h"

namespace {

SegmentColorizer stripes(std::vector<Color> colors) {
    return [colors](int segment) { return colors[static_cast<std::size_t>(segment) % colors.size()]; };
}

const Color steelGrey{120, 122, 130};
const Color darkSteel{48, 50, 58};
const Color wood{120, 78, 44};

}  // namespace

// ---------------------------------------------------------------------------
// Wave swinger
// ---------------------------------------------------------------------------

MeshData buildSwingTower() {
    MeshData tower;
    tower.append(makeCylinder(3.2, 0.4, 40, darkSteel));
    tower.append(makeCylinder(0.55, swingTowerHeight, 24, stripes({astroPalette::gold, astroPalette::neonPurple})), Matrix4::translation({0, 0.4, 0}));
    for (int light = 0; light < 10; ++light) {
        tower.append(makeEllipsoid({0.12, 0.12, 0.12}, 8, 6, astroPalette::goldLight), Matrix4::translation({0, 1.2 + light, 0.58}));
    }
    return tower;
}

MeshData buildSwingCanopy() {
    MeshData canopy;
    canopy.append(makeCone(swingCanopyRadius + 0.4, 2.0, 24, stripes({astroPalette::neonPink, astroPalette::cream, astroPalette::neonCyan})),
                  Matrix4::translation({0, 0.3, 0}));
    canopy.append(makeCylinder(swingCanopyRadius + 0.42, 0.5, 48, stripes({astroPalette::gold, astroPalette::neonPurple}), false),
                  Matrix4::translation({0, -0.2, 0}));
    canopy.append(makeCylinder(swingCanopyRadius + 0.38, 0.06, 48, darkSteel), Matrix4::translation({0, -0.2, 0}));
    for (int bulb = 0; bulb < 32; ++bulb) {
        const double angle = 2.0 * pi * bulb / 32.0;
        canopy.append(makeEllipsoid({0.1, 0.1, 0.1}, 8, 6, astroPalette::goldLight),
                      Matrix4::translation({std::cos(angle) * (swingCanopyRadius + 0.45), -0.25, -std::sin(angle) * (swingCanopyRadius + 0.45)}));
    }
    return canopy;
}

MeshData buildSwingChair() {
    MeshData chair;
    for (double side : {-0.25, 0.25}) {
        chair.append(makeCylinderBetween({side, 0, 0}, {side, -swingChainLength, 0}, 0.025, 6, steelGrey));
    }
    chair.append(makeBox({0.6, 0.08, 0.55}, astroPalette::white), Matrix4::translation({0, -swingChainLength, 0}));
    chair.append(makeBox({0.6, 0.6, 0.06}, astroPalette::white), Matrix4::translation({0, -swingChainLength + 0.3, -0.25}));
    return chair;
}

// ---------------------------------------------------------------------------
// Pirate ship
// ---------------------------------------------------------------------------

MeshData buildShipFrame() {
    MeshData frame;
    for (double end : {-5.5, 5.5}) {
        for (double depth : {-3.0, 3.0}) {
            frame.append(makeCylinderBetween({end, shipPivotHeight, 0}, {end, 0, depth}, 0.25, 12, steelGrey));
        }
    }
    frame.append(makeCylinderBetween({-5.6, shipPivotHeight, 0}, {5.6, shipPivotHeight, 0}, 0.35, 16, darkSteel));
    frame.append(makeBox({13.0, 0.4, 8.0}, Color{70, 60, 55}), Matrix4::translation({0, 0.2, 0}));
    return frame;
}

MeshData buildShipHull() {
    // The boat's LENGTH runs along z, at right angles to the pivot beam (x), so it
    // swings bow-to-stern. Origin = pivot; the hull hangs shipArmLength below it.
    MeshData hull;
    const double deckY = -shipArmLength;
    for (double side : {-1.4, 1.4}) {
        for (double end : {-2.5, 2.5}) hull.append(makeCylinderBetween({side * 0.8, 0, 0}, {side, deckY + 0.6, end}, 0.16, 10, steelGrey));
    }
    hull.append(makeEllipsoid({1.6, 1.3, 6.0}, 32, 16, Color{140, 40, 36}), Matrix4::translation({0, deckY - 0.2, 0}));
    hull.append(makeBox({2.8, 0.15, 11.0}, Color{170, 120, 70}), Matrix4::translation({0, deckY + 0.6, 0}));
    for (double side : {-1.45, 1.45}) hull.append(makeBox({0.15, 0.5, 11.4}, astroPalette::gold), Matrix4::translation({side, deckY + 0.6, 0}));
    hull.append(makeCylinderBetween({0, deckY + 0.6, 3.5}, {0, deckY + 6.0, 3.5}, 0.12, 10, wood));
    MeshData sail;
    const Vec3 normal{1, 0, 0};
    const Color sailColor{240, 232, 214};
    sail.vertices = {makeVertex({0.1, deckY + 1.8, 1.4}, normal, sailColor), makeVertex({0.1, deckY + 1.8, 5.6}, normal, sailColor),
                     makeVertex({0.1, deckY + 5.6, 5.2}, normal, sailColor), makeVertex({0.1, deckY + 5.6, 1.8}, normal, sailColor)};
    sail.addTriangle(0, 2, 1);
    sail.addTriangle(0, 3, 2);
    hull.append(sail);
    return hull;
}

// ---------------------------------------------------------------------------
// Teacups
// ---------------------------------------------------------------------------

MeshData buildTeacupPlatform() {
    MeshData platform;
    platform.append(makeCylinder(teacupPlatformRadius + 0.4, 0.3, 64, darkSteel));
    platform.append(makeCylinder(teacupPlatformRadius, 0.12, 64, stripes({Color{90, 170, 210}, Color{235, 235, 245}})), Matrix4::translation({0, 0.3, 0}));
    return platform;
}

MeshData buildTeacupTurntable() { return makeCylinder(2.4, 0.15, 40, astroPalette::gold); }

MeshData buildTeacup() {
    // Built white with pink stripes; tinted per cup. A flared cup is a cone frustum: a cone
    // whose tip is cut off by placing a smaller cone inside... here simply a short wide
    // cylinder on a narrower foot, plus a torus handle and a central wheel.
    MeshData cup;
    cup.append(makeCylinder(0.45, 0.25, 24, astroPalette::white));
    cup.append(makeCylinder(0.95, 0.85, 32, stripes({astroPalette::white, Color{255, 200, 220}}), false), Matrix4::translation({0, 0.25, 0}));
    cup.append(makeCylinder(0.88, 0.05, 32, Color{150, 90, 110}), Matrix4::translation({0, 0.4, 0}));
    cup.append(makeTorus(0.75, 0.07, 32, 8, astroPalette::gold), Matrix4::translation({0, 1.1, 0}) * Matrix4::rotationX(pi / 2.0));
    cup.append(makeTorus(0.3, 0.08, 16, 8, astroPalette::white), Matrix4::translation({1.05, 0.7, 0}));
    cup.append(makeCylinderBetween({0, 0.4, 0}, {0, 0.95, 0}, 0.05, 8, steelGrey));
    cup.append(makeTorus(0.25, 0.04, 16, 6, astroPalette::gold), Matrix4::translation({0, 0.95, 0}) * Matrix4::rotationX(pi / 2.0));
    return cup;
}

// ---------------------------------------------------------------------------
// Bumper cars
// ---------------------------------------------------------------------------

MeshData buildBumperArena() {
    MeshData arena;
    const double w = bumperArenaHalfWidth, d = bumperArenaHalfDepth;
    arena.append(makeBox({2 * w + 1.0, 0.25, 2 * d + 1.0}, Color{60, 62, 70}), Matrix4::translation({0, 0.12, 0}));
    arena.append(makeBox({2 * w, 0.02, 2 * d}, Color{40, 40, 48}), Matrix4::translation({0, 0.26, 0}));
    // Padded rails around the floor.
    for (double side : {-1.0, 1.0}) {
        arena.append(makeBox({2 * w + 1.0, 0.5, 0.3}, Color{230, 200, 40}), Matrix4::translation({0, 0.5, side * (d + 0.4)}));
        arena.append(makeBox({0.3, 0.5, 2 * d + 1.0}, Color{230, 200, 40}), Matrix4::translation({side * (w + 0.4), 0.5, 0}));
    }
    // Roof on posts, with a mesh ceiling.
    for (double x : {-w, 0.0, w}) {
        for (double z : {-d, d}) arena.append(makeCylinderBetween({x, 0.25, z}, {x, 5.0, z}, 0.15, 10, steelGrey));
    }
    arena.append(makeBox({2 * w + 2.0, 0.5, 2 * d + 2.0}, Color{200, 40, 60}), Matrix4::translation({0, 5.25, 0}));
    arena.append(makeBox({2 * w + 2.2, 0.8, 0.2}, astroPalette::cream), Matrix4::translation({0, 4.8, d + 1.0}));
    for (int bulb = 0; bulb < 24; ++bulb) {
        arena.append(makeEllipsoid({0.1, 0.1, 0.1}, 8, 6, astroPalette::goldLight),
                     Matrix4::translation({-w - 0.8 + bulb * (2 * w + 1.6) / 23.0, 4.75, d + 1.12}));
    }
    return arena;
}

MeshData buildBumperCar() {
    MeshData car;
    car.append(makeEllipsoid({0.75, 0.3, 1.0}, 20, 10, astroPalette::white), Matrix4::translation({0, 0.5, 0}));
    car.append(makeTorus(0.95, 0.12, 32, 8, darkSteel), Matrix4::translation({0, 0.45, 0}) * Matrix4::rotationX(pi / 2.0) * Matrix4::scaling({0.8, 1.05, 1.0}));
    car.append(makeCylinderBetween({0, 0.6, -0.6}, {0, 4.9, -0.6}, 0.03, 6, steelGrey));  // pole to the ceiling grid
    car.append(makeBox({0.6, 0.5, 0.6}, Color{30, 30, 36}), Matrix4::translation({0, 0.8, -0.2}));
    return car;
}

// ---------------------------------------------------------------------------
// Park furniture
// ---------------------------------------------------------------------------

MeshData buildFoodStall(Color stripeA, Color stripeB) {
    MeshData stall;
    stall.append(makeBox({3.2, 2.4, 2.4}, Color{235, 228, 214}), Matrix4::translation({0, 1.2, 0}));
    stall.append(makeBox({3.4, 0.15, 0.7}, Color{150, 100, 60}), Matrix4::translation({0, 1.05, 1.5}));  // counter
    stall.append(makeBox({2.6, 1.0, 0.05}, Color{40, 30, 30}), Matrix4::translation({0, 1.7, 1.21}));   // serving hatch
    // Striped awning: a row of thin tilted boxes alternating colours.
    for (int stripe = 0; stripe < 8; ++stripe) {
        const double x = -1.6 + 0.4 * stripe + 0.2;
        stall.append(makeBox({0.4, 0.06, 1.6}, stripe % 2 == 0 ? stripeA : stripeB),
                     Matrix4::translation({x, 2.55, 1.6}) * Matrix4::rotationX(0.35));
    }
    stall.append(makeBox({2.6, 0.7, 0.1}, stripeA), Matrix4::translation({0, 3.2, 0.9}));  // sign board
    stall.append(makeBox({2.4, 0.5, 0.12}, astroPalette::cream), Matrix4::translation({0, 3.2, 0.92}));
    return stall;
}

MeshData buildTree(int variant) {
    SimpleRandom random(static_cast<std::uint32_t>(variant * 7919 + 17));
    MeshData tree;
    const double height = random.nextRange(5.0, 7.5);
    tree.append(makeCylinder(0.9, 0.35, 16, Color{96, 92, 86}));  // planter
    tree.append(makeCylinderBetween({0, 0, 0}, {0, height * 0.6, 0}, 0.22, 10, Color{92, 66, 44}));
    const Color greens[3] = {Color{58, 110, 48}, Color{78, 128, 56}, Color{46, 92, 44}};
    for (int clump = 0; clump < 6; ++clump) {
        const double angle = random.nextRange(0.0, 2.0 * pi);
        const double spread = random.nextRange(0.4, 1.3);
        const double size = random.nextRange(1.2, 1.9);
        tree.append(makeEllipsoid({size, size * 0.85, size}, 14, 10, greens[clump % 3]),
                    Matrix4::translation({std::cos(angle) * spread, height * random.nextRange(0.65, 0.9), std::sin(angle) * spread}));
    }
    return tree;
}

MeshData buildGrassPatch() {
    // A flat disc with a rim slightly darker than the middle (vertex colours interpolate).
    MeshData grass;
    const Vec3 up{0, 1, 0};
    const int segments = 40;
    grass.vertices.push_back(makeVertex({0, 0.03, 0}, up, Color{92, 140, 62}));
    for (int segment = 0; segment <= segments; ++segment) {
        const double angle = 2.0 * pi * segment / segments;
        grass.vertices.push_back(makeVertex({std::cos(angle), 0.03, -std::sin(angle)}, up, Color{70, 112, 50}));
    }
    for (std::uint32_t segment = 1; segment <= static_cast<std::uint32_t>(segments); ++segment) grass.addTriangle(0, segment, segment + 1);
    return grass;
}

MeshData buildLampPost() {
    MeshData lamp;
    lamp.append(makeCylinder(0.25, 0.3, 12, Color{30, 30, 34}));
    lamp.append(makeCylinderBetween({0, 0.3, 0}, {0, lampGlobeHeight - 0.3, 0}, 0.08, 10, Color{30, 30, 34}));
    lamp.append(makeCone(0.35, 0.3, 12, Color{30, 30, 34}), Matrix4::translation({0, lampGlobeHeight + 0.25, 0}));
    return lamp;
}

MeshData buildLampGlobe() { return makeEllipsoid({0.28, 0.32, 0.28}, 14, 10, Color{255, 236, 190}); }

MeshData buildStatue() {
    // A figure built from the same primitives as the visitors, larger, on a stone plinth.
    MeshData statue;
    statue.append(makeBox({1.4, 1.2, 1.4}, Color{120, 116, 110}), Matrix4::translation({0, 0.6, 0}));
    const Color gold = astroPalette::brass;
    statue.append(makeCylinderBetween({-0.18, 1.2, 0}, {-0.2, 2.4, 0}, 0.16, 12, gold));
    statue.append(makeCylinderBetween({0.18, 1.2, 0}, {0.2, 2.4, 0}, 0.16, 12, gold));
    statue.append(makeEllipsoid({0.38, 0.6, 0.26}, 20, 12, gold), Matrix4::translation({0, 2.95, 0}));
    statue.append(makeEllipsoid({0.2, 0.24, 0.2}, 16, 10, gold), Matrix4::translation({0, 3.8, 0}));
    statue.append(makeCylinderBetween({-0.4, 3.4, 0}, {-0.55, 2.6, 0.25}, 0.1, 10, gold));
    statue.append(makeCylinderBetween({0.4, 3.4, 0}, {0.75, 4.1, 0.1}, 0.1, 10, gold));  // a raised arm
    return statue;
}

MeshData buildWreckedCar() {
    MeshData car;
    const Color rust{110, 64, 40}, burnt{40, 34, 32}, paint{170, 140, 100};
    car.append(makeBox({1.9, 0.7, 4.4}, rust), Matrix4::translation({0, 0.65, 0}));
    car.append(makeBox({1.7, 0.6, 2.2}, paint), Matrix4::translation({0, 1.3, -0.2}) * Matrix4::rotationX(0.05));
    car.append(makeBox({1.72, 0.45, 1.9}, burnt), Matrix4::translation({0, 1.33, -0.2}));  // empty window frames
    for (double x : {-0.85, 0.85}) {
        for (double z : {-1.4, 1.4}) {
            car.append(makeCylinderBetween({x - 0.15, 0.38, z}, {x + 0.15, 0.38, z}, 0.38, 14, burnt));
        }
    }
    return car;
}

MeshData buildExitGate() {
    MeshData gate;
    for (double x : {-2.2, 2.2}) gate.append(makeCylinderBetween({x, 0, 0}, {x, 4.2, 0}, 0.18, 12, Color{30, 30, 34}));
    gate.append(makeBox({5.0, 0.9, 0.25}, astroPalette::neonCyan), Matrix4::translation({0, 4.4, 0}));
    for (double x : {-1.1, 0.0, 1.1}) {
        gate.append(makeCylinder(0.15, 1.0, 12, steelGrey), Matrix4::translation({x, 0, 0}));
        gate.append(makeTorus(0.4, 0.04, 16, 6, steelGrey), Matrix4::translation({x, 0.9, 0}));
    }
    return gate;
}

MeshData buildPathRibbon(const std::vector<Vec3>& path, double width) {
    MeshData ribbon;
    const Vec3 up{0, 1, 0};
    const Color paving{150, 140, 128};
    const std::size_t count = path.size();
    double travelled = 0.0;
    for (std::size_t index = 0; index < count; ++index) {
        const Vec3& previous = path[(index + count - 1) % count];
        const Vec3& next = path[(index + 1) % count];
        const Vec3 side = crossProduct((next - previous).normalized(), up).normalized();
        if (index > 0) travelled += (path[index] - path[index - 1]).length();
        // Texture coordinates in metres / 4, so the slab texture keeps its scale along the path.
        ribbon.vertices.push_back(makeVertex(path[index] - side * (width * 0.5) + Vec3(0, 0.02, 0), up, paving, 0.0, travelled / 4.0));
        ribbon.vertices.push_back(makeVertex(path[index] + side * (width * 0.5) + Vec3(0, 0.02, 0), up, paving, width / 4.0, travelled / 4.0));
    }
    for (std::uint32_t index = 0; index + 1 < static_cast<std::uint32_t>(count); ++index) {
        const std::uint32_t a = index * 2;
        ribbon.addTriangle(a, a + 1, a + 2);  // counter-clockwise seen from above (normal up)
        ribbon.addTriangle(a + 1, a + 3, a + 2);
    }
    return ribbon;
}

MeshData buildEntranceWall(double fromX, double toX) {
    // A tall black palisade along z = 0 with stone pillars (the cover's fence line).
    MeshData wall;
    const Color black = astroPalette::fenceBlack;
    const double length = toX - fromX;
    for (double x = fromX; x <= toX; x += 0.22) wall.append(makeBox({0.05, 2.6, 0.05}, black), Matrix4::translation({x, 1.3, 0}));
    for (double y : {0.3, 2.3}) wall.append(makeBox({length, 0.08, 0.08}, black), Matrix4::translation({(fromX + toX) * 0.5, y, 0}));
    for (double x = fromX; x <= toX + 0.01; x += 8.0) {
        wall.append(makeBox({0.7, 3.1, 0.7}, Color{150, 140, 128}), Matrix4::translation({x, 1.55, 0}));
        wall.append(makeEllipsoid({0.3, 0.3, 0.3}, 12, 8, astroPalette::gold), Matrix4::translation({x, 3.4, 0}));
    }
    return wall;
}
