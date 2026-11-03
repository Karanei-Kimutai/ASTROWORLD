/**
 * @file astroworldPark.h
 * @brief The OpenGL renderer for the park: uploads the ParkScene's meshes once,
 *        then draws any ParkFrame with the chosen camera and shading options.
 */
#pragma once

#include <array>
#include <string>

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
    bool wireframe = false;     // draw triangle edges only, to show the tessellation
    bool textures = true;       // texture mapping on / off
    bool mipmapping = true;     // trilinear mipmaps on / off (off shows texture aliasing)
    bool showLogo = true;
};

class AstroworldPark {
public:
    /** @brief Width and height of the image the park is rendered at (same as the software renderer). */
    static constexpr int renderWidth = 1280;
    static constexpr int renderHeight = 720;

    /** @brief Compile shaders, upload every mesh and texture. Needs a current GL context. */
    bool initialise(const ParkScene& scene, std::string& errorOut);

    /** @brief Draw one frame into the currently bound frame buffer. */
    void render(const ParkFrame& frame, const CameraMatrices& camera, const RenderOptions& options);

    /** @brief Draw a texture as a screen overlay with its top-left at (left, top) in pixels. */
    void drawOverlay(const Texture2D& texture, int left, int top);

    /** @brief Upload a software-rendered RGBA image into a texture. */
    static void uploadImage(Texture2D& texture, const RgbaImage& image, bool repeat = false, bool mipmapped = false) {
        texture.upload(image.width, image.height, image.pixels.data(), repeat, mipmapped);
    }

private:
    void drawSky(const ParkFrame& frame, const CameraMatrices& camera);
    void setLighting(const SceneLighting& lighting, const CameraMatrices& camera, const RenderOptions& options);

    ShaderProgram litShader;
    ShaderProgram skyShader;
    ShaderProgram beamShader;
    ShaderProgram overlayShader;
    GLuint emptyVertexArray = 0;  // core profile needs a VAO bound even for the attribute-less sky triangle

    std::array<GpuMesh, meshIdCount> meshes;
    GpuMesh overlayQuad;
    Texture2D signTexture, groundTexture, logoTexture, footerTexture;
};
