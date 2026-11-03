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
 * @brief The ASTROWORLD colour palette, sampled from the album artwork.
 *
 * Two moods share one scene:
 *   - DAY (album cover): saturated blue sky, white cumulus, sandy plaza, brass head
 *   - NIGHT (logo / alternate cover): deep navy starfield, teal lattice coaster, neon
 * The wordmark uses its own flat, high-saturation letter colours (a
 * near-complementary rainbow), listed under "wordmark".
 */
namespace astroPalette {
// Sky - day (album cover) and night (starfield), top -> horizon
inline constexpr Color daySkyTop{28, 88, 196};
inline constexpr Color daySkyMid{86, 148, 224};
inline constexpr Color daySkyHorizon{196, 218, 240};
inline constexpr Color nightSkyTop{3, 5, 18};
inline constexpr Color nightSkyMid{10, 14, 42};
inline constexpr Color nightSkyHorizon{44, 30, 82};

// Clouds
inline constexpr Color cloudLight{255, 255, 255};
inline constexpr Color cloudShadow{176, 194, 222};

// The brass inflatable head (matte gold)
inline constexpr Color brassLight{252, 222, 150};
inline constexpr Color brass{204, 156, 62};
inline constexpr Color brassDark{140, 98, 34};
inline constexpr Color brassShadow{72, 48, 18};

// Glow pouring out of the mouth
inline constexpr Color portalCore{255, 248, 210};
inline constexpr Color portalGlow{255, 196, 70};
inline constexpr Color portalEdge{240, 120, 30};

// Gold accents used on props (hub, bulbs, trims)
inline constexpr Color goldLight{255, 240, 170};
inline constexpr Color gold{255, 190, 40};
inline constexpr Color goldDark{196, 120, 18};

// Neon accents
inline constexpr Color neonPink{255, 56, 170};
inline constexpr Color neonCyan{56, 232, 255};
inline constexpr Color neonOrange{255, 140, 20};
inline constexpr Color neonPurple{170, 80, 255};

// Wordmark letter colours (flat, one per letter)
inline constexpr Color letterYellow{255, 232, 0};
inline constexpr Color letterRed{238, 34, 46};
inline constexpr Color letterWhite{250, 250, 250};
inline constexpr Color letterBlue{20, 146, 240};
inline constexpr Color letterMagenta{230, 20, 230};
inline constexpr Color letterGreen{40, 226, 140};
inline constexpr Color letterOrange{242, 152, 40};
inline constexpr Color letterRose{232, 30, 120};
inline constexpr Color letterGrey{172, 172, 180};
inline constexpr Color earthOcean{22, 92, 190};
inline constexpr Color earthLand{70, 140, 60};

// Rides and structures
inline constexpr Color coasterTeal{38, 188, 178};
inline constexpr Color coasterTealDark{16, 96, 98};
inline constexpr Color fenceBlack{22, 22, 28};
inline constexpr Color rocketSilver{214, 218, 226};
inline constexpr Color rocketRed{214, 38, 44};
inline constexpr Color steel{74, 56, 104};
inline constexpr Color steelLight{150, 128, 190};
inline constexpr Color steelDark{46, 32, 70};

// Ground - day sandy concrete, night dark plaza
inline constexpr Color dayGroundFar{186, 172, 150};
inline constexpr Color dayGroundNear{216, 200, 172};
inline constexpr Color nightGroundFar{34, 26, 44};
inline constexpr Color nightGroundNear{20, 14, 26};
inline constexpr Color daySkyline{120, 140, 170};
inline constexpr Color nightSkyline{24, 16, 40};

// Utility colours
inline constexpr Color ink{12, 4, 20};
inline constexpr Color crowd{16, 6, 24};
inline constexpr Color cream{255, 236, 210};
inline constexpr Color white{255, 255, 255};
inline constexpr Color black{0, 0, 0};
}  // namespace astroPalette
