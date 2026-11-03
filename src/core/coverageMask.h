/**
 * @file coverageMask.h
 * @brief An off-screen 1-bit coverage (stencil) buffer with supersampled compositing.
 *
 * Some shapes are easier to describe as "add these pieces, then cut these
 * holes out" - e.g. the letter D is a filled bowl with its counter removed.
 * Drawing directly into the scene cannot "cut" (the sky behind would be lost),
 * so shapes are first rasterized into a CoverageMask:
 *
 *   covered = true   -> pixel belongs to the shape   (like writing to a stencil buffer)
 *   covered = false  -> pixel is erased / a hole
 *
 * The mask is rendered at N times the final resolution and then composited:
 * each screen pixel takes the fraction of its N x N sub-samples that are
 * covered as its alpha. That is SUPERSAMPLING ANTI-ALIASING (SSAA) - edges
 * get smooth partial-coverage pixels instead of jagged steps.
 */
#pragma once

#include <cstdint>
#include <vector>

#include "raster/rasterTypes.h"

class CoverageMask {
public:
    CoverageMask(int widthValue, int heightValue);

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    /** @brief Mark (or clear) the horizontal run [xStart, xEnd] of row y (clipped). */
    void fillSpan(int y, int xStart, int xEnd, bool covered);

    bool isCovered(int x, int y) const;

    /** @brief A SpanCallback that adds (covered = true) or erases (false) coverage. */
    SpanCallback makeSpanWriter(bool covered);

private:
    int width;
    int height;
    std::vector<std::uint8_t> cells;
};

/**
 * @brief Blend a supersampled mask onto the frame buffer.
 *
 * Mask pixel (mx, my) lands on screen pixel (screenLeft + mx / factor, screenTop + my / factor).
 *
 * @param supersampleFactor N: the mask is N x N sub-samples per screen pixel.
 * @param colourShader      Colour of the shape at each screen pixel.
 * @param opacity           Extra alpha multiplier (used for soft drop shadows).
 */
void compositeCoverageMask(FrameBuffer& frameBuffer, const CoverageMask& mask, int supersampleFactor, int screenLeft,
                           int screenTop, const PixelShader& colourShader, double opacity = 1.0);
