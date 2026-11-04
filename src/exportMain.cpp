/**
 * @file exportMain.cpp
 * @brief Exports the 3D park and its animation for Blender (side project, branch 'blender').
 *
 * Our engine stays the single source of truth: the same ParkScene that the
 * OpenGL renderer and the ray tracer draw is sampled frame by frame and written
 * to compact files that tools/blender/buildAstroworld.py turns into a Blender
 * scene with production materials, sky, camera moves and lighting.
 *
 * Output (default output/blender/):
 *   meshes.json + meshes.bin   every mesh: float32 positions, normals, colours (sRGB), UVs; uint32 triangles
 *   objects.json               one entry per instance (its mesh and material) + the timeline's metadata
 *   frames.bin                 float32 [object][frame][16]: 3x4 model matrix (row-major), RGB tint, emission
 *   lighting.json              per frame: day/night, sun, sky colours, exposure, point lights, dread sway
 *   ground.bmp, sign.bmp       textures drawn by the CP1 software renderer
 *
 * Coordinates are ours (metres, y up, the head's face toward +z); the Blender
 * script rotates the whole park into Blender's z-up world.
 *
 * Usage: astroworldExport [--fps 24] [--seconds 30] [--start 0] [--out output/blender]
 */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "animation/astroworldTimeline.h"
#include "mesh/meshBuilder.h"
#include "park3d/headModel.h"
#include "park3d/parkScene.h"
#include "park3d/parkTextures.h"

namespace {

/** @brief Readable names for every MeshId, in enum order (used as Blender object/mesh names). */
const char* const meshNames[] = {
    "ground", "meadow", "pathRibbon", "entranceWallLeft", "entranceWallMiddle", "entranceWallRight", "exitGate",
    "head", "tunnel", "dreads",
    "ferrisStand", "ferrisRotor", "ferrisBulbs", "gondola",
    "coasterTrack", "coasterTruss", "coasterCar",
    "dropColumn", "dropLights", "dropSeats",
    "carouselBase", "carouselRotor", "carouselBulbs", "horse",
    "swingTower", "swingCanopy", "swingChair",
    "shipFrame", "shipHull",
    "teacupPlatform", "teacupTurntable", "teacup",
    "bumperArena", "bumperCar",
    "foodStallPink", "foodStallBlue", "foodStallYellow",
    "tree0", "tree1", "tree2", "tree3", "grassPatch", "lampPost", "lampGlobe",
    "statue", "wreckedCar", "rocket", "bigTop", "signFrame", "signFace", "buntingFlags", "buntingStrings",
    "personTorso", "personHead", "personArm", "personLeg", "rider", "riderHead",
    "balloon", "balloonStrings", "searchlightBeam", "mirrorBallPlinth", "mirrorBall",
};
static_assert(sizeof(meshNames) / sizeof(meshNames[0]) == meshIdCount, "meshNames must list every MeshId");

/** @brief Lines cannot be rendered by Blender's engines: rebuild every segment as a thin tube. */
MeshData linesToTubes(const MeshData& lines, double radius) {
    MeshData tubes;
    for (std::size_t index = 0; index + 1 < lines.indices.size(); index += 2) {
        const MeshVertex& a = lines.vertices[lines.indices[index]];
        const MeshVertex& b = lines.vertices[lines.indices[index + 1]];
        const Vec3 start{a.position[0], a.position[1], a.position[2]};
        const Vec3 end{b.position[0], b.position[1], b.position[2]};
        if ((end - start).length() < 1e-6) continue;
        const Color color{static_cast<std::uint8_t>(a.color[0] * 255), static_cast<std::uint8_t>(a.color[1] * 255), static_cast<std::uint8_t>(a.color[2] * 255)};
        tubes.append(makeCylinderBetween(start, end, radius, 6, color));
    }
    return tubes;
}

/** @brief Minimal 24-bit BMP writer (Blender reads BMP; our FrameBuffer only writes PPM). */
bool saveBmp(const FrameBuffer& image, const std::string& path) {
    const int width = image.getWidth(), height = image.getHeight();
    const int rowSize = (width * 3 + 3) & ~3;  // rows are padded to 4 bytes
    const std::uint32_t dataSize = static_cast<std::uint32_t>(rowSize * height);
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    auto write16 = [&](std::uint16_t v) { file.put(static_cast<char>(v & 0xFF)).put(static_cast<char>(v >> 8)); };
    auto write32 = [&](std::uint32_t v) { for (int i = 0; i < 4; ++i) file.put(static_cast<char>((v >> (8 * i)) & 0xFF)); };
    file.put('B').put('M');
    write32(54 + dataSize); write32(0); write32(54);
    write32(40); write32(static_cast<std::uint32_t>(width)); write32(static_cast<std::uint32_t>(height));
    write16(1); write16(24); write32(0); write32(dataSize); write32(2835); write32(2835); write32(0); write32(0);
    std::vector<char> row(static_cast<std::size_t>(rowSize), 0);
    for (int y = height - 1; y >= 0; --y) {  // BMP stores the bottom row first, as BGR
        for (int x = 0; x < width; ++x) {
            const Color c = image.getPixel(x, y);
            row[x * 3] = static_cast<char>(c.b); row[x * 3 + 1] = static_cast<char>(c.g); row[x * 3 + 2] = static_cast<char>(c.r);
        }
        file.write(row.data(), rowSize);
    }
    return static_cast<bool>(file);
}

/** @brief Writes float/uint arrays into one binary file and remembers their byte offsets. */
class BinaryWriter {
public:
    explicit BinaryWriter(const std::string& path) : file(path, std::ios::binary) {}
    template <typename T>
    std::size_t write(const std::vector<T>& values) {
        const std::size_t offset = position;
        file.write(reinterpret_cast<const char*>(values.data()), static_cast<std::streamsize>(values.size() * sizeof(T)));
        position += values.size() * sizeof(T);
        return offset;
    }
    bool ok() const { return static_cast<bool>(file); }

private:
    std::ofstream file;
    std::size_t position = 0;
};

std::string jsonColor(Color c) { return "[" + std::to_string(c.r) + "," + std::to_string(c.g) + "," + std::to_string(c.b) + "]"; }
std::string jsonVector(const Vec3& v) { return "[" + std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z) + "]"; }

