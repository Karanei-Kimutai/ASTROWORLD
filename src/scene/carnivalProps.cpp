/**
 * @file carnivalProps.cpp
 * @brief Rocket, big-top tent, park sign and bunting.
 */
#include "scene/carnivalProps.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "curves/bezier.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Brightness across a vertical cylinder: u = -1 left edge .. +1 right edge, lit from the left. */
double cylinderBrightness(double u) {
    const double shifted = u + 0.35;
    return std::clamp(0.5 + 0.6 * (1.0 - shifted * shifted), 0.35, 1.1);
}

}  // namespace

void drawRocket(FrameBuffer& frameBuffer, const Vec2& baseCenter, double height) {
    const double halfWidth = height * 0.085;
    const double bodyBottom = baseCenter.y - height * 0.08;
    const double bodyTop = baseCenter.y - height * 0.66;
    const double tipY = baseCenter.y - height;
    const double centerX = baseCenter.x;

    // Shader for every silver part: cylindrical shading across x, red where a band is.
    const auto silverAt = [centerX, halfWidth](int x, int, Color base) {
        return scaleColor(base, cylinderBrightness((x - centerX) / halfWidth));
    };

    // Plinth.
    fillPolygon(frameBuffer, {{centerX - halfWidth * 1.6, baseCenter.y - height * 0.03}, {centerX + halfWidth * 1.6, baseCenter.y - height * 0.03},
                              {centerX + halfWidth * 1.8, baseCenter.y}, {centerX - halfWidth * 1.8, baseCenter.y}},
                Color{70, 66, 72});

    // Side fins (behind the body) - curved trailing edges from cubic Beziers.
    for (double side : {-1.0, 1.0}) {
        std::vector<Vec2> fin = {{centerX + side * halfWidth * 0.8, bodyBottom - height * 0.22}};
        const std::vector<Vec2> edge = sampleCubicBezier(
            {centerX + side * halfWidth * 0.8, bodyBottom - height * 0.22}, {centerX + side * halfWidth * 2.4, bodyBottom - height * 0.12},
            {centerX + side * halfWidth * 2.4, bodyBottom}, {centerX + side * halfWidth * 2.2, baseCenter.y - height * 0.02}, 12);
        fin.insert(fin.end(), edge.begin(), edge.end());
        fin.push_back({centerX + side * halfWidth * 0.8, bodyBottom});
        fillPolygon(frameBuffer, fin, makeShade(astroPalette::rocketRed, side < 0 ? 0.0 : 0.3));
    }

    // Body: a cylinder with two red bands.
    const double bandOneTop = bodyBottom - height * 0.10, bandOneBottom = bodyBottom - height * 0.06;
    const double bandTwoTop = bodyTop + height * 0.05, bandTwoBottom = bodyTop + height * 0.09;
    fillPolygon(frameBuffer, {{centerX - halfWidth, bodyTop}, {centerX + halfWidth, bodyTop}, {centerX + halfWidth, bodyBottom}, {centerX - halfWidth, bodyBottom}},
                [=](int x, int y) {
                    const bool inBand = (y >= bandOneTop && y <= bandOneBottom) || (y >= bandTwoTop && y <= bandTwoBottom);
                    return silverAt(x, y, inBand ? astroPalette::rocketRed : astroPalette::rocketSilver);
                });

    // Nose cone: two Bezier curves meeting at the tip, red tip above 85%.
    std::vector<Vec2> cone = sampleCubicBezier({centerX - halfWidth, bodyTop}, {centerX - halfWidth, bodyTop - height * 0.18},
                                               {centerX - halfWidth * 0.3, tipY + height * 0.06}, {centerX, tipY}, 16);
    const std::vector<Vec2> rightSide = sampleCubicBezier({centerX, tipY}, {centerX + halfWidth * 0.3, tipY + height * 0.06},
                                                          {centerX + halfWidth, bodyTop - height * 0.18}, {centerX + halfWidth, bodyTop}, 16);
    cone.insert(cone.end(), rightSide.begin(), rightSide.end());
    const double redTipLine = tipY + height * 0.08;
    fillPolygon(frameBuffer, cone, [=](int x, int y) {
        return silverAt(x, y, y < redTipLine ? astroPalette::rocketRed : astroPalette::rocketSilver);
    });

    // Centre fin seen edge-on, and a porthole.
    fillPolygon(frameBuffer, {{centerX - halfWidth * 0.18, bodyBottom - height * 0.18}, {centerX + halfWidth * 0.18, bodyBottom - height * 0.18},
                              {centerX + halfWidth * 0.3, baseCenter.y - height * 0.02}, {centerX - halfWidth * 0.3, baseCenter.y - height * 0.02}},
                makeShade(astroPalette::rocketRed, 0.15));
    const int portholeY = roundToInt(bodyTop + height * 0.2);
    fillCircle(frameBuffer, roundToInt(centerX), portholeY, roundToInt(halfWidth * 0.45), Color{60, 70, 90});
    fillCircle(frameBuffer, roundToInt(centerX), portholeY, roundToInt(halfWidth * 0.32), Color{120, 180, 230});
    drawCircle(frameBuffer, roundToInt(centerX), portholeY, roundToInt(halfWidth * 0.45), Color{150, 150, 160});
}

