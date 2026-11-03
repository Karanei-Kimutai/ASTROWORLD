/**
 * @file dropTower.h
 * @brief The drop tower: a shaded column with a ring of seats and a beacon on top.
 */
#pragma once

#include "core/frameBuffer.h"

struct DropTower {
    double centerX = 400.0;
    double topY = 130.0;
    double groundY = 600.0;
    double columnWidth = 26.0;
    double seatY = 300.0;  // height of the seat ring; fixed in CP1, animated (drop!) in CP2

    void draw(FrameBuffer& frameBuffer) const;
};
