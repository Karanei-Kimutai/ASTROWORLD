/**
 * @file ferrisWheel.cpp
 * @brief Ferris wheel drawing.
 */
#include "scene/ferrisWheel.h"

#include <cmath>
#include <vector>

#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"

void FerrisWheel::draw(FrameBuffer& frameBuffer) const {
    const int hubX = roundToInt(hubPosition.x);
    const int hubY = roundToInt(hubPosition.y);
    const double angleStep = 2.0 * pi / gondolaCount;

    // ---- A-frame support legs (thick strokes) ----------------------------------
    const double legSpread = radius * 0.6;
    const Vec2 leftFoot{hubPosition.x - legSpread, double(groundY)};
    const Vec2 rightFoot{hubPosition.x + legSpread, double(groundY)};
    drawThickLine(frameBuffer, hubPosition, leftFoot, 4.0, astroPalette::steel);
    drawThickLine(frameBuffer, hubPosition, rightFoot, 4.0, astroPalette::steel);
    drawThickLine(frameBuffer, lerpVec2(hubPosition, leftFoot, 0.6), lerpVec2(hubPosition, rightFoot, 0.6), 2.0,
                  astroPalette::steel);

    // ---- Spokes: one Bresenham line from the hub to each gondola mount ---------
    for (int spoke = 0; spoke < gondolaCount; ++spoke) {
        const double angle = rotationAngle + spoke * angleStep;
        const Vec2 rimPoint = hubPosition + Vec2(std::cos(angle), std::sin(angle)) * radius;
        drawLineBresenham(frameBuffer, hubPosition, rimPoint, astroPalette::steelLight);
    }

    // ---- Rims: concentric midpoint circles give a 3-pixel-thick neon ring ------
    for (int thickness = 0; thickness < 3; ++thickness) drawCircle(frameBuffer, hubX, hubY, radius - thickness, astroPalette::neonPink);
    const int innerRadius = roundToInt(radius * 0.62);
    for (int thickness = 0; thickness < 2; ++thickness) drawCircle(frameBuffer, hubX, hubY, innerRadius - thickness, astroPalette::neonCyan);

    // ---- Rim lights: 3 per spoke, alternating colours, each with a soft glow ---
    const int lightCount = gondolaCount * 3;
    for (int light = 0; light < lightCount; ++light) {
        const double angle = rotationAngle + light * 2.0 * pi / lightCount;
        const Vec2 lightPoint = hubPosition + Vec2(std::cos(angle), std::sin(angle)) * (radius - 1.0);
        const Color lightColor = light % 2 == 0 ? astroPalette::gold : astroPalette::neonCyan;
        blendCircle(frameBuffer, roundToInt(lightPoint.x), roundToInt(lightPoint.y), 6, lightColor, 0.25);
        fillCircle(frameBuffer, roundToInt(lightPoint.x), roundToInt(lightPoint.y), 2, makeTint(lightColor, 0.4));
    }

    // ---- Hub --------------------------------------------------------------------
    fillCircle(frameBuffer, hubX, hubY, 12, astroPalette::gold);
    drawCircle(frameBuffer, hubX, hubY, 12, astroPalette::goldDark);
    fillCircle(frameBuffer, hubX, hubY, 4, astroPalette::goldDark);

    // ---- Gondolas: they HANG from the rim, so they stay upright whatever the angle ----
    for (int gondola = 0; gondola < gondolaCount; ++gondola) {
        const double angle = rotationAngle + gondola * angleStep;
        const Vec2 mountPoint = hubPosition + Vec2(std::cos(angle), std::sin(angle)) * radius;
        const Vec2 hangerEnd = mountPoint + Vec2(0.0, 12.0);
        drawLineBresenham(frameBuffer, mountPoint, hangerEnd, astroPalette::steelLight);

        // Cabin body: a trapezoid, coloured by spreading hues evenly around the HSV wheel.
        const Color cabinColor = hsvToRgb(gondola * 360.0 / gondolaCount, 0.7, 0.95);
        const std::vector<Vec2> cabin = {hangerEnd + Vec2(-11.0, 0.0), hangerEnd + Vec2(11.0, 0.0),
                                         hangerEnd + Vec2(9.0, 17.0), hangerEnd + Vec2(-9.0, 17.0)};
        fillPolygon(frameBuffer, cabin, cabinColor);
        // Window band (a shade of the cabin colour) and a roof (an ellipse).
        const std::vector<Vec2> window = {hangerEnd + Vec2(-8.0, 4.0), hangerEnd + Vec2(8.0, 4.0),
                                          hangerEnd + Vec2(7.5, 9.0), hangerEnd + Vec2(-7.5, 9.0)};
        fillPolygon(frameBuffer, window, makeShade(cabinColor, 0.55));
        fillEllipse(frameBuffer, roundToInt(hangerEnd.x), roundToInt(hangerEnd.y), 12, 3, makeShade(cabinColor, 0.3));
    }
}
