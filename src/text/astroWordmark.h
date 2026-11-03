/**
 * @file astroWordmark.h
 * @brief The "ASTROWORLD" title lettering, built entirely from vector strokes.
 *
 * Each letter is described by centre-line strokes in a unit box (lines,
 * Bezier curves, a Catmull-Rom spline for the S, an ellipse for the O) and
 * then stroked thickly. Because the letters are VECTOR descriptions they can
 * be drawn at any size without blockiness (contrast with bitmapFont).
 *
 * Retro theme-park look, drawn in layered passes:
 *   1. drop shadow   2. neon pink glow ring   3. dark outline
 *   4. gold gradient fill   5. glossy highlight
 */
#pragma once

#include <string>

#include "core/frameBuffer.h"
#include "core/vec2.h"

/** @brief Size and look of the wordmark. */
struct WordmarkStyle {
    double letterHeight = 90.0;          // cap height in pixels
    double strokeRadiusFraction = 0.11;  // stroke half-width as a fraction of letterHeight
    double slant = 0.16;                 // italic shear (x offset per unit of height)
    double letterSpacing = 0.30;         // gap between letters, in letterHeight units
};

/** @brief Total width in pixels of the wordmark for the given style. */
double measureWordmark(const std::string& text, const WordmarkStyle& style);

/** @brief Draw the wordmark with its bounding box's top-left at topLeft. */
void drawWordmark(FrameBuffer& frameBuffer, const std::string& text, const Vec2& topLeft, const WordmarkStyle& style);
