/**
 * @file backdrop.cpp
 * @brief Sky, clouds, stars, searchlights, skyline, fence and ground.
 */
#include "scene/backdrop.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "animation/easing.h"
#include "core/simpleRandom.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/clipping.h"
#include "raster/lines.h"
#include "raster/strokes.h"

void drawSky(FrameBuffer& frameBuffer, int horizonY, double nightAmount) {
    // First blend each gradient stop between its day and night colour...
    const Color topColor = lerpColor(astroPalette::daySkyTop, astroPalette::nightSkyTop, nightAmount);
    const Color midColor = lerpColor(astroPalette::daySkyMid, astroPalette::nightSkyMid, nightAmount);
    const Color horizonColor = lerpColor(astroPalette::daySkyHorizon, astroPalette::nightSkyHorizon, nightAmount);

    // ...then interpolate down the screen in HSV space (keeps the blues saturated).
    // The colour depends only on y, so it is computed ONCE PER ROW into a table and
    // the shader just looks it up - much cheaper than an HSV conversion per pixel.
    std::vector<Color> rowColors(static_cast<std::size_t>(std::max(horizonY, 1)));
    for (int y = 0; y < horizonY; ++y) {
        const double t = static_cast<double>(y) / horizonY;
        rowColors[static_cast<std::size_t>(y)] = t < 0.5 ? lerpColorHsv(topColor, midColor, t / 0.5)
                                                         : lerpColorHsv(midColor, horizonColor, (t - 0.5) / 0.5);
    }
    const PixelShader skyShader = [&rowColors, horizonY](int, int y) {
        return rowColors[static_cast<std::size_t>(std::clamp(y, 0, horizonY - 1))];
    };
    const double width = frameBuffer.getWidth();
    fillPolygon(frameBuffer, {{0.0, 0.0}, {width, 0.0}, {width, double(horizonY)}, {0.0, double(horizonY)}}, skyShader);
}

void drawClouds(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed, double opacity, double timeSeconds) {
    if (opacity <= 0.0) return;
    SimpleRandom random(seed);
    const int cloudCount = 7;
    for (int cloud = 0; cloud < cloudCount; ++cloud) {
        // Spread clouds evenly across the width with some jitter.
        const double cloudWidth = random.nextRange(170.0, 340.0);
        const double startX = (cloud + 0.5) * frameBuffer.getWidth() / cloudCount + random.nextRange(-60.0, 60.0);
        const double baseY = random.nextRange(horizonY * 0.18, horizonY * 0.8);

        // Drift: translate right over time. Lower clouds look nearer, so they move
        // faster (parallax). Wrap across a band wider than the screen so clouds
        // slide fully off one edge before reappearing at the other.
        const double speed = 6.0 + 14.0 * (baseY / horizonY);  // px/s
        const double wrapWidth = frameBuffer.getWidth() + 2.0 * 220.0;
        const double centerX = wrapTime(startX + 220.0 + speed * timeSeconds, wrapWidth) - 220.0;

        // Puffs: taller toward the middle of the cloud, flat along the base.
        struct Puff {
            double x, y, radius;
        };
        std::vector<Puff> puffs;
        const int puffCount = 16;
        for (int index = 0; index < puffCount; ++index) {
            const double across = random.nextRange(-0.5, 0.5);
            const double heightProfile = 1.0 - std::fabs(across) * 1.6;
            const double radius = cloudWidth * random.nextRange(0.10, 0.19) * (0.6 + 0.4 * std::max(0.0, heightProfile));
            puffs.push_back({centerX + across * cloudWidth,
                             baseY - radius * 0.4 - random.nextRange(0.0, 0.32) * cloudWidth * std::max(0.0, heightProfile),
                             radius});
        }
        // Shadowed undersides first, then the sunlit tops slightly up and to the left.
        for (const Puff& puff : puffs) {
            blendSoftEllipse(frameBuffer, puff.x, puff.y + puff.radius * 0.3, puff.radius * 1.1, puff.radius * 0.85,
                             astroPalette::cloudShadow, 0.75 * opacity);
        }
        for (const Puff& puff : puffs) {
            blendSoftEllipse(frameBuffer, puff.x - puff.radius * 0.12, puff.y - puff.radius * 0.15, puff.radius,
                             puff.radius * 0.8, astroPalette::cloudLight, 0.85 * opacity);
        }
    }
}

