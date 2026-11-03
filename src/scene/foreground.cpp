/**
 * @file foreground.cpp
 * @brief Crowd, fence lights and balloon vendor.
 */
#include "scene/foreground.h"

#include <algorithm>
#include <vector>

#include "core/simpleRandom.h"
#include "curves/bezier.h"
#include "fill/boundaryFill.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"

void drawPersonSilhouette(FrameBuffer& frameBuffer, const Vec2& feetPosition, double height, Color color) {
    const double shoulderWidth = height * 0.34;
    const double hipWidth = height * 0.22;
    const double shoulderY = feetPosition.y - height * 0.74;
    // Body: a trapezoid from the shoulders down to the feet.
    fillPolygon(frameBuffer,
                {{feetPosition.x - shoulderWidth * 0.5, shoulderY}, {feetPosition.x + shoulderWidth * 0.5, shoulderY},
                 {feetPosition.x + hipWidth * 0.5, feetPosition.y}, {feetPosition.x - hipWidth * 0.5, feetPosition.y}},
                color);
    // Head: a filled midpoint circle above the shoulders.
    fillCircle(frameBuffer, roundToInt(feetPosition.x), roundToInt(feetPosition.y - height * 0.87),
               std::max(1, roundToInt(height * 0.13)), color);
}

void drawCrowd(FrameBuffer& frameBuffer, int pathTopY, double topLeftX, double topRightX, double bottomLeftX,
               double bottomRightX, std::uint32_t seed) {
    SimpleRandom random(seed);
    const double bottomY = frameBuffer.getHeight();

    struct Visitor {
        Vec2 feet;
        double height;
    };
    std::vector<Visitor> visitors;
    for (int index = 0; index < 16; ++index) {
        const double depth = random.nextRange(0.04, 0.9);  // 0 = at the head, 1 = at the viewer
        const double y = pathTopY + depth * (bottomY - pathTopY);
        // The path widens toward the viewer; pick x inside the path at this depth.
        const double leftX = topLeftX + (bottomLeftX - topLeftX) * depth;
        const double rightX = topRightX + (bottomRightX - topRightX) * depth;
        const double x = random.nextRange(leftX + 10.0, rightX - 10.0);
        visitors.push_back({{x, y}, 14.0 + depth * 50.0});  // nearer = taller (perspective scaling)
    }
    // Painter's algorithm: farthest (smallest y) first.
    std::sort(visitors.begin(), visitors.end(), [](const Visitor& a, const Visitor& b) { return a.feet.y < b.feet.y; });
    for (const Visitor& visitor : visitors) {
        drawPersonSilhouette(frameBuffer, visitor.feet, visitor.height, astroPalette::crowd);
    }
}

