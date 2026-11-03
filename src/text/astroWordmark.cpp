/**
 * @file astroWordmark.cpp
 * @brief Glyph construction and layered rendering of the ASTROWORLD wordmark.
 */
#include "text/astroWordmark.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

#include "curves/bezier.h"
#include "curves/spline.h"
#include "raster/strokes.h"

namespace {

using Stroke = std::vector<Vec2>;

/** @brief A letter: how far to advance the cursor, and its centre-line strokes in unit space. */
struct AstroGlyph {
    double advance = 0.0;
    std::vector<Stroke> strokes;
};

/** @brief Append the points of 'tail' to 'head'. */
void appendStroke(Stroke& head, const Stroke& tail) { head.insert(head.end(), tail.begin(), tail.end()); }

/**
 * @brief Build the strokes of one letter.
 *
 * Unit space: x from 0 to 'advance', y from 0 (cap line) to 1 (baseline).
 */
AstroGlyph buildAstroGlyph(char letter) {
    AstroGlyph glyph;
    switch (std::toupper(static_cast<unsigned char>(letter))) {
        case 'A':
            glyph.advance = 0.90;
            glyph.strokes = {{{0.0, 1.0}, {0.45, 0.0}, {0.90, 1.0}}, {{0.20, 0.64}, {0.70, 0.64}}};
            break;
        case 'S':
            // A Catmull-Rom spline passes through every point, so we can "trace" the S directly.
            glyph.advance = 0.72;
            glyph.strokes = {sampleCatmullRomSpline({{0.68, 0.14}, {0.38, 0.0}, {0.08, 0.14}, {0.14, 0.38},
                                                     {0.38, 0.50}, {0.62, 0.63}, {0.64, 0.87}, {0.36, 1.0},
                                                     {0.04, 0.86}},
                                                    10, false)};
            break;
        case 'T':
            glyph.advance = 0.80;
            glyph.strokes = {{{0.0, 0.0}, {0.80, 0.0}}, {{0.40, 0.0}, {0.40, 1.0}}};
            break;
        case 'R': {
            // Stem + bowl (cubic Bezier) + leg.
            glyph.advance = 0.78;
            Stroke bowl = {{0.0, 1.0}, {0.0, 0.0}, {0.36, 0.0}};
            appendStroke(bowl, sampleCubicBezier({0.36, 0.0}, {0.80, 0.0}, {0.80, 0.52}, {0.36, 0.52}, 12));
            bowl.push_back({0.0, 0.52});
            glyph.strokes = {bowl, {{0.34, 0.52}, {0.78, 1.0}}};
            break;
        }
        case 'O': {
            // Parametric ellipse x = cx + rx cos(a), y = cy + ry sin(a).
            glyph.advance = 0.90;
            Stroke ring;
            for (int step = 0; step <= 48; ++step) {
                const double angle = 2.0 * pi * step / 48.0;
                ring.push_back({0.45 + 0.45 * std::cos(angle), 0.5 + 0.5 * std::sin(angle)});
            }
            glyph.strokes = {ring};
            break;
        }
        case 'W':
            glyph.advance = 1.15;
            glyph.strokes = {{{0.0, 0.0}, {0.25, 1.0}, {0.575, 0.30}, {0.90, 1.0}, {1.15, 0.0}}};
            break;
        case 'L':
            glyph.advance = 0.68;
            glyph.strokes = {{{0.0, 0.0}, {0.0, 1.0}, {0.68, 1.0}}};
            break;
        case 'D': {
            glyph.advance = 0.80;
            Stroke bowl = {{0.0, 0.0}, {0.28, 0.0}};
            appendStroke(bowl, sampleCubicBezier({0.28, 0.0}, {0.84, 0.0}, {0.84, 1.0}, {0.28, 1.0}, 16));
            bowl.push_back({0.0, 1.0});
            glyph.strokes = {{{0.0, 0.0}, {0.0, 1.0}}, bowl};
            break;
        }
        case ' ':
            glyph.advance = 0.5;
            break;
        default:
            glyph.advance = 0.6;  // unsupported letters leave a gap
            break;
    }
    return glyph;
}

/** @brief Draw every stroke with the same radius, offset and shader. */
void strokeAll(FrameBuffer& frameBuffer, const std::vector<Stroke>& strokes, const Vec2& offset, double radius,
               const PixelShader& shader) {
    for (const Stroke& stroke : strokes) {
        Stroke shifted;
        shifted.reserve(stroke.size());
        for (const Vec2& point : stroke) shifted.push_back(point + offset);
        drawThickPolyline(frameBuffer, shifted, radius, shader);
    }
}

}  // namespace

