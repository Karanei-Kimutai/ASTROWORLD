/**
 * @file dropTower.cpp
 * @brief Drop tower drawing.
 */
#include "scene/dropTower.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "core/vec2.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"

void DropTower::draw(FrameBuffer& frameBuffer) const {
    const double halfWidth = columnWidth * 0.5;
    const Color towerBase{150, 120, 190};

    // ---- Column with cylindrical shading -----------------------------------------
    // Brightness peaks a little left of centre (light from the left) and falls off to the edges.
    const double columnCenter = centerX;
    const PixelShader cylinderShader = [columnCenter, halfWidth, towerBase](int x, int) {
        const double u = (x - columnCenter) / halfWidth;  // -1 at left edge, +1 at right edge
        const double brightness = std::clamp(0.45 + 0.55 * (1.0 - (u + 0.35) * (u + 0.35)), 0.25, 1.0);
        return scaleColor(towerBase, brightness);
    };
    fillPolygon(frameBuffer,
                {{centerX - halfWidth, topY}, {centerX + halfWidth, topY}, {centerX + halfWidth, groundY},
                 {centerX - halfWidth, groundY}},
                cylinderShader);

    // ---- Lattice bracing (zig-zag of Bresenham lines) ---------------------------
    const double braceStep = 22.0;
    for (double y = topY; y + braceStep <= groundY; y += braceStep) {
        const bool leftToRight = static_cast<int>((y - topY) / braceStep) % 2 == 0;
        const double fromX = leftToRight ? centerX - halfWidth : centerX + halfWidth;
        const double toX = leftToRight ? centerX + halfWidth : centerX - halfWidth;
        drawLineBresenham(frameBuffer, {fromX, y}, {toX, y + braceStep}, makeShade(towerBase, 0.5));
    }

    // ---- Chase lights down the middle -------------------------------------------
    int lightIndex = 0;
    for (double y = topY + 20.0; y < groundY - 10.0; y += 16.0, ++lightIndex) {
        const Color lightColor = lightIndex % 2 == 0 ? astroPalette::neonPink : astroPalette::gold;
        blendCircle(frameBuffer, roundToInt(centerX), roundToInt(y), 5, lightColor, 0.3);
        fillCircle(frameBuffer, roundToInt(centerX), roundToInt(y), 2, makeTint(lightColor, 0.3));
    }

    // ---- Cap, antenna and red beacon --------------------------------------------
    fillPolygon(frameBuffer, {{centerX - halfWidth - 10.0, topY}, {centerX + halfWidth + 10.0, topY}, {centerX, topY - 40.0}},
                astroPalette::gold);
    drawLineBresenham(frameBuffer, {centerX, topY - 40.0}, {centerX, topY - 70.0}, astroPalette::steelLight);
    const Color beaconRed{255, 40, 60};
    for (int glowRing = 4; glowRing >= 1; --glowRing) {
        blendCircle(frameBuffer, roundToInt(centerX), roundToInt(topY - 72.0), glowRing * 4, beaconRed, 0.18);
    }
    fillCircle(frameBuffer, roundToInt(centerX), roundToInt(topY - 72.0), 3, makeTint(beaconRed, 0.5));

    // ---- Seat ring: a dark ellipse with gold trims and riders' seats ------------
    const int ringRadiusX = roundToInt(halfWidth + 30.0);
    const int seatRow = roundToInt(seatY);
    fillEllipse(frameBuffer, roundToInt(centerX), seatRow + 6, ringRadiusX, 12, astroPalette::steelDark);
    for (int seat = -3; seat <= 3; ++seat) {
        const double seatX = centerX + seat * 9.0;
        const Color seatColor = hsvToRgb(300.0 + seat * 20.0, 0.75, 0.95);
        fillPolygon(frameBuffer, {{seatX - 3.5, seatY - 2.0}, {seatX + 3.5, seatY - 2.0}, {seatX + 3.5, seatY + 12.0}, {seatX - 3.5, seatY + 12.0}},
                    seatColor);
    }
    drawEllipse(frameBuffer, roundToInt(centerX), seatRow, ringRadiusX, 10, astroPalette::gold);
    drawEllipse(frameBuffer, roundToInt(centerX), seatRow + 13, ringRadiusX, 10, astroPalette::goldDark);
}
