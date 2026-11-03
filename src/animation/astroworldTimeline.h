/**
 * @file astroworldTimeline.h
 * @brief The park's shared animation timeline, used by BOTH renderers.
 *
 * The software renderer (CP1/CP2) and the OpenGL renderer (CP3) ask the same
 * functions where things should be at time t, so the two always agree:
 * same day/night cycle, same drop-tower ride, same coaster physics.
 */
#pragma once

#include <vector>

#include "core/vec2.h"

/** @brief Length of one full animation loop (day -> dusk -> night -> dawn), in seconds. */
inline constexpr double animationLoopSeconds = 30.0;

/** @brief 0 = day, 1 = night, from smoothstep keyframes over the 30 s loop. */
double nightAmountAt(double timeSeconds);

/**
 * @brief Drop tower seat position, 0 = bottom, 1 = top, over a 9 s ride cycle:
 *        ease-in-out climb, pause at the top, accelerating drop that bounces.
 */
double dropTowerLiftAt(double timeSeconds);

/**
 * @brief The coaster's side profile in the 1280x720 scene's pixel space
 *        (x right, y down): the points the track spline passes through.
 */
const std::vector<Vec2>& coasterProfilePoints();

/** @brief The ground line (pixel y) the coaster profile stands on. */
inline constexpr double coasterProfileGroundY = 500.0;

/**
 * @brief Lead car position along the track (fraction 0..1) at time t: chain lift,
 *        then speed from energy conservation (see RollerCoaster::trainFractionAtTime).
 */
double coasterTrainFractionAt(double timeSeconds);
