/**
 * @file transformShowcase.h
 * @brief A labelled 4x2 grid demonstrating the CP2 transformations.
 *
 * Panels (left to right, top to bottom):
 *   1 Translation          2 Rotation about a pivot    3 Scaling about a fixed point   4 Shear and reflection
 *   5 Order matters (TR/RT) 6 Hierarchy (wheel)        7 4x4 rotation (orthographic)   8 Perspective projection
 */
#pragma once

#include "core/frameBuffer.h"

/** @brief Render the showcase into a 1280x720 frame buffer. */
void renderTransformShowcase(FrameBuffer& frameBuffer);
