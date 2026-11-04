/**
 * @file astroworldPark.cpp
 * @brief Drawing a ParkFrame with OpenGL: shadow map, HDR PBR scene, bloom, tone mapping.
 */
#include "glScene/astroworldPark.h"

#include <algorithm>
#include <cmath>

#include "glScene/shaders.h"
#include "mesh/meshBuilder.h"
#include "park3d/parkTextures.h"
#include "text/astroWordmark.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Bitmap text with a drop shadow, for the software-rendered caption textures. */
void drawShadowedText(FrameBuffer& frameBuffer, const std::string& text, int left, int top, int scale) {
    drawBitmapText(frameBuffer, text, left + 2, top + 2, scale, astroPalette::black);
    drawBitmapText(frameBuffer, text, left, top, scale, astroPalette::white);
}

/** @brief sRGB colour -> linear 0..1 vector (for shader uniforms that do their maths in linear light). */
Vec3 linear(Color color) {
    auto channel = [](std::uint8_t value) { return std::pow(value / 255.0, 2.2); };
    return {channel(color.r), channel(color.g), channel(color.b)};
}

/**
 * @brief The sun's camera for the shadow map: an ORTHOGRAPHIC projection (sunlight
 *        is parallel) looking along the sun direction over the whole park.
 */
Matrix4 sunViewProjection(const Vec3& sunDirection) {
    const Vec3 centre{0.0, 0.0, -38.0};
    const double halfSize = 112.0;
    const Vec3 up = std::fabs(sunDirection.y) > 0.95 ? Vec3(0, 0, -1) : Vec3(0, 1, 0);
    const Matrix4 view = Matrix4::lookAt(centre + sunDirection * 200.0, centre, up);
    return Matrix4::orthographic(-halfSize, halfSize, -halfSize, halfSize, 1.0, 420.0) * view;
}

/** @brief The GPU mesh for a mesh id. */
const GpuMesh& meshFor(const std::array<GpuMesh, meshIdCount>& meshes, MeshId id) { return meshes[static_cast<std::size_t>(id)]; }

}  // namespace

