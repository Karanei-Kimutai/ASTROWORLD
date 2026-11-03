/**
 * @file carnivalProps.h
 * @brief Props from the album cover: the rocket statue, striped big-top tent,
 *        the ASTROWORLD sign on posts, and bunting (pennant flags).
 */
#pragma once

#include "core/frameBuffer.h"
#include "core/vec2.h"

/** @brief Silver rocket with red bands, nose cone and fins, standing on a plinth. */
void drawRocket(FrameBuffer& frameBuffer, const Vec2& baseCenter, double height);

/** @brief Striped circus big top: curved roof, scalloped valance, striped walls and a flag. */
void drawBigTopTent(FrameBuffer& frameBuffer, const Vec2& baseCenter, double width, double height);

/** @brief "ASTROWORLD" park sign on two posts, with a bulb border. */
void drawAstroworldSign(FrameBuffer& frameBuffer, const Vec2& topLeft, double width, double height, int groundY);

/**
 * @brief A string of triangular pennants hanging along a sagging cubic Bezier.
 * @param sag        How far the middle of the string droops below the straight line.
 * @param colorShift Rotates the colour sequence so neighbouring strings differ.
 */
void drawBunting(FrameBuffer& frameBuffer, const Vec2& start, const Vec2& end, double sag, int flagCount,
                 int colorShift);
