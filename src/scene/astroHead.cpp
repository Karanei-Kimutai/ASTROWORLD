/**
 * @file astroHead.cpp
 * @brief Drawing the golden head, layer by layer from back to front.
 *
 * Techniques used:
 *   - Closed Catmull-Rom splines for the face silhouette and hair cap, filled with scan-line fill
 *   - Cubic Bezier curves for dreads, brows, eyelids, nose and braids, drawn as thick strokes
 *   - Midpoint ellipses / circles for ears, eyes, nostrils and the open mouth
 *   - A per-pixel gold shader (scene/goldShading.h) for the inflatable sheen
 *   - Alpha blending for contour shadows and the glowing portal inside the mouth
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
#include "scene/goldShading.h"

namespace {

/** @brief Radii (in head units) of the imaginary ellipsoid the face shader lights. */
constexpr double faceShadingRadiusU = 0.47;
constexpr double faceShadingRadiusV = 0.56;

/**
 * @brief Blend a colour over an ellipse with alpha fading from maxAlpha at the
 *        centre to 0 at the edge: alpha = maxAlpha * (1 - d^2)^2, where d is the
 *        normalised elliptical distance. A per-fragment alpha, like a soft brush.
 */
void blendSoftEllipse(FrameBuffer& frameBuffer, const Vec2& center, double radiusX, double radiusY, Color color,
                      double maxAlpha) {
    rasterizeEllipseFilled(roundToInt(center.x), roundToInt(center.y), roundToInt(radiusX), roundToInt(radiusY),
                           [&](int y, int xStart, int xEnd) {
                               for (int x = xStart; x <= xEnd; ++x) {
                                   const double du = (x - center.x) / radiusX;
                                   const double dv = (y - center.y) / radiusY;
                                   const double falloff = std::max(0.0, 1.0 - (du * du + dv * dv));
                                   frameBuffer.blendPixel(x, y, color, maxAlpha * falloff * falloff);
                               }
                           });
}

}  // namespace

void AstroHead::draw(FrameBuffer& frameBuffer) const {
    // Contact shadow on the ground under the chin.
    const Vec2 shadowCenter = toScreen(0.0, 0.52);
    blendEllipse(frameBuffer, roundToInt(shadowCenter.x), roundToInt(shadowCenter.y), roundToInt(0.5 * scale),
                 roundToInt(0.045 * scale), astroPalette::ink, 0.55);

    // Painter's algorithm: draw from the back of the head to the front.
    drawBackHair(frameBuffer);
    drawEars(frameBuffer);
    drawFace(frameBuffer);
    drawHairCap(frameBuffer);
    drawFrontHair(frameBuffer);
    drawEyesAndBrows(frameBuffer);
    drawNose(frameBuffer);
    drawMouth(frameBuffer);
}

void AstroHead::drawBackHair(FrameBuffer& frameBuffer) const {
    // Long dreads hanging behind the head; only the parts outside the face silhouette stay visible.
    const PixelShader hairShader = makeGoldShader(toScreen(0.0, -0.02), faceShadingRadiusU * scale * 1.15,
                                                  faceShadingRadiusV * scale * 1.15, 0.72);
    const double dreadRadius = 0.022 * scale;
    for (double side : {-1.0, 1.0}) {
        for (int strand = 0; strand < 10; ++strand) {
            const double s = strand;
            const std::vector<Vec2> dread =
                sampleCubicBezier(toScreen(side * (0.30 + 0.012 * s), -0.36 + 0.02 * s),
                                  toScreen(side * (0.47 + 0.010 * s), -0.30 + 0.03 * s),
                                  toScreen(side * 0.52, -0.05 + 0.03 * s),
                                  toScreen(side * (0.50 - 0.005 * s), 0.06 + 0.035 * s), 16);
            drawThickPolyline(frameBuffer, dread, dreadRadius, hairShader);
        }
    }
}