bool AstroworldPark::initialise(const ParkScene& scene, std::string& errorOut) {
    if (!litShader.build(shaders::litVertex().c_str(), shaders::litFragment().c_str(), errorOut)) return false;
    if (!depthShader.build(shaders::depthVertex, shaders::depthFragment, errorOut)) return false;
    if (!skyShader.build(shaders::fullScreenVertex, shaders::skyFragment, errorOut)) return false;
    if (!beamShader.build(shaders::beamVertex, shaders::beamFragment, errorOut)) return false;
    if (!brightPassShader.build(shaders::fullScreenVertex, shaders::brightPassFragment, errorOut)) return false;
    if (!blurShader.build(shaders::fullScreenVertex, shaders::blurFragment, errorOut)) return false;
    if (!compositeShader.build(shaders::fullScreenVertex, shaders::compositeFragment, errorOut)) return false;
    if (!overlayShader.build(shaders::overlayVertex, shaders::overlayFragment, errorOut)) return false;
    glGenVertexArrays(1, &emptyVertexArray);
    if (!frames.create(renderWidth, renderHeight, 4, errorOut)) return false;
    if (!shadowMap.create(shadowMapSize, errorOut)) return false;

    for (std::size_t index = 0; index < meshIdCount; ++index) {
        const MeshId id = static_cast<MeshId>(index);
        meshes[index].upload(scene.mesh(id), ParkScene::isLineMesh(id) ? GL_LINES : GL_TRIANGLES);
    }
    overlayQuad.upload(makeTexturedQuad(2.0, 2.0, astroPalette::white));

    // ---- Textures drawn by the CP1/CP2 software renderer ------------------------------
    uploadImage(signTexture, toOpaqueRgba(drawSignTexture()));
    uploadImage(groundTexture, toOpaqueRgba(drawGroundTexture()), true, true);
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

void AstroworldPark::buildBatches(const ParkFrame& frame) {
    batches.clear();
    float columns[16];
    for (const Instance& instance : frame.instances) {
        if (instance.material.lightBeam) continue;
        const Material& surface = instance.material;
        const BatchKey key{static_cast<int>(instance.mesh), surface.metallic, surface.roughness, surface.emissive, surface.emissionStrength,
                           static_cast<int>(surface.texture)};
        Batch& batch = batches[key];
        if (batch.count == 0) {
            batch.mesh = instance.mesh;
            batch.material = surface;
        }
        instance.model.toColumnMajor(columns);
        batch.instanceData.insert(batch.instanceData.end(), columns, columns + 16);
        batch.instanceData.insert(batch.instanceData.end(), {surface.tint.r / 255.0f, surface.tint.g / 255.0f, surface.tint.b / 255.0f});
        ++batch.count;
    }
}

void AstroworldPark::renderShadowMap(const Matrix4& lightViewProjection) {
    // Depth only, from the sun. Polygon offset pushes the stored depths slightly away
    // so surfaces do not shadow themselves ("shadow acne").
    shadowMap.bindForWriting();
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);
    depthShader.use();
    depthShader.setMatrix("lightViewProjection", lightViewProjection);
    for (const auto& [key, batch] : batches) {
        const MeshId id = batch.mesh;
        if (ParkScene::isLineMesh(id) || id == MeshId::ground || id == MeshId::meadow || id == MeshId::pathRibbon || id == MeshId::tunnel ||
            id == MeshId::grassPatch) {
            continue;  // flat or glowing: nothing worth a shadow
        }
        meshFor(meshes, id).drawInstanced(batch.instanceData, batch.count);
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void AstroworldPark::drawFullScreen() {
    glBindVertexArray(emptyVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void AstroworldPark::drawSky(const ParkFrame& frame, const CameraMatrices& camera) {
    const SceneLighting& lighting = frame.lighting;
    skyShader.use();
    skyShader.setMatrix("inverseViewProjection", (camera.projection * camera.view).inverse());
    skyShader.setVec3("skyTop", linear(lighting.skyTop));
    skyShader.setVec3("skyMid", linear(lighting.skyMid));
    skyShader.setVec3("skyHorizon", linear(lighting.skyHorizon));
    skyShader.setVec3("horizonGlow", linear(lighting.horizonGlow));
    skyShader.setVec3("sunDirection", lighting.sunDirection);
    skyShader.setVec3("sunColor", lighting.sunRadiance * 0.3);
    skyShader.setFloat("nightAmount", lighting.nightAmount);
    skyShader.setFloat("timeSeconds", frame.timeSeconds);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    drawFullScreen();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void AstroworldPark::setLighting(const ParkFrame& frame, const CameraMatrices& camera, const RenderOptions& options,
                                 const Matrix4& lightViewProjection) {
    const SceneLighting& lighting = frame.lighting;
    litShader.use();
    litShader.setMatrix("view", camera.view);
    litShader.setMatrix("projection", camera.projection);
    litShader.setVec3("cameraPosition", camera.eye);
    litShader.setVec3("sunDirection", lighting.sunDirection);
    litShader.setVec3("sunRadiance", lighting.sunRadiance);
    litShader.setVec3("skyZenith", linear(lighting.skyTop));
    litShader.setVec3("skyHorizon", linear(lighting.skyHorizon));
    litShader.setVec3("groundColor", linear(lighting.groundColor));
    litShader.setFloat("ambientStrength", lighting.ambientStrength);
    litShader.setVec3("fogColor", linear(lighting.fogColor));
    litShader.setFloat("fogDensity", lighting.fogDensity);
    litShader.setInt("shadingMode", static_cast<int>(options.shading));
    litShader.setInt("diffuseTexture", 0);
    litShader.setInt("shadowMap", 1);
    litShader.setMatrix("lightViewProjection", lightViewProjection);
    litShader.setFloat("shadowTexelSize", 1.0 / shadowMapSize);
    litShader.setInt("shadowsEnabled", options.shadows ? 1 : 0);
    shadowMap.bindForReading(1);

    // The shader handles 16 point lights: use the ones nearest the camera.
    std::vector<PointLight> lights = lighting.pointLights;
    std::sort(lights.begin(), lights.end(), [&](const PointLight& a, const PointLight& b) {
        return (a.position - camera.eye).length() < (b.position - camera.eye).length();
    });
    if (lights.size() > 16) lights.resize(16);
    std::vector<Vec3> positions, radiance;
    for (const PointLight& light : lights) {
        positions.push_back(light.position);
        radiance.push_back(light.radiance);
    }
    litShader.setInt("pointLightCount", static_cast<int>(lights.size()));
    litShader.setVec3Array("pointLightPositions", positions);
    litShader.setVec3Array("pointLightRadiance", radiance);
    glActiveTexture(GL_TEXTURE0);
}

void AstroworldPark::postProcess(const ParkFrame& frame, const RenderOptions& options) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // Bloom: bright pass at half resolution, then blur horizontally and vertically, twice.
    frames.bindBloom(0);
    brightPassShader.use();
    brightPassShader.setInt("sceneTexture", 0);
    brightPassShader.setFloat("threshold", 1.0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frames.sceneTexture());
    drawFullScreen();
    blurShader.use();
    blurShader.setInt("sourceTexture", 0);
    for (int pass = 0; pass < 4; ++pass) {
        const int source = pass % 2;
        frames.bindBloom(1 - source);
        glBindTexture(GL_TEXTURE_2D, frames.bloomTexture(source));
        blurShader.setVec2("texelStep", source == 0 ? 1.0 / frames.bloomWidth() : 0.0, source == 0 ? 0.0 : 1.0 / frames.bloomHeight());
        drawFullScreen();
    }

    // Composite: scene + bloom, exposure, ACES tone mapping, gamma, vignette -> 8-bit image.
    frames.bindFinal();
    compositeShader.use();
    compositeShader.setInt("sceneTexture", 0);
    compositeShader.setInt("bloomTexture", 1);
    compositeShader.setFloat("exposure", frame.lighting.exposure);
    compositeShader.setFloat("bloomStrength", options.bloom ? 0.6 : 0.0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frames.sceneTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, frames.bloomTexture(0));
    drawFullScreen();
    glActiveTexture(GL_TEXTURE0);
}

void AstroworldPark::render(const ParkFrame& frame, const CameraMatrices& camera, const RenderOptions& options) {
    meshes[static_cast<std::size_t>(MeshId::dreads)].upload(frame.dreads);
    meshes[static_cast<std::size_t>(MeshId::balloonStrings)].upload(frame.balloonStrings, GL_LINES);
    groundTexture.setMipmapFiltering(options.mipmapping);
    buildBatches(frame);

    // ---- 1. Shadow map ------------------------------------------------------------------------
    const Matrix4 lightViewProjection = sunViewProjection(frame.lighting.sunDirection);
    if (options.shadows) renderShadowMap(lightViewProjection);

    // ---- 2. HDR scene: one instanced draw per batch -------------------------------------------
    frames.bindScene();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    drawSky(frame, camera);

    setLighting(frame, camera, options, lightViewProjection);
    if (options.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (const auto& [key, batch] : batches) {
        const Material& surface = batch.material;
        litShader.setFloat("metallic", surface.metallic);
        litShader.setFloat("roughness", surface.roughness);
        litShader.setFloat("emissive", surface.emissive);
        litShader.setFloat("emissionStrength", surface.emissionStrength);
        const bool textured = options.textures && surface.texture != TextureId::none;
        litShader.setInt("useTexture", textured ? 1 : 0);
        if (textured) (surface.texture == TextureId::sign ? signTexture : groundTexture).bind(0);
        meshFor(meshes, batch.mesh).drawInstanced(batch.instanceData, batch.count);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Searchlight beams: additive light, depth-tested but not depth-writing.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    beamShader.use();
    beamShader.setMatrix("view", camera.view);
    beamShader.setMatrix("projection", camera.projection);
    for (const Instance& instance : frame.instances) {
        if (!instance.material.lightBeam) continue;
        beamShader.setMatrix("model", instance.model);
        beamShader.setColor("tint", instance.material.tint);
        meshFor(meshes, instance.mesh).draw();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // ---- 3-5. Resolve, bloom, tone map -----------------------------------------------------------
    frames.resolveScene();
    postProcess(frame, options);

    // ---- 6. Overlays on the final 8-bit image ------------------------------------------------------
    if (options.showLogo) {
        drawOverlay(logoTexture, (renderWidth - logoTexture.getWidth()) / 2, 6);
        drawOverlay(footerTexture, (renderWidth - footerTexture.getWidth()) / 2, renderHeight - 28);
    }
}

void AstroworldPark::drawOverlay(const Texture2D& texture, int left, int top) {
    if (!texture.isValid()) return;
    frames.bindFinal();
    const double width = texture.getWidth() * 2.0 / renderWidth;
    const double height = texture.getHeight() * 2.0 / renderHeight;
    const double centerX = -1.0 + (left + texture.getWidth() * 0.5) * 2.0 / renderWidth;
    const double centerY = 1.0 - (top + texture.getHeight() * 0.5) * 2.0 / renderHeight;
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    overlayShader.use();
    overlayShader.setMatrix("model", Matrix4::translation({centerX, centerY, 0.0}) * Matrix4::scaling({width * 0.5, height * 0.5, 1.0}));
    overlayShader.setInt("overlayTexture", 0);
    texture.bind(0);
    overlayQuad.draw();
    glDisable(GL_BLEND);
}
