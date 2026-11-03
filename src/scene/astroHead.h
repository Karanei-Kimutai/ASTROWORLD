/**
 * @file astroHead.h
 * @brief The giant golden inflatable head - the centrepiece and entrance of ASTROWORLD.
 *
 * Every feature is defined in HEAD UNITS: (u, v) relative to the head centre,
 * scaled by 'scale' pixels. u = +-0.44 is roughly the cheek width, v = -0.52
 * the crown and v = +0.52 the chin. Defining the shape this way means the
 * whole head can be moved or resized by changing two numbers - and in CP2 a
 * transformation matrix will replace toScreen().
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct AstroHead {
    Vec2 center;           // screen position of head unit (0, 0)
    double scale = 400.0;  // pixels per head unit

    void draw(FrameBuffer& frameBuffer) const;

private:
    /** @brief Head units -> screen pixels (a scale followed by a translation). */
    Vec2 toScreen(double u, double v) const { return {center.x + u * scale, center.y + v * scale}; }

    void drawBackHair(FrameBuffer& frameBuffer) const;
    void drawEars(FrameBuffer& frameBuffer) const;
    void drawFace(FrameBuffer& frameBuffer) const;
    void drawHairCap(FrameBuffer& frameBuffer) const;
    void drawFrontHair(FrameBuffer& frameBuffer) const;
    void drawEyesAndBrows(FrameBuffer& frameBuffer) const;
    void drawNose(FrameBuffer& frameBuffer) const;
    void drawMouth(FrameBuffer& frameBuffer) const;
};
