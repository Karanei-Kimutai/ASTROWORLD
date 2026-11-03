/**
 * @file transformShowcase.cpp
 * @brief Transformation demonstration panels (3x3 for 2D, 4x4 for 3D).
 */
#include "showcase/transformShowcase.h"

#include <array>
#include <cmath>
#include <vector>

#include "fill/scanLineFill.h"
#include "math/matrix3.h"
#include "math/matrix4.h"
#include "raster/circles.h"
#include "raster/clipping.h"
#include "raster/lines.h"
#include "showcase/showcasePanel.h"

namespace {

/**
 * @brief An ASYMMETRIC test shape (a pennant on a pole) in local coordinates, so
 *        rotations and reflections are easy to see. Its origin is the pole's foot.
 */
const std::vector<Vec2>& pennantShape() {
    static const std::vector<Vec2> shape = {{-3.0, 0.0}, {-3.0, -44.0}, {24.0, -34.0}, {3.0, -24.0}, {3.0, 0.0}};
    return shape;
}

/** @brief Fill a transformed copy of the pennant and outline it. */
void drawPennant(FrameBuffer& frameBuffer, const Matrix3& model, Color color) {
    const std::vector<Vec2> points = model.transformPoints(pennantShape());
    fillPolygon(frameBuffer, points, color);
    drawPolylineBresenham(frameBuffer, points, makeShade(color, 0.5), true);
}

/** @brief Mark a point with a small white cross. */
void markPoint(FrameBuffer& frameBuffer, const Vec2& point) {
    drawLineBresenham(frameBuffer, point + Vec2(-5, 0), point + Vec2(5, 0), astroPalette::white);
    drawLineBresenham(frameBuffer, point + Vec2(0, -5), point + Vec2(0, 5), astroPalette::white);
}

void drawTranslationPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 start{panel.left + 50.0, panel.top + 300.0};
    const Vec2 step{45.0, -35.0};
    for (int copy = 0; copy < 6; ++copy) {
        // T(start + copy * step): the same shape moved, never rotated or resized.
        const Matrix3 model = Matrix3::translation(start + step * copy);
        drawPennant(frameBuffer, model, hsvToRgb(190.0 + copy * 25.0, 0.7, 0.4 + copy * 0.12));
    }
    drawLineDda(frameBuffer, roundToInt(start.x), roundToInt(start.y) + 8, roundToInt(start.x + step.x * 5), roundToInt(start.y + step.y * 5) + 8,
                astroPalette::goldLight);
}

void drawRotationPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 pivot{panel.centerX(), panel.centerY() + 10.0};
    // The pennant sits 70 px to the right of the pivot, then is rotated about the pivot.
    const Matrix3 placement = Matrix3::translation(pivot + Vec2(70.0, 0.0));
    for (int copy = 0; copy < 8; ++copy) {
        const Matrix3 model = Matrix3::rotationAbout(pivot, copy * 2.0 * pi / 8.0) * placement;
        drawPennant(frameBuffer, model, hsvToRgb(copy * 45.0, 0.75, 1.0));
    }
    drawCircle(frameBuffer, roundToInt(pivot.x), roundToInt(pivot.y), 70, Color{80, 70, 110});
    markPoint(frameBuffer, pivot);
}

void drawScalingPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 fixedPoint{panel.centerX() - 60.0, panel.centerY() + 100.0};
    // Largest first so smaller copies sit on top; the fixed point never moves.
    for (int copy = 4; copy >= 0; --copy) {
        const double factor = 0.6 + copy * 0.55;
        const Matrix3 model = Matrix3::scalingAbout(fixedPoint, factor, factor) * Matrix3::translation(fixedPoint);
        drawPennant(frameBuffer, model, hsvToRgb(30.0 + copy * 30.0, 0.75, 1.0));
    }
    markPoint(frameBuffer, fixedPoint);
}

void drawShearReflectPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 base{panel.centerX(), panel.top + 170.0};
    // Original (left), sheared copy (right).
    drawPennant(frameBuffer, Matrix3::translation(base + Vec2(-90.0, 0.0)) * Matrix3::scaling(1.6), astroPalette::letterBlue);
    drawPennant(frameBuffer, Matrix3::translation(base + Vec2(40.0, 0.0)) * Matrix3::shearing(-0.6, 0.0) * Matrix3::scaling(1.6),
                astroPalette::letterGreen);
    // Reflection across a horizontal axis: S(1, -1) about a point on the axis.
    const double axisY = panel.top + 220.0;
    for (int x = panel.left + 20; x < panel.left + panelWidth - 20; x += 8) {
        drawLineBresenham(frameBuffer, x, roundToInt(axisY), x + 4, roundToInt(axisY), astroPalette::white);  // dashed axis
    }
    const Matrix3 original = Matrix3::translation(base.x - 25.0, axisY - 6.0) * Matrix3::scaling(1.4);
    const Matrix3 mirror = Matrix3::translation(0.0, axisY) * Matrix3::reflectionAcrossXAxis() * Matrix3::translation(0.0, -axisY);
    drawPennant(frameBuffer, original, astroPalette::letterOrange);
    drawPennant(frameBuffer, mirror * original, makeTone(astroPalette::letterOrange, 0.5));
}

void drawOrderPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 origin{panel.centerX() - 60.0, panel.centerY() + 60.0};
    const Matrix3 toOrigin = Matrix3::translation(origin);
    const Matrix3 rotate = Matrix3::rotation(-pi / 3.0);
    const Matrix3 move = Matrix3::translation(120.0, 0.0);
    // Axes at the shared origin.
    drawLineBresenham(frameBuffer, origin, origin + Vec2(170.0, 0.0), Color{90, 80, 120});
    drawLineBresenham(frameBuffer, origin, origin + Vec2(0.0, -170.0), Color{90, 80, 120});
    drawPennant(frameBuffer, toOrigin, Color{120, 120, 140});                          // original
    drawPennant(frameBuffer, toOrigin * move * rotate, astroPalette::letterMagenta);   // T * R: rotate, then move
    drawPennant(frameBuffer, toOrigin * rotate * move, astroPalette::letterYellow);    // R * T: move, then rotate
    markPoint(frameBuffer, origin);
    drawBitmapText(frameBuffer, "T*R", panel.left + 230, panel.top + 285, 2, astroPalette::letterMagenta);
    drawBitmapText(frameBuffer, "R*T", panel.left + 160, panel.top + 90, 2, astroPalette::letterYellow);
}

void drawHierarchyPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 hub{panel.centerX(), panel.centerY()};
    const double wheelAngle = 0.5;
    const Matrix3 wheel = Matrix3::translation(hub) * Matrix3::rotation(wheelAngle);
    drawCircle(frameBuffer, roundToInt(hub.x), roundToInt(hub.y), 90, astroPalette::neonPink);
    for (int spoke = 0; spoke < 6; ++spoke) {
        const Matrix3 spokeModel = wheel * Matrix3::rotation(spoke * pi / 3.0);
        const Vec2 mount = spokeModel.transformPoint({90.0, 0.0});
        drawLineBresenham(frameBuffer, hub, mount, astroPalette::steelLight);
        // Wrong: inheriting the full parent transform tilts the child (faint).
        const std::vector<Vec2> inherited = (spokeModel * Matrix3::translation(90.0, 0.0) * Matrix3::rotation(-spoke * pi / 3.0))
                                                .transformPoints({{-10.0, 0.0}, {10.0, 0.0}, {10.0, 16.0}, {-10.0, 16.0}});
        drawPolylineBresenham(frameBuffer, inherited, Color{200, 90, 120}, true);
        // Right: the gondola takes only the parent's POSITION, so it hangs level.
        fillPolygon(frameBuffer, Matrix3::translation(mount).transformPoints({{-10.0, 0.0}, {10.0, 0.0}, {10.0, 16.0}, {-10.0, 16.0}}),
                    hsvToRgb(spoke * 60.0, 0.7, 1.0));
    }
    fillCircle(frameBuffer, roundToInt(hub.x), roundToInt(hub.y), 6, astroPalette::gold);
}

/** @brief The 12 edges of a cube with corners at (+-1, +-1, +-1). */
const std::array<std::array<int, 2>, 12>& cubeEdges() {
    static const std::array<std::array<int, 2>, 12> edges = {{{0, 1}, {1, 3}, {3, 2}, {2, 0}, {4, 5}, {5, 7}, {7, 6}, {6, 4},
                                                              {0, 4}, {1, 5}, {2, 6}, {3, 7}}};
    return edges;
}

Vec3 cubeCorner(int index) {
    return {(index & 1) ? 1.0 : -1.0, (index & 2) ? 1.0 : -1.0, (index & 4) ? 1.0 : -1.0};
}

/**
 * @brief Draw a wireframe cube through a full model -> projection chain, mapping
 *        normalised device coordinates [-1, 1] to the panel (the VIEWPORT transform).
 */
