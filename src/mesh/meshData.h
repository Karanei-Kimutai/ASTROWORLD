/**
 * @file meshData.h
 * @brief CPU-side triangle meshes: the vertex data that gets uploaded to the GPU.
 *
 * A mesh is a list of VERTICES (each with a position, a surface normal for
 * lighting, a colour and texture coordinates) plus a list of INDICES - every
 * three indices form one triangle (or every two, one line). Sharing vertices
 * through indices avoids storing the same corner several times.
 */
#pragma once

#include <cstdint>
#include <vector>

#include "core/color.h"
#include "math/matrix4.h"
#include "math/vec3.h"

/** @brief One vertex exactly as the GPU reads it: 11 floats, tightly packed. */
struct MeshVertex {
    float position[3];
    float normal[3];
    float color[3];
    float textureCoordinate[2];
};

/** @brief Make a vertex from double-precision maths types. */
inline MeshVertex makeVertex(const Vec3& position, const Vec3& normal, Color color, double u = 0.0, double v = 0.0) {
    return MeshVertex{{static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z)},
                      {static_cast<float>(normal.x), static_cast<float>(normal.y), static_cast<float>(normal.z)},
                      {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f},
                      {static_cast<float>(u), static_cast<float>(v)}};
}

struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;

    /** @brief Index the next vertex will get - used while building triangles. */
    std::uint32_t nextIndex() const { return static_cast<std::uint32_t>(vertices.size()); }

    void addTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c) { indices.insert(indices.end(), {a, b, c}); }
    void addLine(std::uint32_t a, std::uint32_t b) { indices.insert(indices.end(), {a, b}); }

    /**
     * @brief Append another mesh, moved by a model matrix. Positions use the
     *        matrix itself; normals use its inverse-transpose so they stay
     *        perpendicular to surfaces even under non-uniform scaling.
     *        (Only for triangle meshes when mirroring; line meshes are appended as-is.)
     */
    void append(const MeshData& other, const Matrix4& transform = Matrix4::identity()) {
        const Matrix4 normalMatrix = transform.inverse().transposed();
        const std::uint32_t offset = nextIndex();
        for (MeshVertex vertex : other.vertices) {
            const Vec3 position = transform.transformPoint({vertex.position[0], vertex.position[1], vertex.position[2]});
            const Vec3 normal = normalMatrix.transformDirection({vertex.normal[0], vertex.normal[1], vertex.normal[2]}).normalized();
            vertex.position[0] = static_cast<float>(position.x);
            vertex.position[1] = static_cast<float>(position.y);
            vertex.position[2] = static_cast<float>(position.z);
            vertex.normal[0] = static_cast<float>(normal.x);
            vertex.normal[1] = static_cast<float>(normal.y);
            vertex.normal[2] = static_cast<float>(normal.z);
            vertices.push_back(vertex);
        }
        // A mirroring transform (negative determinant) turns counter-clockwise triangles
        // clockwise, which would make the GPU treat their front as their back. Swap two
        // corners of every triangle to restore the winding.
        const double determinant =
            transform.at(0, 0) * (transform.at(1, 1) * transform.at(2, 2) - transform.at(1, 2) * transform.at(2, 1)) -
            transform.at(0, 1) * (transform.at(1, 0) * transform.at(2, 2) - transform.at(1, 2) * transform.at(2, 0)) +
            transform.at(0, 2) * (transform.at(1, 0) * transform.at(2, 1) - transform.at(1, 1) * transform.at(2, 0));
        const bool mirrored = determinant < 0.0 && other.indices.size() % 3 == 0;
        for (std::size_t index = 0; index < other.indices.size(); ++index) {
            std::size_t source = index;
            if (mirrored && index % 3 == 1) source = index + 1;
            if (mirrored && index % 3 == 2) source = index - 1;
            indices.push_back(other.indices[source] + offset);
        }
    }
};
