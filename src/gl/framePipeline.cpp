/**
 * @file framePipeline.cpp
 * @brief Creation and use of the HDR, bloom, final and shadow frame buffers.
 */
#include "gl/framePipeline.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace {

/** @brief A texture suitable as a render-target colour attachment. */
GLuint createColorTexture(int width, int height, GLint internalFormat, GLenum type) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

/** @brief A frame buffer whose only colour attachment is the given texture. */
GLuint framebufferForTexture(GLuint texture) {
    GLuint framebuffer = 0;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    return framebuffer;
}

bool complete(const char* name, std::string& errorOut) {
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) return true;
    errorOut = std::string(name) + " framebuffer incomplete";
    return false;
}

}  // namespace

FramePipeline::~FramePipeline() {
    const GLuint framebuffers[5] = {msaaFramebuffer, hdrFramebuffer, bloomFramebuffers[0], bloomFramebuffers[1], finalFramebuffer};
    for (GLuint framebuffer : framebuffers) {
        if (framebuffer != 0) glDeleteFramebuffers(1, &framebuffer);
    }
    const GLuint renderbuffers[2] = {msaaColor, msaaDepth};
    for (GLuint renderbuffer : renderbuffers) {
        if (renderbuffer != 0) glDeleteRenderbuffers(1, &renderbuffer);
    }
    const GLuint textures[4] = {hdrTexture, bloomTextures[0], bloomTextures[1], finalTexture};
    for (GLuint texture : textures) {
        if (texture != 0) glDeleteTextures(1, &texture);
    }
}

bool FramePipeline::create(int widthValue, int heightValue, int sampleCount, std::string& errorOut) {
    width = widthValue;
    height = heightValue;

    // 1. Multisampled HDR colour (16-bit floats per channel) + depth.
    glGenFramebuffers(1, &msaaFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
    glGenRenderbuffers(1, &msaaColor);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_RGBA16F, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColor);
    glGenRenderbuffers(1, &msaaDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, msaaDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaaDepth);
    if (!complete("HDR multisample", errorOut)) return false;

    // 2. Resolved HDR texture (sampled by the post-processing shaders).
    hdrTexture = createColorTexture(width, height, GL_RGBA16F, GL_FLOAT);
    hdrFramebuffer = framebufferForTexture(hdrTexture);
    if (!complete("HDR resolve", errorOut)) return false;

    // 3. Two half-resolution bloom buffers.
    for (int index = 0; index < 2; ++index) {
        bloomTextures[index] = createColorTexture(width / 2, height / 2, GL_RGBA16F, GL_FLOAT);
        bloomFramebuffers[index] = framebufferForTexture(bloomTextures[index]);
        if (!complete("bloom", errorOut)) return false;
    }

    // 4. Final 8-bit image.
    finalTexture = createColorTexture(width, height, GL_RGBA8, GL_UNSIGNED_BYTE);
    finalFramebuffer = framebufferForTexture(finalTexture);
    if (!complete("final", errorOut)) return false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void FramePipeline::bindScene() const {
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFramebuffer);
    glViewport(0, 0, width, height);
}

void FramePipeline::resolveScene() const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFramebuffer);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void FramePipeline::bindBloom(int index) const {
    glBindFramebuffer(GL_FRAMEBUFFER, bloomFramebuffers[index]);
    glViewport(0, 0, width / 2, height / 2);
}

void FramePipeline::bindFinal() const {
    glBindFramebuffer(GL_FRAMEBUFFER, finalFramebuffer);
    glViewport(0, 0, width, height);
}

void FramePipeline::presentToWindow(int windowWidth, int windowHeight) const {
    const double scale = std::min(static_cast<double>(windowWidth) / width, static_cast<double>(windowHeight) / height);
    const int targetWidth = static_cast<int>(width * scale);
    const int targetHeight = static_cast<int>(height * scale);
    const int left = (windowWidth - targetWidth) / 2;
    const int bottom = (windowHeight - targetHeight) / 2;
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, finalFramebuffer);
    glBlitFramebuffer(0, 0, width, height, left, bottom, left + targetWidth, bottom + targetHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

FrameBuffer FramePipeline::readBack() const {
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * height * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, finalFramebuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    FrameBuffer image(width, height);
    for (int row = 0; row < height; ++row) {
        const int sourceRow = height - 1 - row;
        for (int x = 0; x < width; ++x) {
            const std::uint8_t* pixel = &rgba[(static_cast<std::size_t>(sourceRow) * width + x) * 4];
            image.setPixel(x, row, Color{pixel[0], pixel[1], pixel[2]});
        }
    }
    return image;
}

ShadowMap::~ShadowMap() {
    if (framebuffer != 0) glDeleteFramebuffers(1, &framebuffer);
    if (depthTexture != 0) glDeleteTextures(1, &depthTexture);
}

bool ShadowMap::create(int sizeValue, std::string& errorOut) {
    size = sizeValue;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // Hardware depth COMPARISON with linear filtering: each lookup returns the fraction of
    // the 2x2 nearest texels that are lit (built-in percentage-closer filtering).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    glDrawBuffer(GL_NONE);  // depth only: no colour is written
    glReadBuffer(GL_NONE);
    const bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!ok) errorOut = "shadow map framebuffer incomplete";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}

void ShadowMap::bindForWriting() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, size, size);
}

void ShadowMap::bindForReading(int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
    glBindTexture(GL_TEXTURE_2D, depthTexture);
}
