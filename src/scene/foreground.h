/**
 * @file foreground.h
 * @brief Foreground details: visitors walking into the head, and the balloon vendor.
 */
#pragma once

#include <cstdint>

#include "core/frameBuffer.h"
#include "core/vec2.h"

/**
 * @brief A person silhouette with feet at 'feetPosition'.
 *
 * The figure is modelled once in LOCAL units (feet at the origin, height 1)
 * and placed with a model matrix T(feet) * S(height). The legs are children
 * that rotate about the hip by +-walkSwing radians, giving a walk cycle.
 */
void drawPersonSilhouette(FrameBuffer& frameBuffer, const Vec2& feetPosition, double height, double walkSwing, Color color);

/**
 * @brief Visitors walking toward the glowing mouth and disappearing inside.
 *
 * Each visitor loops along a straight path from a start point near the viewer
 * to the doorway. Their size follows their depth (perspective scaling), and
 * at the doorway they shrink to nothing - a scaling about the feet - as if
 * stepping into the light. Drawn back-to-front (painter's algorithm).
 *
 * @param portalBase Ground point at the centre of the mouth doorway.
 */
void drawCrowd(FrameBuffer& frameBuffer, const Vec2& portalBase, double timeSeconds, std::uint32_t seed);

/**
 * @brief A balloon vendor holding a bunch of balloons that sway in the breeze.
 *
 * Each balloon centre is rotated about the vendor's hand (Matrix3::rotationAbout).
 * Its outline is a midpoint ellipse; its interior is painted with a 4-connected
 * BOUNDARY FILL from the centre (8-connected would leak through the diagonal
 * steps of the ellipse outline).
 */
void drawBalloonVendor(FrameBuffer& frameBuffer, const Vec2& feetPosition, double timeSeconds);