void AstroHead::drawEars(FrameBuffer& frameBuffer) const {
    for (double side : {-1.0, 1.0}) {
        const Vec2 earCenter = toScreen(side * 0.44, -0.02);
        const double earRadiusX = 0.06 * scale;
        const double earRadiusY = 0.11 * scale;
        fillEllipse(frameBuffer, roundToInt(earCenter.x), roundToInt(earCenter.y), roundToInt(earRadiusX),
                    roundToInt(earRadiusY), makeGoldShader(earCenter, earRadiusX, earRadiusY, 0.85));
        // Inner ear: a darker blended ellipse.
        blendEllipse(frameBuffer, roundToInt(earCenter.x + side * 0.01 * scale), roundToInt(earCenter.y),
                     roundToInt(earRadiusX * 0.5), roundToInt(earRadiusY * 0.6), astroPalette::goldShadow, 0.55);
    }
}

void AstroHead::drawFace(FrameBuffer& frameBuffer) const {
    // Silhouette: a closed Catmull-Rom spline through 14 points (crown -> cheek -> jaw -> chin -> ...).
    const double outlineUnits[][2] = {{0.0, -0.52},  {0.30, -0.47},  {0.42, -0.30},  {0.44, -0.05}, {0.40, 0.15},
                                      {0.32, 0.33},  {0.18, 0.47},   {0.0, 0.52},    {-0.18, 0.47}, {-0.32, 0.33},
                                      {-0.40, 0.15}, {-0.44, -0.05}, {-0.42, -0.30}, {-0.30, -0.47}};
    std::vector<Vec2> outlinePoints;
    for (const auto& point : outlineUnits) outlinePoints.push_back(toScreen(point[0], point[1]));
    const std::vector<Vec2> faceOutline = sampleCatmullRomSpline(outlinePoints, 12, true);

    // The outline polygon is filled with the scan-line algorithm; the gold shader colours each pixel.
    fillPolygon(frameBuffer, faceOutline,
                makeGoldShader(toScreen(0.0, -0.02), faceShadingRadiusU * scale, faceShadingRadiusV * scale, 1.0));

    // Sculpted contours: soft shadows under the cheekbones. Alpha falls off
    // smoothly with distance from the centre, so the shadow has no hard edge.
    for (double side : {-1.0, 1.0}) {
        blendSoftEllipse(frameBuffer, toScreen(side * 0.31, 0.12), 0.10 * scale, 0.16 * scale, astroPalette::goldShadow, 0.22);
    }
    const Vec2 chinShadow = toScreen(0.0, 0.43);
    blendEllipse(frameBuffer, roundToInt(chinShadow.x), roundToInt(chinShadow.y), roundToInt(0.11 * scale),
                 roundToInt(0.035 * scale), astroPalette::goldShadow, 0.22);
}

void AstroHead::drawHairCap(FrameBuffer& frameBuffer) const {
    // Closed spline: over the crown, then back along the hairline across the forehead.
    const double capUnits[][2] = {{-0.43, -0.12}, {-0.43, -0.32}, {-0.30, -0.49}, {0.0, -0.55},  {0.30, -0.49},
                                  {0.43, -0.32},  {0.43, -0.12},  {0.36, -0.21},  {0.20, -0.29}, {0.0, -0.31},
                                  {-0.20, -0.29}, {-0.36, -0.21}};
    std::vector<Vec2> capPoints;
    for (const auto& point : capUnits) capPoints.push_back(toScreen(point[0], point[1]));
    fillPolygon(frameBuffer, sampleCatmullRomSpline(capPoints, 10, true),
                makeGoldShader(toScreen(0.0, -0.02), faceShadingRadiusU * scale, faceShadingRadiusV * scale, 0.72));

    // Braided rows running from the hairline back over the crown (thin Bezier strokes).
    const Color braidLine = lerpColor(astroPalette::goldShadow, astroPalette::goldDark, 0.3);
    for (int row = -5; row <= 5; ++row) {
        const double r = row;
        const double hairlineV = -0.30 + std::fabs(r) * 0.012;
        const std::vector<Vec2> braid = sampleCubicBezier(toScreen(r * 0.072, hairlineV), toScreen(r * 0.076, -0.42),
                                                          toScreen(r * 0.05, -0.50), toScreen(r * 0.02, -0.535), 14);
        drawThickPolyline(frameBuffer, braid, 0.005 * scale, braidLine);
        // Little glints along each braid suggest the woven texture.
        for (std::size_t knot = 2; knot + 1 < braid.size(); knot += 3) {
            fillCircle(frameBuffer, roundToInt(braid[knot].x + 0.008 * scale), roundToInt(braid[knot].y), 1,
                       astroPalette::goldLight);
        }
    }
}