void drawBigTopTent(FrameBuffer& frameBuffer, const Vec2& baseCenter, double width, double height) {
    const double halfWidth = width * 0.5;
    const double eaveY = baseCenter.y - height * 0.38;  // where the roof meets the walls
    const Vec2 apex{baseCenter.x, baseCenter.y - height};
    const Color stripeColors[3] = {Color{30, 140, 112}, Color{36, 74, 164}, Color{232, 200, 64}};

    // Walls: vertical stripes, darker toward the sides.
    fillPolygon(frameBuffer, {{baseCenter.x - halfWidth * 0.92, eaveY}, {baseCenter.x + halfWidth * 0.92, eaveY},
                              {baseCenter.x + halfWidth * 0.92, baseCenter.y}, {baseCenter.x - halfWidth * 0.92, baseCenter.y}},
                [=](int x, int) {
                    const double u = (x - baseCenter.x) / halfWidth;
                    const int stripe = static_cast<int>(std::floor((u + 1.0) * 9.0));
                    return scaleColor(stripeColors[stripe % 3], 0.75 * cylinderBrightness(u));
                });

    // Roof: concave curved sides (cubic Beziers) from the eaves up to the apex.
    std::vector<Vec2> roof = sampleCubicBezier({baseCenter.x - halfWidth, eaveY}, {baseCenter.x - halfWidth * 0.55, eaveY - height * 0.1},
                                               {baseCenter.x - halfWidth * 0.12, apex.y + height * 0.25}, apex, 16);
    const std::vector<Vec2> rightRoof = sampleCubicBezier(apex, {baseCenter.x + halfWidth * 0.12, apex.y + height * 0.25},
                                                          {baseCenter.x + halfWidth * 0.55, eaveY - height * 0.1}, {baseCenter.x + halfWidth, eaveY}, 16);
    roof.insert(roof.end(), rightRoof.begin(), rightRoof.end());
    // Stripes radiate from the apex: pick the stripe by the angle from the apex.
    fillPolygon(frameBuffer, roof, [=](int x, int y) {
        const double angle = std::atan2(x - apex.x, y - apex.y + 1e-6);
        const int stripe = static_cast<int>(std::floor(angle * 7.0 + 20.0));
        return scaleColor(stripeColors[stripe % 3], cylinderBrightness((x - baseCenter.x) / halfWidth));
    });

    // Scalloped valance: a row of half-ellipses hanging from the eaves.
    const int scallopCount = 12;
    const double scallopRadius = width / scallopCount * 0.5;
    for (int scallop = 0; scallop < scallopCount; ++scallop) {
        const double scallopX = baseCenter.x - halfWidth + scallopRadius * (2 * scallop + 1);
        const Color color = stripeColors[scallop % 3];
        rasterizeEllipseFilled(roundToInt(scallopX), roundToInt(eaveY), roundToInt(scallopRadius), roundToInt(scallopRadius * 0.8),
                               [&](int y, int xStart, int xEnd) {
                                   if (y >= eaveY) frameBuffer.fillSpan(y, xStart, xEnd, makeShade(color, 0.1));  // lower half only
                               });
    }

    // Doorway and flag.
    fillPolygon(frameBuffer, {{baseCenter.x - width * 0.08, baseCenter.y}, {baseCenter.x - width * 0.05, eaveY + height * 0.12},
                              {baseCenter.x + width * 0.05, eaveY + height * 0.12}, {baseCenter.x + width * 0.08, baseCenter.y}},
                Color{30, 20, 30});
    drawLineBresenham(frameBuffer, apex, apex + Vec2(0.0, -22.0), Color{80, 80, 80});
    fillPolygon(frameBuffer, {apex + Vec2(0.0, -22.0), apex + Vec2(16.0, -17.0), apex + Vec2(0.0, -12.0)}, astroPalette::rocketRed);
}

