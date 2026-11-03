/**
 * @file backdrop.cpp
 * @brief Sky, stars, searchlights, skyline, ground and entrance path.
 */
#include "scene/backdrop.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/simpleRandom.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/clipping.h"
#include "raster/lines.h"
#include "raster/strokes.h"

void drawSkyGradient(FrameBuffer& frameBuffer, int horizonY) {
    const double width = frameBuffer.getWidth();
    const std::vector<Vec2> skyRectangle = {{0.0, 0.0}, {width, 0.0}, {width, double(horizonY)}, {0.0, double(horizonY)}};

    // Fragment shader: colour depends only on height. Two HSV segments give
    // indigo -> violet at the top and violet -> magenta -> orange at the horizon.
    const PixelShader skyShader = [horizonY](int, int y) {
        const double t = std::clamp(static_cast<double>(y) / horizonY, 0.0, 1.0);
        if (t < 0.55) return lerpColorHsv(astroPalette::skyTop, astroPalette::skyMid, t / 0.55);
        return lerpColorHsv(astroPalette::skyMid, astroPalette::skyHorizon, (t - 0.55) / 0.45);
    };
    fillPolygon(frameBuffer, skyRectangle, skyShader);
}

void drawHorizonGlow(FrameBuffer& frameBuffer, const Vec2& center, int radiusX, int radiusY) {
    // Many faint, shrinking ellipses: overlapping alpha builds a smooth falloff.
    const int layerCount = 10;
    for (int layer = 0; layer < layerCount; ++layer) {
        const double t = static_cast<double>(layer) / layerCount;
        const Color glowColor = lerpColor(astroPalette::neonPink, astroPalette::neonOrange, t);
        blendEllipse(frameBuffer, roundToInt(center.x), roundToInt(center.y), roundToInt(radiusX * (1.0 - t * 0.8)),
                     roundToInt(radiusY * (1.0 - t * 0.8)), glowColor, 0.07);
    }
}

void drawStars(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed) {
    SimpleRandom random(seed);
    const int starCount = 260;
    const double starBandHeight = horizonY * 0.62;  // stars fade out before the bright horizon
    const Color starTints[3] = {astroPalette::white, Color{200, 240, 255}, Color{255, 210, 240}};

    for (int star = 0; star < starCount; ++star) {
        const int x = random.nextInt(0, frameBuffer.getWidth() - 1);
        const int y = random.nextInt(0, static_cast<int>(starBandHeight));
        const double heightFade = 1.0 - y / starBandHeight;
        const double brightness = random.nextRange(0.35, 1.0) * (0.35 + 0.65 * heightFade);
        const Color tint = starTints[random.nextInt(0, 2)];

        if (random.nextDouble() < 0.08) {
            // Sparkle: a small plus sign made of two Bresenham lines.
            const int armLength = random.nextInt(2, 4);
            rasterizeLineBresenham(x - armLength, y, x + armLength, y, makeBlendPlotter(frameBuffer, tint, brightness * 0.7));
            rasterizeLineBresenham(x, y - armLength, x, y + armLength, makeBlendPlotter(frameBuffer, tint, brightness * 0.7));
            frameBuffer.setPixel(x, y, astroPalette::white);
        } else {
            frameBuffer.blendPixel(x, y, tint, brightness);
        }
    }
}

void drawSearchlight(FrameBuffer& frameBuffer, const Vec2& origin, double angleDegrees, double spreadDegrees,
                     Color beamColor) {
    const ClipWindow screenWindow{0.0, 0.0, frameBuffer.getWidth() - 1.0, frameBuffer.getHeight() - 1.0};
    const int rayCount = 70;
    const double rayLength = 2200.0;  // far beyond the screen - clipping trims it

    for (int ray = 0; ray < rayCount; ++ray) {
        const double angle = (angleDegrees + spreadDegrees * (static_cast<double>(ray) / (rayCount - 1) - 0.5)) * pi / 180.0;
        // Screen y points down, so "up" is -sin(angle).
        Vec2 rayStart = origin;
        Vec2 rayEnd = origin + Vec2(std::cos(angle), -std::sin(angle)) * rayLength;
        if (!clipLineCohenSutherland(rayStart, rayEnd, screenWindow)) continue;
        rasterizeLineDda(roundToInt(rayStart.x), roundToInt(rayStart.y), roundToInt(rayEnd.x), roundToInt(rayEnd.y),
                         makeBlendPlotter(frameBuffer, beamColor, 0.03));
    }
}

