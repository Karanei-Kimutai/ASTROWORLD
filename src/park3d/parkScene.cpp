/**
 * @file parkScene.cpp
 * @brief Where everything in the 3D park stands, and how it moves over time.
 *
 * World units are roughly metres: y up, the head stands at the origin with its
 * mouth facing +z, the fence runs along z = -9.5 and the rides stand behind it.
 * Every animated value comes from the shared timeline (animation/astroworldTimeline.h)
 * or the same oscillators the 2D renderer uses, so all renderers agree.
 */
#include "park3d/parkScene.h"

#include <algorithm>
#include <cmath>

#include "animation/astroworldTimeline.h"
#include "animation/easing.h"
#include "core/simpleRandom.h"
#include "mesh/meshBuilder.h"
#include "park3d/headModel.h"
#include "park3d/parkModels.h"

namespace {

const Vec3 ferrisPosition{-8.0, 0.0, -17.0};
const Vec3 dropTowerPosition{16.5, 0.0, -20.0};
const Vec3 carouselPosition{9.5, 0.0, 4.0};
const Vec3 rocketPosition{-6.5, 0.0, 3.5};
const Vec3 bigTopPosition{-14.0, 0.0, -13.5};
const Vec3 signPosition{-21.0, 0.0, -11.0};
const Vec3 vendorPosition{-9.5, 0.0, 8.5};
const Vec3 searchlightPositions[2] = {{-6.0, 0.0, -24.0}, {7.0, 0.0, -26.0}};
const double fenceZ = -9.5;

Vec3 lerpVector(const Vec3& a, const Vec3& b, double t) { return a + (b - a) * t; }

/** @brief Shorthand for building a material. */
Material material(Color tint, double emissive = 0.0, double specularStrength = 0.25, double shininess = 24.0, double reflectivity = 0.0) {
    Material result;
    result.tint = tint;
    result.emissive = emissive;
    result.specularStrength = specularStrength;
    result.shininess = shininess;
    result.reflectivity = reflectivity;
    return result;
}

const Color white = astroPalette::white;

}  // namespace

