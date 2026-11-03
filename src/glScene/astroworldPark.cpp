/**
 * @file astroworldPark.cpp
 * @brief Drawing a ParkFrame with OpenGL.
 *
 * Each frame:
 *   1. the sky triangle (gradient, stars, clouds) fills the background
 *   2. every opaque instance is drawn with its MODEL matrix; the shared VIEW and
 *      PROJECTION matrices are set once - the depth buffer (z-buffer) resolves
 *      visibility per sample, so no sorting is needed
 *   3. searchlight beams are added on top (additive blending, depth test on,
 *      depth writes off - transparent light should not hide what is behind it)
 *   4. overlays (logo, captions) are alpha-blended in screen space
 */
#include "glScene/astroworldPark.h"

#include <algorithm>
#include <cmath>

#include "core/simpleRandom.h"
#include "glScene/shaders.h"
#include "mesh/meshBuilder.h"
#include "raster/circles.h"
#include "text/astroWordmark.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Bitmap text with a drop shadow, for the software-rendered caption textures. */
void drawShadowedText(FrameBuffer& frameBuffer, const std::string& text, int left, int top, int scale) {
    drawBitmapText(frameBuffer, text, left + 2, top + 2, scale, astroPalette::black);
    drawBitmapText(frameBuffer, text, left, top, scale, astroPalette::white);
}

/**
 * @brief One 4 x 4 m concrete slab, drawn by the CP1 software renderer: speckled
 *        concrete, darker expansion joints along its edges (so tiles line up into
 *        a slab grid) and scattered popcorn, as on the album cover's plaza.
 */
void drawGroundTile(FrameBuffer& canvas) {
    const int size = canvas.getWidth();
    SimpleRandom random(57u);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double speckle = random.nextRange(-0.06, 0.06);
            canvas.setPixel(x, y, scaleColor(astroPalette::dayGroundNear, 1.0 + speckle));
        }
    }
    const Color joint = makeShade(astroPalette::dayGroundNear, 0.3);
    for (int edge = 0; edge < 4; ++edge) {
        canvas.fillSpan(edge, 0, size - 1, joint);
        for (int y = 0; y < size; ++y) canvas.setPixel(edge, y, joint);
    }
    for (int kernel = 0; kernel < 90; ++kernel) {
        const int x = random.nextInt(8, size - 8);
        const int y = random.nextInt(8, size - 8);
        fillCircle(canvas, x, y, random.nextInt(2, 4), Color{255, 248, 220});
        canvas.setPixel(x - 1, y - 1, astroPalette::white);
    }
}

}  // namespace

bool AstroworldPark::initialise(const ParkScene& scene, std::string& errorOut) {
    if (!litShader.build(shaders::litVertex().c_str(), shaders::litFragment().c_str(), errorOut)) return false;
    if (!skyShader.build(shaders::skyVertex, shaders::skyFragment, errorOut)) return false;
    if (!beamShader.build(shaders::beamVertex, shaders::beamFragment, errorOut)) return false;
    if (!overlayShader.build(shaders::overlayVertex, shaders::overlayFragment, errorOut)) return false;
    glGenVertexArrays(1, &emptyVertexArray);

    // Upload every static mesh once. (Dreads and balloon strings change each frame.)
    for (std::size_t index = 0; index < meshIdCount; ++index) {
        const MeshId id = static_cast<MeshId>(index);
        meshes[index].upload(scene.mesh(id), ParkScene::isLineMesh(id) ? GL_LINES : GL_TRIANGLES);
    }
    overlayQuad.upload(makeTexturedQuad(2.0, 2.0, astroPalette::white));

    // ---- Textures drawn by the CP1/CP2 software renderer ------------------------------
    uploadImage(signTexture, renderOverlayImage(590, 115, [](FrameBuffer& canvas) {
                    canvas.clear(Color{232, 236, 244});
                    drawBitmapText(canvas, "ASTROWORLD", (590 - measureBitmapText("ASTROWORLD", 9)) / 2, (115 - 63) / 2, 9, Color{36, 40, 64});
                }));
    // The ground tile REPEATS across the plaza and is MIPMAPPED for distant slabs.
    uploadImage(groundTexture, renderOverlayImage(256, 256, drawGroundTile), true, true);
    WordmarkStyle style;
    style.letterHeight = 66.0;
    const int logoWidth = static_cast<int>(measureWordmark(style)) + 24;
    uploadImage(logoTexture, renderOverlayImage(logoWidth, 112, [style](FrameBuffer& canvas) {
                    drawShadowedText(canvas, "GREETINGS FROM", 12, 2, 2);
                    drawWordmark(canvas, {10.0, 24.0}, style);
                }));
    uploadImage(footerTexture, renderOverlayImage(240, 20, [](FrameBuffer& canvas) {
                    drawShadowedText(canvas, "WISH YOU WERE HERE", (240 - measureBitmapText("WISH YOU WERE HERE", 2)) / 2, 2, 2);
                }));
    return true;
}

