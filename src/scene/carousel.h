/**
 * @file carousel.h
 * @brief The carousel: midpoint-ellipse platform and canopy, striped cone, and horses
 *        positioned in 3D with 4x4 matrices (rotation about y, tilted view, parallel projection).
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct Carousel {
    Vec2 baseCenter;             // centre of the platform ellipse
    double radius = 100.0;       // platform radius (horizontal)
    double rotationAngle = 0.0;  // radians about the vertical axis
    double bobPhase = 0.0;       // radians: drives the horses' up-and-down gallop

    void draw(FrameBuffer& frameBuffer) const;
};
