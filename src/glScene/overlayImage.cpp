/**
 * @file overlayImage.cpp
 * @brief Difference matting of software-rendered images into RGBA.
 */
#include "glScene/overlayImage.h"

#include <algorithm>

RgbaImage renderOverlayImage(int width, int height, const std::function<void(FrameBuffer&)>& draw) {
    FrameBuffer onBlack(width, height, astroPalette::black);
    FrameBuffer onWhite(width, height, astroPalette::white);
    draw(onBlack);
    draw(onWhite);

    RgbaImage image;
    image.width = width;
    image.height = height;
    image.pixels.resize(static_cast<std::size_t>(width) * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Color black = onBlack.getPixel(x, y);
            const Color white = onWhite.getPixel(x, y);
            // Average the three channels' estimate of (1 - a) for robustness against rounding.
            const double transparency = ((white.r - black.r) + (white.g - black.g) + (white.b - black.b)) / (3.0 * 255.0);
            const double alpha = std::clamp(1.0 - transparency, 0.0, 1.0);
            std::uint8_t* pixel = &image.pixels[(static_cast<std::size_t>(y) * width + x) * 4];
            // Un-premultiply: the colour on black is a * C.
            pixel[0] = alpha > 0.0 ? clampChannel(black.r / alpha) : 0;
            pixel[1] = alpha > 0.0 ? clampChannel(black.g / alpha) : 0;
            pixel[2] = alpha > 0.0 ? clampChannel(black.b / alpha) : 0;
            pixel[3] = clampChannel(alpha * 255.0);
        }
    }
    return image;
}

RgbaImage toOpaqueRgba(const FrameBuffer& image) {
    RgbaImage result;
    result.width = image.getWidth();
    result.height = image.getHeight();
    result.pixels.reserve(static_cast<std::size_t>(result.width) * result.height * 4);
    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            const Color color = image.getPixel(x, y);
            result.pixels.insert(result.pixels.end(), {color.r, color.g, color.b, 255});
        }
    }
    return result;
}