/** @brief Append one mesh's arrays to the binary file and its description to the JSON index. */
void writeMesh(BinaryWriter& binary, std::string& json, const std::string& name, const MeshData& mesh, const MeshData* swayTarget) {
    std::vector<float> positions, normals, colors, uvs, swayPositions;
    for (const MeshVertex& v : mesh.vertices) {
        positions.insert(positions.end(), v.position, v.position + 3);
        normals.insert(normals.end(), v.normal, v.normal + 3);
        colors.insert(colors.end(), v.color, v.color + 3);
        uvs.insert(uvs.end(), v.textureCoordinate, v.textureCoordinate + 2);
    }
    json += "\"" + name + "\":{\"vertices\":" + std::to_string(mesh.vertices.size()) + ",\"indices\":" + std::to_string(mesh.indices.size());
    json += ",\"positions\":" + std::to_string(binary.write(positions));
    json += ",\"normals\":" + std::to_string(binary.write(normals));
    json += ",\"colors\":" + std::to_string(binary.write(colors));
    json += ",\"uvs\":" + std::to_string(binary.write(uvs));
    json += ",\"triangles\":" + std::to_string(binary.write(mesh.indices));
    if (swayTarget != nullptr) {
        for (const MeshVertex& v : swayTarget->vertices) swayPositions.insert(swayPositions.end(), v.position, v.position + 3);
        json += ",\"swayPositions\":" + std::to_string(binary.write(swayPositions));
    }
    json += "},";
}

}  // namespace