double measureWordmark(const std::string& text, const WordmarkStyle& style) {
    double unitWidth = 0.0;
    for (std::size_t index = 0; index < text.size(); ++index) {
        unitWidth += buildAstroGlyph(text[index]).advance;
        if (index + 1 < text.size()) unitWidth += style.letterSpacing;
    }
    unitWidth += style.slant;  // the slant pushes the top of the last letter right
    const double strokeRadius = style.letterHeight * style.strokeRadiusFraction;
    return unitWidth * style.letterHeight + 2.0 * strokeRadius;
}

void drawWordmark(FrameBuffer& frameBuffer, const std::string& text, const Vec2& topLeft, const WordmarkStyle& style) {
    const double letterHeight = style.letterHeight;
    const double strokeRadius = letterHeight * style.strokeRadiusFraction;
    const double outlineWidth = letterHeight * 0.045;

    // ---- Lay out all letters in screen space ----------------------------------
    // Map unit (u, v) -> screen, applying the italic shear x += slant * (1 - v).
    std::vector<Stroke> screenStrokes;
    double cursor = 0.0;
    for (char letter : text) {
        const AstroGlyph glyph = buildAstroGlyph(letter);
        for (const Stroke& stroke : glyph.strokes) {
            Stroke screenStroke;
            screenStroke.reserve(stroke.size());
            for (const Vec2& unitPoint : stroke) {
                screenStroke.push_back({topLeft.x + strokeRadius +
                                            (cursor + unitPoint.x + style.slant * (1.0 - unitPoint.y)) * letterHeight,
                                        topLeft.y + strokeRadius + unitPoint.y * letterHeight});
            }
            screenStrokes.push_back(screenStroke);
        }
        cursor += glyph.advance + style.letterSpacing;
    }

    auto solid = [](Color color) -> PixelShader { return [color](int, int) { return color; }; };

    // ---- Pass 1: drop shadow (offset down-right, drawn first so it sits behind) ----
    strokeAll(frameBuffer, screenStrokes, {letterHeight * 0.05, letterHeight * 0.07}, strokeRadius + 2.0 * outlineWidth,
              solid(astroPalette::ink));

    // ---- Pass 2 + 3: neon glow ring then dark outline ---------------------------
    strokeAll(frameBuffer, screenStrokes, {0.0, 0.0}, strokeRadius + 2.0 * outlineWidth, solid(astroPalette::neonPink));
    strokeAll(frameBuffer, screenStrokes, {0.0, 0.0}, strokeRadius + outlineWidth, solid(Color{58, 14, 74}));

    // ---- Pass 4: vertical gold gradient fill (fragment shader on y) -------------
    const double gradientTop = topLeft.y;
    const double gradientHeight = letterHeight + 2.0 * strokeRadius;
    const PixelShader goldGradient = [gradientTop, gradientHeight](int, int y) {
        const double t = std::clamp((y - gradientTop) / gradientHeight, 0.0, 1.0);
        if (t < 0.45) return lerpColor(astroPalette::goldLight, astroPalette::gold, t / 0.45);
        return lerpColor(astroPalette::gold, astroPalette::neonOrange, (t - 0.45) / 0.55);
    };
    strokeAll(frameBuffer, screenStrokes, {0.0, 0.0}, strokeRadius, goldGradient);

    // ---- Pass 5: glossy highlight, offset toward the light (up-left) ------------
    strokeAll(frameBuffer, screenStrokes, {-strokeRadius * 0.25, -strokeRadius * 0.4}, strokeRadius * 0.28,
              solid(lerpColor(astroPalette::goldLight, astroPalette::white, 0.55)));
}
