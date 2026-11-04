/**
 * @file parkScene.cpp
 * @brief The expanded ASTROWORLD park: layout, ride animation, crowd and lighting at any time.
 *
 * World units are metres: y up, the giant head is the park's ENTRANCE GATE at
 * the origin with its face toward +z. In front lies the entrance plaza from
 * the album cover (rocket, gold statue, wrecked car, bunting, balloon vendor);
 * behind the palisade wall (z = 0) is the park itself. Visitors walk in
 * through the tunnel in the mouth, loop around the rides, and leave through
 * the exit gate in the wall - nobody vanishes.
 *
 * Every animated value is a pure function of time, from the shared timeline
 * (astroworldTimeline.h) or the physics below, so all renderers agree.
 */
#include "park3d/parkScene.h"

#include <algorithm>
#include <cmath>

#include "animation/astroworldTimeline.h"
#include "animation/easing.h"
#include "animation/keyframeTrack.h"
#include "core/simpleRandom.h"
#include "curves/spline.h"
#include "mesh/meshBuilder.h"
#include "park3d/headModel.h"
#include "park3d/parkModels.h"
#include "park3d/parkRides.h"

namespace {

// ---- Where everything stands -------------------------------------------------------------
const Vec3 ferrisPosition{38.0, 0.0, -52.0};
const double ferrisScale = 2.4;
const Vec3 dropTowerPosition{-6.0, 0.0, -102.0};
const double dropTowerScale = 2.6;
const Vec3 carouselPosition{14.0, 0.0, -44.0};
const double carouselScale = 1.25;
const Vec3 swingPosition{-22.0, 0.0, -50.0};
const Vec3 shipPosition{30.0, 0.0, -88.0};
const Vec3 teacupPosition{-12.0, 0.0, -20.0};
const Vec3 bumperPosition{28.0, 0.0, -17.0};
const Vec3 globePosition{0.0, 0.0, -33.0};
const double globeScale = 2.0;
const Vec3 bigTopPosition{52.0, 0.0, -26.0};
const Vec3 signPosition{-36.0, 0.0, -5.0};
const Vec3 rocketPosition{-13.0, 0.0, 9.0};
const Vec3 statuePosition{-8.5, 0.0, 5.0};
const Vec3 wreckPosition{12.5, 0.0, 10.0};
const Vec3 vendorPosition{-18.0, 0.0, 15.0};
const Vec3 exitGatePosition{-23.5, 0.0, 0.0};
const Vec3 searchlightPositions[2] = {{-30.0, 0.0, -112.0}, {36.0, 0.0, -118.0}};
const double gravity = 9.81;
const double parkPavingSize = 220.0;
const double parkCentreZ = -45.0;

struct Placement {
    Vec3 position;
    double rotation;  // radians about y
    MeshId mesh;
};
const Placement stallPlacements[] = {
    {{-7.5, 0.0, -13.0}, pi / 2.0, MeshId::foodStallPink},     {{7.5, 0.0, -13.0}, -pi / 2.0, MeshId::foodStallBlue},
    {{-7.5, 0.0, -24.0}, pi / 2.0, MeshId::foodStallYellow},   {{-17.0, 0.0, 26.0}, pi / 2.0, MeshId::foodStallBlue},
    {{18.0, 0.0, 22.0}, -pi / 2.0, MeshId::foodStallPink},     {{4.0, 0.0, -80.0}, 0.0, MeshId::foodStallYellow},
};
const Vec3 treePositions[] = {
    {-30, 0, 22}, {30, 0, 24}, {-38, 0, 9}, {38, 0, 9}, {-44, 0, 32}, {44, 0, 34}, {-26, 0, 38}, {27, 0, 40},
    {-40, 0, -28}, {-46, 0, -62}, {50, 0, -42}, {52, 0, -70}, {14, 0, -76}, {-16, 0, -86}, {-4, 0, -58}, {10, 0, -62},
    {-34, 0, -82}, {18, 0, -100}, {44, 0, -100}, {-50, 0, -12}, {12, 0, -24}, {-10, 0, -44},
};

/** @brief Control points of the visitors' walking loop (y = 0): plaza -> mouth -> park -> exit gate -> plaza. */
const std::vector<Vec3>& routeControlPoints() {
    static const std::vector<Vec3> points = {
        {0, 0, 40},     {3, 0, 24},     {0, 0, 13},     {0, 0, 6},      {0, 0, -1},   {0, 0, -8},    {0, 0, -18},
        {6, 0, -27},    {18, 0, -34},   {24, 0, -50},   {17, 0, -64},   {2, 0, -73},  {-14, 0, -70}, {-30, 0, -63},
        {-35, 0, -42},  {-29, 0, -28},  {-23.5, 0, -13}, {-23.5, 0, -4}, {-23.5, 0, 4}, {-20, 0, 14},  {-12, 0, 26},
        {-4, 0, 38},
    };
    return points;
}

Material makeMaterial(Color tint, double metallic, double roughness, double emissive = 0.0, double emissionStrength = 1.0) {
    Material result;
    result.tint = tint;
    result.metallic = metallic;
    result.roughness = roughness;
    result.emissive = emissive;
    result.emissionStrength = emissionStrength;
    return result;
}

const Color white = astroPalette::white;
/** @brief Gold vinyl of the inflatable: metallic, fairly glossy - it mirrors the sky. */
const Material goldVinyl = makeMaterial(white, 0.85, 0.3);
const Material paint = makeMaterial(white, 0.0, 0.45);
const Material matte = makeMaterial(white, 0.0, 0.85);
const Material steel = makeMaterial(white, 0.8, 0.4);

/** @brief Glowing bulbs: emissive and HDR-bright at night so they bloom. */
Material bulbs(double night) { return makeMaterial(white, 0.0, 0.3, 0.35 + 0.65 * night, 1.5 + 6.0 * night); }

Vec3 lerpVector(const Vec3& a, const Vec3& b, double t) { return a + (b - a) * t; }

/**
 * @brief Swing ride spin over a 30 s cycle: speeds up, spins, slows, rests (0..1).
 */
const KeyframeTrack& swingSpinTrack() {
    static const KeyframeTrack track = KeyframeTrack().addKey(0.0, 0.0).addKey(6.0, 1.0, easing::easeInOutCubic).addKey(21.0, 1.0)
                                           .addKey(27.0, 0.0, easing::easeInOutCubic).addKey(30.0, 0.0);
    return track;
}

/** @brief Total angle turned by the swing ride: maximum speed x the integral of the spin factor. */
double swingAngleAt(double time) {
    const double maxSpeed = 1.15;  // rad/s, about 11 rpm
    const KeyframeTrack& track = swingSpinTrack();
    const double cycle = track.duration();
    // The integral over one whole cycle, and over the partial one (midpoint rule).
    auto integrate = [&](double until) {
        const int steps = 120;
        double sum = 0.0;
        for (int step = 0; step < steps; ++step) sum += track.sample((step + 0.5) * until / steps);
        return sum * until / steps;
    };
    static const double perCycle = integrate(cycle);
    const double cycles = std::floor(time / cycle);
    return maxSpeed * (cycles * perCycle + integrate(time - cycles * cycle));
}

/** @brief Pirate ship swing amplitude over a 40 s cycle: builds up, peaks, dies away (radians). */
const KeyframeTrack& shipAmplitudeTrack() {
    static const KeyframeTrack track = KeyframeTrack().addKey(0.0, 0.0).addKey(14.0, 1.2, easing::smoothStep).addKey(27.0, 1.2)
                                           .addKey(37.0, 0.0, easing::smoothStep).addKey(40.0, 0.0);
    return track;
}

const Color skinTones[6] = {{255, 224, 196}, {234, 192, 160}, {198, 140, 100}, {160, 108, 72}, {118, 78, 52}, {88, 58, 40}};

/** @brief A plausible outfit colour: mostly muted, sometimes bright, sometimes denim/black/white. */
Color clothesColor(SimpleRandom& random) {
    const double pick = random.nextDouble();
    if (pick < 0.15) return Color{40, 40, 46};
    if (pick < 0.27) return Color{235, 235, 230};
    if (pick < 0.40) return Color{70, 95, 140};
    return hsvToRgb(random.nextRange(0.0, 360.0), random.nextRange(0.35, 0.8), random.nextRange(0.55, 0.95));
}

}  // namespace