void drawFenceWithLights(FrameBuffer& frameBuffer, double fenceTopY, double fenceBottomY, double gapStartX,
                         double gapEndX) {
    const double width = frameBuffer.getWidth();
    const double postSpacing = 100.0;
    const double postTopY = fenceTopY - 30.0;
    const Color bulbColors[4] = {astroPalette::neonPink, astroPalette::gold, astroPalette::neonCyan,
                                 astroPalette::neonOrange};

    // Rails on each side of the path opening.
    for (double railY : {fenceTopY + 12.0, fenceBottomY - 10.0}) {
        drawThickLine(frameBuffer, {0.0, railY}, {gapStartX, railY}, 2.0, astroPalette::steelDark);
        drawThickLine(frameBuffer, {gapEndX, railY}, {width, railY}, 2.0, astroPalette::steelDark);
    }

    // Collect post positions, skipping the path gap.
    std::vector<double> posts;
    for (double x = 30.0; x < width; x += postSpacing) {
        if (x > gapStartX - 10.0 && x < gapEndX + 10.0) continue;
        posts.push_back(x);
    }

    int bulbIndex = 0;
    for (std::size_t index = 0; index < posts.size(); ++index) {
        const double postX = posts[index];
        drawThickLine(frameBuffer, {postX, postTopY}, {postX, fenceBottomY}, 3.0, astroPalette::steelDark);
        fillCircle(frameBuffer, roundToInt(postX), roundToInt(postTopY), 4, astroPalette::gold);

        // String light to the next post on the same side of the gap.
        if (index + 1 >= posts.size()) continue;
        const double nextX = posts[index + 1];
        if (nextX - postX > postSpacing * 1.5) continue;  // the path gap lies between these posts

        // A cubic Bezier whose inner control points hang below the posts makes a natural sag.
        const Vec2 start{postX, postTopY};
        const Vec2 end{nextX, postTopY};
        const std::vector<Vec2> wire = sampleCubicBezier(start, start + Vec2(30.0, 34.0), end + Vec2(-30.0, 34.0), end, 20);
        drawPolylineBresenham(frameBuffer, wire, astroPalette::ink);
        for (std::size_t bulb = 2; bulb + 1 < wire.size(); bulb += 3) {
            const Color bulbColor = bulbColors[bulbIndex++ % 4];
            blendCircle(frameBuffer, roundToInt(wire[bulb].x), roundToInt(wire[bulb].y) + 3, 7, bulbColor, 0.3);
            fillCircle(frameBuffer, roundToInt(wire[bulb].x), roundToInt(wire[bulb].y) + 3, 3, makeTint(bulbColor, 0.35));
        }
    }
}

void drawBalloonVendor(FrameBuffer& frameBuffer, const Vec2& feetPosition) {
    const double vendorHeight = 70.0;
    const Vec2 hand = feetPosition + Vec2(10.0, -vendorHeight * 0.62);

    // Balloon centres relative to the hand, chosen so no two outlines overlap.
    const Vec2 balloonOffsets[5] = {{-45.0, -150.0}, {5.0, -170.0}, {55.0, -145.0}, {-20.0, -95.0}, {40.0, -88.0}};
    const Color balloonColors[5] = {astroPalette::neonPink, astroPalette::gold, astroPalette::neonCyan,
                                    astroPalette::neonPurple, astroPalette::neonOrange};
    const int balloonRadiusX = 20;
    const int balloonRadiusY = 26;

    // Strings first so balloons sit on top of them.
    for (const Vec2& offset : balloonOffsets) {
        const Vec2 knot = hand + offset + Vec2(0.0, balloonRadiusY + 4.0);
        const std::vector<Vec2> string =
            sampleCubicBezier(knot, knot + Vec2(6.0, 25.0), hand + Vec2(-6.0, -30.0), hand, 16);
        drawPolylineBresenham(frameBuffer, string, Color{220, 210, 230});
    }

    for (int index = 0; index < 5; ++index) {
        const Vec2 center = hand + balloonOffsets[index];
        const int centerX = roundToInt(center.x);
        const int centerY = roundToInt(center.y);
        // 1. Outline with the midpoint ellipse in a unique boundary colour.
        drawEllipse(frameBuffer, centerX, centerY, balloonRadiusX, balloonRadiusY, astroPalette::ink);
        // 2. Seed fill from the centre, 4-connected, stopping at the outline.
        boundaryFill4(frameBuffer, centerX, centerY, balloonColors[index], astroPalette::ink);
        // 3. Shine and knot.
        blendEllipse(frameBuffer, centerX - 7, centerY - 10, 5, 8, astroPalette::white, 0.45);
        fillPolygon(frameBuffer,
                    {{center.x - 4.0, center.y + balloonRadiusY + 5.0}, {center.x + 4.0, center.y + balloonRadiusY + 5.0},
                     {center.x, center.y + balloonRadiusY - 1.0}},
                    makeShade(balloonColors[index], 0.3));
    }

    drawPersonSilhouette(frameBuffer, feetPosition, vendorHeight, astroPalette::crowd);
    drawThickLine(frameBuffer, feetPosition + Vec2(4.0, -vendorHeight * 0.7), hand, 3.0, astroPalette::crowd);  // raised arm
}
