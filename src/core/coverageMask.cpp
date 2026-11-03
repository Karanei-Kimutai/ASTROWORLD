/**
 * @file coverageMask.cpp
 * @brief Coverage mask storage and supersampled compositing.
 */
#include "core/coverageMask.h"

#include <algorithm>
#include <utility>

CoverageMask::CoverageMask(int widthValue, int heightValue)
    : width(std::max(1, widthValue)),
      height(std::max(1, heightValue)),
      cells(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0) {}

void CoverageMask::fillSpan(int y, int xStart, int xEnd, bool covered) {
    if (y < 0 || y >= height) return;
    if (xStart > xEnd) std::swap(xStart, xEnd);
    xStart = std::max(xStart, 0);
    xEnd = std::min(xEnd, width - 1);
    for (int x = xStart; x <= xEnd; ++x) cells[static_cast<std::size_t>(y) * width + x] = covered ? 1 : 0;
}

bool CoverageMask::isCovered(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) return false;
    return cells[static_cast<std::size_t>(y) * width + x] != 0;
}

SpanCallback CoverageMask::makeSpanWriter(bool covered) {
    return [this, covered](int y, int xStart, int xEnd) { fillSpan(y, xStart, xEnd, covered); };
}

void compositeCoverageMask(FrameBuffer& frameBuffer, const CoverageMask& mask, int supersampleFactor, int screenLeft,
                           int screenTop, const PixelShader& colourShader, double opacity) {
    const int factor = std::max(1, supersampleFactor);
    const int screenWidth = mask.getWidth() / factor;
    const int screenHeight = mask.getHeight() / factor;
    const double samplesPerPixel = static_cast<double>(factor * factor);

    for (int pixelY = 0; pixelY < screenHeight; ++pixelY) {
        for (int pixelX = 0; pixelX < screenWidth; ++pixelX) {
            // Box filter: count the covered sub-samples inside this screen pixel.
            int coveredCount = 0;
            for (int subY = 0; subY < factor; ++subY) {
                for (int subX = 0; subX < factor; ++subX) {
                    if (mask.isCovered(pixelX * factor + subX, pixelY * factor + subY)) ++coveredCount;
                }
            }
            if (coveredCount == 0) continue;
            const int screenX = screenLeft + pixelX;
            const int screenY = screenTop + pixelY;
            frameBuffer.blendPixel(screenX, screenY, colourShader(screenX, screenY),
                                   opacity * coveredCount / samplesPerPixel);
        }
    }
}
