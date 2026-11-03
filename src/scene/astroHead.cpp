/**
 * @file astroHead.cpp
 * @brief Drawing the brass head layer by layer, from back to front.
 *
 * Techniques used:
 *   - Closed Catmull-Rom spline for the face silhouette, filled with scan-line fill
 *   - Catmull-Rom centre lines for the dreads, drawn as TAPERED strokes in three
 *     layers (dark base, mid tone, highlight) to fake a lit cylinder
 *   - Cubic Bezier curves for eyelids, lash lines and the lip rim
 *   - Midpoint ellipses for ears, nose, nostrils and lips
 *   - A per-pixel brass shader (scene/brassShading.h)
 *   - Soft alpha-falloff ellipses for sculpted shadows, highlights and the mouth glow
 */
#include "scene/astroHead.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "curves/bezier.h"
#include "curves/spline.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"
#include "scene/astroHeadShape.h"
#include "scene/brassShading.h"

namespace {

/** @brief Radii (in head units) of the imaginary ellipsoid the face shader lights. */
constexpr double faceShadingRadiusU = 0.47;
constexpr double faceShadingRadiusV = 0.64;

}  // namespace

void AstroHead::draw(FrameBuffer& frameBuffer) const {
    // Painter's algorithm: back of the head first, front-most details last.
    drawOuterDreads(frameBuffer);
    drawEars(frameBuffer);
    drawFace(frameBuffer);
    drawClosedEyes(frameBuffer);
    drawNose(frameBuffer);
    drawMouthPortal(frameBuffer);
    drawFrontDreads(frameBuffer);
}

void AstroHead::drawDread(FrameBuffer& frameBuffer, const Vec2* unitPoints, int pointCount, double side,
                          double swayWeight) const {
    // Bend in the wind: each control point is rotated about the ROOT by an angle that
    // grows along the dread (0 at the root, full at the tip), so the dread curls
    // instead of swinging stiffly. Both sides bend the same way, like a breeze.
    const Vec2 root{side * unitPoints[0].x, unitPoints[0].y};
    std::vector<Vec2> screenPoints;
    for (int index = 0; index < pointCount; ++index) {
        const double along = static_cast<double>(index) / (pointCount - 1);
        const Matrix3 bend = Matrix3::rotationAbout(root, dreadSwayAngle * swayWeight * along);
        const Vec2 unitPoint = bend.transformPoint({side * unitPoints[index].x, unitPoints[index].y});
        screenPoints.push_back(toScreen(unitPoint.x, unitPoint.y));
    }
    const std::vector<Vec2> centreLine = sampleCatmullRomSpline(screenPoints, 10, false);

    // A thick tube that tapers from root to tip. Three offset strokes fake cylindrical
    // lighting: dark base, mid tone shifted toward the light, narrow highlight.
    const double rootRadius = 0.043 * pixelsPerUnit();
    const double tipRadius = 0.024 * pixelsPerUnit();
    auto offsetLine = [&](double offsetX, double offsetY) {
        std::vector<Vec2> shifted;
        for (const Vec2& point : centreLine) shifted.push_back(point + Vec2(offsetX, offsetY));
        return shifted;
    };
    const double averageRadius = (rootRadius + tipRadius) * 0.5;
    drawTaperedPolyline(frameBuffer, centreLine, rootRadius, tipRadius, astroPalette::brassDark);
    drawTaperedPolyline(frameBuffer, offsetLine(-0.22 * averageRadius, -0.1 * averageRadius), rootRadius * 0.72,
                        tipRadius * 0.7, astroPalette::brass);
    drawTaperedPolyline(frameBuffer, offsetLine(-0.45 * averageRadius, -0.2 * averageRadius), rootRadius * 0.26,
                        tipRadius * 0.25, lerpColor(astroPalette::brass, astroPalette::brassLight, 0.7));
}

void AstroHead::drawOuterDreads(FrameBuffer& frameBuffer) const {
    // Outermost dreads sit behind the head: their roots are hidden by the crown.
    for (double side : {-1.0, 1.0}) drawDread(frameBuffer, astroHeadShape::outerDread, astroHeadShape::dreadPointCount, side, astroHeadShape::outerDreadSway);
}

