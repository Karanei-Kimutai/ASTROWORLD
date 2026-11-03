/**
 * @file parkScene.h
 * @brief A renderer-independent description of the 3D park at any moment in time.
 *
 * The park is a list of INSTANCES: "draw mesh M with model matrix T and
 * material P". This is the classic scene-graph split between WHAT is in the
 * scene and HOW it is drawn:
 *   - the OpenGL renderer uploads each mesh once and issues one draw call per instance;
 *   - the ray tracer (final phase) transforms the same meshes into world-space
 *     triangles and traces rays through them.
 * Both therefore show exactly the same geometry, lighting and animation.
 */
#pragma once

#include <array>
#include <vector>

#include "mesh/meshData.h"

/** @brief Every mesh the park uses, by name. */
enum class MeshId {
    ground, fence, head, portal, dreads,
    ferrisStand, ferrisRotor, ferrisBulbs, gondola,
    coasterTrack, coasterTruss, coasterCar,
    dropColumn, dropLights, dropSeats,
    carouselBase, carouselRotor, carouselBulbs, horse,
    rocket, bigTop, signFrame, signFace, buntingFlags, buntingStrings,
    personTorso, personLeg, balloon, balloonStrings, searchlightBeam,
    count
};
inline constexpr std::size_t meshIdCount = static_cast<std::size_t>(MeshId::count);

/** @brief Which image (if any) is mapped onto a surface. */
enum class TextureId { none, sign, ground };

/** @brief How a surface responds to light. */
struct Material {
    Color tint{255, 255, 255};       // multiplies the mesh's vertex colours
    double emissive = 0.0;           // 0 = lit, 1 = glows with its own colour
    double specularStrength = 0.25;  // Phong specular coefficient
    double shininess = 24.0;         // Phong specular exponent
    double reflectivity = 0.0;       // mirror reflection (used by the ray tracer)
    TextureId texture = TextureId::none;
    bool lightBeam = false;          // transparent additive searchlight beam (OpenGL only)
};

/** @brief One thing to draw: a mesh, where it is, and what it is made of. */
struct Instance {
    MeshId mesh;
    Matrix4 model;
    Material material;
};

/** @brief Lights, fog and sky for one moment (from the day/night amount). */
struct SceneLighting {
    double nightAmount = 0.0;
    Vec3 sunDirection;      // unit vector toward the sun / moon
    Vec3 sunColor;
    Vec3 ambientColor;
    Vec3 portalPosition;    // point light inside the mouth
    Vec3 portalColor;
    double portalIntensity = 0.0;
    Color fogColor;
    double fogDensity = 0.0;
    Color skyTop, skyMid, skyHorizon;
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
    void addRides(std::vector<Instance>& instances, double time, double night) const;
    void addPeople(ParkFrame& frame, double time) const;
    void addSearchlights(std::vector<Instance>& instances, double time, double night) const;

    /** @brief Point and unit tangent on the coaster track at an arc-length distance. */
    void coasterFrameAt(double distance, Vec3& position, Vec3& tangent) const;

    std::array<MeshData, meshIdCount> meshes;
    std::vector<Vec3> coasterPath;
    std::vector<double> coasterCumulativeLength;
};