ParkScene::ParkScene() {
    auto set = [this](MeshId id, MeshData data) { meshes[static_cast<std::size_t>(id)] = std::move(data); };

    // The walking loop, sampled into a smooth closed spline with an arc-length table.
    walkingRoute = sampleCatmullRomSpline3D(routeControlPoints(), 16, true);
    walkingRoute.pop_back();  // closed spline repeats its first point
    routeCumulativeLength.assign(walkingRoute.size() + 1, 0.0);
    for (std::size_t index = 1; index <= walkingRoute.size(); ++index) {
        routeCumulativeLength[index] = routeCumulativeLength[index - 1] +
                                       (walkingRoute[index % walkingRoute.size()] - walkingRoute[index - 1]).length();
    }
    // Lamp posts every ~20 m along the walk, alternating sides.
    for (double distance = 6.0, side = 1.0; distance < routeCumulativeLength.back(); distance += 20.0, side = -side) {
        Vec3 position, tangent;
        routeFrameAt(distance, position, tangent);
        const Vec3 lamp = position + crossProduct(tangent, {0, 1, 0}).normalized() * (3.8 * side);
        if (std::fabs(lamp.z) < 4.5 && std::fabs(lamp.x) < 8.0) continue;  // not inside the head
        lampPositions.push_back(lamp);
    }

    // Paving covers the park (220 m square, 4 m slabs); a meadow stretches beyond it to the horizon.
    set(MeshId::ground, makeGroundPlane(parkPavingSize, parkPavingSize, astroPalette::white, parkPavingSize / 4.0));
    set(MeshId::meadow, makeGroundPlane(1400.0, 1400.0, Color{86, 128, 60}));
    // A ring of trees around the edge of the paving.
    SimpleRandom treeRandom(4242u);
    const double half = parkPavingSize * 0.5;
    for (double along = -half; along < half; along += 13.0) {
        for (const Vec3& point : {Vec3(along, 0, parkCentreZ - half - 3.0), Vec3(along, 0, parkCentreZ + half + 3.0),
                                  Vec3(-half - 3.0, 0, parkCentreZ + along), Vec3(half + 3.0, 0, parkCentreZ + along)}) {
            perimeterTrees.push_back(point + Vec3(treeRandom.nextRange(-3.0, 3.0), 0, treeRandom.nextRange(-3.0, 3.0)));
        }
    }
    set(MeshId::pathRibbon, buildPathRibbon(walkingRoute, 5.0));
    set(MeshId::entranceWallLeft, buildEntranceWall(-70.0, -26.0));
    set(MeshId::entranceWallMiddle, buildEntranceWall(-21.0, -6.6));
    set(MeshId::entranceWallRight, buildEntranceWall(6.6, 70.0));
    set(MeshId::exitGate, buildExitGate());
    set(MeshId::head, buildHeadMesh());
    set(MeshId::tunnel, buildMouthTunnel());
    set(MeshId::ferrisStand, buildFerrisStand());
    set(MeshId::ferrisRotor, buildFerrisRotor());
    set(MeshId::ferrisBulbs, buildFerrisBulbs());
    set(MeshId::gondola, buildGondola());
    coasterPath = buildCoasterTrackPath();
    set(MeshId::coasterTrack, buildCoasterTrack(coasterPath));
    set(MeshId::coasterTruss, buildCoasterTruss(coasterPath));
    set(MeshId::coasterCar, buildCoasterCar());
    set(MeshId::dropColumn, buildDropTowerColumn());
    set(MeshId::dropLights, buildDropTowerLights());
    set(MeshId::dropSeats, buildDropTowerSeats());
    set(MeshId::carouselBase, buildCarouselBase());
    set(MeshId::carouselRotor, buildCarouselRotor());
    set(MeshId::carouselBulbs, buildCarouselBulbs());
    set(MeshId::horse, buildCarouselHorse());
    set(MeshId::swingTower, buildSwingTower());
    set(MeshId::swingCanopy, buildSwingCanopy());
    set(MeshId::swingChair, buildSwingChair());
    set(MeshId::shipFrame, buildShipFrame());
    set(MeshId::shipHull, buildShipHull());
    set(MeshId::teacupPlatform, buildTeacupPlatform());
    set(MeshId::teacupTurntable, buildTeacupTurntable());
    set(MeshId::teacup, buildTeacup());
    set(MeshId::bumperArena, buildBumperArena());
    set(MeshId::bumperCar, buildBumperCar());
    set(MeshId::foodStallPink, buildFoodStall(astroPalette::neonPink, astroPalette::cream));
    set(MeshId::foodStallBlue, buildFoodStall(Color{40, 110, 210}, astroPalette::cream));
    set(MeshId::foodStallYellow, buildFoodStall(Color{250, 200, 40}, Color{220, 40, 50}));
    set(MeshId::tree0, buildTree(0));
    set(MeshId::tree1, buildTree(1));
    set(MeshId::tree2, buildTree(2));
    set(MeshId::tree3, buildTree(3));
    set(MeshId::lampPost, buildLampPost());
    set(MeshId::grassPatch, buildGrassPatch());
    set(MeshId::lampGlobe, buildLampGlobe());
    set(MeshId::statue, buildStatue());
    set(MeshId::wreckedCar, buildWreckedCar());
    set(MeshId::rocket, buildRocket());
    set(MeshId::bigTop, buildBigTop());
    set(MeshId::signFrame, buildSignFrame());
    set(MeshId::signFace, buildSignFace());
    set(MeshId::buntingFlags, buildBuntingFlags());
    set(MeshId::buntingStrings, buildBuntingStrings());
    set(MeshId::personTorso, buildPersonTorso());
    set(MeshId::personHead, buildPersonHead());
    set(MeshId::personArm, buildPersonArm());
    set(MeshId::personLeg, buildPersonLeg());
    set(MeshId::rider, buildRider());
    set(MeshId::riderHead, buildRiderHead());
    set(MeshId::balloon, buildBalloon());
    set(MeshId::searchlightBeam, buildSearchlightBeam());
    set(MeshId::mirrorBallPlinth, buildMirrorBallPlinth());
    set(MeshId::mirrorBall, buildMirrorBall());

    coasterCumulativeLength.assign(coasterPath.size(), 0.0);
    for (std::size_t index = 1; index < coasterPath.size(); ++index) {
        coasterCumulativeLength[index] = coasterCumulativeLength[index - 1] + (coasterPath[index] - coasterPath[index - 1]).length();
    }
}