void drawSkyline(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed) {
    SimpleRandom random(seed);
    const double width = frameBuffer.getWidth();
    const double baseY = horizonY + 4.0;

    // Walk left to right appending outline points for random silhouette pieces.
    std::vector<Vec2> outline;
    outline.push_back({0.0, baseY});
    double cursorX = 0.0;
    while (cursorX < width) {
        const int pieceKind = random.nextInt(0, 2);
        if (pieceKind == 0) {
            // Circus tent: a triangle peak.
            const double tentWidth = random.nextRange(50.0, 100.0);
            const double tentHeight = random.nextRange(30.0, 60.0);
            outline.push_back({cursorX, horizonY - 8.0});
            outline.push_back({cursorX + tentWidth * 0.5, horizonY - 8.0 - tentHeight});
            outline.push_back({cursorX + tentWidth, horizonY - 8.0});
            cursorX += tentWidth;
        } else if (pieceKind == 1) {
            // Flat-roofed building.
            const double buildingWidth = random.nextRange(30.0, 70.0);
            const double buildingHeight = random.nextRange(25.0, 55.0);
            outline.push_back({cursorX, horizonY - buildingHeight});
            outline.push_back({cursorX + buildingWidth, horizonY - buildingHeight});
            cursorX += buildingWidth;
        } else {
            // Tree line: a few random bumps.
            const double treeWidth = random.nextRange(40.0, 90.0);
            const int bumpCount = 4;
            for (int bump = 0; bump <= bumpCount; ++bump) {
                outline.push_back({cursorX + treeWidth * bump / bumpCount, horizonY - random.nextRange(10.0, 28.0)});
            }
            cursorX += treeWidth;
        }
    }
    outline.push_back({width, baseY});
    // The outline is highly CONCAVE - scan-line fill handles it with the even-odd rule.
    fillPolygon(frameBuffer, outline, astroPalette::silhouette);
}

void drawGround(FrameBuffer& frameBuffer, int horizonY) {
    const double width = frameBuffer.getWidth();
    const double height = frameBuffer.getHeight();
    const std::vector<Vec2> groundRectangle = {{0.0, double(horizonY)}, {width, double(horizonY)}, {width, height}, {0.0, height}};
    const PixelShader groundShader = [horizonY, height](int, int y) {
        const double t = std::clamp((y - horizonY) / (height - horizonY), 0.0, 1.0);
        return lerpColor(astroPalette::groundFar, astroPalette::groundNear, t);
    };
    fillPolygon(frameBuffer, groundRectangle, groundShader);
}

void drawEntrancePath(FrameBuffer& frameBuffer, int topY, double topLeftX, double topRightX, double bottomLeftX,
                      double bottomRightX) {
    const double bottomY = frameBuffer.getHeight();
    const std::vector<Vec2> pathShape = {{topLeftX, double(topY)}, {topRightX, double(topY)}, {bottomRightX, bottomY}, {bottomLeftX, bottomY}};

    // Tile lines at sqrt-spaced depths: equal steps in sqrt(depth) are close
    // together near the horizon and far apart near the viewer, imitating perspective.
    const PixelShader pathShader = [topY, bottomY](int, int y) {
        const double depth = std::clamp((y - topY) / (bottomY - topY), 0.0, 1.0);
        const double tileCoordinate = std::sqrt(depth) * 12.0;
        const bool onTileLine = tileCoordinate - std::floor(tileCoordinate) < 0.08;
        const Color base = lerpColor(makeShade(astroPalette::pathColor, 0.35), astroPalette::pathColor, depth);
        return onTileLine ? makeShade(base, 0.35) : base;
    };
    fillPolygon(frameBuffer, pathShape, pathShader);

    // Neon edging along both sides of the path.
    drawThickLine(frameBuffer, {topLeftX, double(topY)}, {bottomLeftX, bottomY}, 1.6, astroPalette::neonPink);
    drawThickLine(frameBuffer, {topRightX, double(topY)}, {bottomRightX, bottomY}, 1.6, astroPalette::neonPink);
}