void AstroHead::drawFrontHair(FrameBuffer& frameBuffer) const {
    // Dreads falling forward past the temples, framing the face.
    const PixelShader hairShader =
        makeGoldShader(toScreen(0.0, -0.02), faceShadingRadiusU * scale, faceShadingRadiusV * scale, 0.88);
    const double dreadRadius = 0.024 * scale;
    for (double side : {-1.0, 1.0}) {
        for (int strand = 0; strand < 6; ++strand) {
            const double s = strand;
            const std::vector<Vec2> dread =
                sampleCubicBezier(toScreen(side * (0.22 + 0.035 * s), -0.30 + 0.02 * s),
                                  toScreen(side * (0.36 + 0.030 * s), -0.28 + 0.03 * s),
                                  toScreen(side * (0.42 + 0.020 * s), -0.10 + 0.04 * s),
                                  toScreen(side * (0.42 + 0.015 * s), 0.05 + 0.05 * s), 16);
            drawThickPolyline(frameBuffer, dread, dreadRadius, hairShader);
            // A thin highlight down the lit side of each dread.
            std::vector<Vec2> highlight;
            for (const Vec2& point : dread) highlight.push_back(point + Vec2(-dreadRadius * 0.35, -dreadRadius * 0.2));
            drawThickPolyline(frameBuffer, highlight, dreadRadius * 0.25, makeTint(astroPalette::gold, 0.45));
        }
    }
}

void AstroHead::drawEyesAndBrows(FrameBuffer& frameBuffer) const {
    const Color lidColor = lerpColor(astroPalette::goldShadow, astroPalette::goldDark, 0.25);
    for (double side : {-1.0, 1.0}) {
        // Eye socket shadow.
        const Vec2 socket = toScreen(side * 0.16, -0.055);
        blendEllipse(frameBuffer, roundToInt(socket.x), roundToInt(socket.y), roundToInt(0.10 * scale),
                     roundToInt(0.06 * scale), astroPalette::goldShadow, 0.25);

        // Eyeball, iris, pupil and catch-light (midpoint ellipse + circles).
        fillEllipse(frameBuffer, roundToInt(socket.x), roundToInt(socket.y), roundToInt(0.075 * scale),
                    roundToInt(0.036 * scale), Color{255, 232, 165});
        const Vec2 iris = toScreen(side * 0.16, -0.052);
        fillCircle(frameBuffer, roundToInt(iris.x), roundToInt(iris.y), roundToInt(0.03 * scale), astroPalette::goldDark);
        fillCircle(frameBuffer, roundToInt(iris.x), roundToInt(iris.y), roundToInt(0.014 * scale), Color{70, 34, 12});
        fillCircle(frameBuffer, roundToInt(iris.x - 0.01 * scale), roundToInt(iris.y - 0.01 * scale),
                   roundToInt(0.006 * scale), astroPalette::white);

        // Heavy upper eyelid and brow: cubic Bezier strokes.
        drawThickPolyline(frameBuffer,
                          sampleCubicBezier(toScreen(side * 0.075, -0.05), toScreen(side * 0.12, -0.10),
                                            toScreen(side * 0.20, -0.10), toScreen(side * 0.245, -0.045), 16),
                          0.012 * scale, lidColor);
        drawThickPolyline(frameBuffer,
                          sampleCubicBezier(toScreen(side * 0.06, -0.14), toScreen(side * 0.12, -0.19),
                                            toScreen(side * 0.22, -0.19), toScreen(side * 0.29, -0.13), 16),
                          0.02 * scale, lidColor);
    }
}