void AstroHead::drawEars(FrameBuffer& frameBuffer) const {
    for (double side : {-1.0, 1.0}) {
        const Vec2 earCenter = toScreen(side * 0.42, -0.04);
        const double earRadiusX = 0.055 * pixelsPerUnit();
        const double earRadiusY = 0.10 * pixelsPerUnit();
        fillEllipse(frameBuffer, roundToInt(earCenter.x), roundToInt(earCenter.y), roundToInt(earRadiusX), roundToInt(earRadiusY),
                    makeBrassShader(earCenter, earRadiusX, earRadiusY, 0.8, rimColor));
        blendSoftEllipse(frameBuffer, earCenter.x, earCenter.y, earRadiusX * 0.5, earRadiusY * 0.6, astroPalette::brassShadow, 0.6);
    }
}

void AstroHead::drawFace(FrameBuffer& frameBuffer) const {
    // Silhouette: a closed Catmull-Rom spline. Tall egg shape, widest at the
    // cheekbones, sides running straight down into the ground.
    const double outlineUnits[][2] = {{0.0, -0.56},  {0.22, -0.53}, {0.35, -0.42}, {0.41, -0.24}, {0.43, -0.04},
                                      {0.42, 0.18},  {0.41, 0.38},  {0.41, 0.52},  {-0.41, 0.52}, {-0.41, 0.38},
                                      {-0.42, 0.18}, {-0.43, -0.04}, {-0.41, -0.24}, {-0.35, -0.42}, {-0.22, -0.53}};
    std::vector<Vec2> outlinePoints;
    for (const auto& point : outlineUnits) outlinePoints.push_back(toScreen(point[0], point[1]));

    // Scan-line fill with the brass shader, discarding rows below the ground line
    // (the spline overshoots slightly at the bottom corners).
    const PixelShader faceShader =
        makeBrassShader(toScreen(0.0, -0.05), faceShadingRadiusU * pixelsPerUnit(), faceShadingRadiusV * pixelsPerUnit(), 1.0, rimColor);
    const int lastRow = roundToInt(groundY());
    fillPolygonScanLine(sampleCatmullRomSpline(outlinePoints, 12, true), [&](int y, int xStart, int xEnd) {
        if (y <= lastRow) frameBuffer.fillSpan(y, xStart, xEnd, faceShader);
    });

    // Sculpting: temples, hollows under the cheekbones, a brow-bone highlight.
    for (double side : {-1.0, 1.0}) {
        const Vec2 temple = toScreen(side * 0.33, -0.26);
        blendSoftEllipse(frameBuffer, temple.x, temple.y, 0.09 * pixelsPerUnit(), 0.14 * pixelsPerUnit(), astroPalette::brassShadow, 0.25);
        const Vec2 cheekHollow = toScreen(side * 0.30, 0.20);
        blendSoftEllipse(frameBuffer, cheekHollow.x, cheekHollow.y, 0.10 * pixelsPerUnit(), 0.16 * pixelsPerUnit(), astroPalette::brassShadow, 0.3);
        const Vec2 browBone = toScreen(side * 0.17, -0.165);
        blendSoftEllipse(frameBuffer, browBone.x, browBone.y, 0.11 * pixelsPerUnit(), 0.035 * pixelsPerUnit(), astroPalette::brassLight, 0.3);
    }
}

void AstroHead::drawClosedEyes(FrameBuffer& frameBuffer) const {
    for (double side : {-1.0, 1.0}) {
        // Socket shadow, then the lit bulge of the closed upper eyelid.
        const Vec2 socket = toScreen(side * 0.16, -0.075);
        blendSoftEllipse(frameBuffer, socket.x, socket.y, 0.13 * pixelsPerUnit(), 0.08 * pixelsPerUnit(), astroPalette::brassShadow, 0.38);
        const Vec2 lid = toScreen(side * 0.155, -0.09);
        blendSoftEllipse(frameBuffer, lid.x, lid.y, 0.095 * pixelsPerUnit(), 0.045 * pixelsPerUnit(), astroPalette::brassLight, 0.5);

        // Crease above the lid and the closed lash line - both cubic Beziers.
        drawThickPolyline(frameBuffer,
                          sampleCubicBezier(toScreen(side * 0.07, -0.115), toScreen(side * 0.12, -0.148), toScreen(side * 0.20, -0.148),
                                            toScreen(side * 0.25, -0.105), 16),
                          0.006 * pixelsPerUnit(), astroPalette::brassDark);
        drawThickPolyline(frameBuffer,
                          sampleCubicBezier(toScreen(side * 0.065, -0.062), toScreen(side * 0.11, -0.036), toScreen(side * 0.20, -0.036),
                                            toScreen(side * 0.255, -0.068), 16),
                          0.009 * pixelsPerUnit(), astroPalette::brassShadow);

        // Soft fold under the eye.
        const Vec2 underEye = toScreen(side * 0.16, -0.02);
        blendSoftEllipse(frameBuffer, underEye.x, underEye.y, 0.085 * pixelsPerUnit(), 0.022 * pixelsPerUnit(), astroPalette::brassShadow, 0.22);
    }
}

