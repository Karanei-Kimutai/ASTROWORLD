/**
 * @file rasterTypes.h
 * @brief Callback types that separate RASTERIZATION from FRAGMENT PROCESSING.
 *
 * Every primitive algorithm (DDA, Bresenham, midpoint circle/ellipse,
 * scan-line fill) only works out which pixels a shape covers. It reports them
 * through one of two callbacks:
 *
 *   - PlotCallback(x, y)              one pixel at a time (lines, outlines)
 *   - SpanCallback(y, xStart, xEnd)   a horizontal run of pixels (filled shapes)
 *
 * The caller decides what to do with them: solid colour, gradient shader,
 * alpha blend, or even "discard if outside some region". This mirrors stages
 * 5 and 6 of the rendering pipeline (rasterization -> fragment processing).
 */
#pragma once

#include <functional>

#include "core/frameBuffer.h"

using PlotCallback = std::function<void(int x, int y)>;
using SpanCallback = std::function<void(int y, int xStart, int xEnd)>;

/** @brief Plot every covered pixel in one solid colour. */
inline PlotCallback makeSolidPlotter(FrameBuffer& frameBuffer, Color color) {
    return [&frameBuffer, color](int x, int y) { frameBuffer.setPixel(x, y, color); };
}

/** @brief Alpha-blend a colour onto every covered pixel. */
inline PlotCallback makeBlendPlotter(FrameBuffer& frameBuffer, Color color, double alpha) {
    return [&frameBuffer, color, alpha](int x, int y) { frameBuffer.blendPixel(x, y, color, alpha); };
}

/** @brief Fill every covered span in one solid colour. */
inline SpanCallback makeSolidSpanWriter(FrameBuffer& frameBuffer, Color color) {
    return [&frameBuffer, color](int y, int xStart, int xEnd) { frameBuffer.fillSpan(y, xStart, xEnd, color); };
}

/** @brief Fill every covered span using a per-pixel shader. */
inline SpanCallback makeShadedSpanWriter(FrameBuffer& frameBuffer, PixelShader shader) {
    return [&frameBuffer, shader](int y, int xStart, int xEnd) { frameBuffer.fillSpan(y, xStart, xEnd, shader); };
}

/** @brief Alpha-blend a colour across every covered span. */
inline SpanCallback makeBlendSpanWriter(FrameBuffer& frameBuffer, Color color, double alpha) {
    return [&frameBuffer, color, alpha](int y, int xStart, int xEnd) {
        frameBuffer.blendSpan(y, xStart, xEnd, color, alpha);
    };
}
