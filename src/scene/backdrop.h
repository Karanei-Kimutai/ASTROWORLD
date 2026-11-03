/**
 * @file backdrop.h
 * @brief Everything behind the rides: sky, clouds, stars, searchlights, skyline, fence and ground.
 *
 * Most functions take a nightAmount in [0, 1]:
 *   0 = the daytime album cover (blue sky, cumulus clouds, sandy plaza)
 *   1 = the night starfield from the logo artwork
 * Values in between blend the two, so CP2 can animate a day -> night cycle.
 */
#pragma once

#include <cstdint>

#include "core/frameBuffer.h"
#include "core/vec2.h"

/** @brief Sky gradient: top/mid/horizon colours interpolated between day and night, then down the screen. */
void drawSky(FrameBuffer& frameBuffer, int horizonY, double nightAmount);

/**
 * @brief Fluffy cumulus clouds, each a cluster of soft-edged (alpha-falloff) ellipses:
 *        a bluish shadow layer below and a white lit layer above.
 */
void drawClouds(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed, double opacity);

/**
 * @brief Star field: faint single pixels plus bright stars with 4-point diffraction
 *        flares (horizontal + vertical Bresenham lines whose alpha fades with distance).
 */
void drawStars(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed, double opacity);

/**
 * @brief A searchlight beam: a fan of DDA rays, each clipped to the screen with
 *        Cohen-Sutherland before rasterizing, blended at low alpha.
 * @param angleDegrees Beam direction, 90 = straight up, measured counter-clockwise from +x.
 */
void drawSearchlight(FrameBuffer& frameBuffer, const Vec2& origin, double angleDegrees, double spreadDegrees,
                     Color beamColor, double opacity);

/** @brief Distant silhouettes of tents, buildings and trees as one concave polygon (scan-line fill). */
void drawSkyline(FrameBuffer& frameBuffer, int horizonY, std::uint32_t seed, Color color);

/** @brief The tall black palisade fence along the horizon (rails, bars and posts as lines). */
void drawHorizonFence(FrameBuffer& frameBuffer, int horizonY, int fenceHeight);

/** @brief Sandy concrete plaza with scattered popcorn (album cover), darkening at night. */
void drawGround(FrameBuffer& frameBuffer, int horizonY, double nightAmount, std::uint32_t seed);