void AstroHead::drawNose(FrameBuffer& frameBuffer) const {
    // Bridge: highlight down the lit side, shadow down the far side.
    const Vec2 bridgeLight = toScreen(-0.015, -0.01);
    blendSoftEllipse(frameBuffer, bridgeLight.x, bridgeLight.y, 0.03 * pixelsPerUnit(), 0.10 * pixelsPerUnit(), astroPalette::brassLight, 0.3);
    const Vec2 bridgeShadow = toScreen(0.06, 0.02);
    blendSoftEllipse(frameBuffer, bridgeShadow.x, bridgeShadow.y, 0.03 * pixelsPerUnit(), 0.09 * pixelsPerUnit(), astroPalette::brassShadow, 0.35);

    // Shadow cast under the nose onto the upper lip.
    const Vec2 underNose = toScreen(0.0, 0.19);
    blendSoftEllipse(frameBuffer, underNose.x, underNose.y, 0.13 * pixelsPerUnit(), 0.035 * pixelsPerUnit(), astroPalette::brassShadow, 0.5);

    // The tip and nostril wings are SCULPTED with soft light and shadow rather
    // than drawn as separate shapes, so they stay part of the same surface.
    const Vec2 tipHighlight = toScreen(-0.012, 0.085);
    blendSoftEllipse(frameBuffer, tipHighlight.x, tipHighlight.y, 0.06 * pixelsPerUnit(), 0.045 * pixelsPerUnit(), astroPalette::brassLight, 0.6);
    const Vec2 underTip = toScreen(0.0, 0.148);
    blendSoftEllipse(frameBuffer, underTip.x, underTip.y, 0.07 * pixelsPerUnit(), 0.025 * pixelsPerUnit(), astroPalette::brassShadow, 0.5);
    for (double side : {-1.0, 1.0}) {
        // Wing: a soft highlight on top, a shadow outside it, and a crease curving round it (Bezier).
        const Vec2 wingLight = toScreen(side * 0.088, 0.115);
        blendSoftEllipse(frameBuffer, wingLight.x, wingLight.y, 0.035 * pixelsPerUnit(), 0.03 * pixelsPerUnit(), astroPalette::brassLight, side < 0 ? 0.4 : 0.2);
        const Vec2 wingShadow = toScreen(side * 0.135, 0.13);
        blendSoftEllipse(frameBuffer, wingShadow.x, wingShadow.y, 0.03 * pixelsPerUnit(), 0.05 * pixelsPerUnit(), astroPalette::brassShadow, 0.35);
        drawThickPolyline(frameBuffer,
                          sampleCubicBezier(toScreen(side * 0.055, 0.075), toScreen(side * 0.125, 0.085), toScreen(side * 0.14, 0.155),
                                            toScreen(side * 0.075, 0.165), 16),
                          0.005 * pixelsPerUnit(), astroPalette::brassDark);
    }
    for (double side : {-1.0, 1.0}) {
        const Vec2 nostril = toScreen(side * 0.047, 0.152);
        fillEllipse(frameBuffer, roundToInt(nostril.x), roundToInt(nostril.y), roundToInt(0.027 * pixelsPerUnit()), roundToInt(0.012 * pixelsPerUnit()),
                    Color{52, 34, 12});
    }
}

