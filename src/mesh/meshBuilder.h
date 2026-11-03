/**
 * @file meshBuilder.h
 * @brief Procedural mesh generators: every 3D object in the park is built from these.
 *
 * Curved surfaces are approximated by many flat triangles (tessellation). Each
 * generator also computes the exact surface NORMAL at every vertex, so the
 * lighting shader can make the faceted mesh look smooth (Phong shading).
 * Unless stated otherwise objects are built around the origin, y up.
 */
#pragma once

#include <functional>
#include <vector>

#include "mesh/meshData.h"

/** @brief Picks a colour per side segment - used for stripes on tents, canopies and rims. */
using SegmentColorizer = std::function<Color(int segment)>;

/** @brief Axis-aligned box centred on the origin. Each face has its own vertices (flat normals). */
MeshData makeBox(const Vec3& size, Color color);

/**
 * @brief Cylinder standing on the origin, from y = 0 to y = height.
 *        Each side segment has its own vertices, so stripes stay crisp.
 */
MeshData makeCylinder(double radius, double height, int segments, const SegmentColorizer& colorizer, bool withCaps = true);
MeshData makeCylinder(double radius, double height, int segments, Color color, bool withCaps = true);

/** @brief Cone with its base on y = 0 and its apex at y = height. */
MeshData makeCone(double radius, double height, int segments, const SegmentColorizer& colorizer);
MeshData makeCone(double radius, double height, int segments, Color color);

/**
 * @brief Ellipsoid (x/a)^2 + (y/b)^2 + (z/c)^2 = 1 centred on the origin.
 *        Its normal is the gradient (x/a^2, y/b^2, z/c^2), normalised - NOT
 *        simply the position, which is only correct for a sphere.
 */
MeshData makeEllipsoid(const Vec3& radii, int slices, int stacks, Color color);

/** @brief Torus lying in the xy-plane (its hole faces +z): a ring of radius majorRadius, tube minorRadius. */
MeshData makeTorus(double majorRadius, double minorRadius, int majorSegments, int minorSegments, Color color);

/**
 * @brief A tube swept along a 3D path, tapering from startRadius to endRadius.
 *
 * The cross-section circle at each point is oriented with PARALLEL-TRANSPORT
 * frames: each frame is the previous one rotated just enough to follow the new
 * tangent, which avoids the sudden twists a fixed "up" vector would cause.
 */
MeshData makeTube(const std::vector<Vec3>& path, double startRadius, double endRadius, int sides, Color color);

/** @brief Cylinder between two arbitrary points (spokes, legs, supports). */
MeshData makeCylinderBetween(const Vec3& start, const Vec3& end, double radius, int segments, Color color);

/** @brief Flat rectangle in the xz-plane facing +y (the ground). */
MeshData makeGroundPlane(double width, double depth, Color color);

/** @brief Flat rectangle in the xy-plane facing +z with texture coordinates 0..1 (signs, labels). */
MeshData makeTexturedQuad(double width, double height, Color color);

/** @brief Line segments (GL_LINES) between consecutive pairs of points. */
MeshData makeLines(const std::vector<Vec3>& endpoints, Color color);
