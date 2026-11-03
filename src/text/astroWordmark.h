/**
 * @file astroWordmark.h
 * @brief The ASTROWORLD logo: psychedelic postcard lettering built from vector shapes.
 *
 * The logo's look (from the "Greetings from ASTROWORLD - Wish you were here"
 * artwork):
 *   - tall, heavy letters whose stems PINCH at the waist and FLARE at both ends
 *   - teardrop-shaped counters
 *   - every letter a different flat colour
 *   - the first O is the Earth (with a smiley face), the second O is a set of
 *     concentric rounded-square rings
 *
 * Each letter is described in a unit box (height 1) as polygons built from
 * Bezier curves and Catmull-Rom splines. Pieces are ADDED to and holes are
 * ERASED from an off-screen CoverageMask rendered at 3x resolution, then the
 * mask is composited with supersampling anti-aliasing (see core/coverageMask.h).
 * Because the letters are vector descriptions, they scale to any size.
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

/** @brief Size and quality settings for the logo. */
struct WordmarkStyle {
    double letterHeight = 78.0;   // cap height in pixels
    double letterSpacing = 0.06;  // gap between letters, in letterHeight units
    int supersampleFactor = 3;    // 3 -> 9 coverage samples per pixel
    Vec2 shadowOffset{3.0, 4.0};  // drop shadow offset in pixels
    double shadowOpacity = 0.55;
};

/** @brief Width in pixels of the "ASTROWORLD" logo for the given style. */
double measureWordmark(const WordmarkStyle& style);

/** @brief Draw the "ASTROWORLD" logo with its bounding box's top-left at topLeft. */
void drawWordmark(FrameBuffer& frameBuffer, const Vec2& topLeft, const WordmarkStyle& style);
