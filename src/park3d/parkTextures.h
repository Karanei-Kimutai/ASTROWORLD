/**
 * @file parkTextures.h
 * @brief The park's surface textures, drawn by the CP1 software renderer.
 *
 * Shared by the OpenGL renderer (uploaded to the GPU) and the ray tracer
 * (sampled on the CPU), so both map exactly the same images.
 */
#pragma once

#include "core/frameBuffer.h"

/**
 * @brief One 4 x 4 m concrete slab: speckled concrete, darker expansion joints
 *        along its edges (so tiles line up into a slab grid) and scattered popcorn.
 */
FrameBuffer drawGroundTexture();

/** @brief The park sign's lettering. */
FrameBuffer drawSignTexture();

/**
 * @brief Bilinear texture lookup with wrap-around (GL_REPEAT behaviour) - the
 *        CPU equivalent of texture() in a shader. u, v in texture space (0..1 per tile),
 *        v = 0 at the TOP row.
 * @return Colour as 0..1 components in r, g, b.
 */
struct TextureSample {
    double r, g, b;
};
TextureSample sampleTextureBilinear(const FrameBuffer& texture, double u, double v);
