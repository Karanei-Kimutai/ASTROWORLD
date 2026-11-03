/**
 * @file scanLineFill.cpp
 * @brief Edge Table / Active Edge Table scan-line fill implementation.
 */
#include "fill/scanLineFill.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

/** @brief One polygon edge as stored in the edge tables. */
struct EdgeRecord {
    int yEnd;              // first scan line the edge NO LONGER covers (exclusive)
    double x;              // x where the edge crosses the current scan line
    double inverseSlope;   // dx/dy: how far x moves per scan line
};

}  // namespace

void fillPolygonScanLine(const std::vector<Vec2>& vertices, const SpanCallback& emitSpan) {
    const std::size_t vertexCount = vertices.size();
    if (vertexCount < 3) return;

    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();
    for (const Vec2& vertex : vertices) {
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }

    // A scan line y covers the polygon if its pixel centre (y + 0.5) lies in [minY, maxY).
    const int firstScanLine = static_cast<int>(std::ceil(minY - 0.5));
    const int lastScanLineExclusive = static_cast<int>(std::ceil(maxY - 0.5));
    if (firstScanLine >= lastScanLineExclusive) return;

    // ---- 1. Build the Edge Table -------------------------------------------
    std::vector<std::vector<EdgeRecord>> edgeTable(static_cast<std::size_t>(lastScanLineExclusive - firstScanLine));
    for (std::size_t index = 0; index < vertexCount; ++index) {
        Vec2 top = vertices[index];
        Vec2 bottom = vertices[(index + 1) % vertexCount];
        if (top.y > bottom.y) std::swap(top, bottom);

        const int yStart = static_cast<int>(std::ceil(top.y - 0.5));
        const int yEnd = static_cast<int>(std::ceil(bottom.y - 0.5));
        if (yStart >= yEnd) continue;  // horizontal (or sub-pixel) edge: never crosses a pixel centre

        const double inverseSlope = (bottom.x - top.x) / (bottom.y - top.y);
        // x at the first pixel centre this edge crosses.
        const double xAtStart = top.x + ((yStart + 0.5) - top.y) * inverseSlope;
        edgeTable[static_cast<std::size_t>(yStart - firstScanLine)].push_back({yEnd, xAtStart, inverseSlope});
    }

    // ---- 2. Sweep scan lines with the Active Edge Table ---------------------
    std::vector<EdgeRecord> activeEdges;
    for (int y = firstScanLine; y < lastScanLineExclusive; ++y) {
        // a. Activate edges that begin on this scan line.
        const auto& newEdges = edgeTable[static_cast<std::size_t>(y - firstScanLine)];
        activeEdges.insert(activeEdges.end(), newEdges.begin(), newEdges.end());

        // b. Retire edges whose span has finished.
        activeEdges.erase(std::remove_if(activeEdges.begin(), activeEdges.end(),
                                         [y](const EdgeRecord& edge) { return edge.yEnd <= y; }),
                          activeEdges.end());

        // c. Sort crossings left to right, then fill between pairs (even-odd rule).
        std::sort(activeEdges.begin(), activeEdges.end(),
                  [](const EdgeRecord& a, const EdgeRecord& b) { return a.x < b.x; });
        for (std::size_t pair = 0; pair + 1 < activeEdges.size(); pair += 2) {
            // Pixel x is inside if its centre x + 0.5 lies in [leftX, rightX).
            const int xStart = static_cast<int>(std::ceil(activeEdges[pair].x - 0.5));
            const int xEnd = static_cast<int>(std::ceil(activeEdges[pair + 1].x - 0.5)) - 1;
            if (xStart <= xEnd) emitSpan(y, xStart, xEnd);
        }

        // d. Step every active edge to the next scan line (incremental update).
        for (EdgeRecord& edge : activeEdges) edge.x += edge.inverseSlope;
    }
}

void fillPolygon(FrameBuffer& frameBuffer, const std::vector<Vec2>& vertices, Color color) {
    fillPolygonScanLine(vertices, makeSolidSpanWriter(frameBuffer, color));
}

void fillPolygon(FrameBuffer& frameBuffer, const std::vector<Vec2>& vertices, const PixelShader& shader) {
    fillPolygonScanLine(vertices, makeShadedSpanWriter(frameBuffer, shader));
}
