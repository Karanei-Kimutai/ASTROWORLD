/**
 * @file astroworldScene.h
 * @brief Composes every component into the full ASTROWORLD park scene.
 */
#pragma once

#include "core/frameBuffer.h"

/** @brief Default output resolution (720p, 16:9). */
inline constexpr int sceneWidth = 1280;
inline constexpr int sceneHeight = 720;

/**
 * @brief Render the CP1 still of the park into the frame buffer.
 *
 * Drawing order (painter's algorithm, far to near):
 *   sky -> stars -> horizon glow -> searchlights -> skyline -> coaster ->
 *   Ferris wheel -> drop tower -> ground -> path -> head -> carousel ->
 *   crowd -> fence lights -> balloon vendor -> wordmark
 */
void renderAstroworldScene(FrameBuffer& frameBuffer);