void ParkScene::coasterFrameAt(double distance, Vec3& position, Vec3& tangent) const {
    distance = std::clamp(distance, 0.0, coasterCumulativeLength.back());
    const auto upper = std::lower_bound(coasterCumulativeLength.begin(), coasterCumulativeLength.end(), distance);
    const std::size_t index = std::clamp<std::size_t>(static_cast<std::size_t>(upper - coasterCumulativeLength.begin()), 1, coasterPath.size() - 1);
    const double segment = coasterCumulativeLength[index] - coasterCumulativeLength[index - 1];
    const double t = segment > 1e-9 ? (distance - coasterCumulativeLength[index - 1]) / segment : 0.0;
    position = lerpVec3(coasterPath[index - 1], coasterPath[index], t);
    tangent = (coasterPath[index] - coasterPath[index - 1]).normalized();
}

void ParkScene::routeFrameAt(double distance, Vec3& position, Vec3& tangent) const {
    distance = wrapTime(distance, routeCumulativeLength.back());
    const auto upper = std::upper_bound(routeCumulativeLength.begin(), routeCumulativeLength.end(), distance);
    const std::size_t index = std::clamp<std::size_t>(static_cast<std::size_t>(upper - routeCumulativeLength.begin()), 1, walkingRoute.size());
    const Vec3& start = walkingRoute[index - 1];
    const Vec3& end = walkingRoute[index % walkingRoute.size()];
    const double segment = routeCumulativeLength[index] - routeCumulativeLength[index - 1];
    position = lerpVec3(start, end, segment > 1e-9 ? (distance - routeCumulativeLength[index - 1]) / segment : 0.0);
    tangent = (end - start).normalized();
}

bool ParkScene::coasterRideView(double timeSeconds, Vec3& eye, Vec3& forward, Vec3& up) const {
    const double distance = coasterTrainFractionAt(timeSeconds) * coasterCumulativeLength.back();
    Vec3 position, tangent;
    coasterFrameAt(distance, position, tangent);
    Vec3 aheadPosition, aheadTangent;
    coasterFrameAt(distance + 4.0, aheadPosition, aheadTangent);
    const Vec3 lateral = crossProduct(tangent, {0, 1, 0}).normalized();
    up = crossProduct(lateral, tangent).normalized();
    eye = position + tangent * 1.0 + up * 1.7;
    forward = (aheadPosition + up * 1.4 - eye).normalized();
    return distance > 0.0;
}

void ParkScene::addRider(ParkFrame& frame, const Matrix4& seat, int seed) const {
    SimpleRandom random(static_cast<std::uint32_t>(seed * 2654435761u + 7u));
    frame.instances.push_back({MeshId::rider, seat, makeMaterial(clothesColor(random), 0.0, 0.8)});
    frame.instances.push_back({MeshId::riderHead, seat, makeMaterial(skinTones[random.nextInt(0, 5)], 0.0, 0.55)});
}

