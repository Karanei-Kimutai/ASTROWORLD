/**
 * @file circles.h
 * @brief Midpoint circle (8-way symmetry) and midpoint ellipse (4-way symmetry).
 */
#pragma once

#include "raster/rasterTypes.h"

/**
 * @brief Midpoint circle outline.
 *
 * Computes only the 45-degree octant from the top of the circle (x from 0 up
 * to x == y) with an integer decision parameter p0 = 1 - r, then mirrors each
 * point into all 8 octants: (+-x, +-y) and (+-y, +-x).
 */
void rasterizeCircleOutline(int centerX, int centerY, int radius, const PlotCallback& plot);

/**
 * @brief Filled midpoint circle, emitted as one horizontal span per row.
 *
 * Uses the same octant walk as the outline; the 8-way symmetry tells us the
 * half-width of the circle on 2 rows per computed point. Each row is emitted
 * exactly once, so blending fills do not double-darken any pixel.
 */
void rasterizeCircleFilled(int centerX, int centerY, int radius, const SpanCallback& emitSpan);

/**
 * @brief Midpoint ellipse outline.
 *
 * An ellipse only has 4-way symmetry (+-x, +-y), so a full quadrant is
 * computed in two regions:
 *   Region 1 - slope shallower than -1: step x every time, decide on y.
 *   Region 2 - slope steeper than -1:  step y every time, decide on x.
 */
void rasterizeEllipseOutline(int centerX, int centerY, int radiusX, int radiusY, const PlotCallback& plot);

/** @brief Filled midpoint ellipse, one span per row. */
void rasterizeEllipseFilled(int centerX, int centerY, int radiusX, int radiusY, const SpanCallback& emitSpan);

// ---- Convenience wrappers that draw straight into a frame buffer ----

void drawCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, Color color);
void fillCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, Color color);
void fillCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, const PixelShader& shader);
void blendCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, Color color, double alpha);

void drawEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY, Color color);
void fillEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY, Color color);
void fillEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY,
                 const PixelShader& shader);
void blendEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY, Color color,
                  double alpha);

/**
 * @brief Blend a colour over an ellipse with alpha fading from maxAlpha at the
 *        centre to 0 at the edge: alpha = maxAlpha * (1 - d^2)^2, where d is the
 *        normalised elliptical distance. A per-fragment alpha, like a soft airbrush.
 *        Used for clouds, glows, haze and contour shadows.
 */
void blendSoftEllipse(FrameBuffer& frameBuffer, double centerX, double centerY, double radiusX, double radiusY,
                      Color color, double maxAlpha);
