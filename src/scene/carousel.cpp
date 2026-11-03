/**
 * @file carousel.cpp
 * @brief Carousel drawing.
 *
 * The carousel is viewed slightly from above, so every horizontal circle
 * (platform, canopy rim, the ring the horses ride on) appears as an ELLIPSE
 * with its vertical radius squashed (~0.18 x horizontal radius).
 */
#include "scene/carousel.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "curves/bezier.h"
#include "fill/scanLineFill.h"
#include "math/matrix4.h"
#include "raster/circles.h"
#include "raster/lines.h"
#include "raster/strokes.h"

namespace {

/** @brief One horse after the model-view transform (view space: x right, y up, z toward the viewer). */
struct HorseSlot {
    Vec3 position;  // horse body (including its bob)
    Vec3 poleBase;  // where its pole meets the platform
    int index;
};

/** @brief A simple stylised horse facing 'direction' (+1 right, -1 left), scaled by 'size'. */
void drawHorse(FrameBuffer& frameBuffer, const Vec2& bodyCenter, double size, int direction, Color coat) {
    const Color mane = makeShade(coat, 0.35);
    // Legs first (behind the body): four short strokes.
    for (double legOffset : {-8.0, -4.0, 4.0, 8.0}) {
        const Vec2 hip = bodyCenter + Vec2(legOffset * size, 3.0 * size);
        drawThickLine(frameBuffer, hip, hip + Vec2(legOffset * 0.2 * size, 9.0 * size), 1.2 * size, mane);
    }
    // Body (ellipse), neck (thick line), head (ellipse), tail (Bezier).
    fillEllipse(frameBuffer, roundToInt(bodyCenter.x), roundToInt(bodyCenter.y), roundToInt(12.0 * size),
                roundToInt(6.0 * size), coat);
    const Vec2 neckBase = bodyCenter + Vec2(direction * 9.0 * size, -3.0 * size);
    const Vec2 headCenter = bodyCenter + Vec2(direction * 15.0 * size, -11.0 * size);
    drawThickLine(frameBuffer, neckBase, headCenter, 3.0 * size, coat);
    fillEllipse(frameBuffer, roundToInt(headCenter.x + direction * 2.0 * size), roundToInt(headCenter.y),
                roundToInt(5.0 * size), roundToInt(3.5 * size), coat);
    drawThickLine(frameBuffer, neckBase + Vec2(-direction * 2.0 * size, -2.0 * size),
                  headCenter + Vec2(-direction * 2.0 * size, -3.0 * size), 1.4 * size, mane);
    const Vec2 tailRoot = bodyCenter + Vec2(-direction * 11.0 * size, -2.0 * size);
    drawThickPolyline(frameBuffer,
                      sampleCubicBezier(tailRoot, tailRoot + Vec2(-direction * 6.0 * size, -4.0 * size),
                                        tailRoot + Vec2(-direction * 8.0 * size, 4.0 * size),
                                        tailRoot + Vec2(-direction * 6.0 * size, 9.0 * size), 8),
                      1.2 * size, mane);
}

}  // namespace

