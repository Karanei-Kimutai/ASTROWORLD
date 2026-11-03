/**
 * @file glResources.h
 * @brief Small RAII wrappers around OpenGL objects: shader programs, meshes on
 *        the GPU, textures and the off-screen render target.
 *
 * Each class creates its OpenGL object in a create/upload call and deletes it
 * in its destructor, so GPU memory is never leaked. They are move-only:
 * copying would mean two owners deleting the same GPU object.
 */
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "core/frameBuffer.h"
#include "gl/glFunctions.h"
#include "math/matrix4.h"
#include "mesh/meshData.h"

/** @brief A linked vertex + fragment shader pair, with cached uniform locations. */
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    /** @brief Compile both stages and link them. On failure errorOut holds the driver's log. */
    bool build(const char* vertexSource, const char* fragmentSource, std::string& errorOut);

    void use() const { glUseProgram(programId); }

    void setInt(const char* name, int value);
    void setFloat(const char* name, double value);
    void setVec3(const char* name, const Vec3& value);
    void setColor(const char* name, Color value);
    void setMatrix(const char* name, const Matrix4& value);

private:
    GLint location(const char* name);

    GLuint programId = 0;
    std::unordered_map<std::string, GLint> uniformLocations;
};

/**
 * @brief A mesh living in GPU memory: a Vertex Array Object describing the
 *        vertex layout, a vertex buffer and an index buffer.
 */
class GpuMesh {
public:
    GpuMesh() = default;
    ~GpuMesh();
    GpuMesh(const GpuMesh&) = delete;
    GpuMesh& operator=(const GpuMesh&) = delete;
    GpuMesh(GpuMesh&& other) noexcept;
    GpuMesh& operator=(GpuMesh&& other) noexcept;

    /**
     * @brief Copy a mesh to the GPU. Can be called again to replace the data
     *        (used for meshes that change every frame, like the swaying dreads).
     * @param primitive GL_TRIANGLES or GL_LINES.
     */
    void upload(const MeshData& mesh, GLenum primitive = GL_TRIANGLES);

    /** @brief Issue the draw call. */
    void draw() const;

private:
    void release();

    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
    GLsizei indexCount = 0;
    GLenum primitiveType = GL_TRIANGLES;
};

/** @brief An RGBA image in GPU memory that shaders can sample (texture mapping). */
class Texture2D {
public:
    Texture2D() = default;
    ~Texture2D();
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    /**
     * @brief Upload width x height RGBA bytes, top row first.
     * @param repeat     Tile the texture (GL_REPEAT) instead of clamping at its edges.
     * @param mipmapped  Also build the MIPMAP chain: half-size, quarter-size, ... copies,
     *                   so distant (minified) surfaces sample a pre-filtered image
     *                   instead of aliasing (shimmering moire patterns).
     */
    void upload(int width, int height, const std::uint8_t* rgbaPixels, bool repeat = false, bool mipmapped = false);

    /** @brief Switch between trilinear mipmap filtering and plain nearest sampling (to show aliasing). */
    void setMipmapFiltering(bool enabled);

    void bind(int textureUnit) const;
    bool isValid() const { return textureId != 0; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    GLuint textureId = 0;
    int width = 0;
    int height = 0;
    bool hasMipmaps = false;
};

/**
 * @brief The GPU's version of our FrameBuffer: an off-screen Frame Buffer Object.
 *
 * The scene is drawn into a MULTISAMPLED colour + depth buffer (MSAA: each
 * pixel stores several coverage samples, smoothing edges). It is then
 * "resolved" (samples averaged) into a normal colour buffer, which can be
 * shown in the window or read back to the CPU for screenshots and video.
 */
class RenderTarget {
public:
    RenderTarget() = default;
    ~RenderTarget();
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    bool create(int widthValue, int heightValue, int sampleCount, std::string& errorOut);

    /** @brief Direct all following draw calls into the multisampled buffer. */
    void bindForDrawing() const;

    /** @brief Average the samples into the resolve buffer. */
    void resolve() const;

    /** @brief Copy the resolved image to the window, letterboxed to keep the aspect ratio. */
    void presentToWindow(int windowWidth, int windowHeight) const;

    /** @brief Read the resolved image back into a CPU FrameBuffer (flipping rows: GL's origin is bottom-left). */
    FrameBuffer readBack() const;

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    GLuint multisampleFramebuffer = 0;
    GLuint multisampleColor = 0;
    GLuint multisampleDepth = 0;
    GLuint resolveFramebuffer = 0;
    GLuint resolveColor = 0;
    int width = 0;
    int height = 0;
};
