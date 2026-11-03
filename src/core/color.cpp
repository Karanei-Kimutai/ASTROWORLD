/**
 * @file color.cpp
 * @brief Implementation of colour-model conversions and colour mixing helpers.
 */
#include "core/color.h"

#include <algorithm>
#include <cmath>

namespace {

/** @brief Round a real channel value and clamp it into a byte [0, 255]. */
std::uint8_t clampChannel(double value) {
    return static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
}

}  // namespace

Color makeColor(int red, int green, int blue) {
    return Color{static_cast<std::uint8_t>(std::clamp(red, 0, 255)),
                 static_cast<std::uint8_t>(std::clamp(green, 0, 255)),
                 static_cast<std::uint8_t>(std::clamp(blue, 0, 255))};
}

Color hsvToRgb(double hue, double saturation, double value) {
    // Wrap hue into [0, 360) and clamp the other two components.
    double wrappedHue = std::fmod(hue, 360.0);
    if (wrappedHue < 0.0) wrappedHue += 360.0;
    saturation = std::clamp(saturation, 0.0, 1.0);
    value = std::clamp(value, 0.0, 1.0);

    // chroma = the "colourfulness" range between the largest and smallest channel.
    const double chroma = value * saturation;
    // secondary = the middle channel, which ramps up/down inside each 60 degree sector.
    const double secondary = chroma * (1.0 - std::fabs(std::fmod(wrappedHue / 60.0, 2.0) - 1.0));
    // offset lifts all channels so the largest one equals 'value'.
    const double offset = value - chroma;

    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    const int sector = static_cast<int>(wrappedHue / 60.0);
    switch (sector) {
        case 0: red = chroma;    green = secondary; blue = 0.0;       break;  // red -> yellow
        case 1: red = secondary; green = chroma;    blue = 0.0;       break;  // yellow -> green
        case 2: red = 0.0;       green = chroma;    blue = secondary; break;  // green -> cyan
        case 3: red = 0.0;       green = secondary; blue = chroma;    break;  // cyan -> blue
        case 4: red = secondary; green = 0.0;       blue = chroma;    break;  // blue -> magenta
        default: red = chroma;   green = 0.0;       blue = secondary; break;  // magenta -> red
    }
    return Color{clampChannel((red + offset) * 255.0), clampChannel((green + offset) * 255.0),
                 clampChannel((blue + offset) * 255.0)};
}

HsvColor rgbToHsv(Color color) {
    const double red = color.r / 255.0;
    const double green = color.g / 255.0;
    const double blue = color.b / 255.0;
    const double maxChannel = std::max({red, green, blue});
    const double minChannel = std::min({red, green, blue});
    const double delta = maxChannel - minChannel;

    HsvColor result;
    // Hue depends on which channel is dominant; grey (delta = 0) has no hue.
    if (delta > 1e-9) {
        if (maxChannel == red) {
            result.hue = 60.0 * std::fmod((green - blue) / delta, 6.0);
        } else if (maxChannel == green) {
            result.hue = 60.0 * ((blue - red) / delta + 2.0);
        } else {
            result.hue = 60.0 * ((red - green) / delta + 4.0);
        }
    }
    if (result.hue < 0.0) result.hue += 360.0;
    result.saturation = maxChannel <= 0.0 ? 0.0 : delta / maxChannel;
    result.value = maxChannel;
    return result;
}

CmykColor rgbToCmyk(Color color) {
    const double red = color.r / 255.0;
    const double green = color.g / 255.0;
    const double blue = color.b / 255.0;

    CmykColor result;
    // Key (black ink) covers the darkness common to all three channels.
    result.key = 1.0 - std::max({red, green, blue});
    if (result.key >= 1.0) return result;  // pure black: only K ink

    // Each coloured ink absorbs its complementary light (C absorbs red, etc.).
    result.cyan = (1.0 - red - result.key) / (1.0 - result.key);
    result.magenta = (1.0 - green - result.key) / (1.0 - result.key);
    result.yellow = (1.0 - blue - result.key) / (1.0 - result.key);
    return result;
}

Color cmykToRgb(const CmykColor& cmyk) {
    return Color{clampChannel(255.0 * (1.0 - cmyk.cyan) * (1.0 - cmyk.key)),
                 clampChannel(255.0 * (1.0 - cmyk.magenta) * (1.0 - cmyk.key)),
                 clampChannel(255.0 * (1.0 - cmyk.yellow) * (1.0 - cmyk.key))};
}

Color lerpColor(Color a, Color b, double t) {
    return Color{clampChannel(a.r + (b.r - a.r) * t), clampChannel(a.g + (b.g - a.g) * t),
                 clampChannel(a.b + (b.b - a.b) * t)};
}

Color lerpColorHsv(Color a, Color b, double t) {
    HsvColor hsvA = rgbToHsv(a);
    HsvColor hsvB = rgbToHsv(b);

    // A grey has an undefined hue - borrow the other colour's hue so the
    // gradient does not swing through unrelated colours.
    if (hsvA.saturation < 1e-3) hsvA.hue = hsvB.hue;
    if (hsvB.saturation < 1e-3) hsvB.hue = hsvA.hue;

    // Take the shortest way round the colour wheel (at most 180 degrees).
    double hueDelta = hsvB.hue - hsvA.hue;
    if (hueDelta > 180.0) hueDelta -= 360.0;
    if (hueDelta < -180.0) hueDelta += 360.0;

    return hsvToRgb(hsvA.hue + hueDelta * t, hsvA.saturation + (hsvB.saturation - hsvA.saturation) * t,
                    hsvA.value + (hsvB.value - hsvA.value) * t);
}

Color makeTint(Color base, double amount) { return lerpColor(base, Color{255, 255, 255}, amount); }

Color makeShade(Color base, double amount) { return lerpColor(base, Color{0, 0, 0}, amount); }

Color makeTone(Color base, double amount) { return lerpColor(base, Color{128, 128, 128}, amount); }

Color scaleColor(Color color, double factor) {
    return Color{clampChannel(color.r * factor), clampChannel(color.g * factor), clampChannel(color.b * factor)};
}

Color addColors(Color a, Color b) { return makeColor(a.r + b.r, a.g + b.g, a.b + b.b); }

std::uint8_t reduceTo8Bit(Color color) {
    // Quantise each channel to its reduced number of levels (rounding, not truncating).
    const int red3 = (color.r * 7 + 127) / 255;    // 0..7  (3 bits)
    const int green3 = (color.g * 7 + 127) / 255;  // 0..7  (3 bits)
    const int blue2 = (color.b * 3 + 127) / 255;   // 0..3  (2 bits)
    return static_cast<std::uint8_t>((red3 << 5) | (green3 << 2) | blue2);
}

Color expandFrom8Bit(std::uint8_t packed) {
    const int red3 = (packed >> 5) & 0x7;
    const int green3 = (packed >> 2) & 0x7;
    const int blue2 = packed & 0x3;
    // Scale each level back to the full 0..255 range.
    return Color{static_cast<std::uint8_t>(red3 * 255 / 7), static_cast<std::uint8_t>(green3 * 255 / 7),
                 static_cast<std::uint8_t>(blue2 * 255 / 3)};
}