void AstroworldPark::drawSky(const ParkFrame& frame, const CameraMatrices& camera) {
    const SceneLighting& lighting = frame.lighting;
    skyShader.use();
    skyShader.setMatrix("inverseViewProjection", (camera.projection * camera.view).inverse());
    skyShader.setColor("skyTop", lighting.skyTop);
    skyShader.setColor("skyMid", lighting.skyMid);
    skyShader.setColor("skyHorizon", lighting.skyHorizon);
    skyShader.setFloat("nightAmount", lighting.nightAmount);
    skyShader.setFloat("timeSeconds", frame.timeSeconds);
    // The sky is infinitely far away: draw it first without touching the depth buffer.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindVertexArray(emptyVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void AstroworldPark::setLighting(const SceneLighting& lighting, const CameraMatrices& camera, const RenderOptions& options) {
    litShader.use();
    litShader.setMatrix("view", camera.view);
    litShader.setMatrix("projection", camera.projection);
    litShader.setVec3("cameraPosition", camera.eye);
    litShader.setVec3("sunDirection", lighting.sunDirection);
    litShader.setVec3("sunColor", lighting.sunColor);
    litShader.setVec3("ambientColor", lighting.ambientColor);
    litShader.setVec3("portalPosition", lighting.portalPosition);
    litShader.setVec3("portalColor", lighting.portalColor);
    litShader.setFloat("portalIntensity", lighting.portalIntensity);
    litShader.setColor("fogColor", lighting.fogColor);
    litShader.setFloat("fogDensity", lighting.fogDensity);
    litShader.setInt("shadingMode", static_cast<int>(options.shading));
    litShader.setInt("diffuseTexture", 0);
}

void AstroworldPark::render(const ParkFrame& frame, const CameraMatrices& camera, const RenderOptions& options) {
    // Per-frame meshes.
    meshes[static_cast<std::size_t>(MeshId::dreads)].upload(frame.dreads);
    meshes[static_cast<std::size_t>(MeshId::balloonStrings)].upload(frame.balloonStrings, GL_LINES);
    groundTexture.setMipmapFiltering(options.mipmapping);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    drawSky(frame, camera);

    // ---- Opaque instances ----------------------------------------------------------------
    setLighting(frame.lighting, camera, options);
    if (options.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (const Instance& instance : frame.instances) {
        if (instance.material.lightBeam) continue;
        const Material& surface = instance.material;
        litShader.setMatrix("model", instance.model);
        litShader.setColor("tint", surface.tint);
        litShader.setFloat("emissive", surface.emissive);
        litShader.setFloat("specularStrength", surface.specularStrength);
        litShader.setFloat("shininess", surface.shininess);
        const bool textured = options.textures && surface.texture != TextureId::none;
        litShader.setInt("useTexture", textured ? 1 : 0);
        if (textured) (surface.texture == TextureId::sign ? signTexture : groundTexture).bind(0);
        meshes[static_cast<std::size_t>(instance.mesh)].draw();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // ---- Searchlight beams: transparent light added on top ---------------------------------
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);  // additive: colour_out = colour_beam + colour_behind
    glDepthMask(GL_FALSE);
    beamShader.use();
    beamShader.setMatrix("view", camera.view);
    beamShader.setMatrix("projection", camera.projection);
    for (const Instance& instance : frame.instances) {
        if (!instance.material.lightBeam) continue;
        beamShader.setMatrix("model", instance.model);
        beamShader.setColor("tint", instance.material.tint);
        meshes[static_cast<std::size_t>(instance.mesh)].draw();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (options.showLogo) {
        drawOverlay(logoTexture, (renderWidth - logoTexture.getWidth()) / 2, 6);
        drawOverlay(footerTexture, (renderWidth - footerTexture.getWidth()) / 2, renderHeight - 28);
    }
}

void AstroworldPark::drawOverlay(const Texture2D& texture, int left, int top) {
    if (!texture.isValid()) return;
    // Map the pixel rectangle to normalised device coordinates (y up) and stretch the 2x2 quad over it.
    const double width = texture.getWidth() * 2.0 / renderWidth;
    const double height = texture.getHeight() * 2.0 / renderHeight;
    const double centerX = -1.0 + (left + texture.getWidth() * 0.5) * 2.0 / renderWidth;
    const double centerY = 1.0 - (top + texture.getHeight() * 0.5) * 2.0 / renderHeight;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // straight alpha "over" compositing
    overlayShader.use();
    overlayShader.setMatrix("model", Matrix4::translation({centerX, centerY, 0.0}) * Matrix4::scaling({width * 0.5, height * 0.5, 1.0}));
    overlayShader.setInt("overlayTexture", 0);
    texture.bind(0);
    overlayQuad.draw();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