void Carousel::draw(FrameBuffer& frameBuffer) const {
    const double centerX = baseCenter.x;
    const double baseY = baseCenter.y;
    const double canopyY = baseY - radius * 0.95;
    const double apexY = canopyY - radius * 0.65;
    const double squash = 0.18;  // vertical/horizontal ratio of the viewing ellipses

    // ---- Platform: two stacked ellipses (edge + top) --------------------------
    fillEllipse(frameBuffer, roundToInt(centerX), roundToInt(baseY), roundToInt(radius * 1.02),
                roundToInt(radius * (squash + 0.02)), astroPalette::steelDark);
    fillEllipse(frameBuffer, roundToInt(centerX), roundToInt(baseY - 6.0), roundToInt(radius),
                roundToInt(radius * squash), astroPalette::goldDark);

    // ---- Centre column ---------------------------------------------------------
    fillPolygon(frameBuffer, {{centerX - 9.0, apexY + 10.0}, {centerX + 9.0, apexY + 10.0}, {centerX + 9.0, baseY - 6.0}, {centerX - 9.0, baseY - 6.0}},
                [centerX](int x, int) {
                    return lerpColor(astroPalette::goldLight, astroPalette::goldDark, std::clamp((x - centerX + 9.0) / 18.0, 0.0, 1.0));
                });

    // ---- Horses on poles: placed in 3D with 4x4 matrices, then projected -----------
    // Model: each horse starts at (0.8 R, bob, 0) and is spun about the vertical (y)
    //        axis:            model_i = Ry(rotationAngle + i * 45 degrees)
    // View:  the camera looks slightly DOWN on the ride, i.e. the world is tilted
    //        about the x axis: view = Rx(viewTilt). sin(viewTilt) = squash, so the
    //        horse ring lines up with the platform ellipse.
    // Projection: orthographic (parallel) - simply drop z after the view transform,
    //        keeping z as the depth for painter's sorting and size.
    const int horseCount = 8;
    const double viewTilt = std::asin(squash);
    const Matrix4 view = Matrix4::rotationX(viewTilt);
    std::vector<HorseSlot> slots;
    for (int index = 0; index < horseCount; ++index) {
        const Matrix4 model = Matrix4::rotationY(rotationAngle + index * 2.0 * pi / horseCount);
        const double bob = 6.0 * std::sin(bobPhase + index * 1.7);  // up-and-down gallop
        const Vec3 viewPosition = (view * model).transformPoint({radius * 0.8, bob, 0.0});
        const Vec3 poleBase = (view * model).transformPoint({radius * 0.8, 0.0, 0.0});
        slots.push_back({viewPosition, poleBase, index});
    }
    // Painter's algorithm: most distant (smallest z) first.
    std::sort(slots.begin(), slots.end(), [](const HorseSlot& a, const HorseSlot& b) { return a.position.z < b.position.z; });

    const double platformTopY = baseY - 6.0;
    const double ringDepth = radius * 0.8;
    for (const HorseSlot& slot : slots) {
        // View space has y UP; screen y points DOWN, hence the minus signs.
        const double poleX = centerX + slot.position.x;
        const double poleBottom = platformTopY - slot.poleBase.y;
        const double depth = std::clamp(slot.position.z / ringDepth, -1.0, 1.0);  // -1 back .. +1 front
        const double size = 0.8 + 0.2 * depth;  // nearer horses look bigger
        drawThickLine(frameBuffer, {poleX, canopyY + 4.0}, {poleX, poleBottom}, 1.6, astroPalette::goldLight);

        // Ry with a positive angle moves the front of the ring to the right.
        const int direction = depth >= 0.0 ? 1 : -1;
        const Vec2 bodyCenter{poleX, (canopyY + poleBottom) * 0.5 - slot.position.y + slot.poleBase.y};
        const Color coat = slot.index % 2 == 0 ? astroPalette::cream : Color{255, 150, 200};
        drawHorse(frameBuffer, bodyCenter, size, direction, coat);
    }

    // ---- Canopy cone: radial stripes picked by the angle from the apex --------
    const double rotation = rotationAngle;
    const PixelShader coneShader = [centerX, apexY, rotation](int x, int y) {
        const double angleFromApex = std::atan2(x - centerX, y - apexY + 1e-6);
        const int stripe = static_cast<int>(std::floor(angleFromApex * 9.0 + rotation * 4.0));
        const Color base = (stripe & 1) ? astroPalette::cream : astroPalette::neonPink;
        // Darken toward the sides so the cone reads as rounded.
        return scaleColor(base, 1.0 - 0.35 * std::fabs(angleFromApex));
    };
    fillPolygon(frameBuffer, {{centerX, apexY}, {centerX + radius * 1.05, canopyY}, {centerX - radius * 1.05, canopyY}}, coneShader);

    // ---- Canopy rim: an ellipse striped by its parametric angle ---------------
    const double rimRadiusX = radius * 1.05;
    const double rimRadiusY = radius * squash;
    const PixelShader rimShader = [centerX, canopyY, rimRadiusX, rimRadiusY, rotation](int x, int y) {
        const double angle = std::atan2((y - canopyY) / rimRadiusY, (x - centerX) / rimRadiusX) + rotation;
        const int stripe = static_cast<int>(std::floor((angle + pi) / (2.0 * pi) * 16.0));
        return (stripe & 1) ? astroPalette::gold : astroPalette::neonPurple;
    };
    fillEllipse(frameBuffer, roundToInt(centerX), roundToInt(canopyY), roundToInt(rimRadiusX), roundToInt(rimRadiusY),
                rimShader);
    drawEllipse(frameBuffer, roundToInt(centerX), roundToInt(canopyY), roundToInt(rimRadiusX), roundToInt(rimRadiusY),
                astroPalette::goldDark);

    // ---- Bulbs around the front of the rim and the platform -------------------
    for (int bulb = 0; bulb < 24; ++bulb) {
        const double angle = bulb * 2.0 * pi / 24.0;
        if (std::sin(angle) < -0.2) continue;  // back half is hidden behind the canopy
        const Vec2 rimBulb{centerX + std::cos(angle) * rimRadiusX, canopyY + std::sin(angle) * rimRadiusY + 3.0};
        blendCircle(frameBuffer, roundToInt(rimBulb.x), roundToInt(rimBulb.y), 5, astroPalette::goldLight, 0.3);
        fillCircle(frameBuffer, roundToInt(rimBulb.x), roundToInt(rimBulb.y), 2, astroPalette::goldLight);
        const Vec2 baseBulb{centerX + std::cos(angle) * radius, baseY - 6.0 + std::sin(angle) * radius * squash + 3.0};
        fillCircle(frameBuffer, roundToInt(baseBulb.x), roundToInt(baseBulb.y), 2, astroPalette::neonCyan);
    }

    // ---- Flag on top -------------------------------------------------------------
    drawLineBresenham(frameBuffer, {centerX, apexY}, {centerX, apexY - 24.0}, astroPalette::goldLight);
    fillPolygon(frameBuffer, {{centerX, apexY - 24.0}, {centerX + 16.0, apexY - 19.0}, {centerX, apexY - 14.0}},
                astroPalette::neonCyan);
}
