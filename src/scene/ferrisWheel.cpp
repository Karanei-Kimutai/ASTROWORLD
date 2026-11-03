/**
 * @file ferrisWheel.cpp
 * @brief Ferris wheel drawing using a transformation hierarchy.
 */
#include "scene/ferrisWheel.h"

#include <cmath>
#include <vector>

#include "curves/bezier.h"
#include "fill/scanLineFill.h"
#include "math/matrix3.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"

namespace {

/**
 * @brief Gondola cabin in its own LOCAL coordinates: the hanger's top is the
 *        origin and the cabin hangs down the +y axis. Built once, transformed per gondola.
 */
struct GondolaShape {
    std::vector<Vec2> body = {{-11.0, 12.0}, {11.0, 12.0}, {9.0, 29.0}, {-9.0, 29.0}};
    std::vector<Vec2> window = {{-8.0, 16.0}, {8.0, 16.0}, {7.5, 21.0}, {-7.5, 21.0}};
    std::vector<Vec2> roof;
    GondolaShape() {
        // Domed roof: a Bezier arc closed along the cabin top (a polygon, so it can rotate).
        roof = sampleCubicBezier({-12.0, 13.0}, {-9.0, 7.0}, {9.0, 7.0}, {12.0, 13.0}, 12);
    }
};

}  // namespace

void FerrisWheel::draw(FrameBuffer& frameBuffer) const {
    const int hubX = roundToInt(hubPosition.x);
    const int hubY = roundToInt(hubPosition.y);
    const double angleStep = 2.0 * pi / gondolaCount;

    // Parent transformation: everything on the wheel turns with it about the hub.
    const Matrix3 wheelModel = Matrix3::translation(hubPosition) * Matrix3::rotation(rotationAngle);

    // ---- A-frame support legs (static, not part of the wheel hierarchy) -------------
    const double legSpread = radius * 0.6;
    const Vec2 leftFoot{hubPosition.x - legSpread, double(groundY)};
    const Vec2 rightFoot{hubPosition.x + legSpread, double(groundY)};
    drawThickLine(frameBuffer, hubPosition, leftFoot, 4.0, astroPalette::steel);
    drawThickLine(frameBuffer, hubPosition, rightFoot, 4.0, astroPalette::steel);
    drawThickLine(frameBuffer, lerpVec2(hubPosition, leftFoot, 0.6), lerpVec2(hubPosition, rightFoot, 0.6), 2.0,
                  astroPalette::steel);

    // ---- Spokes: child transforms of the wheel ----------------------------------------
    std::vector<Vec2> mountPoints;
    for (int spoke = 0; spoke < gondolaCount; ++spoke) {
        const Matrix3 spokeModel = wheelModel * Matrix3::rotation(spoke * angleStep);
        const Vec2 rimPoint = spokeModel.transformPoint({double(radius), 0.0});
        mountPoints.push_back(rimPoint);
        drawLineBresenham(frameBuffer, hubPosition, rimPoint, astroPalette::steelLight);
    }

    // ---- Rims: concentric midpoint circles (a circle looks the same at any rotation) --
    for (int thickness = 0; thickness < 3; ++thickness) drawCircle(frameBuffer, hubX, hubY, radius - thickness, astroPalette::neonPink);
    const int innerRadius = roundToInt(radius * 0.62);
    for (int thickness = 0; thickness < 2; ++thickness) drawCircle(frameBuffer, hubX, hubY, innerRadius - thickness, astroPalette::neonCyan);

    // ---- Rim lights ride on the wheel, so they are transformed by wheelModel too ------
    const int lightCount = gondolaCount * 3;
    for (int light = 0; light < lightCount; ++light) {
        const Matrix3 lightModel = wheelModel * Matrix3::rotation(light * 2.0 * pi / lightCount);
        const Vec2 lightPoint = lightModel.transformPoint({radius - 1.0, 0.0});
        const Color lightColor = light % 2 == 0 ? astroPalette::gold : astroPalette::neonCyan;
        blendCircle(frameBuffer, roundToInt(lightPoint.x), roundToInt(lightPoint.y), 6, lightColor, 0.25);
        fillCircle(frameBuffer, roundToInt(lightPoint.x), roundToInt(lightPoint.y), 2, makeTint(lightColor, 0.4));
    }

    // ---- Hub --------------------------------------------------------------------------
    fillCircle(frameBuffer, hubX, hubY, 12, astroPalette::gold);
    drawCircle(frameBuffer, hubX, hubY, 12, astroPalette::goldDark);
    // A marker on the hub makes its rotation visible.
    const Vec2 hubMarker = wheelModel.transformPoint({7.0, 0.0});
    fillCircle(frameBuffer, roundToInt(hubMarker.x), roundToInt(hubMarker.y), 3, astroPalette::goldDark);

    // ---- Gondolas: positioned by the wheel, oriented by gravity -----------------------
    static const GondolaShape gondola;
    for (int index = 0; index < gondolaCount; ++index) {
        // Each gondola swings with a slightly different phase so they do not move in lockstep.
        const double swing = swingAngle * std::cos(index * 1.3);
        const Matrix3 gondolaModel = Matrix3::translation(mountPoints[static_cast<std::size_t>(index)]) * Matrix3::rotation(swing);

        const Color cabinColor = hsvToRgb(index * 360.0 / gondolaCount, 0.7, 0.95);
        drawLineBresenham(frameBuffer, gondolaModel.transformPoint({0.0, 0.0}), gondolaModel.transformPoint({0.0, 12.0}),
                          astroPalette::steelLight);
        fillPolygon(frameBuffer, gondolaModel.transformPoints(gondola.body), cabinColor);
        fillPolygon(frameBuffer, gondolaModel.transformPoints(gondola.window), makeShade(cabinColor, 0.55));
        fillPolygon(frameBuffer, gondolaModel.transformPoints(gondola.roof), makeShade(cabinColor, 0.3));
    }
}
