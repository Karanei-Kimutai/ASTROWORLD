/**
 * @file astroworldPark.h
 * @brief The OpenGL renderer for the park: uploads the ParkScene's meshes once,
 *        then draws any ParkFrame photorealistically (shadows, PBR, HDR, bloom).
 */
#pragma once

#include <array>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "gl/framePipeline.h"
#include "gl/glResources.h"
#include "glScene/overlayImage.h"
#include "park3d/orbitCamera.h"
#include "park3d/parkScene.h"

/** @brief Surface shading interpolation, switchable at run time to compare them. */
enum class ShadingMode { flat = 0, gouraud = 1, phong = 2 };

inline const char* shadingModeName(ShadingMode mode) {
    switch (mode) {
        case ShadingMode::flat: return "FLAT";
        case ShadingMode::gouraud: return "GOURAUD";
        default: return "PHONG";
    }
}

/** @brief Display options that do not change the scene itself. */
struct RenderOptions {
    ShadingMode shading = ShadingMode::phong;
    bool wireframe = false;
    bool textures = true;
    bool mipmapping = true;
    bool shadows = true;
    bool bloom = true;
    bool showLogo = true;
};

class AstroworldPark {
public:
    static constexpr int renderWidth = 1280;
    static constexpr int renderHeight = 720;
    static constexpr int shadowMapSize = 4096;

    /** @brief Compile shaders, create frame buffers, upload every mesh and texture. Needs a current GL context. */
    bool initialise(const ParkScene& scene, std::string& errorOut);

    /** @brief Render one frame into the final 8-bit image (FramePipeline::bindFinal). */
    void render(const ParkFrame& frame, const CameraMatrices& camera, const RenderOptions& options);

    /** @brief Draw a texture over the final image with its top-left at (left, top) in pixels. */
    void drawOverlay(const Texture2D& texture, int left, int top);

    const FramePipeline& pipeline() const { return frames; }

    static void uploadImage(Texture2D& texture, const RgbaImage& image, bool repeat = false, bool mipmapped = false) {
        texture.upload(image.width, image.height, image.pixels.data(), repeat, mipmapped);
    }

private:
    /**
     * @brief All instances that share a mesh and a material, packed for ONE instanced draw call.
     *        (Thousands of instances become ~150 draws: every visitor's torso, every tree...)
     */
    struct Batch {
        MeshId mesh;
        Material material;
        std::vector<float> instanceData;
        int count = 0;
    };
    using BatchKey = std::tuple<int, double, double, double, double, int>;
    void buildBatches(const ParkFrame& frame);

    void renderShadowMap(const Matrix4& lightViewProjection);
    void drawSky(const ParkFrame& frame, const CameraMatrices& camera);
    void setLighting(const ParkFrame& frame, const CameraMatrices& camera, const RenderOptions& options, const Matrix4& lightViewProjection);
    void drawFullScreen();
    void postProcess(const ParkFrame& frame, const RenderOptions& options);

    ShaderProgram litShader, depthShader, skyShader, beamShader, brightPassShader, blurShader, compositeShader, overlayShader;
    GLuint emptyVertexArray = 0;  // core profile needs a VAO bound even for attribute-less full-screen triangles

    FramePipeline frames;
    ShadowMap shadowMap;
    std::array<GpuMesh, meshIdCount> meshes;
    GpuMesh overlayQuad;
    Texture2D signTexture, groundTexture, logoTexture, footerTexture;
    std::map<BatchKey, Batch> batches;  // rebuilt every frame
};
