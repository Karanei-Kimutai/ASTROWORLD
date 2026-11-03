/**
 * @file glResources.cpp
 * @brief Implementation of the OpenGL resource wrappers.
 */
#include "gl/glResources.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// ShaderProgram
// ---------------------------------------------------------------------------

namespace {

/** @brief Compile one shader stage; returns 0 and fills errorOut on failure. */
GLuint compileStage(GLenum stage, const char* source, std::string& errorOut) {
    const GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        errorOut = std::string(stage == GL_VERTEX_SHADER ? "vertex" : "fragment") + " shader: " + log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

}  // namespace

ShaderProgram::~ShaderProgram() {
    if (programId != 0) glDeleteProgram(programId);
}

bool ShaderProgram::build(const char* vertexSource, const char* fragmentSource, std::string& errorOut) {
    const GLuint vertexShader = compileStage(GL_VERTEX_SHADER, vertexSource, errorOut);
    if (vertexShader == 0) return false;
    const GLuint fragmentShader = compileStage(GL_FRAGMENT_SHADER, fragmentSource, errorOut);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }
    programId = glCreateProgram();
    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);
    glLinkProgram(programId);
    // The stages are now part of the program and can be released.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[2048];
        glGetProgramInfoLog(programId, sizeof(log), nullptr, log);
        errorOut = std::string("link: ") + log;
        return false;
    }
    return true;
}

GLint ShaderProgram::location(const char* name) {
    // Looking a uniform up by name is slow-ish, so remember each answer.
    const auto found = uniformLocations.find(name);
    if (found != uniformLocations.end()) return found->second;
    const GLint value = glGetUniformLocation(programId, name);
    uniformLocations.emplace(name, value);
    return value;
}

void ShaderProgram::setInt(const char* name, int value) { glUniform1i(location(name), value); }
void ShaderProgram::setFloat(const char* name, double value) { glUniform1f(location(name), static_cast<float>(value)); }
void ShaderProgram::setVec3(const char* name, const Vec3& value) {
    glUniform3f(location(name), static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z));
}
void ShaderProgram::setColor(const char* name, Color value) {
    glUniform3f(location(name), value.r / 255.0f, value.g / 255.0f, value.b / 255.0f);
}
void ShaderProgram::setMatrix(const char* name, const Matrix4& value) {
    float columnMajor[16];
    value.toColumnMajor(columnMajor);
    glUniformMatrix4fv(location(name), 1, GL_FALSE, columnMajor);
}

// ---------------------------------------------------------------------------
// GpuMesh
// ---------------------------------------------------------------------------

GpuMesh::~GpuMesh() { release(); }

GpuMesh::GpuMesh(GpuMesh&& other) noexcept { *this = std::move(other); }

GpuMesh& GpuMesh::operator=(GpuMesh&& other) noexcept {
    if (this != &other) {
        release();
        std::swap(vertexArray, other.vertexArray);
        std::swap(vertexBuffer, other.vertexBuffer);
        std::swap(indexBuffer, other.indexBuffer);
        std::swap(indexCount, other.indexCount);
        std::swap(primitiveType, other.primitiveType);
    }
    return *this;
}

void GpuMesh::release() {
    if (indexBuffer != 0) glDeleteBuffers(1, &indexBuffer);
    if (vertexBuffer != 0) glDeleteBuffers(1, &vertexBuffer);
    if (vertexArray != 0) glDeleteVertexArrays(1, &vertexArray);
    vertexArray = vertexBuffer = indexBuffer = 0;
    indexCount = 0;
}

void GpuMesh::upload(const MeshData& mesh, GLenum primitive) {
    if (vertexArray == 0) {
        glGenVertexArrays(1, &vertexArray);
        glGenBuffers(1, &vertexBuffer);
        glGenBuffers(1, &indexBuffer);
    }
    primitiveType = primitive;
    indexCount = static_cast<GLsizei>(mesh.indices.size());

    // The VAO remembers which buffers are bound and how the vertex bytes are laid out.
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(MeshVertex)), mesh.vertices.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)), mesh.indices.data(),
                 GL_DYNAMIC_DRAW);

    // Attribute layout: location 0 position, 1 normal, 2 colour, 3 texture coordinate.
    const GLsizei stride = sizeof(MeshVertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(MeshVertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(MeshVertex, normal)));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(MeshVertex, color)));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(MeshVertex, textureCoordinate)));
    for (GLuint attribute = 0; attribute < 4; ++attribute) glEnableVertexAttribArray(attribute);
    glBindVertexArray(0);
}

