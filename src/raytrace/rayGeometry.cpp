/**
 * @file rayGeometry.cpp
 * @brief Scene flattening, BVH construction and ray-triangle intersection.
 */
#include "raytrace/rayGeometry.h"

#include <algorithm>
#include <cmath>

#include "mesh/meshBuilder.h"

void BoundingBox::expand(const Vec3& point) {
    minimum = {std::min(minimum.x, point.x), std::min(minimum.y, point.y), std::min(minimum.z, point.z)};
    maximum = {std::max(maximum.x, point.x), std::max(maximum.y, point.y), std::max(maximum.z, point.z)};
}

void BoundingBox::expand(const BoundingBox& box) {
    expand(box.minimum);
    expand(box.maximum);
}

namespace {

Vec3 positionOf(const MeshVertex& vertex) { return {vertex.position[0], vertex.position[1], vertex.position[2]}; }
Vec3 normalOf(const MeshVertex& vertex) { return {vertex.normal[0], vertex.normal[1], vertex.normal[2]}; }

/** @brief Slab test: does the ray pass through the box between tMin and tMax? */
bool rayHitsBox(const BoundingBox& box, const Vec3& origin, const Vec3& inverseDirection, double tMin, double tMax) {
    const double origins[3] = {origin.x, origin.y, origin.z};
    const double inverses[3] = {inverseDirection.x, inverseDirection.y, inverseDirection.z};
    const double lows[3] = {box.minimum.x, box.minimum.y, box.minimum.z};
    const double highs[3] = {box.maximum.x, box.maximum.y, box.maximum.z};
    // Intersect the ray's interval with the slab between each pair of parallel planes.
    for (int axis = 0; axis < 3; ++axis) {
        double t0 = (lows[axis] - origins[axis]) * inverses[axis];
        double t1 = (highs[axis] - origins[axis]) * inverses[axis];
        if (t0 > t1) std::swap(t0, t1);
        tMin = std::max(tMin, t0);
        tMax = std::min(tMax, t1);
        if (tMax < tMin) return false;
    }
    return true;
}

/**
 * @brief Moller-Trumbore ray-triangle intersection.
 *
 * Solves origin + t d = P0 + u E1 + v E2 for (t, u, v) with Cramer's rule,
 * using cross products. The hit is inside the triangle when u >= 0, v >= 0, u + v <= 1.
 */
bool intersectTriangle(const RayTriangle& triangle, const Ray& ray, double& t, double& u, double& v) {
    const Vec3 p = crossProduct(ray.direction, triangle.edge2);
    const double determinant = dotProduct(triangle.edge1, p);
    if (std::fabs(determinant) < 1e-12) return false;  // ray parallel to the triangle's plane
    const double inverse = 1.0 / determinant;
    const Vec3 toOrigin = ray.origin - triangle.position0;
    u = dotProduct(toOrigin, p) * inverse;
    if (u < 0.0 || u > 1.0) return false;
    const Vec3 q = crossProduct(toOrigin, triangle.edge1);
    v = dotProduct(ray.direction, q) * inverse;
    if (v < 0.0 || u + v > 1.0) return false;
    t = dotProduct(triangle.edge2, q) * inverse;
    return true;
}

}  // namespace

void RayScene::addMesh(const MeshData& mesh, const Matrix4& model, int materialIndex, bool lines) {
    const Matrix4 normalMatrix = model.inverse().transposed();
    const Color tint = materials[static_cast<std::size_t>(materialIndex)].tint;
    const Vec3 tintFactor{tint.r / 255.0, tint.g / 255.0, tint.b / 255.0};

    if (lines) {
        // A line has no area for a ray to hit: rebuild each segment as a thin 6-sided tube.
        for (std::size_t index = 0; index + 1 < mesh.indices.size(); index += 2) {
            const MeshVertex& a = mesh.vertices[mesh.indices[index]];
            const MeshVertex& b = mesh.vertices[mesh.indices[index + 1]];
            const Color color{static_cast<std::uint8_t>(a.color[0] * 255), static_cast<std::uint8_t>(a.color[1] * 255),
                              static_cast<std::uint8_t>(a.color[2] * 255)};
            const Vec3 start = model.transformPoint(positionOf(a));
            const Vec3 end = model.transformPoint(positionOf(b));
            if ((end - start).length() < 1e-6) continue;
            addMesh(makeCylinderBetween(start, end, 0.025, 6, color), Matrix4::identity(), materialIndex, false);
        }
        return;
    }

    auto colorOf = [&](const MeshVertex& vertex) {
        return Vec3(vertex.color[0] * tintFactor.x, vertex.color[1] * tintFactor.y, vertex.color[2] * tintFactor.z);
    };
    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3) {
        const MeshVertex& a = mesh.vertices[mesh.indices[index]];
        const MeshVertex& b = mesh.vertices[mesh.indices[index + 1]];
        const MeshVertex& c = mesh.vertices[mesh.indices[index + 2]];
        RayTriangle triangle;
        triangle.position0 = model.transformPoint(positionOf(a));
        triangle.edge1 = model.transformPoint(positionOf(b)) - triangle.position0;
        triangle.edge2 = model.transformPoint(positionOf(c)) - triangle.position0;
        triangle.normal0 = normalMatrix.transformDirection(normalOf(a)).normalized();
        triangle.normal1 = normalMatrix.transformDirection(normalOf(b)).normalized();
        triangle.normal2 = normalMatrix.transformDirection(normalOf(c)).normalized();
        triangle.color0 = colorOf(a);
        triangle.color1 = colorOf(b);
        triangle.color2 = colorOf(c);
        triangle.uv0 = {a.textureCoordinate[0], a.textureCoordinate[1]};
        triangle.uv1 = {b.textureCoordinate[0], b.textureCoordinate[1]};
        triangle.uv2 = {c.textureCoordinate[0], c.textureCoordinate[1]};
        triangle.materialIndex = materialIndex;
        triangles.push_back(triangle);
    }
}