void ParkScene::addEntrance(ParkFrame& frame, double time, double night) const {
    auto add = [&](MeshId mesh, const Matrix4& model, const Material& surface) { frame.instances.push_back({mesh, model, surface}); };

    // ---- The gate: the inflatable head, breathing (scaling about its base = the origin) ----
    const double breathing = 1.0 + oscillate(time, 0.25, 0.008);
    const Matrix4 headModel = Matrix4::scaling({breathing, breathing, breathing});
    add(MeshId::head, headModel, goldVinyl);
    add(MeshId::dreads, headModel, goldVinyl);
    add(MeshId::tunnel, headModel, makeMaterial(white, 0.0, 0.5, 1.0, 3.0 + 3.0 * night));
    frame.dreads = buildDreadMesh(oscillate(time, 0.3, 0.03) + oscillate(time, 0.71, 0.01));

    // ---- The palisade wall either side of the head, and the exit gate --------------------
    for (MeshId wall : {MeshId::entranceWallLeft, MeshId::entranceWallMiddle, MeshId::entranceWallRight}) add(wall, Matrix4::identity(), steel);
    add(MeshId::exitGate, Matrix4::translation(exitGatePosition), makeMaterial(white, 0.3, 0.4, 0.25 + 0.6 * night, 1.0 + 3.0 * night));

    // ---- Album-cover props on the entrance plaza -------------------------------------------
    add(MeshId::rocket, Matrix4::translation(rocketPosition) * Matrix4::scaling({1.6, 1.6, 1.6}), makeMaterial(white, 0.75, 0.25));
    add(MeshId::statue, Matrix4::translation(statuePosition) * Matrix4::rotationY(0.4), makeMaterial(white, 0.9, 0.35));
    add(MeshId::wreckedCar, Matrix4::translation(wreckPosition) * Matrix4::rotationY(0.55), makeMaterial(white, 0.2, 0.85));
    add(MeshId::buntingFlags, Matrix4::identity(), makeMaterial(white, 0.0, 0.7));
    add(MeshId::buntingStrings, Matrix4::identity(), makeMaterial(Color{50, 50, 50}, 0.0, 0.9));
    add(MeshId::signFrame, Matrix4::translation(signPosition) * Matrix4::scaling({1.5, 1.5, 1.5}), paint);
    Material signFace = makeMaterial(white, 0.0, 0.5, 0.3 * night, 2.0);
    signFace.texture = TextureId::sign;
    add(MeshId::signFace, Matrix4::translation(signPosition) * Matrix4::scaling({1.5, 1.5, 1.5}), signFace);

    // ---- Balloon vendor with a swaying bunch ----------------------------------------------
    const Matrix4 vendorModel = Matrix4::translation(vendorPosition) * Matrix4::rotationY(0.6);
    add(MeshId::personTorso, vendorModel, makeMaterial(Color{200, 60, 60}, 0.0, 0.8));
    add(MeshId::personHead, vendorModel, makeMaterial(skinTones[3], 0.0, 0.55));
    for (double side : {-1.0, 1.0}) add(MeshId::personLeg, vendorModel * Matrix4::translation({side * 0.1, 0.86, 0}), matte);
    add(MeshId::personArm, vendorModel * Matrix4::translation({0.24, 1.42, 0}) * Matrix4::rotationZ(2.6), makeMaterial(Color{200, 60, 60}, 0.0, 0.8));
    const Vec3 hand = vendorModel.transformPoint({0.55, 1.95, 0.0});
    const Vec3 offsets[6] = {{-0.6, 2.3, 0.0}, {0.1, 2.65, 0.2}, {0.75, 2.2, -0.1}, {-0.25, 1.7, 0.3}, {0.5, 1.6, 0.25}, {0.0, 2.0, -0.4}};
    const Color colors[6] = {astroPalette::neonPink, astroPalette::gold, astroPalette::neonCyan, astroPalette::neonPurple, astroPalette::neonOrange, Color{230, 40, 50}};
    std::vector<Vec3> stringEnds;
    for (int index = 0; index < 6; ++index) {
        const double sway = oscillate(time, 0.25, 0.08) + oscillate(time, 0.6, 0.03, index * 1.9);
        const Vec3 center = hand + Matrix4::rotationZ(sway).transformPoint(offsets[index]);
        add(MeshId::balloon, Matrix4::translation(center), makeMaterial(colors[index], 0.0, 0.18));
        stringEnds.insert(stringEnds.end(), {hand, center + Vec3(0, -0.45, 0)});
    }
    frame.balloonStrings = makeLines(stringEnds, Color{230, 225, 235});
    add(MeshId::balloonStrings, Matrix4::identity(), makeMaterial(white, 0.0, 0.9));
}

