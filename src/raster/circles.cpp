/**
 * @file circles.cpp
 * @brief Midpoint circle and ellipse algorithm implementations.
 */
#include "raster/circles.h"

#include <algorithm>
#include <functional>
#include <vector>

namespace {

/**
 * @brief Walk one octant of a circle (from the top, x = 0 .. x == y) with the
 *        midpoint decision parameter and report each (x, y) offset.
 *
 * Decision parameter p evaluates the circle function f(x, y) = x^2 + y^2 - r^2
 * at the MIDPOINT between the two candidate pixels (x + 1, y) and (x + 1, y - 1):
 *   p < 0  -> midpoint inside the circle  -> keep y,   p += 2x + 1
 *   p >= 0 -> midpoint outside the circle -> y -= 1,   p += 2(x - y) + 1
 * Starting value p0 = 5/4 - r, rounded to 1 - r for integer arithmetic.
 */
void traceCircleOctant(int radius, const std::function<void(int, int)>& visit) {
    int x = 0;
    int y = radius;
    int decision = 1 - radius;
    while (x <= y) {
        visit(x, y);
        ++x;
        if (decision < 0) {
            decision += 2 * x + 1;
        } else {
            --y;
            decision += 2 * (x - y) + 1;
        }
    }
}

/**
 * @brief Walk the first quadrant of an ellipse (from (0, ry) to (rx, 0)) with
 *        the two-region midpoint algorithm, reporting each (x, y) offset.
 *
 * Ellipse function: f(x, y) = ry^2 x^2 + rx^2 y^2 - rx^2 ry^2.
 * The boundary between regions is where the slope is -1, i.e. where
 * 2 ry^2 x >= 2 rx^2 y. The running terms px = 2 ry^2 x and py = 2 rx^2 y are
 * updated incrementally so each step only needs additions.
 */
void traceEllipseQuadrant(int radiusX, int radiusY, const std::function<void(int, int)>& visit) {
    // Degenerate ellipses collapse to straight lines.
    if (radiusX <= 0 || radiusY <= 0) {
        for (int y = 0; y <= std::max(radiusY, 0); ++y) visit(0, y);
        for (int x = 0; x <= std::max(radiusX, 0); ++x) visit(x, 0);
        return;
    }

    const double radiusXSquared = static_cast<double>(radiusX) * radiusX;
    const double radiusYSquared = static_cast<double>(radiusY) * radiusY;

    int x = 0;
    int y = radiusY;
    double slopeX = 0.0;                           // px = 2 ry^2 x
    double slopeY = 2.0 * radiusXSquared * y;      // py = 2 rx^2 y

    // Region 1: p1_0 = ry^2 - rx^2 ry + rx^2 / 4
    double decision1 = radiusYSquared - radiusXSquared * radiusY + 0.25 * radiusXSquared;
    while (slopeX < slopeY) {
        visit(x, y);
        ++x;
        slopeX += 2.0 * radiusYSquared;
        if (decision1 < 0.0) {
            decision1 += radiusYSquared + slopeX;
        } else {
            --y;
            slopeY -= 2.0 * radiusXSquared;
            decision1 += radiusYSquared + slopeX - slopeY;
        }
    }

    // Region 2: p2_0 = ry^2 (x + 1/2)^2 + rx^2 (y - 1)^2 - rx^2 ry^2
    double decision2 = radiusYSquared * (x + 0.5) * (x + 0.5) + radiusXSquared * (y - 1.0) * (y - 1.0) -
                       radiusXSquared * radiusYSquared;
    while (y >= 0) {
        visit(x, y);
        --y;
        slopeY -= 2.0 * radiusXSquared;
        if (decision2 > 0.0) {
            decision2 += radiusXSquared - slopeY;
        } else {
            ++x;
            slopeX += 2.0 * radiusYSquared;
            decision2 += radiusXSquared - slopeY + slopeX;
        }
    }
}

/** @brief Emit one span per row from a table of half-widths indexed by row offset. */
void emitSymmetricSpans(int centerX, int centerY, const std::vector<int>& halfWidths, const SpanCallback& emitSpan) {
    for (std::size_t rowOffset = 0; rowOffset < halfWidths.size(); ++rowOffset) {
        const int halfWidth = halfWidths[rowOffset];
        if (halfWidth < 0) continue;
        const int offset = static_cast<int>(rowOffset);
        emitSpan(centerY + offset, centerX - halfWidth, centerX + halfWidth);
        if (offset != 0) emitSpan(centerY - offset, centerX - halfWidth, centerX + halfWidth);  // mirror row
    }
}

}  // namespace

