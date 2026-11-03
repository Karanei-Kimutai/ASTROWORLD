/**
 * @file headModel.h
 * @brief The brass head as 3D meshes, built from the same head-unit data as the 2D head.
 *
 * Head units (u, v) are wrapped onto an ELLIPSOID: x = u * H, y = (0.5 - v) * H,
 * and z is read off the ellipsoid surface, so every 2D feature position from
 * astroHeadShape.h lands on the curved face. Local origin = the ground point
 * under the head's centre; the mouth faces +z.
 */
#pragma once

#include "mesh/meshData.h"

/** @brief Height scale H: world units per head unit. */
inline constexpr double headUnitSize = 9.3;

/** @brief Everything that does not move: face, ears, eyes, nose, lips. */
MeshData buildHeadMesh();

/** @brief The glowing doorway (vertex colours from white-hot core to orange rim), drawn emissive. */
MeshData buildPortalMesh();

/** @brief The dreads, rebuilt each frame because they bend in the wind by swayAngle radians. */
MeshData buildDreadMesh(double swayAngle);

/** @brief Local position of the point light inside the mouth. */
Vec3 portalLightPosition();
