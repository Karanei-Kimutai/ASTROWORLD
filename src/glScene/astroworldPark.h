/**
 * @file astroworldPark.h
 * @brief The OpenGL ASTROWORLD park: owns every GPU resource and draws one frame at a given time.
 */
#pragma once

#include <string>
#include <vector>

#include "gl/glResources.h"
#include "glScene/orbitCamera.h"
#include "glScene/overlayImage.h"

/** @brief Everything that decides what one OpenGL frame looks like. */
struct ParkFrameSettings {
    double timeSeconds = 0.0;
    double nightAmount = 0.0;
    OrbitCamera camera;
    bool showLogo = true;
};

class AstroworldPark {
public:
    /** @brief Width and height of the image the park is rendered at (same as the software renderer). */
    static constexpr int renderWidth = 1280;
    static constexpr int renderHeight = 720;

    /** @brief Compile shaders, build and upload every mesh and texture. Needs a current GL context. */
    bool initialise(std::string& errorOut);

    /** @brief Draw the whole park into the currently bound frame buffer. */
    void render(const ParkFrameSettings& settings);

    /** @brief Draw a texture as a screen overlay with its top-left at (left, top) in pixels. */
    void drawOverlay(const Texture2D& texture, int left, int top);

    /** @brief Upload a software-rendered RGBA image into a texture. */
    static void uploadImage(Texture2D& texture, const RgbaImage& image) {
        texture.upload(image.width, image.height, image.pixels.data());
    }

private:
    void drawLit(const GpuMesh& mesh, const Matrix4& model, Color tint = astroPalette::white, double emissive = 0.0,
                 double specularStrength = 0.25, double shininess = 24.0);
    void setLighting(const ParkFrameSettings& settings, const Matrix4& view, const Matrix4& projection);
    void drawSky(const ParkFrameSettings& settings, const Matrix4& view, const Matrix4& projection);
    void drawRides(double time);
    void drawHead(double time, double night);
    void drawPeople(double time);

    ShaderProgram litShader;
    ShaderProgram skyShader;
    ShaderProgram overlayShader;
    GLuint emptyVertexArray = 0;  // core profile needs a VAO bound even for the attribute-less sky triangle

    GpuMesh ground, fence;
    GpuMesh head, portal, dreads;
    GpuMesh ferrisStand, ferrisRotor, ferrisBulbs, gondola;
    GpuMesh coasterTrack, coasterTruss, coasterCar;
    GpuMesh dropColumn, dropLights, dropSeats;
    GpuMesh carouselBase, carouselRotor, carouselBulbs, horse;
    GpuMesh rocket, bigTop, signFrame, signFace, buntingFlags, buntingStrings;
    GpuMesh personTorso, personLeg, balloon, balloonStrings;
    GpuMesh overlayQuad;

    Texture2D signTexture, logoTexture, footerTexture;

    std::vector<Vec3> coasterPath;
    std::vector<double> coasterCumulativeLength;
};
