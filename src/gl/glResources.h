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
#include <vector>

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
    void setVec2(const char* name, double x, double y);
    void setVec3Array(const char* name, const std::vector<Vec3>& values);

private:
    GLint location(const char* name);

    GLuint programId = 0;
    std::unordered_map<const char*, GLint> uniformLocations;  // names are string literals, so their address is a stable key
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

    /** @brief Floats per instance: a column-major 4x4 model matrix and an RGB tint. */
    static constexpr int instanceFloats = 19;

    /**
     * @brief GPU INSTANCING: draw 'count' copies of the mesh in ONE call. Each copy's
     *        model matrix and tint arrive as per-instance vertex attributes
     *        (locations 4-7 for the matrix columns, 8 for the tint), advanced once
     *        per instance instead of once per vertex (glVertexAttribDivisor).
     */
    void drawInstanced(const std::vector<float>& instanceData, int count) const;

private:
    void release();

    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
    GLuint instanceBuffer = 0;
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
