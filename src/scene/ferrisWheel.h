/**
 * @file ferrisWheel.h
 * @brief The Ferris wheel: midpoint-circle rims, Bresenham spokes, polygon gondolas.
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct FerrisWheel {
    Vec2 hubPosition;            // centre of the wheel
    int radius = 170;            // outer rim radius in pixels
    int gondolaCount = 12;       // gondolas (and spokes) around the rim
    double rotationAngle = 0.0;  // radians; fixed in CP1, animated in CP2
    int groundY = 600;           // where the support legs meet the ground

    /** @brief Draw legs, spokes, rims, lights, hub and gondolas (back to front). */
    void draw(FrameBuffer& frameBuffer) const;
};