void drawStars(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed, double opacity, double timeSeconds) {
    if (opacity <= 0.0) return;
    SimpleRandom random(seed);
    const int starCount = 420;
    const Color starTints[3] = {astroPalette::white, Color{200, 225, 255}, Color{255, 240, 220}};

    for (int star = 0; star < starCount; ++star) {
        const int x = random.nextInt(0, frameBuffer.getWidth() - 1);
        const int y = random.nextInt(0, horizonY - 1);
        const Color tint = starTints[random.nextInt(0, 2)];
        // Twinkle: brightness oscillates between 55% and 100% at a per-star rate and phase.
        const double twinkle = 0.775 + oscillate(timeSeconds, random.nextRange(0.3, 1.5), 0.225, random.nextRange(0.0, 2.0 * pi));
        const double brightness = random.nextRange(0.25, 1.0) * opacity * twinkle;

        if (random.nextDouble() < 0.06) {
            // Bright star: a soft core plus a 4-point flare. The plot callback
            // sets alpha from the distance to the centre, so the arms fade out.
            const int armLength = random.nextInt(6, 16);
            const PlotCallback flarePlotter = [&](int px, int py) {
                const double distance = std::abs(px - x) + std::abs(py - y);
                const double fade = std::max(0.0, 1.0 - distance / armLength);
                frameBuffer.blendPixel(px, py, tint, opacity * twinkle * fade * fade);
            };
            rasterizeLineBresenham(x - armLength, y, x + armLength, y, flarePlotter);
            rasterizeLineBresenham(x, y - armLength, x, y + armLength, flarePlotter);
            blendSoftEllipse(frameBuffer, x, y, 4.0, 4.0, tint, 0.8 * opacity);
            frameBuffer.setPixel(x, y, astroPalette::white);
        } else {
            frameBuffer.blendPixel(x, y, tint, brightness);
        }
    }
}

void drawSearchlight(FrameBuffer& frameBuffer, const Vec2& origin, double angleDegrees, double spreadDegrees,
                     Color beamColor, double opacity) {
    if (opacity <= 0.0) return;
    const ClipWindow screenWindow{0.0, 0.0, frameBuffer.getWidth() - 1.0, frameBuffer.getHeight() - 1.0};
    const int rayCount = 70;
    const double rayLength = 2200.0;  // far beyond the screen - clipping trims it

    for (int ray = 0; ray < rayCount; ++ray) {
        const double angle =
            (angleDegrees + spreadDegrees * (static_cast<double>(ray) / (rayCount - 1) - 0.5)) * pi / 180.0;
        // Screen y points down, so "up" is -sin(angle).
        Vec2 rayStart = origin;
        Vec2 rayEnd = origin + Vec2(std::cos(angle), -std::sin(angle)) * rayLength;
        if (!clipLineCohenSutherland(rayStart, rayEnd, screenWindow)) continue;
        rasterizeLineDda(roundToInt(rayStart.x), roundToInt(rayStart.y), roundToInt(rayEnd.x), roundToInt(rayEnd.y),
                         makeBlendPlotter(frameBuffer, beamColor, 0.03 * opacity));
    }
}

void drawSkyline(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed, Color color) {
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
            const double tentHeight = random.nextRange(25.0, 50.0);
            outline.push_back({cursorX, horizonY - 8.0});
            outline.push_back({cursorX + tentWidth * 0.5, horizonY - 8.0 - tentHeight});
            outline.push_back({cursorX + tentWidth, horizonY - 8.0});
            cursorX += tentWidth;
        } else if (pieceKind == 1) {
            // Flat-roofed building.
            const double buildingWidth = random.nextRange(30.0, 70.0);
            const double buildingHeight = random.nextRange(25.0, 50.0);
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
    fillPolygon(frameBuffer, outline, color);
}