void AstroHead::drawNose(FrameBuffer& frameBuffer) const {
    const Color noseShadow = lerpColor(astroPalette::gold, astroPalette::goldShadow, 0.55);
    // Bridge shadow on the right side (away from the light).
    drawThickPolyline(frameBuffer,
                      sampleCubicBezier(toScreen(0.035, -0.05), toScreen(0.05, 0.0), toScreen(0.05, 0.05),
                                        toScreen(0.065, 0.10), 12),
                      0.009 * scale, noseShadow);
    // Highlight on the tip.
    const Vec2 tip = toScreen(-0.005, 0.095);
    blendEllipse(frameBuffer, roundToInt(tip.x), roundToInt(tip.y), roundToInt(0.03 * scale), roundToInt(0.022 * scale),
                 astroPalette::goldLight, 0.5);
    // Base of the nose and the nostrils.
    drawThickPolyline(frameBuffer,
                      sampleCubicBezier(toScreen(-0.075, 0.115), toScreen(-0.06, 0.165), toScreen(0.06, 0.165),
                                        toScreen(0.075, 0.115), 16),
                      0.012 * scale, noseShadow);
    for (double side : {-1.0, 1.0}) {
        const Vec2 nostril = toScreen(side * 0.035, 0.135);
        fillEllipse(frameBuffer, roundToInt(nostril.x), roundToInt(nostril.y), roundToInt(0.016 * scale),
                    roundToInt(0.008 * scale), astroPalette::goldShadow);
    }
}

void AstroHead::drawMouth(FrameBuffer& frameBuffer) const {
    const Vec2 mouthCenter = toScreen(0.0, 0.29);
    const int mouthX = roundToInt(mouthCenter.x);
    const int mouthY = roundToInt(mouthCenter.y);

    // Lips: a shaded ellipse slightly larger than the opening.
    const double lipRadiusX = 0.17 * scale;
    const double lipRadiusY = 0.11 * scale;
    fillEllipse(frameBuffer, mouthX, mouthY, roundToInt(lipRadiusX), roundToInt(lipRadiusY),
                makeGoldShader(mouthCenter + Vec2(0.0, -0.02 * scale), lipRadiusX * 1.2, lipRadiusY * 1.4, 0.9));

    // The opening - the dark entrance into ASTROWORLD.
    const double openingRadiusX = 0.125 * scale;
    const double openingRadiusY = 0.072 * scale;
    fillEllipse(frameBuffer, mouthX, mouthY, roundToInt(openingRadiusX), roundToInt(openingRadiusY),
                astroPalette::mouthDark);

    // Portal glow inside: shrinking blended ellipses from pink to orange.
    for (int layer = 0; layer < 6; ++layer) {
        const double t = layer / 6.0;
        blendEllipse(frameBuffer, mouthX, roundToInt(mouthCenter.y + 0.015 * scale),
                     roundToInt(openingRadiusX * (0.9 - 0.12 * layer)), roundToInt(openingRadiusY * (0.75 - 0.1 * layer)),
                     lerpColor(astroPalette::neonPink, astroPalette::neonOrange, t), 0.16);
    }

    // Top teeth: fill a rectangle, but DISCARD pixels outside the opening ellipse
    // (a per-fragment test, like clipping a shape against a curved window).
    const Color teethColor{255, 226, 140};
    const SpanCallback teethWriter = [&](int y, int xStart, int xEnd) {
        for (int x = xStart; x <= xEnd; ++x) {
            const double du = (x - mouthCenter.x) / openingRadiusX;
            const double dv = (y - mouthCenter.y) / openingRadiusY;
            if (du * du + dv * dv <= 1.0) frameBuffer.setPixel(x, y, teethColor);
        }
    };
    fillPolygonScanLine({toScreen(-0.1, 0.21), toScreen(0.1, 0.21), toScreen(0.1, 0.248), toScreen(-0.1, 0.248)},
                        teethWriter);
    for (int tooth = -3; tooth <= 3; ++tooth) {
        const Vec2 gapTop = toScreen(tooth * 0.026, 0.222);
        drawLineBresenham(frameBuffer, gapTop, gapTop + Vec2(0.0, 0.024 * scale), astroPalette::goldDark);
    }

    // Lip line around the opening (2 px), and a highlight on the upper lip.
    drawEllipse(frameBuffer, mouthX, mouthY, roundToInt(openingRadiusX), roundToInt(openingRadiusY), astroPalette::goldShadow);
    drawEllipse(frameBuffer, mouthX, mouthY, roundToInt(openingRadiusX) + 1, roundToInt(openingRadiusY) + 1,
                astroPalette::goldShadow);
    const Vec2 lipHighlight = toScreen(-0.02, 0.198);
    blendEllipse(frameBuffer, roundToInt(lipHighlight.x), roundToInt(lipHighlight.y), roundToInt(0.07 * scale),
                 roundToInt(0.012 * scale), astroPalette::goldLight, 0.55);
}