void ParkScene::addClassicRides(ParkFrame& frame, double time, double night) const {
    auto add = [&](MeshId mesh, const Matrix4& model, const Material& surface) { frame.instances.push_back({mesh, model, surface}); };

    // ---- Ferris wheel (scaled up): a transformation hierarchy ------------------------------
    const double wheelAngle = -(0.12 + 0.12 * time);
    const Matrix4 scale = Matrix4::scaling({ferrisScale, ferrisScale, ferrisScale});
    const Matrix4 rotorModel = Matrix4::translation(ferrisPosition + Vec3(0, ferrisHubHeight * ferrisScale, 0)) * Matrix4::rotationZ(wheelAngle) * scale;
    add(MeshId::ferrisStand, Matrix4::translation(ferrisPosition) * scale, steel);
    add(MeshId::ferrisRotor, rotorModel, makeMaterial(white, 0.5, 0.35));
    add(MeshId::ferrisBulbs, rotorModel, bulbs(night));
    const double swing = oscillate(time, 0.3, 0.06);
    for (int index = 0; index < ferrisGondolaCount; ++index) {
        const double spokeAngle = 2.0 * pi * index / ferrisGondolaCount;
        const Vec3 mount = rotorModel.transformPoint(Vec3(std::cos(spokeAngle), std::sin(spokeAngle), 0.0) * ferrisRadius);
        const Matrix4 hanging = Matrix4::translation(mount) * Matrix4::rotationZ(swing * std::cos(index * 1.3));
        add(MeshId::gondola, hanging * scale, makeMaterial(hsvToRgb(index * 360.0 / ferrisGondolaCount, 0.7, 0.95), 0.1, 0.35));
        for (double side : {-0.4, 0.4}) addRider(frame, hanging * Matrix4::translation({side, -2.95, 0.0}), index * 2 + (side > 0));
    }

    // ---- Roller coaster ------------------------------------------------------------------
    add(MeshId::coasterTrack, Matrix4::identity(), makeMaterial(white, 0.6, 0.35));
    add(MeshId::coasterTruss, Matrix4::identity(), makeMaterial(white, 0.6, 0.4));
    const double leadDistance = coasterTrainFractionAt(time) * coasterCumulativeLength.back();
    for (int car = 0; car < 4; ++car) {
        const double distance = leadDistance - car * 2.2;
        if (distance < 0.0) continue;
        Vec3 position, forward;
        coasterFrameAt(distance, position, forward);
        const Vec3 lateral = crossProduct(forward, {0, 1, 0}).normalized();
        const Vec3 up = crossProduct(lateral, forward);
        add(MeshId::coasterCar, Matrix4::fromBasis(forward, up, crossProduct(forward, up), position + up * 0.1) * Matrix4::scaling({1.6, 1.6, 1.6}),
            makeMaterial(hsvToRgb(190.0 + car * 40.0, 0.8, 1.0), 0.3, 0.3));
    }

    // ---- Drop tower (scaled up): keyframed seat ring with riders facing out ---------------
    const Matrix4 towerModel = Matrix4::translation(dropTowerPosition) * Matrix4::scaling({dropTowerScale, dropTowerScale, dropTowerScale});
    add(MeshId::dropColumn, towerModel, makeMaterial(white, 0.2, 0.4));
    add(MeshId::dropLights, towerModel, bulbs(night));
    const double seatHeight = (1.4 + dropTowerLiftAt(time) * (dropTowerHeight - 3.0)) * dropTowerScale;
    add(MeshId::dropSeats, Matrix4::translation(dropTowerPosition + Vec3(0, seatHeight, 0)) * Matrix4::scaling({dropTowerScale, dropTowerScale, dropTowerScale}),
        makeMaterial(white, 0.3, 0.35));
    for (int seat = 0; seat < 12; seat += 1) {
        addRider(frame, Matrix4::translation(dropTowerPosition + Vec3(0, seatHeight - 1.5, 0)) * Matrix4::rotationY(seat * pi / 6.0) *
                            Matrix4::translation({1.25 * dropTowerScale + 0.35, 0, 0}) * Matrix4::rotationY(pi / 2.0),
                 100 + seat);
    }

    // ---- Carousel: 3D rotation about y, horses bobbing, some with riders ------------------
    const Matrix4 carouselModel = Matrix4::translation(carouselPosition) * Matrix4::rotationY(0.2 + 0.5 * time) *
                                  Matrix4::scaling({carouselScale, carouselScale, carouselScale});
    add(MeshId::carouselBase, Matrix4::translation(carouselPosition) * Matrix4::scaling({carouselScale, carouselScale, carouselScale}), paint);
    add(MeshId::carouselRotor, carouselModel, makeMaterial(white, 0.25, 0.35));
    add(MeshId::carouselBulbs, carouselModel, bulbs(night));
    for (int index = 0; index < 8; ++index) {
        const double bob = 0.18 * std::sin(2.0 * pi * 0.6 * time + index * 1.7);
        const Matrix4 horseModel = carouselModel * Matrix4::rotationY(index * 2.0 * pi / 8.0) * Matrix4::translation({carouselHorseRadius, 1.3 + bob, 0.0});
        add(MeshId::horse, horseModel, makeMaterial(index % 2 == 0 ? astroPalette::cream : Color{255, 160, 205}, 0.0, 0.3));
        if (index % 2 == 0) addRider(frame, horseModel * Matrix4::scaling({0.8, 0.8, 0.8}) * Matrix4::translation({0, 0.15, 0}) * Matrix4::rotationY(pi), 200 + index);
    }

    // ---- The chrome "Astro globe" sculpture at the heart of the park ----------------------
    const Matrix4 globeScaling = Matrix4::scaling({globeScale, globeScale, globeScale});
    add(MeshId::mirrorBallPlinth, Matrix4::translation(globePosition) * globeScaling, makeMaterial(white, 0.1, 0.6));
    add(MeshId::mirrorBall, Matrix4::translation(globePosition + Vec3(0, (0.9 + mirrorBallRadius) * globeScale, 0)) * globeScaling,
        makeMaterial(Color{235, 238, 245}, 1.0, 0.04));

    // ---- Big top and searchlight beams -----------------------------------------------------
    add(MeshId::bigTop, Matrix4::translation(bigTopPosition) * Matrix4::scaling({2.2, 2.2, 2.2}), makeMaterial(white, 0.0, 0.8));
    if (night > 0.01) {
        for (int index = 0; index < 2; ++index) {
            const double sideTilt = (index == 0 ? 0.3 : -0.3) + oscillate(time, 0.06 + index * 0.02, 0.35, index * 1.3);
            const double forwardTilt = 0.2 + oscillate(time, 0.05, 0.12, index);
            Material beam = makeMaterial(scaleColor(index == 0 ? astroPalette::neonCyan : astroPalette::neonPink, night), 0.0, 1.0, 1.0);
            beam.lightBeam = true;
            add(MeshId::searchlightBeam, Matrix4::translation(searchlightPositions[index]) * Matrix4::rotationZ(sideTilt) * Matrix4::rotationX(forwardTilt) *
                                             Matrix4::scaling({1.4, 1.6, 1.4}), beam);
        }
    }
}

