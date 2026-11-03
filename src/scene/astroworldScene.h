/**
 * @file astroworldScene.h
 * @brief Composes every component into the full ASTROWORLD park scene.
 */
#pragma once

#include "core/frameBuffer.h"

/** @brief Default output resolution (720p, 16:9). */
inline constexpr int sceneWidth = 1280;
inline constexpr int sceneHeight = 720;

/** @brief Settings that change how the scene looks without moving anything. */
struct SceneSettings {
    /**
     * 0 = daytime album cover (blue sky, clouds, sunlit plaza),
     * 1 = night starfield (stars, searchlights, dark plaza, brighter portal).
     * In-between values blend the two; CP2 animates this for a day -> night cycle.
     */
    double nightAmount = 0.0;
};

/**
 * @brief Render the park into the frame buffer.
 *
 * Drawing order (painter's algorithm, far to near):
 *   sky -> stars / clouds -> searchlights -> skyline -> Ferris wheel, drop tower,
 *   coaster, sign, tent -> fence -> ground -> bunting -> head -> rocket ->
 *   carousel -> crowd -> balloon vendor -> logo and captions
 */
void renderAstroworldScene(FrameBuffer& frameBuffer, const SceneSettings& settings);
