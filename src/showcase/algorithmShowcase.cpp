/**
 * @file algorithmShowcase.cpp
 * @brief Algorithm demonstration panels.
 */
#include "showcase/algorithmShowcase.h"

#include <cmath>
#include <string>
#include <vector>

#include "core/simpleRandom.h"
#include "core/vec2.h"
#include "curves/bezier.h"
#include "curves/spline.h"
#include "fill/boundaryFill.h"
#include "fill/scanLineFill.h"
#include "raster/circles.h"
#include "raster/clipping.h"
#include "raster/lines.h"
#include "showcase/showcasePanel.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Panels 1 and 2: a starburst of lines in every octant, coloured around the hue wheel. */
void drawStarburst(FrameBuffer& frameBuffer, const Panel& panel, bool useBresenham) {
    const int rayCount = 24;
    for (int ray = 0; ray < rayCount; ++ray) {
        const double angle = ray * 2.0 * pi / rayCount;
        const int endX = roundToInt(panel.centerX() + std::cos(angle) * 125.0);
        const int endY = roundToInt(panel.centerY() + std::sin(angle) * 125.0);
        const Color color = hsvToRgb(ray * 360.0 / rayCount, 0.8, 1.0);
        if (useBresenham) {
            drawLineBresenham(frameBuffer, roundToInt(panel.centerX()), roundToInt(panel.centerY()), endX, endY, color);
        } else {
            drawLineDda(frameBuffer, roundToInt(panel.centerX()), roundToInt(panel.centerY()), endX, endY, color);
        }
    }
}

void drawCirclePanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const int centerX = roundToInt(panel.centerX());
    const int centerY = roundToInt(panel.centerY());
    for (int ring = 1; ring <= 6; ++ring) {
        drawCircle(frameBuffer, centerX, centerY, ring * 20, hsvToRgb(280.0 + ring * 18.0, 0.7, 1.0));
    }
    drawEllipse(frameBuffer, centerX, centerY, 140, 50, astroPalette::neonCyan);
    drawEllipse(frameBuffer, centerX, centerY, 50, 140, astroPalette::neonOrange);
    fillCircle(frameBuffer, centerX, centerY, 10, astroPalette::gold);
}

void drawBezierPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 origin{double(panel.left), double(panel.top)};
    const std::vector<Vec2> controls = {origin + Vec2(40, 300), origin + Vec2(70, 80), origin + Vec2(250, 90),
                                        origin + Vec2(285, 290)};

    // Control polygon (dim) and the curve itself.
    drawPolylineBresenham(frameBuffer, controls, Color{90, 80, 120});
    drawPolylineBresenham(frameBuffer, sampleBezierCurve(controls, 80), astroPalette::neonPink);

    // de Casteljau construction at t = 0.5: each level interpolates the previous one.
    const double t = 0.5;
    std::vector<Vec2> level = controls;
    const Color levelColors[3] = {astroPalette::neonCyan, astroPalette::gold, astroPalette::white};
    for (int depth = 0; level.size() > 1; ++depth) {
        std::vector<Vec2> next;
        for (std::size_t index = 0; index + 1 < level.size(); ++index) next.push_back(lerpVec2(level[index], level[index + 1], t));
        if (next.size() > 1) drawPolylineBresenham(frameBuffer, next, levelColors[depth % 3]);
        for (const Vec2& point : next) fillCircle(frameBuffer, roundToInt(point.x), roundToInt(point.y), 3, levelColors[depth % 3]);
        level = next;
    }
    for (const Vec2& control : controls) fillCircle(frameBuffer, roundToInt(control.x), roundToInt(control.y), 5, astroPalette::neonOrange);
}

void drawSplinePanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const Vec2 origin{double(panel.left), double(panel.top)};
    const std::vector<Vec2> points = {origin + Vec2(30, 280), origin + Vec2(80, 120), origin + Vec2(140, 250),
                                      origin + Vec2(190, 90), origin + Vec2(240, 220), origin + Vec2(290, 110)};
    drawPolylineBresenham(frameBuffer, points, Color{90, 80, 120});
    drawPolylineBresenham(frameBuffer, sampleCatmullRomSpline(points, 24, false), astroPalette::neonCyan);
    // The spline passes THROUGH every point (unlike Bezier inner control points).
    for (const Vec2& point : points) fillCircle(frameBuffer, roundToInt(point.x), roundToInt(point.y), 5, astroPalette::gold);
}

void drawScanLinePanel(FrameBuffer& frameBuffer, const Panel& panel) {
    // Concave 5-pointed star (10 vertices alternating outer/inner radius) with a gradient shader.
    const Vec2 starCenter{panel.centerX() - 70.0, panel.centerY() - 10.0};
    std::vector<Vec2> star;
    for (int vertex = 0; vertex < 10; ++vertex) {
        const double angle = -pi / 2.0 + vertex * pi / 5.0;
        const double radius = vertex % 2 == 0 ? 75.0 : 30.0;
        star.push_back(starCenter + Vec2(std::cos(angle), std::sin(angle)) * radius);
    }
    const double starTop = starCenter.y - 75.0;
    fillPolygon(frameBuffer, star, [starTop](int, int y) {
        return lerpColorHsv(astroPalette::gold, astroPalette::neonPink, (y - starTop) / 140.0);
    });

    // COMPLEX (self-intersecting) pentagram - even-odd rule leaves the centre hollow.
    const Vec2 pentagramCenter{panel.centerX() + 75.0, panel.centerY() + 40.0};
    std::vector<Vec2> pentagram;
    for (int vertex = 0; vertex < 5; ++vertex) {
        const double angle = -pi / 2.0 + vertex * 4.0 * pi / 5.0;  // skip every other point
        pentagram.push_back(pentagramCenter + Vec2(std::cos(angle), std::sin(angle)) * 70.0);
    }
    fillPolygon(frameBuffer, pentagram, astroPalette::neonCyan);
    drawPolylineBresenham(frameBuffer, pentagram, astroPalette::white, true);
}

void drawBoundaryFillPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    // Panel border and divider are drawn in the boundary colour so an 8-connected leak stays inside its half.
    const Color boundary = astroPalette::white;
    const int middleX = panel.left + panelWidth / 2;
    drawLineBresenham(frameBuffer, middleX, panel.top + 40, middleX, panel.top + panelHeight - 1, boundary);
    drawLineBresenham(frameBuffer, panel.left, panel.top + 40, panel.left + panelWidth - 1, panel.top + 40, boundary);

    // Two identical diamonds whose edges are DIAGONAL Bresenham lines (8-connected outlines).
    for (int half = 0; half < 2; ++half) {
        const int centerX = panel.left + panelWidth / 4 + half * panelWidth / 2;
        const int centerY = roundToInt(panel.centerY()) + 10;
        const int size = 55;
        drawLineBresenham(frameBuffer, centerX, centerY - size, centerX + size, centerY, boundary);
        drawLineBresenham(frameBuffer, centerX + size, centerY, centerX, centerY + size, boundary);
        drawLineBresenham(frameBuffer, centerX, centerY + size, centerX - size, centerY, boundary);
        drawLineBresenham(frameBuffer, centerX - size, centerY, centerX, centerY - size, boundary);
        if (half == 0) {
            boundaryFill4(frameBuffer, centerX, centerY, astroPalette::neonPink, boundary);  // stays inside
        } else {
            boundaryFill8(frameBuffer, centerX, centerY, astroPalette::neonOrange, boundary);  // leaks diagonally
        }
    }
    drawBitmapText(frameBuffer, "4-CONN", panel.left + 44, panel.top + panelHeight - 30, 2, panelLabelColor);
    drawBitmapText(frameBuffer, "8-CONN", panel.left + panelWidth / 2 + 44, panel.top + panelHeight - 30, 2, astroPalette::ink);
}

void drawClippingPanel(FrameBuffer& frameBuffer, const Panel& panel) {
    const ClipWindow window{panel.left + 70.0, panel.top + 110.0, panel.left + 250.0, panel.top + 280.0};
    // The clip window itself.
    drawPolylineBresenham(frameBuffer,
                          {{window.xMin, window.yMin}, {window.xMax, window.yMin}, {window.xMax, window.yMax}, {window.xMin, window.yMax}},
                          astroPalette::white, true);

    SimpleRandom random(5u);
    for (int line = 0; line < 16; ++line) {
        Vec2 start{panel.left + random.nextRange(10.0, 310.0), panel.top + random.nextRange(50.0, 350.0)};
        Vec2 end{panel.left + random.nextRange(10.0, 310.0), panel.top + random.nextRange(50.0, 350.0)};
        // Full line in dim red = the part that would be thrown away...
        drawLineDda(frameBuffer, roundToInt(start.x), roundToInt(start.y), roundToInt(end.x), roundToInt(end.y), Color{110, 30, 50});
        // ...then the clipped segment in bright green on top.
        if (clipLineCohenSutherland(start, end, window)) drawLineBresenham(frameBuffer, start, end, Color{80, 255, 140});
    }
}

}  // namespace

void renderAlgorithmShowcase(FrameBuffer& frameBuffer) {
    frameBuffer.clear(panelBackground);

    // Boundary-fill panel goes first: its 8-connected fill floods the right half,
    // so its frame must be drawn in the boundary colour before filling.
    const Panel boundaryPanel = panelAt(2, 1);
    drawPanelFrame(frameBuffer, boundaryPanel, "", astroPalette::white);
    drawBoundaryFillPanel(frameBuffer, boundaryPanel);
    drawBitmapText(frameBuffer, "BOUNDARY FILL", boundaryPanel.left + (panelWidth - measureBitmapText("BOUNDARY FILL", 2)) / 2,
                   boundaryPanel.top + 12, 2, panelLabelColor);

    drawPanelFrame(frameBuffer, panelAt(0, 0), "DDA LINE", panelFrameColor);
    drawStarburst(frameBuffer, panelAt(0, 0), false);
    drawPanelFrame(frameBuffer, panelAt(1, 0), "BRESENHAM LINE", panelFrameColor);
    drawStarburst(frameBuffer, panelAt(1, 0), true);
    drawPanelFrame(frameBuffer, panelAt(2, 0), "MIDPOINT CIRCLE+ELLIPSE", panelFrameColor);
    drawCirclePanel(frameBuffer, panelAt(2, 0));
    drawPanelFrame(frameBuffer, panelAt(3, 0), "BEZIER (DE CASTELJAU)", panelFrameColor);
    drawBezierPanel(frameBuffer, panelAt(3, 0));
    drawPanelFrame(frameBuffer, panelAt(0, 1), "CATMULL-ROM SPLINE", panelFrameColor);
    drawSplinePanel(frameBuffer, panelAt(0, 1));
    drawPanelFrame(frameBuffer, panelAt(1, 1), "SCAN-LINE FILL", panelFrameColor);
    drawScanLinePanel(frameBuffer, panelAt(1, 1));
    drawPanelFrame(frameBuffer, panelAt(3, 1), "COHEN-SUTHERLAND", panelFrameColor);
    drawClippingPanel(frameBuffer, panelAt(3, 1));
}
