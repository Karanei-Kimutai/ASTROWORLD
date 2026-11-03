/**
 * @file rollerCoaster.h
 * @brief The roller coaster: a Catmull-Rom spline track with supports and a train.
 */
#pragma once

#include <vector>

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct RollerCoaster {
    std::vector<Vec2> controlPoints;  // points the track passes through (left to right)
    double groundY = 600.0;
    double trainDistance = 0.3;       // position of the lead car as a fraction of track length (CP2 animates this)
    int carCount = 3;

    void draw(FrameBuffer& frameBuffer) const;
};