void ParkScene::addNewRides(ParkFrame& frame, double time, double night) const {
    auto add = [&](MeshId mesh, const Matrix4& model, const Material& surface) { frame.instances.push_back({mesh, model, surface}); };

    // ---- Wave swinger ----------------------------------------------------------------------
    // Chairs swing out until the chain's tension balances gravity against the centripetal
    // force: tan(theta) = omega^2 r / g, where r = R + L sin(theta) grows as they swing out,
    // so theta is found by fixed-point iteration.
    const double spin = swingSpinTrack().sample(wrapTime(time, swingSpinTrack().duration()));
    const double omega = 1.15 * spin;
    double theta = 0.0;
    for (int iteration = 0; iteration < 6; ++iteration) {
        theta = std::atan(omega * omega * (swingCanopyRadius + swingChainLength * std::sin(theta)) / gravity);
    }
    const double waveTilt = 0.13 * spin * std::sin(0.45 * time);  // the canopy rocks like a wave
    const Matrix4 canopyModel = Matrix4::translation(swingPosition + Vec3(0, swingTowerHeight + 0.4, 0)) * Matrix4::rotationX(waveTilt) *
                                Matrix4::rotationY(swingAngleAt(time));
    add(MeshId::swingTower, Matrix4::translation(swingPosition), makeMaterial(white, 0.3, 0.4));
    add(MeshId::swingCanopy, canopyModel, makeMaterial(white, 0.0, 0.45, 0.15 * night, 2.0));
    for (int chair = 0; chair < swingChairCount; ++chair) {
        const Matrix4 chairModel = canopyModel * Matrix4::rotationY(chair * 2.0 * pi / swingChairCount) *
                                   Matrix4::translation({swingCanopyRadius, -0.2, 0.0}) * Matrix4::rotationZ(theta);
        add(MeshId::swingChair, chairModel, makeMaterial(hsvToRgb(chair * 22.5, 0.6, 1.0), 0.2, 0.4));
        addRider(frame, chairModel * Matrix4::translation({0, -swingChainLength + 0.04, 0.05}) * Matrix4::rotationY(-pi / 2.0), 300 + chair);
    }

    // ---- Pirate ship: a pendulum with period T = 2 pi sqrt(L / g) ---------------------------
    const double period = 2.0 * pi * std::sqrt(shipArmLength / gravity);
    const double amplitude = shipAmplitudeTrack().sample(wrapTime(time, shipAmplitudeTrack().duration()));
    const double shipAngle = amplitude * std::sin(2.0 * pi * time / period);
    const Matrix4 hullModel = Matrix4::translation(shipPosition + Vec3(0, shipPivotHeight, 0)) * Matrix4::rotationX(shipAngle);
    add(MeshId::shipFrame, Matrix4::translation(shipPosition), makeMaterial(white, 0.6, 0.45));
    add(MeshId::shipHull, hullModel, makeMaterial(white, 0.0, 0.5));
    for (int row = 0; row < 5; ++row) {
        for (double side : {-0.7, 0.7}) {
            addRider(frame, hullModel * Matrix4::translation({side, -shipArmLength + 0.68, -4.0 + row * 2.0}) * Matrix4::rotationY(side > 0 ? -pi / 2.0 : pi / 2.0),
                     400 + row * 2 + (side > 0));
        }
    }

    // ---- Teacups: platform -> turntables -> cups, each spinning on its own --------------------
    const Color cupColors[3] = {Color{255, 170, 200}, Color{150, 210, 255}, Color{255, 230, 140}};
    const Matrix4 platformModel = Matrix4::translation(teacupPosition) * Matrix4::rotationY(0.4 * time);
    add(MeshId::teacupPlatform, Matrix4::translation(teacupPosition), paint);
    for (int table = 0; table < 3; ++table) {
        const Matrix4 tableModel = platformModel * Matrix4::rotationY(table * 2.0 * pi / 3.0) * Matrix4::translation({teacupTurntableOrbit, 0.42, 0.0}) *
                                   Matrix4::rotationY(-0.9 * time);
        add(MeshId::teacupTurntable, tableModel, makeMaterial(white, 0.6, 0.35));
        for (int cup = 0; cup < 3; ++cup) {
            const Matrix4 cupModel = tableModel * Matrix4::rotationY(cup * 2.0 * pi / 3.0) * Matrix4::translation({teacupCupOrbit, 0.15, 0.0}) *
                                     Matrix4::rotationY(1.8 * time + cup + table);
            add(MeshId::teacup, cupModel, makeMaterial(cupColors[(table + cup) % 3], 0.0, 0.2));
            addRider(frame, cupModel * Matrix4::translation({-0.45, 0.45, 0.0}) * Matrix4::rotationY(pi / 2.0), 500 + table * 3 + cup);
        }
    }

    // ---- Bumper cars: each car follows its own Lissajous curve, facing where it is going ------
    add(MeshId::bumperArena, Matrix4::translation(bumperPosition), makeMaterial(white, 0.2, 0.5, 0.2 * night, 2.0));
    SimpleRandom random(77u);
    for (int car = 0; car < 8; ++car) {
        const double speedX = random.nextRange(0.25, 0.45), speedZ = random.nextRange(0.3, 0.55);
        const double phaseX = random.nextRange(0.0, 6.28), phaseZ = random.nextRange(0.0, 6.28);
        const double x = (bumperArenaHalfWidth - 1.2) * std::sin(speedX * time + phaseX);
        const double z = (bumperArenaHalfDepth - 1.2) * std::sin(speedZ * time + phaseZ);
        const double velocityX = speedX * std::cos(speedX * time + phaseX), velocityZ = speedZ * std::cos(speedZ * time + phaseZ);
        const Matrix4 carModel = Matrix4::translation(bumperPosition + Vec3(x, 0.25, z)) * Matrix4::rotationY(std::atan2(velocityX, velocityZ));
        add(MeshId::bumperCar, carModel, makeMaterial(hsvToRgb(car * 45.0, 0.85, 0.95), 0.3, 0.2));
        addRider(frame, carModel * Matrix4::translation({0, 0.55, -0.15}), 600 + car);
    }
}

