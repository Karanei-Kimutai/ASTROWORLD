/**
 * @file color.h
 * @brief Colour representation, colour-model conversions and the ASTROWORLD palette.
 *
 * Course concepts covered here:
 *   - RGB (additive, used by the display / frame buffer)
 *   - HSV (hue, saturation, value) - used to *design* colours and gradients
 *   - CMYK (subtractive, print) - conversion shown in the colour showcase
 *   - Tints, shades and tones of a base hue
 *   - Colour-space reduction: 24-bit RGB (8:8:8) -> 8-bit RGB (3:3:2)
 */
#pragma once

#include <cstdint>

/**
 * @brief One 24-bit RGB pixel: 8 bits per channel, 16.7 million possible colours.
 *
 * Exactly 3 bytes (no padding) so a frame buffer row can be written to a
 * PPM file in one block.
 */
struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;

    bool operator==(const Color& other) const { return r == other.r && g == other.g && b == other.b; }
    bool operator!=(const Color& other) const { return !(*this == other); }
};

/** @brief Hue in degrees [0, 360), saturation and value in [0, 1]. */
struct HsvColor {
    double hue = 0.0;
    double saturation = 0.0;
    double value = 0.0;
};

/** @brief Cyan, magenta, yellow and key (black) ink coverage, each in [0, 1]. */
struct CmykColor {
    double cyan = 0.0;
    double magenta = 0.0;
    double yellow = 0.0;
    double key = 0.0;
};

/** @brief Build a colour from int channels, clamping each into [0, 255]. */
Color makeColor(int red, int green, int blue);

/** @brief HSV -> RGB using the six 60-degree hue sectors of the colour hexcone. */
Color hsvToRgb(double hue, double saturation, double value);

/** @brief RGB -> HSV (inverse of hsvToRgb). */
HsvColor rgbToHsv(Color color);

/** @brief RGB (additive light) -> CMYK (subtractive ink). */
CmykColor rgbToCmyk(Color color);

/** @brief CMYK -> RGB. */
Color cmykToRgb(const CmykColor& cmyk);

/** @brief Straight per-channel RGB interpolation a + (b - a) * t. */
Color lerpColor(Color a, Color b, double t);

/**
 * @brief Interpolate in HSV space, travelling the SHORT way round the hue circle.
 *
 * Gives much more vivid gradients than RGB lerp (e.g. purple -> orange passes
 * through magenta and red instead of a muddy grey-brown) - ideal for the
 * ASTROWORLD sunset sky.
 */
Color lerpColorHsv(Color a, Color b, double t);

/** @brief Tint = base mixed with WHITE (amount 0 = base, 1 = white). */
Color makeTint(Color base, double amount);

/** @brief Shade = base mixed with BLACK (amount 0 = base, 1 = black). */
Color makeShade(Color base, double amount);

/** @brief Tone = base mixed with mid GREY (amount 0 = base, 1 = grey). */
Color makeTone(Color base, double amount);

/** @brief Multiply every channel by a factor (brightness), clamped. */
Color scaleColor(Color color, double factor);

/** @brief Saturating additive mix (light + light), as on an RGB display. */
Color addColors(Color a, Color b);

/**
 * @brief Pack a 24-bit colour into one byte as RRRGGGBB (3:3:2).
 *
 * Red and green keep 8 levels each, blue keeps only 4 because the human eye
 * is least sensitive to blue. 256 colours total.
 */
std::uint8_t reduceTo8Bit(Color color);

/** @brief Unpack an RRRGGGBB byte back to a displayable 24-bit colour. */
Color expandFrom8Bit(std::uint8_t packed);

/**
 * @brief The ASTROWORLD colour palette.
 *
 * Built around a split-complementary harmony: deep violet/indigo night sky
 * against warm gold and orange, with hot pink and cyan neon accents.
 */
namespace astroPalette {
// Sky (top -> horizon sunset)
inline constexpr Color skyTop{14, 6, 46};
inline constexpr Color skyMid{92, 28, 128};
inline constexpr Color skyHorizon{255, 118, 72};

// The golden inflatable head and the wordmark
inline constexpr Color goldLight{255, 240, 170};
inline constexpr Color gold{255, 190, 40};
inline constexpr Color goldDark{196, 120, 18};
inline constexpr Color goldShadow{110, 52, 30};

// Neon accents
inline constexpr Color neonPink{255, 56, 170};
inline constexpr Color neonCyan{56, 232, 255};
inline constexpr Color neonOrange{255, 140, 20};
inline constexpr Color neonPurple{170, 80, 255};

// Park structures and ground
inline constexpr Color silhouette{40, 16, 64};
inline constexpr Color groundFar{48, 22, 66};
inline constexpr Color groundNear{20, 9, 30};
inline constexpr Color pathColor{120, 62, 92};
inline constexpr Color steel{74, 56, 104};
inline constexpr Color steelLight{150, 128, 190};
inline constexpr Color steelDark{46, 32, 70};

// Utility colours
inline constexpr Color ink{12, 4, 20};
inline constexpr Color crowd{16, 6, 24};
inline constexpr Color mouthDark{30, 6, 28};
inline constexpr Color cream{255, 236, 210};
inline constexpr Color white{255, 255, 255};
inline constexpr Color black{0, 0, 0};
}  // namespace astroPalette
