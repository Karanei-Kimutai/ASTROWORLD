/**
 * @file boundaryFill.h
 * @brief Boundary fill (seed fill) with 4-connected and 8-connected neighbourhoods.
 *
 * Starting from a seed pixel inside a region, keep painting neighbours until
 * a pixel of the BOUNDARY colour is reached.
 *
 *   4-connected: spreads to N, S, E, W only.
 *   8-connected: also spreads diagonally (NE, NW, SE, SW).
 *
 * Key lesson (shown in the algorithm showcase): outlines drawn by Bresenham /
 * midpoint algorithms are 8-connected - consecutive pixels may only touch at
 * a corner. An 8-connected fill slips through those diagonal gaps and LEAKS;
 * a 4-connected fill cannot. So the balloons in the scene use boundaryFill4.
 *
 * Implemented with an explicit stack instead of recursion so large regions do
 * not overflow the call stack.
 */
#pragma once

#include "core/frameBuffer.h"

/** @return Number of pixels painted. */
int boundaryFill4(FrameBuffer& frameBuffer, int seedX, int seedY, Color fillColor, Color boundaryColor);

/** @return Number of pixels painted. */
int boundaryFill8(FrameBuffer& frameBuffer, int seedX, int seedY, Color fillColor, Color boundaryColor);
