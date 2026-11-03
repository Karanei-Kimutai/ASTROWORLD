/**
 * @file scanLineFill.h
 * @brief Scan-line polygon filling with an Edge Table and Active Edge Table.
 *
 * Works for convex, concave and self-intersecting (complex) polygons using
 * the even-odd rule: along each scan line, pixels between the 1st and 2nd
 * edge crossings are inside, between the 2nd and 3rd outside, and so on.
 */
#pragma once

#include <vector>

#include "core/vec2.h"
#include "raster/rasterTypes.h"

/**
 * @brief Rasterize a polygon and report its interior as horizontal spans.
 *
 * Steps:
 *   1. EDGE TABLE (ET): bucket every non-horizontal edge by the first scan
 *      line it crosses, storing its last scan line, current x and 1/slope.
 *   2. For each scan line y (sampled at pixel centres y + 0.5):
 *      a. move edges starting here from the ET into the ACTIVE EDGE TABLE (AET)
 *      b. drop edges that have ended
 *      c. sort the AET by x and fill between pairs of crossings
 *      d. advance each active edge: x += 1/slope (incremental, no division)
 */
void fillPolygonScanLine(const std::vector<Vec2>& vertices, const SpanCallback& emitSpan);

/** @brief Fill a polygon with one colour. */
void fillPolygon(FrameBuffer& frameBuffer, const std::vector<Vec2>& vertices, Color color);

/** @brief Fill a polygon, colouring each pixel with a shader (gradients, stripes, lighting). */
void fillPolygon(FrameBuffer& frameBuffer, const std::vector<Vec2>& vertices, const PixelShader& shader);