void ParkScene::addFurniture(ParkFrame& frame, double night) const {
    auto add = [&](MeshId mesh, const Matrix4& model, const Material& surface) { frame.instances.push_back({mesh, model, surface}); };
    for (const Placement& stall : stallPlacements) {
        add(stall.mesh, Matrix4::translation(stall.position) * Matrix4::rotationY(stall.rotation), makeMaterial(white, 0.0, 0.6, 0.1 * night, 2.0));
    }
    int variant = 0;
    for (const Vec3& tree : treePositions) {
        const MeshId mesh = static_cast<MeshId>(static_cast<int>(MeshId::tree0) + variant % 4);
        add(MeshId::grassPatch, Matrix4::translation(tree) * Matrix4::scaling({3.4 + variant % 3, 1.0, 3.0 + variant % 2}), makeMaterial(white, 0.0, 0.95));
        add(mesh, Matrix4::translation(tree) * Matrix4::rotationY(variant * 1.7), matte);
        ++variant;
    }
    int ringIndex = 0;
    for (const Vec3& tree : perimeterTrees) {
        const MeshId mesh = static_cast<MeshId>(static_cast<int>(MeshId::tree0) + ringIndex % 4);
        const double size = 1.1 + 0.25 * std::sin(ringIndex * 2.3);
        add(mesh, Matrix4::translation(tree) * Matrix4::rotationY(ringIndex * 0.9) * Matrix4::scaling({size, size, size}), matte);
        ++ringIndex;
    }
    // Larger lawns to break up the paving.
    const Vec3 lawns[][2] = {{{-44, 0, -14}, {12, 1, 7}}, {{46, 0, 12}, {10, 1, 8}}, {{-46, 0, 20}, {10, 1, 9}}, {{8, 0, -96}, {14, 1, 8}},
                             {{-30, 0, -86}, {10, 1, 9}}, {{50, 0, -60}, {8, 1, 14}}};
    for (const auto& lawn : lawns) add(MeshId::grassPatch, Matrix4::translation(lawn[0]) * Matrix4::scaling(lawn[1]), makeMaterial(white, 0.0, 0.95));
    for (const Vec3& lamp : lampPositions) {
        add(MeshId::lampPost, Matrix4::translation(lamp), steel);
        add(MeshId::lampGlobe, Matrix4::translation(lamp + Vec3(0, lampGlobeHeight, 0)), makeMaterial(white, 0.0, 0.2, 0.25 + 0.75 * night, 1.2 + 9.0 * night));
    }
}

void ParkScene::addCrowd(ParkFrame& frame, double time) const {
    auto add = [&](MeshId mesh, const Matrix4& model, const Material& surface) { frame.instances.push_back({mesh, model, surface}); };
    const double loopLength = routeCumulativeLength.back();
    SimpleRandom random(99u);

    // ---- Visitors walking the loop: in through the mouth, round the park, out of the gate ----
    for (int index = 0; index < 140; ++index) {
        const double start = random.nextRange(0.0, loopLength);
        const double speed = random.nextRange(1.0, 1.5);   // m/s
        const double lane = random.nextRange(-1.6, 1.6);   // sideways offset on the path
        const bool child = random.nextDouble() < 0.2;
        const double size = child ? 0.62 : random.nextRange(0.92, 1.08);
        const Material clothes = makeMaterial(clothesColor(random), 0.0, 0.8);
        const Material trousers = makeMaterial(random.nextDouble() < 0.5 ? Color{70, 90, 140} : clothesColor(random), 0.0, 0.85);
        const Material skin = makeMaterial(skinTones[random.nextInt(0, 5)], 0.0, 0.55);

        const double walked = start + speed * time;
        Vec3 position, forward;
        routeFrameAt(walked, position, forward);
        const Vec3 side = crossProduct(forward, {0, 1, 0}).normalized();
        // Legs swing with DISTANCE walked (about 1.4 m per full stride), so feet never slide.
        const double stride = 2.0 * pi * walked / (1.4 * size);
        const Matrix4 body = Matrix4::translation(position + side * lane + Vec3(0, std::fabs(std::sin(stride)) * 0.035 * size, 0)) *
                             Matrix4::rotationY(std::atan2(forward.x, forward.z)) * Matrix4::scaling({size, size, size});
        add(MeshId::personTorso, body, clothes);
        add(MeshId::personHead, body, skin);
        for (double limbSide : {-1.0, 1.0}) {
            const double swingAngle = 0.45 * std::sin(stride) * limbSide;
            add(MeshId::personLeg, body * Matrix4::translation({limbSide * 0.1, 0.86, 0}) * Matrix4::rotationX(swingAngle), trousers);
            add(MeshId::personArm, body * Matrix4::translation({limbSide * 0.24, 1.42, 0}) * Matrix4::rotationX(-swingAngle * 0.8) *
                                       Matrix4::rotationZ(limbSide * 0.08), clothes);
        }
    }

    // ---- People queuing at the food stalls ---------------------------------------------------
    for (const Placement& stall : stallPlacements) {
        for (int person = 0; person < 3; ++person) {
            const Matrix4 standing = Matrix4::translation(stall.position) * Matrix4::rotationY(stall.rotation) *
                                     Matrix4::translation({-1.0 + person * 1.0, 0.0, 2.6 + random.nextRange(0.0, 1.2)}) * Matrix4::rotationY(pi + random.nextRange(-0.3, 0.3));
            const double sway = oscillate(time, 0.2, 0.03, person + stall.position.x);
            const Matrix4 body = standing * Matrix4::rotationZ(sway);
            const Material clothes = makeMaterial(clothesColor(random), 0.0, 0.8);
            add(MeshId::personTorso, body, clothes);
            add(MeshId::personHead, body, makeMaterial(skinTones[random.nextInt(0, 5)], 0.0, 0.55));
            for (double limbSide : {-1.0, 1.0}) {
                add(MeshId::personLeg, body * Matrix4::translation({limbSide * 0.1, 0.86, 0}), makeMaterial(Color{70, 90, 140}, 0.0, 0.85));
                add(MeshId::personArm, body * Matrix4::translation({limbSide * 0.24, 1.42, 0}) * Matrix4::rotationZ(limbSide * 0.1), clothes);
            }
        }
    }
}

