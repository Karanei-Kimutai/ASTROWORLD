/**
 * @file parkTextures.cpp
 * @brief Procedural park textures and CPU texture sampling.
 */
#include "park3d/parkTextures.h"

#include <cmath>

#include "core/simpleRandom.h"
#include "raster/circles.h"
#include "text/bitmapFont.h"

FrameBuffer drawGroundTexture() {
    const int size = 256;
    FrameBuffer canvas(size, size);
    SimpleRandom random(57u);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double speckle = random.nextRange(-0.06, 0.06);
            canvas.setPixel(x, y, scaleColor(astroPalette::dayGroundNear, 1.0 + speckle));
        }
    }
    const Color joint = makeShade(astroPalette::dayGroundNear, 0.3);
    for (int edge = 0; edge < 4; ++edge) {
        canvas.fillSpan(edge, 0, size - 1, joint);
        for (int y = 0; y < size; ++y) canvas.setPixel(edge, y, joint);
    }
    for (int kernel = 0; kernel < 90; ++kernel) {
        const int x = random.nextInt(8, size - 8);
        const int y = random.nextInt(8, size - 8);
        fillCircle(canvas, x, y, random.nextInt(2, 4), Color{255, 248, 220});
        canvas.setPixel(x - 1, y - 1, astroPalette::white);
    }
    return canvas;
}

FrameBuffer drawSignTexture() {
    FrameBuffer canvas(590, 115, Color{232, 236, 244});
    drawBitmapText(canvas, "ASTROWORLD", (590 - measureBitmapText("ASTROWORLD", 9)) / 2, (115 - 63) / 2, 9, Color{36, 40, 64});
    return canvas;
}

TextureSample sampleTextureBilinear(const FrameBuffer& texture, double u, double v) {
    const int width = texture.getWidth();
    const int height = texture.getHeight();
    // Texel centres sit at half-integers; wrap so the texture tiles.
    const double x = (u - std::floor(u)) * width - 0.5;
    const double y = (v - std::floor(v)) * height - 0.5;
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const double fx = x - x0;
    const double fy = y - y0;
    auto texel = [&](int tx, int ty) {
        const Color c = texture.getPixel(((tx % width) + width) % width, ((ty % height) + height) % height);
        return TextureSample{c.r / 255.0, c.g / 255.0, c.b / 255.0};
    };
    const TextureSample a = texel(x0, y0), b = texel(x0 + 1, y0), c = texel(x0, y0 + 1), d = texel(x0 + 1, y0 + 1);
    // Blend horizontally, then vertically.
    auto mix = [](double p, double q, double t) { return p + (q - p) * t; };
    return {mix(mix(a.r, b.r, fx), mix(c.r, d.r, fx), fy), mix(mix(a.g, b.g, fx), mix(c.g, d.g, fx), fy),
            mix(mix(a.b, b.b, fx), mix(c.b, d.b, fx), fy)};
}
