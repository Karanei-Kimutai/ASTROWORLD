/**
 * @file parkScene.h
 * @brief A renderer-independent description of the 3D park at any moment in time.
 *
 * The park is a list of INSTANCES: "draw mesh M with model matrix T and
 * material P", plus the lights. This is the classic scene-graph split between
 * WHAT is in the scene and HOW it is drawn:
 *   - the OpenGL renderer uploads each mesh once and issues one draw call per instance;
 *   - the ray tracer transforms the same meshes into world-space triangles.
 * Both therefore show exactly the same geometry, lighting and animation.
 */
#pragma once

#include <array>
#include <vector>

#include "mesh/meshData.h"

/** @brief Every mesh the park uses, by name. */
enum class MeshId {
    ground, meadow, pathRibbon, entranceWallLeft, entranceWallMiddle, entranceWallRight, exitGate,
    head, tunnel, dreads,
    ferrisStand, ferrisRotor, ferrisBulbs, gondola,
    coasterTrack, coasterTruss, coasterCar,
    dropColumn, dropLights, dropSeats,
    carouselBase, carouselRotor, carouselBulbs, horse,
    swingTower, swingCanopy, swingChair,
    shipFrame, shipHull,
    teacupPlatform, teacupTurntable, teacup,
    bumperArena, bumperCar,
    foodStallPink, foodStallBlue, foodStallYellow,
    tree0, tree1, tree2, tree3, grassPatch, lampPost, lampGlobe,
    statue, wreckedCar, rocket, bigTop, signFrame, signFace, buntingFlags, buntingStrings,
    personTorso, personHead, personArm, personLeg, rider, riderHead,
    balloon, balloonStrings, searchlightBeam, mirrorBallPlinth, mirrorBall,
    count
};
inline constexpr std::size_t meshIdCount = static_cast<std::size_t>(MeshId::count);

/** @brief Which image (if any) is mapped onto a surface. */
enum class TextureId { none, sign, ground };

/**
 * @brief How a surface responds to light - the physically based (metallic / roughness) model.
 *   metallic  0 = dielectric (paint, vinyl, concrete, cloth), 1 = metal (gold, chrome)
 *   roughness 0 = mirror-smooth, 1 = completely matte
 */
struct Material {
    Color tint{255, 255, 255};      // multiplies the mesh's vertex colours (sRGB)
    double metallic = 0.0;
    double roughness = 0.6;
    double emissive = 0.0;          // 0 = lit, 1 = glows with its own colour
    double emissionStrength = 1.0;  // HDR brightness of the glow (above ~1 it blooms)
    TextureId texture = TextureId::none;
    bool lightBeam = false;         // transparent additive searchlight beam (OpenGL only)
};

/** @brief One thing to draw: a mesh, where it is, and what it is made of. */
struct Instance {
    MeshId mesh;
    Matrix4 model;
    Material material;
};

/** @brief A point light (lamp, glowing ride, the mouth). Radiance is linear HDR. */
struct PointLight {
    Vec3 position;
    Vec3 radiance;
};

/** @brief Lights, sky, fog and exposure for one moment (from the day/night amount). */
struct SceneLighting {
    double nightAmount = 0.0;
    Vec3 sunDirection;               // unit vector toward the sun / moon
    Vec3 sunRadiance;                // linear HDR
    Color skyTop, skyMid, skyHorizon;  // sRGB
    Color horizonGlow;               // warm glow low on the horizon (sRGB, strongest at night)
    Color groundColor;               // light bounced up from the ground (sRGB)
    double ambientStrength = 1.0;
    std::vector<PointLight> pointLights;
    Color fogColor;                  // sRGB
    double fogDensity = 0.0;
    double exposure = 1.0;
};

/** @brief Everything needed to render one frame. */
struct ParkFrame {
    double timeSeconds = 0.0;
    std::vector<Instance> instances;
    MeshData dreads;          // re-meshed every frame (they bend in the wind)
    MeshData balloonStrings;  // follow the swaying balloons
    SceneLighting lighting;
};

class ParkScene {
public:
    /** @brief Build every static mesh (CPU only - no graphics API needed). */
    ParkScene();

    const MeshData& mesh(MeshId id) const { return meshes[static_cast<std::size_t>(id)]; }

    /** @brief True for meshes made of line segments rather than triangles. */
    static bool isLineMesh(MeshId id) {
        return id == MeshId::coasterTruss || id == MeshId::buntingStrings || id == MeshId::balloonStrings;
    }

    /** @brief The whole park at time t (seconds), with the day/night amount given. */
    ParkFrame frameAt(double timeSeconds, double nightAmount) const;

    /**
     * @brief First-person view from the front car of the coaster at time t.
     * @return false if the train is not on the track at that moment.
     */
    bool coasterRideView(double timeSeconds, Vec3& eye, Vec3& forward, Vec3& up) const;

private:
    void addEntrance(ParkFrame& frame, double time, double night) const;
    void addClassicRides(ParkFrame& frame, double time, double night) const;
    void addNewRides(ParkFrame& frame, double time, double night) const;
    void addFurniture(ParkFrame& frame, double night) const;
    void addCrowd(ParkFrame& frame, double time) const;
    void addLighting(ParkFrame& frame, double time, double night) const;
    void addRider(ParkFrame& frame, const Matrix4& seat, int seed) const;

    void coasterFrameAt(double distance, Vec3& position, Vec3& tangent) const;
    /** @brief Point and direction on the visitors' walking loop at an arc-length distance. */
    void routeFrameAt(double distance, Vec3& position, Vec3& tangent) const;

    std::array<MeshData, meshIdCount> meshes;
    std::vector<Vec3> coasterPath;
    std::vector<double> coasterCumulativeLength;
    std::vector<Vec3> walkingRoute;
    std::vector<double> routeCumulativeLength;
    std::vector<Vec3> lampPositions;
    std::vector<Vec3> perimeterTrees;
};