void AstroHead::drawMouthPortal(FrameBuffer& frameBuffer) const {
    // Lips: one big shaded ellipse whose lower half is buried in the ground.
    const Vec2 lipCenter = toScreen(0.0, 0.37);
    const double lipRadiusX = 0.21 * pixelsPerUnit();
    const double lipRadiusY = 0.15 * pixelsPerUnit();
    const int lastRow = roundToInt(groundY());
    const PixelShader lipShader = makeBrassShader(lipCenter + Vec2(0.0, -0.03 * pixelsPerUnit()), lipRadiusX * 1.1, lipRadiusY * 1.3, 0.95, rimColor);
    rasterizeEllipseFilled(roundToInt(lipCenter.x), roundToInt(lipCenter.y), roundToInt(lipRadiusX), roundToInt(lipRadiusY),
                           [&](int y, int xStart, int xEnd) {
                               if (y <= lastRow) frameBuffer.fillSpan(y, xStart, xEnd, lipShader);
                           });
    // Upper-lip highlight.
    const Vec2 upperLip = toScreen(-0.02, 0.255);
    blendSoftEllipse(frameBuffer, upperLip.x, upperLip.y, 0.11 * pixelsPerUnit(), 0.03 * pixelsPerUnit(), astroPalette::brassLight, 0.5);

    // The doorway: straight sides rising from the ground into a rounded arch (Bezier).
    using astroHeadShape::doorwayArchControlV;
    using astroHeadShape::doorwayArchStartV;
    using astroHeadShape::doorwayHalfWidth;
    std::vector<Vec2> doorway = {toScreen(-doorwayHalfWidth, 0.5), toScreen(-doorwayHalfWidth, doorwayArchStartV)};
    const std::vector<Vec2> arch =
        sampleCubicBezier(toScreen(-doorwayHalfWidth, doorwayArchStartV), toScreen(-doorwayHalfWidth, doorwayArchControlV),
                          toScreen(doorwayHalfWidth, doorwayArchControlV), toScreen(doorwayHalfWidth, doorwayArchStartV), 24);
    doorway.insert(doorway.end(), arch.begin(), arch.end());
    doorway.push_back(toScreen(doorwayHalfWidth, 0.5));

    // Fragment shader: blinding white-yellow at the bottom centre, through gold, to orange at the rim.
    const Vec2 lightSource = toScreen(0.0, 0.5);
    const double doorwayRadius = 0.2 * pixelsPerUnit();
    fillPolygon(frameBuffer, doorway, [=](int x, int y) {
        const double distance = std::min(1.0, (Vec2(x, y) - lightSource).length() / doorwayRadius);
        if (distance < 0.5) return lerpColor(astroPalette::portalCore, astroPalette::portalGlow, distance / 0.5);
        return lerpColor(astroPalette::portalGlow, astroPalette::portalEdge, (distance - 0.5) / 0.5);
    });

    // Inner lip rim around the arch.
    std::vector<Vec2> rim = {toScreen(-doorwayHalfWidth, 0.5)};
    rim.insert(rim.end(), arch.begin(), arch.end());
    rim.push_back(toScreen(doorwayHalfWidth, 0.5));
    drawThickPolyline(frameBuffer, rim, 0.01 * pixelsPerUnit(), astroPalette::brassDark);

    // Light spilling out: a haze over the lips and a pool of light on the ground.
    const Vec2 hazeCenter = toScreen(0.0, 0.44);
    blendSoftEllipse(frameBuffer, hazeCenter.x, hazeCenter.y, 0.34 * pixelsPerUnit(), 0.2 * pixelsPerUnit(), astroPalette::portalGlow, 0.45 * glowStrength);
    const Vec2 poolCenter = toScreen(0.0, 0.53);
    blendSoftEllipse(frameBuffer, poolCenter.x, poolCenter.y, 0.75 * pixelsPerUnit(), 0.12 * pixelsPerUnit(), astroPalette::portalGlow, 0.55 * glowStrength);
    blendSoftEllipse(frameBuffer, poolCenter.x, poolCenter.y, 0.3 * pixelsPerUnit(), 0.05 * pixelsPerUnit(), astroPalette::portalCore, 0.6 * glowStrength);
}

void AstroHead::drawFrontDreads(FrameBuffer& frameBuffer) const {
    // Middle dreads hang beside the cheeks; the front pair falls past the temples.
    for (double side : {-1.0, 1.0}) drawDread(frameBuffer, astroHeadShape::middleDread, astroHeadShape::dreadPointCount, side, astroHeadShape::middleDreadSway);
    drawDread(frameBuffer, astroHeadShape::frontDreadRight, astroHeadShape::dreadPointCount, 1.0, astroHeadShape::frontDreadSway);
    drawDread(frameBuffer, astroHeadShape::frontDreadLeft, astroHeadShape::dreadPointCount, -1.0, astroHeadShape::frontDreadSway);
}
