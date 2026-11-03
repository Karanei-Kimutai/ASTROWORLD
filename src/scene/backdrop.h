/**
 * @file backdrop.h
 * @brief Everything behind the rides: sky, stars, searchlights, skyline, ground and path.
 */
#pragma once

#include <cstdint>

#include "core/frameBuffer.h"
#include "core/vec2.h"

/** @brief Sunset sky: vertical 3-stop gradient interpolated in HSV space, filled with scan-line fill. */
void drawSkyGradient(FrameBuffer& frameBuffer, int horizonY);

/** @brief Soft glow on the horizon made of stacked alpha-blended midpoint ellipses. */
void drawHorizonGlow(FrameBuffer& frameBuffer, const Vec2& center, int radiusX, int radiusY);

/** @brief Deterministic star field: blended single pixels plus a few Bresenham "sparkle" crosses. */
void drawStars(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed);

/**
 * @brief A searchlight beam: a fan of DDA rays, each clipped to the screen with
 *        Cohen-Sutherland before rasterizing, blended at low alpha.
 * @param angleDegrees Beam direction, 90 = straight up, measured counter-clockwise from +x.
 */
void drawSearchlight(FrameBuffer& frameBuffer, const Vec2& origin, double angleDegrees, double spreadDegrees,
                     Color beamColor);

/** @brief Distant silhouettes of tents, buildings and trees as one concave polygon (scan-line fill). */
void drawSkyline(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed);

/** @brief Ground plane from the horizon to the bottom of the screen. */
void drawGround(FrameBuffer& frameBuffer, int horizonY);

/**
 * @brief The entrance path leading toward the head - a trapezoid with tile lines
 *        spaced to suggest one-point perspective (lines bunch up toward the horizon).
 */
void drawEntrancePath(FrameBuffer& frameBuffer, int topY, double topLeftX, double topRightX, double bottomLeftX,
                      double bottomRightX);
