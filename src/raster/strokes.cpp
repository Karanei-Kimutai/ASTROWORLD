/**
 * @file strokes.cpp
 * @brief Thick stroke implementation (scan-line quad + midpoint circle caps).
 */
#include "raster/strokes.h"

#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/lines.h"

void fillThickSegment(const Vec2& start, const Vec2& end, double radius, const SpanCallback& emitSpan) {
    // Very thin strokes: a plain Bresenham line already looks right.
    if (radius < 0.75) {
        rasterizeLineBresenham(roundToInt(start.x), roundToInt(start.y), roundToInt(end.x), roundToInt(end.y),
                               [&](int x, int y) { emitSpan(y, x, x); });
        return;
    }

    const Vec2 direction = (end - start).normalized();
    if (direction.length() > 0.0) {
        // Offset the centre line sideways by +-radius to get the four quad corners.
        const Vec2 sideOffset = direction.perpendicular() * radius;
        const std::vector<Vec2> quad = {start + sideOffset, end + sideOffset, end - sideOffset, start - sideOffset};
        fillPolygonScanLine(quad, emitSpan);
    }
    // Round caps hide the gaps between consecutive segments of a polyline.
    const int capRadius = roundToInt(radius);
    rasterizeCircleFilled(roundToInt(start.x), roundToInt(start.y), capRadius, emitSpan);
    rasterizeCircleFilled(roundToInt(end.x), roundToInt(end.y), capRadius, emitSpan);
}

void drawThickLine(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, double radius, Color color) {
    fillThickSegment(start, end, radius, makeSolidSpanWriter(frameBuffer, color));
}

void drawThickLine(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, double radius,
                   const PixelShader& shader) {
    fillThickSegment(start, end, radius, makeShadedSpanWriter(frameBuffer, shader));
}

void drawThickPolyline(FrameBuffer& frameBuffer, const std::vector<Vec2>& points, double radius, Color color) {
    const SpanCallback writer = makeSolidSpanWriter(frameBuffer, color);
    for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        fillThickSegment(points[index], points[index + 1], radius, writer);
    }
}

void drawThickPolyline(FrameBuffer& frameBuffer, const std::vector<Vec2>& points, double radius,
                       const PixelShader& shader) {
    const SpanCallback writer = makeShadedSpanWriter(frameBuffer, shader);
    for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        fillThickSegment(points[index], points[index + 1], radius, writer);
    }
}
