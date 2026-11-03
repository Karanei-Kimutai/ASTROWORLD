/**
 * @file foreground.h
 * @brief Foreground details: crowd silhouettes walking into the head, balloon vendor.
 */
#pragma once

#include <cstdint>

#include "core/frameBuffer.h"
#include "core/vec2.h"

/** @brief A person silhouette standing with feet at 'feetPosition' (circle head + polygon body). */
void drawPersonSilhouette(FrameBuffer& frameBuffer, const Vec2& feetPosition, double height, Color color);

/**
 * @brief Visitors walking toward the glowing mouth, scaled by distance and drawn
 *        back-to-front (painter's algorithm) so nearer people overlap farther ones.
 * @param portalBase Ground point at the centre of the mouth doorway.
 */
void drawCrowd(FrameBuffer& frameBuffer, const Vec2& portalBase, std::uint32_t seed);

/**
 * @brief A balloon vendor holding a bunch of balloons.
 *
 * Each balloon outline is a midpoint ellipse; its interior is painted with a
 * 4-connected BOUNDARY FILL from the centre. (8-connected would leak through
 * the diagonal steps of the ellipse outline.)
 */
void drawBalloonVendor(FrameBuffer& frameBuffer, const Vec2& feetPosition);
