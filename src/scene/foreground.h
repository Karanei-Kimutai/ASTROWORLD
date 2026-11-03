/**
 * @file foreground.h
 * @brief Foreground details: crowd silhouettes, fence with string lights, balloon vendor.
 */
#pragma once

#include <cstdint>

#include "core/frameBuffer.h"
#include "core/vec2.h"

/** @brief A person silhouette standing with feet at 'feetPosition' (circle head + polygon body). */
void drawPersonSilhouette(FrameBuffer& frameBuffer, const Vec2& feetPosition, double height, Color color);

/**
 * @brief Visitors walking up the entrance path, scaled by depth and drawn
 *        back-to-front (painter's algorithm) so nearer people overlap farther ones.
 */
void drawCrowd(FrameBuffer& frameBuffer, int pathTopY, double topLeftX, double topRightX, double bottomLeftX,
               double bottomRightX, std::uint32_t seed);

/**
 * @brief Fence posts across the bottom of the screen with Bezier-curve string
 *        lights sagging between them; the gap [gapStartX, gapEndX] is left open for the path.
 */
void drawFenceWithLights(FrameBuffer& frameBuffer, double fenceTopY, double fenceBottomY, double gapStartX,
                         double gapEndX);

/**
 * @brief A balloon vendor holding a bunch of balloons.
 *
 * Each balloon outline is a midpoint ellipse; its interior is painted with a
 * 4-connected BOUNDARY FILL from the centre. (8-connected would leak through
 * the diagonal steps of the ellipse outline.)
 */
void drawBalloonVendor(FrameBuffer& frameBuffer, const Vec2& feetPosition);
