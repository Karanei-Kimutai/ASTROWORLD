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
 * and v = +0.5 the ground line (the chin is "buried").
 *
 * Head units are mapped to the screen by a MODEL MATRIX (math/matrix3.h):
 *
 *   model = S_about(base, breathingScale) * T(center) * S(scale)
 *
 * read right to left: scale units to pixels, move to the head's position,
 * then "breathe" by scaling about the base point on the ground - a fixed-point
 * scaling T(base) * S * T(-base), so the head inflates upward and outward
 * while its mouth stays planted on the ground.
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"
#include "math/matrix3.h"

struct AstroHead {
    Vec2 center;                          // screen position of head unit (0, 0)
    double scale = 390.0;                 // pixels per head unit
    Color rimColor{120, 170, 230};        // environment reflected at the silhouette edge
    double glowStrength = 1.0;            // brightness of the light pouring out of the mouth
    double breathingScale = 1.0;          // inflatable "breathing": 1 = at rest
    double dreadSwayAngle = 0.0;          // radians: how far the dreads currently bend in the wind

    void draw(FrameBuffer& frameBuffer) const;

    /** @brief Screen y of the ground line the mouth sits on. */
    double groundY() const { return center.y + 0.5 * scale; }

private:
    /** @brief The model matrix described above. */
    Matrix3 modelMatrix() const {
        const Vec2 basePoint{center.x, groundY()};
        return Matrix3::scalingAbout(basePoint, breathingScale, breathingScale) * Matrix3::translation(center) *
               Matrix3::scaling(scale);
    }

    /** @brief Head units -> screen pixels through the model matrix. */
    Vec2 toScreen(double u, double v) const { return modelMatrix().transformPoint({u, v}); }

    /** @brief Length in pixels of one head unit after the model transform (for radii). */
    double pixelsPerUnit() const { return modelMatrix().transformVector({1.0, 0.0}).length(); }

    /**
     * @brief One dread from its unit-space centre line.
     * @param side       +1 right, -1 left (mirrors u).
     * @param swayWeight How strongly this dread bends with dreadSwayAngle.
     */
    void drawDread(FrameBuffer& frameBuffer, const Vec2* unitPoints, int pointCount, double side, double swayWeight) const;
    void drawOuterDreads(FrameBuffer& frameBuffer) const;
    void drawEars(FrameBuffer& frameBuffer) const;
    void drawFace(FrameBuffer& frameBuffer) const;
    void drawClosedEyes(FrameBuffer& frameBuffer) const;
    void drawNose(FrameBuffer& frameBuffer) const;
    void drawMouthPortal(FrameBuffer& frameBuffer) const;
    void drawFrontDreads(FrameBuffer& frameBuffer) const;
};
