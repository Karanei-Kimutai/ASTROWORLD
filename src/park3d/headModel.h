/**
 * @file headModel.h
 * @brief The giant inflatable head - ASTROWORLD's entrance gate - as 3D meshes.
 *
 * The head is ONE smooth, high-resolution surface: an ellipsoid whose vertices
 * are pushed out (or in) along their normals by a sum of soft bumps and creases
 * - nose, pouting upper lip, closed eyelids, cheeks, brow - so every feature
 * flows into the next like an inflated vinyl balloon. Faint seams run down the
 * sides and back, as on a real inflatable. A tunnel is cut through the mouth so
 * visitors walk through it into the park.
 *
 * Coordinates: head units (u, v) as in astroHeadShape.h, u right, v down,
 * v = 0.5 on the ground; world = (u, 0.5 - v, depth) * headUnitSize.
 * The head stands at the origin with its face toward +z.
 */
#pragma once

#include "mesh/meshData.h"

/** @brief World units (metres) per head unit: the head is ~15 m tall, ~8 visitors high. */
inline constexpr double headUnitSize = 14.0;

/** @brief The sculpted face and skull, with the mouth opening cut through. */
MeshData buildHeadMesh();

/** @brief The tunnel through the mouth: glowing walls and the inflated lip rims at both ends. */
MeshData buildMouthTunnel();

/** @brief The dreads, rebuilt each frame because they bend in the wind by swayAngle radians. */
MeshData buildDreadMesh(double swayAngle);

/** @brief Where the warm light inside the mouth sits (world space, head at the origin). */
Vec3 portalLightPosition();

/** @brief World z of the tunnel's front and back openings at ground level. */
double mouthTunnelFrontZ();
double mouthTunnelBackZ();

/** @brief Half-width of the doorway in metres. */
double mouthDoorwayHalfWidth();