void RayScene::build(const ParkScene& scene, const ParkFrame& frame) {
    triangles.clear();
    materials.clear();
    nodes.clear();
    for (const Instance& instance : frame.instances) {
        if (instance.material.lightBeam) continue;
        const MeshData* mesh = &scene.mesh(instance.mesh);
        if (instance.mesh == MeshId::dreads) mesh = &frame.dreads;
        if (instance.mesh == MeshId::balloonStrings) mesh = &frame.balloonStrings;
        materials.push_back(instance.material);
        addMesh(*mesh, instance.model, static_cast<int>(materials.size()) - 1, ParkScene::isLineMesh(instance.mesh));
    }
    nodes.reserve(triangles.size() * 2 / 4 + 1);
    buildNode(0, static_cast<int>(triangles.size()), 0);
}

int RayScene::buildNode(int first, int count, int depth) {
    BvhNode node;
    BoundingBox centroids;
    for (int index = first; index < first + count; ++index) {
        const RayTriangle& triangle = triangles[static_cast<std::size_t>(index)];
        node.bounds.expand(triangle.position0);
        node.bounds.expand(triangle.position0 + triangle.edge1);
        node.bounds.expand(triangle.position0 + triangle.edge2);
        centroids.expand(triangle.position0 + (triangle.edge1 + triangle.edge2) * (1.0 / 3.0));
    }
    const int nodeIndex = static_cast<int>(nodes.size());
    nodes.push_back(node);

    if (count <= 4 || depth > 40) {
        nodes[static_cast<std::size_t>(nodeIndex)].firstTriangle = first;
        nodes[static_cast<std::size_t>(nodeIndex)].count = count;
        return nodeIndex;
    }
    // Split at the MEDIAN centroid along the longest axis of the centroid box:
    // each half gets the same number of triangles, so the tree stays balanced.
    const Vec3 extent = centroids.maximum - centroids.minimum;
    const int axis = extent.x > extent.y && extent.x > extent.z ? 0 : (extent.y > extent.z ? 1 : 2);
    auto key = [axis](const RayTriangle& triangle) {
        const Vec3 centre = triangle.position0 + (triangle.edge1 + triangle.edge2) * (1.0 / 3.0);
        return axis == 0 ? centre.x : (axis == 1 ? centre.y : centre.z);
    };
    const int middle = first + count / 2;
    std::nth_element(triangles.begin() + first, triangles.begin() + middle, triangles.begin() + first + count,
                     [&](const RayTriangle& a, const RayTriangle& b) { return key(a) < key(b); });
    const int left = buildNode(first, middle - first, depth + 1);
    const int right = buildNode(middle, first + count - middle, depth + 1);
    nodes[static_cast<std::size_t>(nodeIndex)].left = left;
    nodes[static_cast<std::size_t>(nodeIndex)].right = right;
    return nodeIndex;
}

template <bool AnyHit>
bool RayScene::traverse(const Ray& ray, double minimumDistance, double maximumDistance, RayHit& hit) const {
    if (nodes.empty()) return false;
    const Vec3 inverseDirection{1.0 / ray.direction.x, 1.0 / ray.direction.y, 1.0 / ray.direction.z};
    bool found = false;
    double closest = maximumDistance;
    int stack[128];
    int stackSize = 0;
    stack[stackSize++] = 0;
    while (stackSize > 0) {
        const BvhNode& node = nodes[static_cast<std::size_t>(stack[--stackSize])];
        if (!rayHitsBox(node.bounds, ray.origin, inverseDirection, minimumDistance, closest)) continue;
        if (node.count > 0) {
            for (int index = node.firstTriangle; index < node.firstTriangle + node.count; ++index) {
                const RayTriangle& triangle = triangles[static_cast<std::size_t>(index)];
                if (AnyHit && materials[static_cast<std::size_t>(triangle.materialIndex)].emissive >= 0.5) continue;  // lights cast no shadow
                double t, u, v;
                if (intersectTriangle(triangle, ray, t, u, v) && t > minimumDistance && t < closest) {
                    if (AnyHit) return true;
                    closest = t;
                    hit = {t, index, u, v};
                    found = true;
                }
            }
        } else {
            stack[stackSize++] = node.left;
            stack[stackSize++] = node.right;
        }
    }
    return found;
}

bool RayScene::intersect(const Ray& ray, double minimumDistance, double maximumDistance, RayHit& hit) const {
    return traverse<false>(ray, minimumDistance, maximumDistance, hit);
}

bool RayScene::occluded(const Ray& ray, double maximumDistance) const {
    RayHit ignored;
    return traverse<true>(ray, 1e-4, maximumDistance, ignored);
}
