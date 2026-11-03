/**
 * @file astroHeadShape.h
 * @brief The head's shape data in HEAD UNITS, shared by the software (2D) and OpenGL (3D) renderers.
 *
 * Head units: u to the right, v DOWN, (0, 0) = head centre, v = -0.56 crown,
 * v = +0.5 ground line, u = +-0.43 cheeks. The 2D renderer maps them to
 * pixels with a Matrix3; the 3D renderer maps them onto an ellipsoid.
 */
#pragma once

#include "core/vec2.h"

namespace astroHeadShape {

/** @brief Number of control points in each dread centre line. */
inline constexpr int dreadPointCount = 7;

/**
 * @brief Dread centre lines for the RIGHT side of the head (u > 0); the left side mirrors them.
 *
 * Each dread roots on the crown, rises in a tall arch above the head, then
 * falls down the side. Roots are spaced across the crown so the arches never cross.
 */
inline const Vec2 outerDread[] = {{0.26, -0.46}, {0.40, -0.59}, {0.52, -0.50}, {0.57, -0.28}, {0.58, 0.0}, {0.59, 0.22}, {0.63, 0.30}};
inline const Vec2 middleDread[] = {{0.16, -0.51}, {0.26, -0.67}, {0.40, -0.63}, {0.46, -0.42}, {0.48, -0.15}, {0.48, 0.08}, {0.51, 0.16}};
inline const Vec2 frontDreadRight[] = {{0.06, -0.54}, {0.11, -0.71}, {0.24, -0.69}, {0.30, -0.50}, {0.32, -0.25}, {0.31, 0.0}, {0.34, 0.08}};
inline const Vec2 frontDreadLeft[] = {{0.06, -0.54}, {0.12, -0.71}, {0.25, -0.69}, {0.32, -0.50}, {0.34, -0.20}, {0.33, 0.12}, {0.37, 0.22}};

/** @brief How strongly each dread bends with the wind (outer dreads swing most). */
inline constexpr double outerDreadSway = 1.0;
inline constexpr double middleDreadSway = 0.8;
inline constexpr double frontDreadSway = 0.6;

/** @brief Half-width of the mouth doorway and the height (v) where its straight sides end and the arch begins. */
inline constexpr double doorwayHalfWidth = 0.135;
inline constexpr double doorwayArchStartV = 0.40;
inline constexpr double doorwayArchControlV = 0.30;

}  // namespace astroHeadShape
