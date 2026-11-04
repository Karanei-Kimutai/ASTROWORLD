/**
 * @file framePipeline.h
 * @brief The GPU frame buffers behind every OpenGL frame: HDR scene, bloom, final image, shadow map.
 *
 * PHOTOREALISTIC PIPELINE
 *   1. SHADOW PASS   - the scene is drawn from the sun into a depth texture (the shadow map)
 *   2. SCENE PASS    - lit in LINEAR, HIGH DYNAMIC RANGE floating-point colour (RGBA16F),
 *                      4x multisampled; values above 1.0 are kept (a bulb can be 8x brighter than paper)
 *   3. RESOLVE       - the samples are averaged into an HDR texture
 *   4. BLOOM         - bright parts are extracted at half resolution and blurred, imitating
 *                      the glow real lenses and eyes see around strong lights
 *   5. COMPOSITE     - exposure, ACES filmic TONE MAPPING (HDR -> displayable 0..1),
 *                      GAMMA correction (linear -> sRGB) and a vignette, into an 8-bit image
 *   6. Overlays (logo, help) are drawn on the 8-bit image, which is shown or read back.
 */
#pragma once

#include <string>

#include "core/frameBuffer.h"
#include "gl/glFunctions.h"

class FramePipeline {
public:
    FramePipeline() = default;
    ~FramePipeline();
    FramePipeline(const FramePipeline&) = delete;
    FramePipeline& operator=(const FramePipeline&) = delete;

    bool create(int widthValue, int heightValue, int sampleCount, std::string& errorOut);

    /** @brief Draw into the multisampled HDR scene buffer. */
    void bindScene() const;
    /** @brief Average the MSAA samples into the HDR texture. */
    void resolveScene() const;
    GLuint sceneTexture() const { return hdrTexture; }

    /** @brief The two half-resolution bloom buffers used for ping-pong blurring. */
    void bindBloom(int index) const;
    GLuint bloomTexture(int index) const { return bloomTextures[index]; }
    int bloomWidth() const { return width / 2; }
    int bloomHeight() const { return height / 2; }

    /** @brief The final tone-mapped 8-bit image. */
    void bindFinal() const;

    /** @brief Copy the final image to the window, letterboxed. */
    void presentToWindow(int windowWidth, int windowHeight) const;
    /** @brief Read the final image back to the CPU (rows flipped: GL's origin is bottom-left). */
    FrameBuffer readBack() const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width = 0;
    int height = 0;
    GLuint msaaFramebuffer = 0, msaaColor = 0, msaaDepth = 0;
    GLuint hdrFramebuffer = 0, hdrTexture = 0;
    GLuint bloomFramebuffers[2] = {0, 0};
    GLuint bloomTextures[2] = {0, 0};
    GLuint finalFramebuffer = 0, finalTexture = 0;
};

/**
 * @brief Depth texture rendered from the light. A surface point is in shadow if
 *        something nearer to the sun was recorded at its position in this map.
 */
class ShadowMap {
public:
    ShadowMap() = default;
    ~ShadowMap();
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    bool create(int sizeValue, std::string& errorOut);
    void bindForWriting() const;
    void bindForReading(int textureUnit) const;
    int getSize() const { return size; }

private:
    int size = 0;
    GLuint framebuffer = 0;
    GLuint depthTexture = 0;
};
