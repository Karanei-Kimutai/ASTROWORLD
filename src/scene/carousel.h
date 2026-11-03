/**
 * @file carousel.h
 * @brief The carousel: midpoint-ellipse platform and canopy, striped cone, horses on poles.
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct Carousel {
    Vec2 baseCenter;             // centre of the platform ellipse
    double radius = 100.0;       // platform radius (horizontal)
    double rotationAngle = 0.0;  // radians; fixed in CP1, animated in CP2

    void draw(FrameBuffer& frameBuffer) const;
};