ParkScene::ParkScene() {
    auto set = [this](MeshId id, MeshData data) { meshes[static_cast<std::size_t>(id)] = std::move(data); };
    // 400 x 400 units with the ground texture repeating every 4 units.
    set(MeshId::ground, makeGroundPlane(400.0, 400.0, astroPalette::white, 100.0));
    set(MeshId::fence, buildFence(45.0));
    set(MeshId::head, buildHeadMesh());
    set(MeshId::portal, buildPortalMesh());
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
    set(MeshId::rocket, buildRocket());
    set(MeshId::bigTop, buildBigTop());
    set(MeshId::signFrame, buildSignFrame());
    set(MeshId::signFace, buildSignFace());
    set(MeshId::buntingFlags, buildBuntingFlags());
    set(MeshId::buntingStrings, buildBuntingStrings());
    set(MeshId::personTorso, buildPersonTorso());
    set(MeshId::personLeg, buildPersonLeg());
    set(MeshId::balloon, buildBalloon());
    set(MeshId::searchlightBeam, buildSearchlightBeam());

    // Arc length along the coaster, for placing cars by distance.
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

bool ParkScene::coasterRideView(double timeSeconds, Vec3& eye, Vec3& forward, Vec3& up) const {
    const double distance = coasterTrainFractionAt(timeSeconds) * coasterCumulativeLength.back();
    Vec3 position, tangent;
    coasterFrameAt(distance, position, tangent);
    // Look a little ahead along the track (smoother than the instantaneous tangent).
    Vec3 aheadPosition, aheadTangent;
    coasterFrameAt(distance + 3.0, aheadPosition, aheadTangent);
    const Vec3 lateral = crossProduct(tangent, {0, 1, 0}).normalized();
    up = crossProduct(lateral, tangent).normalized();
    // Sit at the nose of the front car, ahead of the riders' heads (car-local x = +0.65).
    eye = position + tangent * 0.65 + up * 1.05;
    forward = (aheadPosition + up * 1.0 - eye).normalized();
    return distance > 0.0;
}

void ParkScene::addRides(std::vector<Instance>& instances, double time, double night) const {
    auto add = [&](MeshId mesh, const Matrix4& model, const Material& surface) { instances.push_back({mesh, model, surface}); };

    // ---- Ferris wheel: a transformation hierarchy ------------------------------------
    // rotor = T(hub) * Rz(-angle): negative so it turns clockwise as seen from the front, like the 2D wheel.
    const double wheelAngle = -(0.12 + 0.25 * time);
    const Matrix4 rotorModel = Matrix4::translation(ferrisPosition + Vec3(0, ferrisHubHeight, 0)) * Matrix4::rotationZ(wheelAngle);
    add(MeshId::ferrisStand, Matrix4::translation(ferrisPosition), material(white));
    add(MeshId::ferrisRotor, rotorModel, material(white, 0.0, 0.5, 32.0, 0.15));
    add(MeshId::ferrisBulbs, rotorModel, material(white, 1.0));
    const double swing = oscillate(time, 0.4, 0.08);
    for (int index = 0; index < ferrisGondolaCount; ++index) {
        const double spokeAngle = 2.0 * pi * index / ferrisGondolaCount;
        // The mount point rides on the rotor, but the gondola takes only its POSITION, so it hangs level.
        const Vec3 mount = rotorModel.transformPoint(Vec3(std::cos(spokeAngle), std::sin(spokeAngle), 0.0) * ferrisRadius);
        add(MeshId::gondola, Matrix4::translation(mount) * Matrix4::rotationZ(swing * std::cos(index * 1.3)),
            material(hsvToRgb(index * 360.0 / ferrisGondolaCount, 0.7, 0.95), 0.0, 0.4, 24.0));
    }

    // ---- Roller coaster: cars placed by arc length, oriented along the track ----------
    add(MeshId::coasterTrack, Matrix4::identity(), material(white, 0.0, 0.6, 40.0, 0.2));
    add(MeshId::coasterTruss, Matrix4::identity(), material(white, 0.85));
    const double leadDistance = coasterTrainFractionAt(time) * coasterCumulativeLength.back();
    for (int car = 0; car < 3; ++car) {
        const double distance = leadDistance - car * 1.35;
        if (distance < 0.0) continue;
        Vec3 position, forward;
        coasterFrameAt(distance, position, forward);
        // Car axes: forward along the tangent, up perpendicular to it, sideways from their cross product.
        const Vec3 lateral = crossProduct(forward, {0, 1, 0}).normalized();
        const Vec3 up = crossProduct(lateral, forward);
        add(MeshId::coasterCar, Matrix4::fromBasis(forward, up, crossProduct(forward, up), position + up * 0.1),
            material(hsvToRgb(190.0 + car * 40.0, 0.8, 1.0), 0.0, 0.6, 48.0, 0.1));
    }

    // ---- Drop tower: keyframed seat ring -----------------------------------------------
    add(MeshId::dropColumn, Matrix4::translation(dropTowerPosition), material(white, 0.0, 0.4, 32.0));
    add(MeshId::dropLights, Matrix4::translation(dropTowerPosition), material(white, 1.0));
    const double seatHeight = 1.4 + dropTowerLiftAt(time) * (dropTowerHeight - 3.0);
    add(MeshId::dropSeats, Matrix4::translation(dropTowerPosition + Vec3(0, seatHeight, 0)), material(white, 0.0, 0.5, 32.0, 0.1));

    // ---- Carousel: 3D rotation about y, horses bobbing on their poles ------------------
    const Matrix4 carouselModel = Matrix4::translation(carouselPosition) * Matrix4::rotationY(0.2 + 0.7 * time);
    add(MeshId::carouselBase, Matrix4::translation(carouselPosition), material(white));
    add(MeshId::carouselRotor, carouselModel, material(white, 0.0, 0.35, 24.0));
    add(MeshId::carouselBulbs, carouselModel, material(white, 1.0));
    for (int index = 0; index < 8; ++index) {
        const double bob = 0.18 * std::sin(2.0 * pi * 0.8 * time + index * 1.7);
        add(MeshId::horse,
            carouselModel * Matrix4::rotationY(index * 2.0 * pi / 8.0) * Matrix4::translation({carouselHorseRadius, 1.3 + bob, 0.0}),
            material(index % 2 == 0 ? astroPalette::cream : Color{255, 160, 205}, 0.0, 0.5, 32.0));
    }

    // ---- Static props -------------------------------------------------------------------
    add(MeshId::rocket, Matrix4::translation(rocketPosition), material(white, 0.0, 0.9, 64.0, 0.35));
    add(MeshId::bigTop, Matrix4::translation(bigTopPosition), material(white));
    add(MeshId::signFrame, Matrix4::translation(signPosition), material(white));
    Material signFace = material(white);
    signFace.texture = TextureId::sign;
    add(MeshId::signFace, Matrix4::translation(signPosition), signFace);
    add(MeshId::buntingFlags, Matrix4::identity(), material(white));
    add(MeshId::buntingStrings, Matrix4::identity(), material(white, 1.0));

    // ---- The head: inflatable breathing = scaling about its base (the local origin) ----
    const double breathing = 1.0 + oscillate(time, 0.25, 0.012);
    const Matrix4 headModel = Matrix4::scaling({breathing, breathing, breathing});
    add(MeshId::head, headModel, material(white, 0.0, 0.45, 18.0, 0.12));
    add(MeshId::dreads, headModel, material(white, 0.0, 0.45, 18.0, 0.12));
    add(MeshId::portal, headModel, material(scaleColor(white, 0.9 + 0.1 * night), 1.0));
}

void ParkScene::addPeople(ParkFrame& frame, double time) const {
    auto add = [&](MeshId mesh, const Matrix4& model, const Material& surface) { frame.instances.push_back({mesh, model, surface}); };

    // ---- Visitors walking into the mouth --------------------------------------------------
    const double doorwayZ = portalLightPosition().z - 1.2;
    SimpleRandom random(99u);
    for (int index = 0; index < 14; ++index) {
        const Vec3 start{random.nextRange(-13.0, 13.0), 0.0, random.nextRange(15.0, 19.0)};
        const Vec3 end{random.nextRange(-0.6, 0.6), 0.0, doorwayZ};
        const double walkDuration = random.nextRange(12.0, 18.0);
        const double phase = random.nextDouble();
        const Color clothes = hsvToRgb(random.nextRange(0.0, 360.0), 0.55, 0.75);

        const double progress = wrapTime(time / walkDuration + phase, 1.0);
        const Vec3 feet = lerpVec3(start, end, progress);
        const double heading = std::atan2(end.x - start.x, end.z - start.z);
        // Shrink into the light over the last 6% of the walk (scaling about the feet).
        const double size = progress > 0.94 ? (1.0 - progress) / 0.06 : 1.0;
        const double stride = 2.0 * pi * 1.4 * time + index;
        const Matrix4 bodyModel = Matrix4::translation(feet + Vec3(0, std::fabs(std::sin(stride)) * 0.04, 0)) *
                                  Matrix4::rotationY(heading) * Matrix4::scaling({size, size, size});
        add(MeshId::personTorso, bodyModel, material(clothes));
        // Legs: children of the body, swinging about the hip in opposite phase.
        for (double side : {-1.0, 1.0}) {
            add(MeshId::personLeg, bodyModel * Matrix4::translation({side * 0.1, 0.88, 0}) * Matrix4::rotationX(side * 0.4 * std::sin(stride)),
                material(white));
        }
    }

    // ---- Balloon vendor ------------------------------------------------------------------
    const Matrix4 vendorModel = Matrix4::translation(vendorPosition) * Matrix4::rotationY(0.4);
    add(MeshId::personTorso, vendorModel, material(Color{200, 60, 60}));
    for (double side : {-1.0, 1.0}) add(MeshId::personLeg, vendorModel * Matrix4::translation({side * 0.1, 0.88, 0}), material(white));
    const Vec3 hand = vendorPosition + Vec3(0.3, 1.45, 0.1);
    const Vec3 offsets[5] = {{-0.6, 2.3, 0.0}, {0.1, 2.65, 0.2}, {0.75, 2.2, -0.1}, {-0.25, 1.7, 0.3}, {0.5, 1.6, 0.25}};
    const Color colors[5] = {astroPalette::neonPink, astroPalette::gold, astroPalette::neonCyan, astroPalette::neonPurple, astroPalette::neonOrange};
    std::vector<Vec3> stringEnds;
    for (int index = 0; index < 5; ++index) {
        // The bunch sways about the hand (rotation about a pivot).
        const double sway = oscillate(time, 0.25, 0.08) + oscillate(time, 0.6, 0.03, index * 1.9);
        const Vec3 center = hand + Matrix4::rotationZ(sway).transformPoint(offsets[index]);
        add(MeshId::balloon, Matrix4::translation(center), material(colors[index], 0.0, 0.8, 48.0, 0.2));
        stringEnds.insert(stringEnds.end(), {hand, center + Vec3(0, -0.45, 0)});
    }
    frame.balloonStrings = makeLines(stringEnds, Color{230, 225, 235});
    add(MeshId::balloonStrings, Matrix4::identity(), material(white, 1.0));
}

void ParkScene::addSearchlights(std::vector<Instance>& instances, double time, double night) const {
    if (night <= 0.01) return;
    for (int index = 0; index < 2; ++index) {
        // Sweep: tilt the upward beam about z (side to side) and x (toward/away), like the 2D beams.
        const double sideTilt = (index == 0 ? 0.35 : -0.3) + oscillate(time, 0.07 + index * 0.02, 0.35, index * 1.3);
        const double forwardTilt = -0.15 + oscillate(time, 0.05, 0.1, index);
        const Matrix4 model = Matrix4::translation(searchlightPositions[index] + Vec3(0, 0.6, 0)) * Matrix4::rotationZ(sideTilt) *
                              Matrix4::rotationX(forwardTilt);
        // The beam fades in with the night: its tint is the lamp colour scaled by nightAmount.
        Material beam = material(scaleColor(index == 0 ? astroPalette::neonCyan : astroPalette::neonPink, night), 1.0);
        beam.lightBeam = true;
        instances.push_back({MeshId::searchlightBeam, model, beam});
    }
}

ParkFrame ParkScene::frameAt(double timeSeconds, double nightAmount) const {
    ParkFrame frame;
    frame.timeSeconds = timeSeconds;
    const double night = std::clamp(nightAmount, 0.0, 1.0);

    // ---- Lighting: sun by day, dim blue moon by night, a warm point light in the mouth ----
    SceneLighting& light = frame.lighting;
    light.nightAmount = night;
    light.sunDirection = Vec3(-0.45, 0.75, 0.55).normalized();
    light.sunColor = lerpVector({1.0, 0.95, 0.85}, {0.18, 0.2, 0.36}, night);
    light.ambientColor = lerpVector({0.42, 0.47, 0.58}, {0.09, 0.08, 0.18}, night);
    light.portalPosition = portalLightPosition();
    light.portalColor = {1.0, 0.68, 0.3};
    light.portalIntensity = (0.6 + 2.6 * night) * (1.0 + oscillate(timeSeconds, 0.8, 0.12));
    light.skyTop = lerpColor(astroPalette::daySkyTop, astroPalette::nightSkyTop, night);
    light.skyMid = lerpColor(astroPalette::daySkyMid, astroPalette::nightSkyMid, night);
    light.skyHorizon = lerpColor(astroPalette::daySkyHorizon, astroPalette::nightSkyHorizon, night);
    light.fogColor = light.skyHorizon;
    light.fogDensity = 0.009;

    // ---- Instances ------------------------------------------------------------------------
    Material groundMaterial = material(lerpColor(astroPalette::white, Color{90, 80, 105}, night), 0.0, 0.05, 8.0);
    groundMaterial.texture = TextureId::ground;
    frame.instances.push_back({MeshId::ground, Matrix4::identity(), groundMaterial});
    frame.instances.push_back({MeshId::fence, Matrix4::translation({0, 0, fenceZ}), material(white, 0.0, 0.3, 16.0)});
    addRides(frame.instances, timeSeconds, night);
    frame.dreads = buildDreadMesh(oscillate(timeSeconds, 0.3, 0.035) + oscillate(timeSeconds, 0.71, 0.012));
    addPeople(frame, timeSeconds);
    addSearchlights(frame.instances, timeSeconds, night);
    return frame;
}
