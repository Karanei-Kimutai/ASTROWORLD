/**
 * @file ferrisWheel.h
 * @brief The Ferris wheel: midpoint-circle rims, Bresenham spokes, polygon gondolas,
 *        animated as a HIERARCHY of 2D transformations.
 *
 *   wheelModel   = T(hub) * R(rotationAngle)            the wheel turns about its hub
 *   spokeModel_i = wheelModel * R(i * 360 / count)     each spoke is a child of the wheel
 *   mountPoint_i = spokeModel_i * (radius, 0)           where gondola i hangs
 *   gondolaModel = T(mountPoint_i) * R(swing_i)        the gondola does NOT inherit the
 *                                                      wheel's rotation - it hangs level
 *                                                      and only swings a little
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

struct FerrisWheel {
    Vec2 hubPosition;            // centre of the wheel
    int radius = 170;            // outer rim radius in pixels
    int gondolaCount = 12;       // gondolas (and spokes) around the rim
    double rotationAngle = 0.0;  // radians (positive = clockwise on screen)
    double swingAngle = 0.0;     // radians: how far the gondolas currently swing on their hangers
    int groundY = 600;           // where the support legs meet the ground

    /** @brief Draw legs, spokes, rims, lights, hub and gondolas (back to front). */
    void draw(FrameBuffer& frameBuffer) const;
};
