/**
 * @file boundaryFill.cpp
 * @brief Stack-based boundary fill implementation.
 */
#include "fill/boundaryFill.h"

#include <utility>
#include <vector>

namespace {

/** @brief (dx, dy) neighbour offsets. The first four are the 4-connected set. */
const int neighbourOffsets[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

int boundaryFillWithNeighbours(FrameBuffer& frameBuffer, int seedX, int seedY, Color fillColor,
                               Color boundaryColor, int neighbourCount) {
    if (fillColor == boundaryColor) return 0;  // would never terminate meaningfully

    int paintedCount = 0;
    std::vector<std::pair<int, int>> pending;
    pending.emplace_back(seedX, seedY);

    while (!pending.empty()) {
        const auto [x, y] = pending.back();
        pending.pop_back();

        if (!frameBuffer.contains(x, y)) continue;
        const Color current = frameBuffer.getPixel(x, y);
        // Stop at the boundary, and skip pixels we have already painted.
        if (current == boundaryColor || current == fillColor) continue;

        frameBuffer.setPixel(x, y, fillColor);
        ++paintedCount;
        for (int neighbour = 0; neighbour < neighbourCount; ++neighbour) {
            pending.emplace_back(x + neighbourOffsets[neighbour][0], y + neighbourOffsets[neighbour][1]);
        }
    }
    return paintedCount;
}

}  // namespace

int boundaryFill4(FrameBuffer& frameBuffer, int seedX, int seedY, Color fillColor, Color boundaryColor) {
    return boundaryFillWithNeighbours(frameBuffer, seedX, seedY, fillColor, boundaryColor, 4);
}

int boundaryFill8(FrameBuffer& frameBuffer, int seedX, int seedY, Color fillColor, Color boundaryColor) {
    return boundaryFillWithNeighbours(frameBuffer, seedX, seedY, fillColor, boundaryColor, 8);
}