void ParkScene::addLighting(ParkFrame& frame, double time, double night) const {
    SceneLighting& light = frame.lighting;
    light.nightAmount = night;
    // Afternoon sun from the upper left (as on the cover), handing over to a cool moon.
    light.sunDirection = lerpVector(Vec3(-0.55, 0.58, 0.6), Vec3(0.35, 0.55, -0.75), night).normalized();
    const double dusk = 4.0 * night * (1.0 - night);  // peaks at the middle of the transition
    light.sunRadiance = Vec3(2.9, 2.7, 2.4) * std::pow(1.0 - night, 1.5) + Vec3(1.2, 0.5, 0.15) * dusk + Vec3(0.09, 0.11, 0.2) * night;
    light.skyTop = lerpColor(Color{16, 60, 168}, Color{6, 6, 24}, night);
    light.skyMid = lerpColor(Color{64, 126, 214}, Color{36, 18, 64}, night);
    light.skyHorizon = lerpColor(Color{168, 198, 232}, Color{110, 46, 86}, night);
    light.horizonGlow = scaleColor(Color{255, 120, 50}, 0.55 * night + 0.6 * dusk);
    light.groundColor = lerpColor(Color{170, 156, 136}, Color{36, 28, 40}, night);
    light.ambientStrength = 0.7 - 0.25 * night;
    light.fogColor = lerpColor(Color{172, 194, 226}, Color{46, 26, 56}, night);
    light.fogDensity = 0.0015 + 0.0035 * night;
    light.exposure = 0.95 + 0.75 * night;

    // ---- Point lights (the OpenGL renderer uses the 16 nearest the camera) --------------------
    const double pulse = 1.0 + oscillate(time, 0.8, 0.1);
    light.pointLights.push_back({portalLightPosition(), Vec3(1.0, 0.62, 0.28) * ((60.0 + 70.0 * night) * pulse)});
    light.pointLights.push_back({{0.0, 2.2, mouthTunnelBackZ() - 1.0}, Vec3(1.0, 0.62, 0.28) * (10.0 + 50.0 * night)});
    // Warm uplights at the foot of the head make it glow at night, as on the night cover.
    for (double side : {-1.0, 1.0}) {
        light.pointLights.push_back({{side * 5.5, 0.6, 9.0}, Vec3(1.0, 0.58, 0.25) * (140.0 * night)});
    }
    if (night > 0.02) {
        for (const Vec3& lamp : lampPositions) light.pointLights.push_back({lamp + Vec3(0, lampGlobeHeight, 0), Vec3(1.0, 0.82, 0.58) * (30.0 * night)});
        light.pointLights.push_back({carouselPosition + Vec3(0, 3.2, 0), Vec3(1.0, 0.8, 0.5) * (45.0 * night)});
        light.pointLights.push_back({swingPosition + Vec3(0, swingTowerHeight - 0.5, 0), Vec3(0.9, 0.55, 1.0) * (60.0 * night)});
        light.pointLights.push_back({bumperPosition + Vec3(0, 4.5, 0), Vec3(1.0, 0.75, 0.4) * (50.0 * night)});
        light.pointLights.push_back({ferrisPosition + Vec3(0, 6.0, 4.0), Vec3(1.0, 0.35, 0.75) * (60.0 * night)});
        light.pointLights.push_back({exitGatePosition + Vec3(0, 4.0, 1.5), Vec3(0.4, 0.9, 1.0) * (25.0 * night)});
        for (const Placement& stall : stallPlacements) {
            light.pointLights.push_back({Matrix4::translation(stall.position).transformPoint(Matrix4::rotationY(stall.rotation).transformPoint({0, 2.4, 2.0})),
                                         Vec3(1.0, 0.85, 0.6) * (18.0 * night)});
        }
    }
}

ParkFrame ParkScene::frameAt(double timeSeconds, double nightAmount) const {
    ParkFrame frame;
    frame.timeSeconds = timeSeconds;
    const double night = std::clamp(nightAmount, 0.0, 1.0);

    Material ground = makeMaterial(Color{206, 192, 172}, 0.0, 0.9);
    ground.texture = TextureId::ground;
    frame.instances.push_back({MeshId::ground, Matrix4::translation({0.0, 0.0, parkCentreZ}), ground});
    frame.instances.push_back({MeshId::meadow, Matrix4::translation({0.0, -0.03, 0.0}), makeMaterial(white, 0.0, 0.95)});
    Material paving = makeMaterial(Color{176, 168, 160}, 0.0, 0.8);
    paving.texture = TextureId::ground;
    frame.instances.push_back({MeshId::pathRibbon, Matrix4::identity(), paving});

    addEntrance(frame, timeSeconds, night);
    addClassicRides(frame, timeSeconds, night);
    addNewRides(frame, timeSeconds, night);
    addFurniture(frame, night);
    addCrowd(frame, timeSeconds);
    addLighting(frame, timeSeconds, night);
    return frame;
}