void drawHorizonFence(FrameBuffer& frameBuffer, int horizonY, int fenceHeight) {
    const int width = frameBuffer.getWidth();
    const int topY = horizonY - fenceHeight;
    // Two horizontal rails (Bresenham, 2 px thick each).
    for (int railY : {topY + 6, horizonY - 8}) {
        drawLineBresenham(frameBuffer, 0, railY, width - 1, railY, astroPalette::fenceBlack);
        drawLineBresenham(frameBuffer, 0, railY + 1, width - 1, railY + 1, astroPalette::fenceBlack);
    }
    // Pickets every 7 px with pointed tips; thicker posts with ball caps every 140 px.
    for (int x = 2; x < width; x += 7) {
        drawLineBresenham(frameBuffer, x, topY + 2, x, horizonY, astroPalette::fenceBlack);
        frameBuffer.setPixel(x, topY, astroPalette::fenceBlack);
    }
    for (int x = 40; x < width; x += 140) {
        drawThickLine(frameBuffer, {double(x), double(topY - 4)}, {double(x), double(horizonY)}, 2.5, astroPalette::fenceBlack);
        fillCircle(frameBuffer, x, topY - 7, 4, astroPalette::fenceBlack);
    }
}

void drawGround(FrameBuffer& frameBuffer, int horizonY, double nightAmount, std::uint32_t seed) {
    const double width = frameBuffer.getWidth();
    const double height = frameBuffer.getHeight();
    const Color farColor = lerpColor(astroPalette::dayGroundFar, astroPalette::nightGroundFar, nightAmount);
    const Color nearColor = lerpColor(astroPalette::dayGroundNear, astroPalette::nightGroundNear, nightAmount);
    // Colour depends only on the row, so build a per-row table (as for the sky).
    std::vector<Color> rowColors(static_cast<std::size_t>(height));
    for (int y = horizonY; y < static_cast<int>(height); ++y) {
        rowColors[static_cast<std::size_t>(y)] = lerpColor(farColor, nearColor, (y - horizonY) / (height - horizonY));
    }
    fillPolygon(frameBuffer, {{0.0, double(horizonY)}, {width, double(horizonY)}, {width, height}, {0.0, height}},
                [&rowColors, horizonY, height](int, int y) {
                    return rowColors[static_cast<std::size_t>(std::clamp(y, horizonY, static_cast<int>(height) - 1))];
                });

    // Concrete expansion joints converging toward a vanishing point at the head (one-point perspective).
    const Vec2 vanishingPoint{width * 0.5, double(horizonY) - 60.0};
    const Color jointColor = makeShade(nearColor, 0.18);
    for (int joint = -6; joint <= 6; ++joint) {
        if (joint == 0) continue;
        Vec2 jointStart = lerpVec2(vanishingPoint, {width * 0.5 + joint * 260.0, height}, 0.45);
        Vec2 jointEnd{width * 0.5 + joint * 260.0, height};
        const ClipWindow groundWindow{0.0, double(horizonY), width - 1.0, height - 1.0};
        if (clipLineCohenSutherland(jointStart, jointEnd, groundWindow)) {
            rasterizeLineDda(roundToInt(jointStart.x), roundToInt(jointStart.y), roundToInt(jointEnd.x),
                             roundToInt(jointEnd.y), makeBlendPlotter(frameBuffer, jointColor, 0.35));
        }
    }

    // Popcorn: more and bigger kernels nearer the viewer (perspective scaling).
    SimpleRandom random(seed);
    const Color kernelColor = lerpColor(Color{255, 246, 214}, Color{120, 110, 90}, nightAmount);
    for (int kernel = 0; kernel < 700; ++kernel) {
        const double depth = std::sqrt(random.nextDouble());  // biased toward the front
        const double y = horizonY + depth * (height - horizonY);
        const double x = random.nextRange(0.0, width);
        const int size = depth < 0.5 ? 0 : depth < 0.85 ? 1 : 2;
        if (size == 0) {
            frameBuffer.blendPixel(roundToInt(x), roundToInt(y), kernelColor, 0.8);
        } else {
            fillCircle(frameBuffer, roundToInt(x), roundToInt(y), size, kernelColor);
        }
    }
}
