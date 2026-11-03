/**
 * @file rayGeometry.h
 * @brief Triangles in world space and a Bounding Volume Hierarchy (BVH) to find
 *        which triangle a ray hits first.
 *
 * Testing every ray against every one of ~100 000 triangles would be far too
 * slow. A BVH is a binary tree of axis-aligned boxes: each node's box encloses
 * all triangles below it. A ray that misses a box skips that entire subtree, so
 * a lookup costs roughly log(n) box tests plus a handful of triangle tests.
 */
#pragma once

#include <functional>
#include <vector>

#include "core/vec2.h"
#include "math/vec3.h"
#include "park3d/parkScene.h"

/** @brief A ray: origin + t * direction (direction is unit length). */
struct Ray {
    Vec3 origin;
    Vec3 direction;
};

/** @brief One world-space triangle with everything shading needs at its corners. */
struct RayTriangle {
    Vec3 position0, edge1, edge2;  // corner 0 and the two edges from it (precomputed for intersection)
    Vec3 normal0, normal1, normal2;
    Vec3 color0, color1, color2;   // vertex colour x material tint, 0..1
    Vec2 uv0, uv1, uv2;
    int materialIndex = 0;
};

/** @brief Where a ray hit: distance along the ray, which triangle, and barycentric weights. */
struct RayHit {
    double distance = 0.0;
    int triangle = -1;
    double barycentricU = 0.0;  // weight of corner 1
    double barycentricV = 0.0;  // weight of corner 2 (corner 0 gets 1 - u - v)
};

/** @brief Axis-aligned bounding box. */
struct BoundingBox {
    Vec3 minimum{1e30, 1e30, 1e30};
    Vec3 maximum{-1e30, -1e30, -1e30};
    void expand(const Vec3& point);
    void expand(const BoundingBox& box);
    Vec3 centre() const { return (minimum + maximum) * 0.5; }
};

/** @brief All the triangles of one frame, their materials, and the BVH over them. */
class RayScene {
public:
    /**
     * @brief Transform every instance of a ParkFrame into world-space triangles.
     *        Line meshes (coaster lattice, strings) become thin tubes; searchlight
     *        beams are skipped (they are a real-time effect, not geometry).
     */
    void build(const ParkScene& scene, const ParkFrame& frame);

    /** @brief Closest hit along the ray within (minimumDistance, maximumDistance). */
    bool intersect(const Ray& ray, double minimumDistance, double maximumDistance, RayHit& hit) const;

    /**
     * @brief Is anything (that blocks light) between the origin and maximumDistance?
     *        Used for SHADOW rays; glowing surfaces (bulbs, the portal) are lights, so they do not cast shadows.
     */
    bool occluded(const Ray& ray, double maximumDistance) const;

    const RayTriangle& triangle(int index) const { return triangles[static_cast<std::size_t>(index)]; }
    const Material& material(int index) const { return materials[static_cast<std::size_t>(index)]; }
    std::size_t triangleCount() const { return triangles.size(); }

private:
    struct BvhNode {
        BoundingBox bounds;
        int left = -1, right = -1;        // child nodes (interior nodes)
        int firstTriangle = 0, count = 0; // triangle range (leaves have count > 0)
    };

    void addMesh(const MeshData& mesh, const Matrix4& model, int materialIndex, bool lines);
    int buildNode(int first, int count, int depth);
    template <bool AnyHit>
    bool traverse(const Ray& ray, double minimumDistance, double maximumDistance, RayHit& hit) const;

    std::vector<RayTriangle> triangles;
    std::vector<Material> materials;
    std::vector<BvhNode> nodes;
};