void rasterizeCircleOutline(int centerX, int centerY, int radius, const PlotCallback& plot) {
    if (radius < 0) return;
    traceCircleOctant(radius, [&](int x, int y) {
        // 8-way symmetry: one computed point gives eight pixels.
        plot(centerX + x, centerY + y);
        plot(centerX - x, centerY + y);
        plot(centerX + x, centerY - y);
        plot(centerX - x, centerY - y);
        plot(centerX + y, centerY + x);
        plot(centerX - y, centerY + x);
        plot(centerX + y, centerY - x);
        plot(centerX - y, centerY - x);
    });
}

void rasterizeCircleFilled(int centerX, int centerY, int radius, const SpanCallback& emitSpan) {
    if (radius < 0) return;
    // halfWidths[d] = how far the circle extends left/right on row centerY +- d.
    std::vector<int> halfWidths(static_cast<std::size_t>(radius) + 1, -1);
    traceCircleOctant(radius, [&](int x, int y) {
        // The octant point (x, y) fixes the half-width of row y; its mirror (y, x) fixes row x.
        halfWidths[y] = std::max(halfWidths[y], x);
        halfWidths[x] = std::max(halfWidths[x], y);
    });
    emitSymmetricSpans(centerX, centerY, halfWidths, emitSpan);
}

void rasterizeEllipseOutline(int centerX, int centerY, int radiusX, int radiusY, const PlotCallback& plot) {
    traceEllipseQuadrant(radiusX, radiusY, [&](int x, int y) {
        // 4-way symmetry: mirror across both axes.
        plot(centerX + x, centerY + y);
        plot(centerX - x, centerY + y);
        plot(centerX + x, centerY - y);
        plot(centerX - x, centerY - y);
    });
}

void rasterizeEllipseFilled(int centerX, int centerY, int radiusX, int radiusY, const SpanCallback& emitSpan) {
    if (radiusX < 0 || radiusY < 0) return;
    std::vector<int> halfWidths(static_cast<std::size_t>(radiusY) + 1, -1);
    traceEllipseQuadrant(radiusX, radiusY, [&](int x, int y) {
        if (y >= 0 && y <= radiusY) halfWidths[y] = std::max(halfWidths[y], x);
    });
    emitSymmetricSpans(centerX, centerY, halfWidths, emitSpan);
}

void drawCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, Color color) {
    rasterizeCircleOutline(centerX, centerY, radius, makeSolidPlotter(frameBuffer, color));
}

void fillCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, Color color) {
    rasterizeCircleFilled(centerX, centerY, radius, makeSolidSpanWriter(frameBuffer, color));
}

void fillCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, const PixelShader& shader) {
    rasterizeCircleFilled(centerX, centerY, radius, makeShadedSpanWriter(frameBuffer, shader));
}

void blendCircle(FrameBuffer& frameBuffer, int centerX, int centerY, int radius, Color color, double alpha) {
    rasterizeCircleFilled(centerX, centerY, radius, makeBlendSpanWriter(frameBuffer, color, alpha));
}

void drawEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY, Color color) {
    rasterizeEllipseOutline(centerX, centerY, radiusX, radiusY, makeSolidPlotter(frameBuffer, color));
}

void fillEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY, Color color) {
    rasterizeEllipseFilled(centerX, centerY, radiusX, radiusY, makeSolidSpanWriter(frameBuffer, color));
}

void fillEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY,
                 const PixelShader& shader) {
    rasterizeEllipseFilled(centerX, centerY, radiusX, radiusY, makeShadedSpanWriter(frameBuffer, shader));
}

void blendEllipse(FrameBuffer& frameBuffer, int centerX, int centerY, int radiusX, int radiusY, Color color,
                  double alpha) {
    rasterizeEllipseFilled(centerX, centerY, radiusX, radiusY, makeBlendSpanWriter(frameBuffer, color, alpha));
}
