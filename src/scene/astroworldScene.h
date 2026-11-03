/**
 * @file astroworldScene.h
 * @brief Composes every component into the full ASTROWORLD park scene, at any moment in time.
 */
#pragma once

#include "animation/astroworldTimeline.h"
#include "core/frameBuffer.h"

/** @brief Default output resolution (720p, 16:9). */
inline constexpr int sceneWidth = 1280;
inline constexpr int sceneHeight = 720;

/** @brief Everything that decides what one frame looks like. */
struct SceneSettings {
    /**
     * 0 = daytime album cover (blue sky, clouds, sunlit plaza),
     * 1 = night starfield (stars, searchlights, dark plaza, brighter portal).
     * In-between values blend the two.
     */
    double nightAmount = 0.0;

    /** Animation clock: every moving part is a pure function of this time. */
    double timeSeconds = 0.0;
};

/**
 * @brief Settings for a moment of the looping animation: the time, plus a
 *        nightAmount taken from the day/night keyframe track.
 */
SceneSettings sampleAnimation(double timeSeconds);

/**
 * @brief Render the park into the frame buffer.
 *
 * Every frame is a PURE FUNCTION of the settings - nothing is carried over from
 * the previous frame - so any frame can be rendered on its own, in any order
 * (useful for exporting video and for scrubbing the timeline in the window).
 *
 * Drawing order (painter's algorithm, far to near):
 *   sky -> stars / clouds -> searchlights -> skyline -> Ferris wheel, drop tower,
 *   coaster, sign, tent -> fence -> ground -> bunting -> head -> rocket ->
 *   carousel -> crowd -> balloon vendor -> logo and captions
 */
void renderAstroworldScene(FrameBuffer& frameBuffer, const SceneSettings& settings);
