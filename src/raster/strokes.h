/**
 * @file strokes.h
 * @brief Thick lines ("strokes") with round caps, built from filled primitives.
 *
 * A 1-pixel Bresenham line is too thin for coaster rails, letters and hair.
 * A thick segment is drawn as:
 *   - a rectangle (quad) around the centre line, filled with the scan-line algorithm
 *   - a filled midpoint circle at each end (round cap / round join)
 */
#pragma once

#include <vector>

#include "core/vec2.h"
#include "raster/rasterTypes.h"

/** @brief Rasterize one thick segment and report its coverage as spans. */
void fillThickSegment(const Vec2& start, const Vec2& end, double radius, const SpanCallback& emitSpan);

void drawThickLine(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, double radius, Color color);
void drawThickLine(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, double radius,
                   const PixelShader& shader);

/** @brief Thick stroke through consecutive points (round joins at every vertex). */
void drawThickPolyline(FrameBuffer& frameBuffer, const std::vector<Vec2>& points, double radius, Color color);
void drawThickPolyline(FrameBuffer& frameBuffer, const std::vector<Vec2>& points, double radius,
                       const PixelShader& shader);