void drawAstroworldSign(FrameBuffer& frameBuffer, const Vec2& topLeft, double width, double height, int groundY) {
    // Posts.
    for (double postX : {topLeft.x + width * 0.15, topLeft.x + width * 0.85}) {
        drawThickLine(frameBuffer, {postX, topLeft.y + height}, {postX, double(groundY)}, 2.5, Color{60, 60, 70});
    }
    // Board: dark frame, lighter panel.
    fillPolygon(frameBuffer, {topLeft, topLeft + Vec2(width, 0.0), topLeft + Vec2(width, height), topLeft + Vec2(0.0, height)}, Color{44, 48, 70});
    fillPolygon(frameBuffer, {topLeft + Vec2(4.0, 4.0), topLeft + Vec2(width - 4.0, 4.0), topLeft + Vec2(width - 4.0, height - 4.0), topLeft + Vec2(4.0, height - 4.0)},
                Color{232, 236, 244});
    // Lettering in the bitmap font, scaled to fit the board.
    const std::string label = "ASTROWORLD";
    const int pixelScale = std::max(1, static_cast<int>((width - 16.0) / measureBitmapText(label, 1)));
    const int textWidth = measureBitmapText(label, pixelScale);
    drawBitmapText(frameBuffer, label, roundToInt(topLeft.x + (width - textWidth) * 0.5), roundToInt(topLeft.y + (height - 7.0 * pixelScale) * 0.5),
                   pixelScale, Color{36, 40, 64});
    // Bulbs along the top edge.
    for (double bulbX = topLeft.x + 6.0; bulbX < topLeft.x + width - 4.0; bulbX += 12.0) {
        fillCircle(frameBuffer, roundToInt(bulbX), roundToInt(topLeft.y - 2.0), 2, astroPalette::goldLight);
    }
}

void drawBunting(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, double sag, int flagCount, int colorShift) {
    const Color flagColors[5] = {Color{220, 40, 50}, Color{250, 210, 40}, Color{40, 110, 210}, Color{245, 245, 245}, Color{240, 130, 40}};
    // The string: a cubic Bezier whose inner control points are pulled down by 'sag'.
    const Vec2 controlOne = lerpVec2(start, end, 0.33) + Vec2(0.0, sag);
    const Vec2 controlTwo = lerpVec2(start, end, 0.66) + Vec2(0.0, sag);
    drawPolylineBresenham(frameBuffer, sampleCubicBezier(start, controlOne, controlTwo, end, 40), Color{60, 60, 60});

    // Each pennant hangs from two points on the curve and points straight down.
    for (int flag = 0; flag < flagCount; ++flag) {
        const double t0 = (flag + 0.15) / flagCount;
        const double t1 = (flag + 0.85) / flagCount;
        const Vec2 left = evaluateCubicBezier(start, controlOne, controlTwo, end, t0);
        const Vec2 right = evaluateCubicBezier(start, controlOne, controlTwo, end, t1);
        const double flagLength = (right - left).length() * 1.3;
        const Vec2 tip = lerpVec2(left, right, 0.5) + Vec2(0.0, flagLength);
        fillPolygon(frameBuffer, {left, right, tip}, flagColors[(flag + colorShift) % 5]);
    }
}
