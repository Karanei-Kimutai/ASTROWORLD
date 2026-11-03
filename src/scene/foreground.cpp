/**
 * @file foreground.cpp
 * @brief Crowd and balloon vendor.
 */
#include "scene/foreground.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "animation/easing.h"
#include "core/simpleRandom.h"
#include "curves/bezier.h"
#include "fill/boundaryFill.h"
#include "fill/scanLineFill.h"
#include "math/matrix3.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"

void drawPersonSilhouette(FrameBuffer& frameBuffer, const Vec2& feetPosition, double height, double walkSwing, Color color) {
    if (height < 1.0) return;
    const Matrix3 model = Matrix3::translation(feetPosition) * Matrix3::scaling(height);

    // Legs: child transforms rotating about the hip in opposite directions.
    const Vec2 hip{0.0, -0.42};
    for (double direction : {-1.0, 1.0}) {
        const Matrix3 legModel = model * Matrix3::rotationAbout(hip, direction * walkSwing);
        drawThickLine(frameBuffer, legModel.transformPoint(hip), legModel.transformPoint({0.0, 0.0}), height * 0.055, color);
    }
    // Torso: a trapezoid from the shoulders to the hips.
    fillPolygon(frameBuffer, model.transformPoints({{-0.17, -0.74}, {0.17, -0.74}, {0.12, -0.40}, {-0.12, -0.40}}), color);
    // Head: a midpoint circle at the transformed centre (uniform scaling keeps circles circular).
    const Vec2 headCenter = model.transformPoint({0.0, -0.87});
    fillCircle(frameBuffer, roundToInt(headCenter.x), roundToInt(headCenter.y), std::max(1, roundToInt(height * 0.13)), color);
}

void drawCrowd(FrameBuffer& frameBuffer, const Vec2& portalBase, double timeSeconds, std::uint32_t seed) {
    SimpleRandom random(seed);
    const double bottomY = frameBuffer.getHeight();

    struct Visitor {
        Vec2 feet;
        double height;
        double walkSwing;
    };
    std::vector<Visitor> visitors;
    for (int index = 0; index < 14; ++index) {
        // Fixed per-visitor parameters (same random sequence every frame).
        const Vec2 start{portalBase.x + random.nextRange(-480.0, 480.0), bottomY + random.nextRange(-60.0, 30.0)};
        const Vec2 end{portalBase.x + random.nextRange(-35.0, 35.0), portalBase.y + 2.0};
        const double walkDuration = random.nextRange(9.0, 14.0);
        const double phase = random.nextDouble();

        // Progress along the walk, looping: 0 = starting near the viewer, 1 = at the doorway.
        const double progress = wrapTime(timeSeconds / walkDuration + phase, 1.0);
        const Vec2 feet = lerpVec2(start, end, progress);

        // Perspective scaling: size follows how far down the screen (how near) the feet are.
        const double depth = std::clamp((feet.y - portalBase.y) / (bottomY - portalBase.y), 0.0, 1.2);
        double height = 30.0 + depth * 70.0;
        // Entering the light: shrink to nothing over the last 8% of the walk.
        if (progress > 0.92) height *= (1.0 - progress) / 0.08;

        // Walk cycle: legs swing at ~1.6 strides per second; a little bounce on each step.
        const double stridePhase = 2.0 * pi * 1.6 * timeSeconds + index;
        const double bounce = std::fabs(std::sin(stridePhase)) * height * 0.03;
        visitors.push_back({feet + Vec2(0.0, -bounce), height, 0.35 * std::sin(stridePhase)});
    }
    // Painter's algorithm: farthest (smallest y) first.
    std::sort(visitors.begin(), visitors.end(), [](const Visitor& a, const Visitor& b) { return a.feet.y < b.feet.y; });
    for (const Visitor& visitor : visitors) {
        drawPersonSilhouette(frameBuffer, visitor.feet, visitor.height, visitor.walkSwing, astroPalette::crowd);
    }
}

void drawBalloonVendor(FrameBuffer& frameBuffer, const Vec2& feetPosition, double timeSeconds) {
    const double vendorHeight = 70.0;
    const Vec2 hand = feetPosition + Vec2(10.0, -vendorHeight * 0.62);

    // Balloon centres relative to the hand, chosen so no two outlines overlap.
    const Vec2 balloonOffsets[5] = {{-45.0, -150.0}, {5.0, -170.0}, {55.0, -145.0}, {-20.0, -95.0}, {40.0, -88.0}};
    const Color balloonColors[5] = {astroPalette::neonPink, astroPalette::gold, astroPalette::neonCyan,
                                    astroPalette::neonPurple, astroPalette::neonOrange};
    const int balloonRadiusX = 20;
    const int balloonRadiusY = 26;

    // The whole bunch sways about the hand; each balloon adds a small wobble of its own.
    std::vector<Vec2> centers;
    for (int index = 0; index < 5; ++index) {
        const double swayAngle = oscillate(timeSeconds, 0.25, 0.06) + oscillate(timeSeconds, 0.6, 0.02, index * 1.9);
        centers.push_back(Matrix3::rotationAbout(hand, swayAngle).transformPoint(hand + balloonOffsets[index]));
    }

    // Strings first so balloons sit on top of them.
    for (const Vec2& center : centers) {
        const Vec2 knot = center + Vec2(0.0, balloonRadiusY + 4.0);
        drawPolylineBresenham(frameBuffer, sampleCubicBezier(knot, knot + Vec2(6.0, 25.0), hand + Vec2(-6.0, -30.0), hand, 16),
                              Color{220, 210, 230});
    }

    for (int index = 0; index < 5; ++index) {
        const Vec2& center = centers[static_cast<std::size_t>(index)];
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

    drawPersonSilhouette(frameBuffer, feetPosition, vendorHeight, 0.0, astroPalette::crowd);
    drawThickLine(frameBuffer, feetPosition + Vec2(4.0, -vendorHeight * 0.7), hand, 3.0, astroPalette::crowd);  // raised arm
}
