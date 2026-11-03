/**
 * @file lines.cpp
 * @brief DDA and Bresenham line algorithm implementations.
 */
#include "raster/lines.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

void rasterizeLineDda(int startX, int startY, int endX, int endY, const PlotCallback& plot) {
    const int deltaX = endX - startX;
    const int deltaY = endY - startY;
    // Number of unit steps = length along the dominant (longer) axis.
    const int stepCount = std::max(std::abs(deltaX), std::abs(deltaY));
    if (stepCount == 0) {
        plot(startX, startY);  // degenerate line: a single point
        return;
    }

    // Per-step increments: one of them is exactly +/-1, the other a fraction.
    const double xIncrement = static_cast<double>(deltaX) / stepCount;
    const double yIncrement = static_cast<double>(deltaY) / stepCount;

    double x = startX;
    double y = startY;
    for (int step = 0; step <= stepCount; ++step) {
        plot(roundToInt(x), roundToInt(y));  // snap the exact position to the nearest pixel
        x += xIncrement;
        y += yIncrement;
    }
}

void rasterizeLineBresenham(int startX, int startY, int endX, int endY, const PlotCallback& plot) {
    const int deltaX = std::abs(endX - startX);
    const int deltaY = std::abs(endY - startY);

    if (deltaY <= deltaX) {
        // GENTLE line (|slope| <= 1): x is the driving axis, y occasionally steps.
        // Always walk left -> right so x only ever increments.
        if (startX > endX) {
            std::swap(startX, endX);
            std::swap(startY, endY);
        }
        const int yStep = endY >= startY ? 1 : -1;  // handles downward and upward slopes
        int decision = 2 * deltaY - deltaX;          // p0 = 2dy - dx
        int y = startY;
        for (int x = startX; x <= endX; ++x) {
            plot(x, y);
            if (decision >= 0) {
                // The true line has passed the midpoint between rows: move to the next row.
                y += yStep;
                decision += 2 * (deltaY - deltaX);
            } else {
                // Midpoint is still on the far side: stay on this row.
                decision += 2 * deltaY;
            }
        }
    } else {
        // STEEP line (|slope| > 1): the same algorithm with x and y swapped.
        if (startY > endY) {
            std::swap(startX, endX);
            std::swap(startY, endY);
        }
        const int xStep = endX >= startX ? 1 : -1;
        int decision = 2 * deltaX - deltaY;
        int x = startX;
        for (int y = startY; y <= endY; ++y) {
            plot(x, y);
            if (decision >= 0) {
                x += xStep;
                decision += 2 * (deltaX - deltaY);
            } else {
                decision += 2 * deltaX;
            }
        }
    }
}

void drawLineDda(FrameBuffer& frameBuffer, int startX, int startY, int endX, int endY, Color color) {
    rasterizeLineDda(startX, startY, endX, endY, makeSolidPlotter(frameBuffer, color));
}

void drawLineBresenham(FrameBuffer& frameBuffer, int startX, int startY, int endX, int endY, Color color) {
    rasterizeLineBresenham(startX, startY, endX, endY, makeSolidPlotter(frameBuffer, color));
}

void drawLineBresenham(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, Color color) {
    drawLineBresenham(frameBuffer, roundToInt(start.x), roundToInt(start.y), roundToInt(end.x), roundToInt(end.y),
                      color);
}

void drawPolylineBresenham(FrameBuffer& frameBuffer, const std::vector<Vec2>& points, Color color, bool closed) {
    if (points.size() < 2) return;
    for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        drawLineBresenham(frameBuffer, points[index], points[index + 1], color);
    }
    if (closed) drawLineBresenham(frameBuffer, points.back(), points.front(), color);
}