void GpuMesh::draw() const {
    if (vertexArray == 0 || indexCount == 0) return;
    glBindVertexArray(vertexArray);
    glDrawElements(primitiveType, indexCount, GL_UNSIGNED_INT, nullptr);
}

// ---------------------------------------------------------------------------
// Texture2D
// ---------------------------------------------------------------------------

Texture2D::~Texture2D() {
    if (textureId != 0) glDeleteTextures(1, &textureId);
}

void Texture2D::upload(int widthValue, int heightValue, const std::uint8_t* rgbaPixels, bool repeat, bool mipmapped) {
    if (textureId == 0) glGenTextures(1, &textureId);
    width = widthValue;
    height = heightValue;
    hasMipmaps = mipmapped;
    glBindTexture(GL_TEXTURE_2D, textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels);
    const GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (mipmapped) glGenerateMipmap(GL_TEXTURE_2D);
    setMipmapFiltering(mipmapped);
}

void Texture2D::setMipmapFiltering(bool enabled) {
    glBindTexture(GL_TEXTURE_2D, textureId);
    // Trilinear: blend the two nearest mip levels, each sampled bilinearly. Without
    // mipmaps a minified texture is point-sampled and distant detail aliases.
    const GLint minFilter = enabled && hasMipmaps ? GL_LINEAR_MIPMAP_LINEAR : (hasMipmaps ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
}

void Texture2D::bind(int textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(textureUnit));
    glBindTexture(GL_TEXTURE_2D, textureId);
}

// ---------------------------------------------------------------------------
// RenderTarget
// ---------------------------------------------------------------------------

RenderTarget::~RenderTarget() {
    if (multisampleFramebuffer != 0) glDeleteFramebuffers(1, &multisampleFramebuffer);
    if (resolveFramebuffer != 0) glDeleteFramebuffers(1, &resolveFramebuffer);
    const GLuint renderbuffers[3] = {multisampleColor, multisampleDepth, resolveColor};
    for (GLuint renderbuffer : renderbuffers) {
        if (renderbuffer != 0) glDeleteRenderbuffers(1, &renderbuffer);
    }
}

bool RenderTarget::create(int widthValue, int heightValue, int sampleCount, std::string& errorOut) {
    width = widthValue;
    height = heightValue;

    // Multisampled colour + depth (the depth buffer is what makes hidden-surface removal work).
    glGenFramebuffers(1, &multisampleFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, multisampleFramebuffer);
    glGenRenderbuffers(1, &multisampleColor);
    glBindRenderbuffer(GL_RENDERBUFFER, multisampleColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, multisampleColor);
    glGenRenderbuffers(1, &multisampleDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, multisampleDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, multisampleDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        errorOut = "multisampled framebuffer incomplete";
        return false;
    }

    // Single-sample colour buffer the samples are resolved into.
    glGenFramebuffers(1, &resolveFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFramebuffer);
    glGenRenderbuffers(1, &resolveColor);
    glBindRenderbuffer(GL_RENDERBUFFER, resolveColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, resolveColor);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        errorOut = "resolve framebuffer incomplete";
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void RenderTarget::bindForDrawing() const {
    glBindFramebuffer(GL_FRAMEBUFFER, multisampleFramebuffer);
    glViewport(0, 0, width, height);
}

void RenderTarget::resolve() const {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampleFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFramebuffer);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void RenderTarget::presentToWindow(int windowWidth, int windowHeight) const {
    // Largest 16:9 rectangle that fits the window, centred (letterboxing).
    const double scale = std::min(static_cast<double>(windowWidth) / width, static_cast<double>(windowHeight) / height);
    const int targetWidth = static_cast<int>(width * scale);
    const int targetHeight = static_cast<int>(height * scale);
    const int left = (windowWidth - targetWidth) / 2;
    const int bottom = (windowHeight - targetHeight) / 2;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, resolveFramebuffer);
    glBlitFramebuffer(0, 0, width, height, left, bottom, left + targetWidth, bottom + targetHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

FrameBuffer RenderTarget::readBack() const {
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * height * 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, resolveFramebuffer);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    FrameBuffer image(width, height);
    for (int row = 0; row < height; ++row) {
        const int sourceRow = height - 1 - row;  // OpenGL row 0 is the BOTTOM of the image
        for (int x = 0; x < width; ++x) {
            const std::uint8_t* pixel = &rgba[(static_cast<std::size_t>(sourceRow) * width + x) * 4];
            image.setPixel(x, row, Color{pixel[0], pixel[1], pixel[2]});
        }
    }
    return image;
}
