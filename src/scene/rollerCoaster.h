/**
 * @file rollerCoaster.h
 * @brief The roller coaster: a Catmull-Rom spline track with a lattice truss, and a train
 *        whose cars are rotated onto the track with 2D transformation matrices.
 */
#pragma once

#include <vector>

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct RollerCoaster {
    std::vector<Vec2> controlPoints;  // points the track passes through (left to right)
    double groundY = 600.0;
    double trainDistance = 0.3;       // position of the lead car as a fraction of track length
    int carCount = 3;

    void draw(FrameBuffer& frameBuffer) const;

    /**
     * @brief Where the lead car is (fraction of track length) at a given time.
     *
     * Physically based: a slow constant-speed chain lift to the first summit,
     * then speed from conservation of energy (fast in the dips, slow over the
     * crests). The ride loops when the train reaches the end of the track.
     */
    double trainFractionAtTime(double timeSeconds) const;
};
