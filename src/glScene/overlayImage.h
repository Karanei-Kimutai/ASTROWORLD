/**
 * @file overlayImage.h
 * @brief Turn anything the SOFTWARE renderer can draw into a transparent RGBA texture.
 *
 * Our FrameBuffer has no alpha channel, so the drawing is done TWICE: once on
 * a black background and once on a white one. Because every drawing operation
 * is a linear blend (dst = src*a + dst*(1-a)):
 *
 *   onBlack = a * C                   onWhite = a * C + (1 - a) * 255
 *   =>  a = 1 - (onWhite - onBlack) / 255,      C = onBlack / a
 *
 * This "difference matting" recovers the exact colour AND coverage, including
 * the anti-aliased edges of the logo - the CP1/CP2 renderer becomes a texture
 * generator for the OpenGL renderer.
 */
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "core/frameBuffer.h"

struct RgbaImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> pixels;  // RGBA, top row first
};

RgbaImage renderOverlayImage(int width, int height, const std::function<void(FrameBuffer&)>& draw);
