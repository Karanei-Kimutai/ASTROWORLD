/**
 * @file frameBuffer.cpp
 * @brief Frame buffer storage, pixel access, colour reduction and PPM output.
 */
#include "core/frameBuffer.h"

#include <algorithm>
#include <fstream>
#include <utility>

// The PPM writer dumps the pixel array directly, so Color must be exactly 3 bytes.
static_assert(sizeof(Color) == 3, "Color must be tightly packed RGB");

FrameBuffer::FrameBuffer(int widthValue, int heightValue, Color clearColor)
    : width(std::max(1, widthValue)),
      height(std::max(1, heightValue)),
      pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), clearColor) {}

bool FrameBuffer::contains(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }

void FrameBuffer::setPixel(int x, int y, Color color) {
    if (!contains(x, y)) return;
    pixels[static_cast<std::size_t>(y) * width + x] = color;
}

Color FrameBuffer::getPixel(int x, int y) const {
    if (!contains(x, y)) return Color{};
    return pixels[static_cast<std::size_t>(y) * width + x];
}

void FrameBuffer::blendPixel(int x, int y, Color color, double alpha) {
    if (!contains(x, y) || alpha <= 0.0) return;
    Color& destination = pixels[static_cast<std::size_t>(y) * width + x];
    destination = lerpColor(destination, color, std::min(alpha, 1.0));
}

void FrameBuffer::clear(Color color) { std::fill(pixels.begin(), pixels.end(), color); }

void FrameBuffer::fillSpan(int y, int xStart, int xEnd, Color color) {
    if (y < 0 || y >= height) return;
    if (xStart > xEnd) std::swap(xStart, xEnd);
    // Clip the run to the visible columns once, instead of testing every pixel.
    xStart = std::max(xStart, 0);
    xEnd = std::min(xEnd, width - 1);
    for (int x = xStart; x <= xEnd; ++x) pixels[static_cast<std::size_t>(y) * width + x] = color;
}

void FrameBuffer::fillSpan(int y, int xStart, int xEnd, const PixelShader& shader) {
    if (y < 0 || y >= height) return;
    if (xStart > xEnd) std::swap(xStart, xEnd);
    xStart = std::max(xStart, 0);
    xEnd = std::min(xEnd, width - 1);
    for (int x = xStart; x <= xEnd; ++x) pixels[static_cast<std::size_t>(y) * width + x] = shader(x, y);
}

void FrameBuffer::blendSpan(int y, int xStart, int xEnd, Color color, double alpha) {
    if (y < 0 || y >= height) return;
    if (xStart > xEnd) std::swap(xStart, xEnd);
    xStart = std::max(xStart, 0);
    xEnd = std::min(xEnd, width - 1);
    for (int x = xStart; x <= xEnd; ++x) blendPixel(x, y, color, alpha);
}

FrameBuffer FrameBuffer::reducedTo8Bit(bool useDithering) const {
    // Classic 4x4 Bayer matrix: thresholds 0..15 spread so neighbouring
    // pixels round in different directions, hiding the coarse colour steps.
    static const int bayerMatrix[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    // Distance between neighbouring quantisation levels per channel.
    const double redGreenStep = 255.0 / 7.0;
    const double blueStep = 255.0 / 3.0;

    FrameBuffer result(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color source = getPixel(x, y);
            if (useDithering) {
                // Threshold in [-0.5, 0.5): nudge the colour up or down by up to half a level.
                const double threshold = (bayerMatrix[y & 3][x & 3] + 0.5) / 16.0 - 0.5;
                source = makeColor(static_cast<int>(source.r + threshold * redGreenStep),
                                   static_cast<int>(source.g + threshold * redGreenStep),
                                   static_cast<int>(source.b + threshold * blueStep));
            }
            result.setPixel(x, y, expandFrom8Bit(reduceTo8Bit(source)));
        }
    }
    return result;
}

bool FrameBuffer::savePpm(const std::string& filePath) const {
    std::ofstream file(filePath, std::ios::binary);
    if (!file) return false;
    // Header: magic number, dimensions, maximum channel value. Then raw RGB triples.
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(pixels.data()),
               static_cast<std::streamsize>(pixels.size() * sizeof(Color)));
    return static_cast<bool>(file);
}