void drawCube(FrameBuffer& frameBuffer, const Panel& panel, const Matrix4& modelViewProjection, double viewportHalfSize) {
    std::array<Vec2, 8> screen{};
    for (int corner = 0; corner < 8; ++corner) {
        const Vec3 ndc = modelViewProjection.transformPoint(cubeCorner(corner));
        // NDC y is UP, screen y is DOWN.
        screen[static_cast<std::size_t>(corner)] = {panel.centerX() + ndc.x * viewportHalfSize, panel.centerY() - ndc.y * viewportHalfSize};
    }
    for (const auto& edge : cubeEdges()) {
        const Color color = hsvToRgb(170.0 + (edge[0] + edge[1]) * 12.0, 0.7, 1.0);
        drawLineBresenham(frameBuffer, screen[static_cast<std::size_t>(edge[0])], screen[static_cast<std::size_t>(edge[1])], color);
    }
    for (const Vec2& point : screen) fillCircle(frameBuffer, roundToInt(point.x), roundToInt(point.y), 3, astroPalette::gold);
}

void drawOrthographicCubePanel(FrameBuffer& frameBuffer, const Panel& panel) {
    // model = Ry(35 deg) * Rx(25 deg); orthographic projection of a 6 x 6 box.
    const Matrix4 model = Matrix4::rotationY(35.0 * pi / 180.0) * Matrix4::rotationX(25.0 * pi / 180.0);
    const Matrix4 projection = Matrix4::orthographic(-3.0, 3.0, -3.0, 3.0, -10.0, 10.0);
    drawCube(frameBuffer, panel, projection * model, 140.0);
}

void drawPerspectivePanel(FrameBuffer& frameBuffer, const Panel& panel) {
    // Same cube, pushed 5 units in front of the camera, seen through a 60 degree perspective lens.
    const Matrix4 model = Matrix4::translation({0.0, 0.0, -5.0}) * Matrix4::rotationY(35.0 * pi / 180.0) *
                          Matrix4::rotationX(25.0 * pi / 180.0);
    const Matrix4 projection = Matrix4::perspective(60.0 * pi / 180.0, 1.0, 0.1, 50.0);

    // A ground grid receding into the distance: its parallel lines CONVERGE (one-point perspective).
    // Each line is clipped to the panel (Cohen-Sutherland) so it cannot spill into its neighbours.
    const ClipWindow panelWindow{panel.left + 1.0, panel.top + 40.0, panel.left + panelWidth - 2.0, panel.top + panelHeight - 2.0};
    for (int line = -4; line <= 4; ++line) {
        const Vec3 nearPoint = projection.transformPoint({line * 1.0, -2.0, -2.5});
        const Vec3 farPoint = projection.transformPoint({line * 1.0, -2.0, -40.0});
        Vec2 start{panel.centerX() + nearPoint.x * 140.0, panel.centerY() - nearPoint.y * 140.0};
        Vec2 end{panel.centerX() + farPoint.x * 140.0, panel.centerY() - farPoint.y * 140.0};
        if (clipLineCohenSutherland(start, end, panelWindow)) {
            drawLineDda(frameBuffer, roundToInt(start.x), roundToInt(start.y), roundToInt(end.x), roundToInt(end.y), Color{80, 70, 110});
        }
    }
    drawCube(frameBuffer, panel, projection * model, 140.0);
}

}  // namespace

void renderTransformShowcase(FrameBuffer& frameBuffer) {
    frameBuffer.clear(panelBackground);
    const struct {
        const char* title;
        void (*draw)(FrameBuffer&, const Panel&);
    } panels[8] = {
        {"TRANSLATION", drawTranslationPanel},     {"ROTATE ABOUT PIVOT", drawRotationPanel},
        {"SCALE ABOUT POINT", drawScalingPanel},   {"SHEAR + REFLECT", drawShearReflectPanel},
        {"ORDER: T*R VS R*T", drawOrderPanel},     {"HIERARCHY", drawHierarchyPanel},
        {"4X4 ROTATE (ORTHO)", drawOrthographicCubePanel}, {"PERSPECTIVE", drawPerspectivePanel},
    };
    for (int index = 0; index < 8; ++index) {
        const Panel panel = panelAt(index % 4, index / 4);
        drawPanelFrame(frameBuffer, panel, panels[index].title, panelFrameColor);
        panels[index].draw(frameBuffer, panel);
    }
}
