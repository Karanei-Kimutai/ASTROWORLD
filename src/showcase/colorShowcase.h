/**
 * @file colorShowcase.h
 * @brief Colour theory demonstration: HSV wheel, tints/shades/tones, RGB vs CMYK,
 *        24-bit vs 8-bit reduction, and the ASTROWORLD palette.
 */
#pragma once

#include "core/frameBuffer.h"

/** @brief Render the colour showcase into a 1280x720 frame buffer. */
void renderColorShowcase(FrameBuffer& frameBuffer);
