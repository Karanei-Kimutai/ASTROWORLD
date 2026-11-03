/**
 * @file lines.h
 * @brief Line drawing: DDA (floating point) and Bresenham (integer only).
 */
#pragma once

#include <vector>

#include "core/vec2.h"
#include "raster/rasterTypes.h"

/**
 * @brief Digital Differential Analyzer line rasterization.
 *
 * Steps one pixel at a time along the longer axis and adds a constant
 * floating-point increment (dy/steps, dx/steps) to the other axis, rounding
 * each sample to the nearest pixel. Simple, but needs floating-point adds and
 * rounding at every step.
 */
void rasterizeLineDda(int startX, int startY, int endX, int endY, const PlotCallback& plot);

/**
 * @brief Bresenham's line rasterization using an integer decision parameter.
 *
 * For |slope| <= 1 the decision parameter starts at p0 = 2*dy - dx and after
 * each step becomes p + 2*dy (stay on the same row) or p + 2*dy - 2*dx (move
 * one row). Steep lines swap the roles of x and y. Only integer additions are
 * used, which is why it was the hardware standard.
 */
void rasterizeLineBresenham(int startX, int startY, int endX, int endY, const PlotCallback& plot);

/** @brief Draw a solid DDA line. */
void drawLineDda(FrameBuffer& frameBuffer, int startX, int startY, int endX, int endY, Color color);

/** @brief Draw a solid Bresenham line. */
void drawLineBresenham(FrameBuffer& frameBuffer, int startX, int startY, int endX, int endY, Color color);

/** @brief Draw a solid Bresenham line between two real-valued points (rounded to pixels). */
void drawLineBresenham(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, Color color);

/** @brief Connect consecutive points with 1-pixel Bresenham lines (optionally closing the loop). */
void drawPolylineBresenham(FrameBuffer& frameBuffer, const std::vector<Vec2>& points, Color color,
                           bool closed = false);
