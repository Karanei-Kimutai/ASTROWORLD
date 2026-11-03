/**
 * @file clipping.h
 * @brief Cohen-Sutherland line clipping against an axis-aligned window.
 */
#pragma once

#include "core/vec2.h"

/** @brief Rectangular clip window in screen coordinates (inclusive bounds). */
struct ClipWindow {
    double xMin = 0.0;
    double yMin = 0.0;
    double xMax = 0.0;
    double yMax = 0.0;
};

/**
 * @brief 4-bit region code ("outcode") flags.
 *
 * Screen y grows downward, so ABOVE means y < yMin.
 */
enum OutCodeBits : int {
    outCodeInside = 0,
    outCodeLeft = 1,
    outCodeRight = 2,
    outCodeAbove = 4,
    outCodeBelow = 8,
};

/** @brief Region code of a point relative to the window. */
int computeOutCode(const Vec2& point, const ClipWindow& window);

/**
 * @brief Clip the segment start-end to the window, modifying both endpoints in place.
 *
 * Cohen-Sutherland:
 *   1. Both outcodes 0           -> trivially ACCEPT (fully inside).
 *   2. outcodeA & outcodeB != 0  -> trivially REJECT (both on the same outside side).
 *   3. Otherwise pick an endpoint that is outside, slide it to the window
 *      edge it violates (using the line equation), recompute its code, repeat.
 *
 * @return false if no part of the segment is visible.
 */
bool clipLineCohenSutherland(Vec2& start, Vec2& end, const ClipWindow& window);