int main(int argumentCount, char** argumentValues) {
    int fps = 24;
    double seconds = 30.0, start = 0.0;
    std::filesystem::path out = "output/blender";
    for (int index = 1; index + 1 < argumentCount; ++index) {
        const std::string argument = argumentValues[index];
        if (argument == "--fps") fps = std::max(1, std::atoi(argumentValues[++index]));
        else if (argument == "--seconds") seconds = std::atof(argumentValues[++index]);
        else if (argument == "--start") start = std::atof(argumentValues[++index]);
        else if (argument == "--out") out = argumentValues[++index];
    }
    std::filesystem::create_directories(out);
    const ParkScene scene;
    const int frameCount = static_cast<int>(std::lround(seconds * fps));
    const double swayReference = 0.05;  // the dread bend stored as the shape key

    // ---- Meshes -----------------------------------------------------------------------------
    {
        BinaryWriter binary((out / "meshes.bin").string());
        std::string json = "{";
        for (std::size_t index = 0; index < meshIdCount; ++index) {
            const MeshId id = static_cast<MeshId>(index);
            if (id == MeshId::balloonStrings) continue;  // dynamic and hair-thin: left out
            if (id == MeshId::dreads) {
                const MeshData rest = buildDreadMesh(0.0);
                const MeshData bent = buildDreadMesh(swayReference);
                writeMesh(binary, json, meshNames[index], rest, &bent);
            } else if (ParkScene::isLineMesh(id)) {
                writeMesh(binary, json, meshNames[index], linesToTubes(scene.mesh(id), 0.025), nullptr);
            } else {
                writeMesh(binary, json, meshNames[index], scene.mesh(id), nullptr);
            }
        }
        json.back() = '}';
        std::ofstream(out / "meshes.json") << json;
        if (!binary.ok()) { std::cerr << "failed writing meshes.bin\n"; return 1; }
    }

    // ---- Animation: sample every frame, matching instances by (mesh, occurrence) ---------------
    struct ExportObject {
        MeshId mesh;
        int occurrence;
        Material material;
        std::vector<float> frames;  // frameCount x 16
        bool varies = false;
    };
    std::vector<ExportObject> objects;
    std::map<std::pair<int, int>, std::size_t> objectIndex;
    std::string lightingJson = "{\"fps\":" + std::to_string(fps) + ",\"frames\":[";
    std::size_t maxLights = 0;

    for (int frameNumber = 0; frameNumber < frameCount; ++frameNumber) {
        const double time = start + static_cast<double>(frameNumber) / fps;
        const ParkFrame frame = scene.frameAt(time, nightAmountAt(time));
        std::map<int, int> occurrences;
        for (const Instance& instance : frame.instances) {
            if (instance.mesh == MeshId::balloonStrings) continue;
            const int occurrence = occurrences[static_cast<int>(instance.mesh)]++;
            const auto key = std::make_pair(static_cast<int>(instance.mesh), occurrence);
            auto found = objectIndex.find(key);
            if (found == objectIndex.end()) {
                found = objectIndex.emplace(key, objects.size()).first;
                objects.push_back({instance.mesh, occurrence, instance.material, std::vector<float>(static_cast<std::size_t>(frameCount) * 16, 0.0f)});
            }
            ExportObject& object = objects[found->second];
            float* row = &object.frames[static_cast<std::size_t>(frameNumber) * 16];
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 4; ++c) row[r * 4 + c] = static_cast<float>(instance.model.at(r, c));
            }
            row[12] = instance.material.tint.r / 255.0f;
            row[13] = instance.material.tint.g / 255.0f;
            row[14] = instance.material.tint.b / 255.0f;
            row[15] = static_cast<float>(instance.material.emissive * instance.material.emissionStrength);
        }
        // Lighting for this frame.
        const SceneLighting& light = frame.lighting;
        maxLights = std::max(maxLights, light.pointLights.size());
        lightingJson += "{\"time\":" + std::to_string(time) + ",\"night\":" + std::to_string(light.nightAmount) +
                        ",\"sunDirection\":" + jsonVector(light.sunDirection) + ",\"sunRadiance\":" + jsonVector(light.sunRadiance) +
                        ",\"skyTop\":" + jsonColor(light.skyTop) + ",\"skyMid\":" + jsonColor(light.skyMid) + ",\"skyHorizon\":" + jsonColor(light.skyHorizon) +
                        ",\"horizonGlow\":" + jsonColor(light.horizonGlow) + ",\"exposure\":" + std::to_string(light.exposure) +
                        ",\"fogDensity\":" + std::to_string(light.fogDensity) + ",\"dreadSway\":" + std::to_string(frame.dreadSwayAngle / swayReference) +
                        ",\"lights\":[";
        for (const PointLight& point : light.pointLights) lightingJson += "[" + jsonVector(point.position) + "," + jsonVector(point.radiance) + "],";
        if (lightingJson.back() == ',') lightingJson.pop_back();
        lightingJson += "]},";
        if ((frameNumber + 1) % fps == 0) std::cerr << "\r  sampled " << frameNumber + 1 << "/" << frameCount << " frames" << std::flush;
    }
    std::cerr << "\n";
    lightingJson.back() = ']';
    lightingJson += ",\"maxLights\":" + std::to_string(maxLights) + "}";
    std::ofstream(out / "lighting.json") << lightingJson;

    // ---- Objects + per-frame data ------------------------------------------------------------
    BinaryWriter frames((out / "frames.bin").string());
    std::string objectsJson = "{\"fps\":" + std::to_string(fps) + ",\"frameCount\":" + std::to_string(frameCount) + ",\"objects\":[";
    std::size_t animated = 0;
    for (ExportObject& object : objects) {
        for (int frameNumber = 1; frameNumber < frameCount && !object.varies; ++frameNumber) {
            object.varies = !std::equal(object.frames.begin(), object.frames.begin() + 16, object.frames.begin() + frameNumber * 16);
        }
        animated += object.varies;
        frames.write(object.frames);
        const Material& m = object.material;
        objectsJson += "{\"mesh\":\"" + std::string(meshNames[static_cast<int>(object.mesh)]) + "\",\"occurrence\":" + std::to_string(object.occurrence) +
                       ",\"animated\":" + (object.varies ? "true" : "false") + ",\"metallic\":" + std::to_string(m.metallic) +
                       ",\"roughness\":" + std::to_string(m.roughness) + ",\"texture\":\"" +
                       (m.texture == TextureId::ground ? "ground" : m.texture == TextureId::sign ? "sign" : "") + "\",\"lightBeam\":" +
                       (m.lightBeam ? "true" : "false") + "},";
    }
    objectsJson.back() = ']';
    objectsJson += "}";
    std::ofstream(out / "objects.json") << objectsJson;

    // ---- Textures -------------------------------------------------------------------------------
    saveBmp(drawGroundTexture(), (out / "ground.bmp").string());
    saveBmp(drawSignTexture(), (out / "sign.bmp").string());

    std::cerr << "Exported " << objects.size() << " objects (" << animated << " animated), " << frameCount << " frames at " << fps
              << " fps, up to " << maxLights << " point lights -> " << out.string() << "\n";
    return frames.ok() ? 0 : 1;
}
