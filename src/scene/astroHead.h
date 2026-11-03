/**
 * @file astroHead.h
 * @brief The giant brass inflatable head - the centrepiece and the entrance of ASTROWORLD.
 *
 * Matches the album cover: eyes closed, broad nose, long thick dreads arching
 * from the crown, and a wide-open mouth resting on the ground that forms a
 * glowing doorway visitors walk into.
 *
 * Every feature is defined in HEAD UNITS: (u, v) relative to the head centre,
 * scaled by 'scale' pixels. u = +-0.43 is the cheek width, v = -0.56 the crown
 * and v = +0.5 the ground line (the chin is "buried"). The whole head can be
 * moved or resized by changing two numbers; in CP2 a transformation matrix
 * replaces toScreen().
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct AstroHead {
    Vec2 center;                          // screen position of head unit (0, 0)
    double scale = 390.0;                 // pixels per head unit
    Color rimColor{120, 170, 230};        // environment reflected at the silhouette edge
    double glowStrength = 1.0;            // brightness of the light pouring out of the mouth

    void draw(FrameBuffer& frameBuffer) const;

    /** @brief Screen y of the ground line the mouth sits on. */
    double groundY() const { return center.y + 0.5 * scale; }

private:
    /** @brief Head units -> screen pixels (a scale followed by a translation). */
    Vec2 toScreen(double u, double v) const { return {center.x + u * scale, center.y + v * scale}; }

    void drawDread(FrameBuffer& frameBuffer, const Vec2* unitPoints, int pointCount, double side) const;
    void drawOuterDreads(FrameBuffer& frameBuffer) const;
    void drawEars(FrameBuffer& frameBuffer) const;
    void drawFace(FrameBuffer& frameBuffer) const;
    void drawClosedEyes(FrameBuffer& frameBuffer) const;
    void drawNose(FrameBuffer& frameBuffer) const;
    void drawMouthPortal(FrameBuffer& frameBuffer) const;
    void drawFrontDreads(FrameBuffer& frameBuffer) const;
};
