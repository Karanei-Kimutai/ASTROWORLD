/**
 * @file algorithmShowcase.h
 * @brief A labelled 4x2 grid demonstrating each CP1 algorithm in isolation.
 *
 * Panels (left to right, top to bottom):
 *   1 DDA line            2 Bresenham line       3 Midpoint circle + ellipse   4 Bezier (de Casteljau)
 *   5 Catmull-Rom spline  6 Scan-line fill       7 Boundary fill 4 vs 8        8 Cohen-Sutherland clipping
 */
#pragma once

#include "core/frameBuffer.h"

/** @brief Render the showcase into a 1280x720 frame buffer. */
void renderAlgorithmShowcase(FrameBuffer& frameBuffer);
