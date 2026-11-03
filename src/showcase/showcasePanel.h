/**
 * @file showcasePanel.h
 * @brief Shared layout for the demonstration images: a 4 x 2 grid of labelled 320 x 360 panels.
 */
#pragma once

#include <string>

#include "core/frameBuffer.h"
#include "raster/lines.h"
#include "text/bitmapFont.h"

constexpr int panelWidth = 320;
constexpr int panelHeight = 360;
inline constexpr Color panelBackground{18, 10, 36};
inline constexpr Color panelFrameColor{90, 60, 140};
inline constexpr Color panelLabelColor = astroPalette::goldLight;

/** @brief Pixel rectangle of one panel plus helpers for its centre. */
struct Panel {
    int left;
    int top;
    double centerX() const { return left + panelWidth * 0.5; }
    double centerY() const { return top + panelHeight * 0.5 + 16.0; }  // leave room for the title
};

inline Panel panelAt(int column, int row) { return {column * panelWidth, row * panelHeight}; }

/** @brief Draw the panel border with Bresenham lines and its title in the bitmap font. */
inline void drawPanelFrame(FrameBuffer& frameBuffer, const Panel& panel, const std::string& title, Color borderColor) {
    const int right = panel.left + panelWidth - 1;
    const int bottom = panel.top + panelHeight - 1;
    drawLineBresenham(frameBuffer, panel.left, panel.top, right, panel.top, borderColor);
    drawLineBresenham(frameBuffer, right, panel.top, right, bottom, borderColor);
    drawLineBresenham(frameBuffer, right, bottom, panel.left, bottom, borderColor);
    drawLineBresenham(frameBuffer, panel.left, bottom, panel.left, panel.top, borderColor);
    drawBitmapText(frameBuffer, title, panel.left + (panelWidth - measureBitmapText(title, 2)) / 2, panel.top + 12, 2,
                   panelLabelColor);
}
