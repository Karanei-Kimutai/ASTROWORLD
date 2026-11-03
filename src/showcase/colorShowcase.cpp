/**
 * @file colorShowcase.cpp
 * @brief Colour theory demonstration panels.
 */
#include "showcase/colorShowcase.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "core/vec2.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "text/bitmapFont.h"

namespace {

const Color backgroundColor{18, 10, 36};
const Color labelColor = astroPalette::goldLight;

/** @brief Fill an axis-aligned rectangle (via scan-line fill) with a solid colour. */
void fillRectangle(FrameBuffer& frameBuffer, int left, int top, int width, int height, Color color) {
    fillPolygon(frameBuffer, {{double(left), double(top)}, {double(left + width), double(top)},
                              {double(left + width), double(top + height)}, {double(left), double(top + height)}},
                color);
}

void drawLabel(FrameBuffer& frameBuffer, const std::string& text, int centerX, int top) {
    drawBitmapText(frameBuffer, text, centerX - measureBitmapText(text, 2) / 2, top, 2, labelColor);
}

/** @brief Hue = angle, saturation = distance from centre, value = 1. */
void drawHsvWheel(FrameBuffer& frameBuffer, int centerX, int centerY, int radius) {
    fillCircle(frameBuffer, centerX, centerY, radius, [centerX, centerY, radius](int x, int y) {
        const double dx = x - centerX;
        const double dy = y - centerY;
        double hue = std::atan2(-dy, dx) * 180.0 / pi;  // -dy so hue increases anticlockwise on screen
        if (hue < 0.0) hue += 360.0;
        return hsvToRgb(hue, std::min(1.0, std::sqrt(dx * dx + dy * dy) / radius), 1.0);
    });
}

/** @brief Three rows of swatches: tints (+white), shades (+black), tones (+grey). */
void drawTintShadeTone(FrameBuffer& frameBuffer, int left, int top, Color base) {
    const int swatchSize = 40;
    const char* rowNames[3] = {"TINT", "SHADE", "TONE"};
    for (int row = 0; row < 3; ++row) {
        drawBitmapText(frameBuffer, rowNames[row], left, top + row * 60 + 14, 2, labelColor);
        for (int step = 0; step < 5; ++step) {
            const double amount = step / 5.0;
            const Color swatch = row == 0 ? makeTint(base, amount) : row == 1 ? makeShade(base, amount) : makeTone(base, amount);
            fillRectangle(frameBuffer, left + 70 + step * (swatchSize + 4), top + row * 60, swatchSize, swatchSize, swatch);
        }
    }
}

/** @brief Additive mixing: overlapping red, green and blue light circles on black. */
void drawAdditiveCircles(FrameBuffer& frameBuffer, int centerX, int centerY) {
    const Vec2 centers[3] = {{centerX - 0.0, centerY - 40.0}, {centerX - 38.0, centerY + 25.0}, {centerX + 38.0, centerY + 25.0}};
    const double radius = 62.0;
    fillCircle(frameBuffer, centerX, centerY, 125, astroPalette::black);
    fillCircle(frameBuffer, centerX, centerY + 5, 110, [&centers, radius](int x, int y) {
        Color result{0, 0, 0};
        const Color lights[3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
        for (int light = 0; light < 3; ++light) {
            if ((Vec2(x, y) - centers[light]).length() <= radius) result = addColors(result, lights[light]);
        }
        return result;
    });
}

/** @brief Subtractive mixing: overlapping cyan, magenta and yellow inks on white paper. */
void drawSubtractiveCircles(FrameBuffer& frameBuffer, int centerX, int centerY) {
    const Vec2 centers[3] = {{centerX - 0.0, centerY - 40.0}, {centerX - 38.0, centerY + 25.0}, {centerX + 38.0, centerY + 25.0}};
    const double radius = 62.0;
    fillCircle(frameBuffer, centerX, centerY, 125, astroPalette::white);
    fillCircle(frameBuffer, centerX, centerY + 5, 110, [&centers, radius](int x, int y) {
        CmykColor ink;
        if ((Vec2(x, y) - centers[0]).length() <= radius) ink.cyan = 1.0;
        if ((Vec2(x, y) - centers[1]).length() <= radius) ink.magenta = 1.0;
        if ((Vec2(x, y) - centers[2]).length() <= radius) ink.yellow = 1.0;
        return cmykToRgb(ink);
    });
}

/** @brief Hue sweep in 24-bit, then reduced to 8-bit (3:3:2) plain and dithered. */
void drawBitDepthComparison(FrameBuffer& frameBuffer, int left, int top, int width) {
    const int stripHeight = 60;
    const char* labels[3] = {"24-BIT", "8-BIT", "8-BIT DITHER"};
    // Draw the 24-bit sweep, then copy it through reducedTo8Bit for the lower strips.
    FrameBuffer sweep(width, stripHeight);
    for (int y = 0; y < stripHeight; ++y) {
        sweep.fillSpan(y, 0, width - 1, [width, y, stripHeight](int x, int) {
            return hsvToRgb(x * 360.0 / width, 0.85, 0.35 + 0.65 * (1.0 - static_cast<double>(y) / stripHeight));
        });
    }
    const FrameBuffer strips[3] = {sweep, sweep.reducedTo8Bit(false), sweep.reducedTo8Bit(true)};
    for (int strip = 0; strip < 3; ++strip) {
        const int stripTop = top + strip * (stripHeight + 34);
        drawBitmapText(frameBuffer, labels[strip], left, stripTop, 2, labelColor);
        for (int y = 0; y < stripHeight; ++y) {
            for (int x = 0; x < width; ++x) frameBuffer.setPixel(left + x, stripTop + 18 + y, strips[strip].getPixel(x, y));
        }
    }
}

/** @brief The scene palette with each colour's hex code and CMYK percentages. */
void drawPalette(FrameBuffer& frameBuffer, int left, int top) {
    struct NamedColor {
        const char* name;
        Color color;
    };
    const NamedColor entries[] = {
        {"DAY SKY", astroPalette::daySkyTop},     {"NIGHT SKY", astroPalette::nightSkyMid},
        {"BRASS", astroPalette::brass},           {"BRASS LIGHT", astroPalette::brassLight},
        {"PORTAL GLOW", astroPalette::portalGlow}, {"COASTER TEAL", astroPalette::coasterTeal},
        {"LETTER YELLOW", astroPalette::letterYellow}, {"LETTER MAGENTA", astroPalette::letterMagenta},
        {"LETTER BLUE", astroPalette::letterBlue}, {"LETTER GREEN", astroPalette::letterGreen},
    };
    int index = 0;
    for (const NamedColor& entry : entries) {
        const int column = index / 5;
        const int row = index % 5;
        const int swatchLeft = left + column * 310;
        const int swatchTop = top + row * 52;
        fillRectangle(frameBuffer, swatchLeft, swatchTop, 40, 40, entry.color);
        const CmykColor cmyk = rgbToCmyk(entry.color);
        char details[64];
        std::snprintf(details, sizeof(details), "#%02X%02X%02X C%d M%d Y%d K%d", entry.color.r, entry.color.g, entry.color.b,
                      static_cast<int>(cmyk.cyan * 100), static_cast<int>(cmyk.magenta * 100),
                      static_cast<int>(cmyk.yellow * 100), static_cast<int>(cmyk.key * 100));
        drawBitmapText(frameBuffer, entry.name, swatchLeft + 50, swatchTop + 4, 2, labelColor);
        drawBitmapText(frameBuffer, details, swatchLeft + 50, swatchTop + 24, 1, astroPalette::cream);
        ++index;
    }
}

}  // namespace

void renderColorShowcase(FrameBuffer& frameBuffer) {
    frameBuffer.clear(backgroundColor);

    drawLabel(frameBuffer, "HSV HUE WHEEL", 160, 12);
    drawHsvWheel(frameBuffer, 160, 190, 130);

    drawLabel(frameBuffer, "TINTS SHADES TONES", 480, 12);
    drawTintShadeTone(frameBuffer, 345, 70, astroPalette::brass);

    drawLabel(frameBuffer, "RGB ADDITIVE", 800, 12);
    drawAdditiveCircles(frameBuffer, 800, 190);

    drawLabel(frameBuffer, "CMYK SUBTRACTIVE", 1120, 12);
    drawSubtractiveCircles(frameBuffer, 1120, 190);

    drawLabel(frameBuffer, "COLOUR DEPTH REDUCTION", 320, 352);
    drawBitDepthComparison(frameBuffer, 20, 385, 600);

    drawLabel(frameBuffer, "ASTROWORLD PALETTE", 960, 352);
    drawPalette(frameBuffer, 660, 390);
}
