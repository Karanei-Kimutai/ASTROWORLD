/**
 * @file bitmapFont.h
 * @brief A 5x7 pixel bitmap font for labels (a raster font).
 *
 * Each glyph is a 5-column x 7-row grid of on/off pixels. Scaling it up just
 * repeats pixels, which is why bitmap (raster) text looks blocky when
 * enlarged - compare with the vector-stroked ASTROWORLD wordmark, which stays
 * smooth at any size.
 */
#pragma once

#include <string>

#include "core/frameBuffer.h"

/**
 * @brief Draw text with its top-left corner at (x, y).
 * @param pixelScale Size of one font "pixel" in screen pixels.
 */
void drawBitmapText(FrameBuffer& frameBuffer, const std::string& text, int x, int y, int pixelScale, Color color);

/** @brief Width in screen pixels that drawBitmapText would use. */
int measureBitmapText(const std::string& text, int pixelScale);
