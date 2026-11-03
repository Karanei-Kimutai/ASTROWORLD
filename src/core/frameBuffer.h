/**
 * @file frameBuffer.h
 * @brief A CPU-side frame buffer: the block of memory that holds one image.
 *
 * On real hardware the frame buffer lives in VRAM and the display controller
 * scans it out to the monitor. Here it is an ordinary array in system RAM;
 * "display" means writing it to a PPM image file. Every algorithm in the
 * project ultimately ends in setPixel() on one of these.
 *
 * Memory layout: row-major, top row first. Pixel (x, y) is at index y * width + x.
 */
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/color.h"

/**
 * @brief A per-pixel colour function, the software version of a fragment shader.
 *
 * Fill routines decide WHICH pixels are covered (rasterization); a
 * PixelShader decides WHAT COLOUR each covered pixel gets (fragment processing).
 */
using PixelShader = std::function<Color(int x, int y)>;

class FrameBuffer {
public:
    /** @brief Allocate width * height pixels, all set to clearColor. */
    FrameBuffer(int widthValue, int heightValue, Color clearColor = Color{});

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    /** @brief True if (x, y) is inside the image. */
    bool contains(int x, int y) const;

    /**
     * @brief Write one pixel. Out-of-range coordinates are silently ignored.
     *
     * This bounds test is the last line of clipping in the pipeline (a
     * "scissor test"): geometry may extend beyond the screen, but nothing is
     * ever written outside the buffer's memory.
     */
    void setPixel(int x, int y, Color color);

    /** @brief Read one pixel (black if out of range). */
    Color getPixel(int x, int y) const;

    /**
     * @brief Alpha-blend a colour onto the existing pixel: dst = dst + (src - dst) * alpha.
     *
     * Used for glows, light beams and soft shadows (an output-merger operation).
     */
    void blendPixel(int x, int y, Color color, double alpha);

    /** @brief Set every pixel to one colour. */
    void clear(Color color);

    /** @brief Fill the horizontal run [xStart, xEnd] of row y with a solid colour (clipped). */
    void fillSpan(int y, int xStart, int xEnd, Color color);

    /** @brief Fill a horizontal run, asking the shader for each pixel's colour (clipped). */
    void fillSpan(int y, int xStart, int xEnd, const PixelShader& shader);

    /** @brief Alpha-blend a solid colour across a horizontal run (clipped). */
    void blendSpan(int y, int xStart, int xEnd, Color color, double alpha);

    /**
     * @brief Copy of this image reduced to 256 colours (3:3:2) and expanded back for viewing.
     *
     * @param useDithering If true, apply a 4x4 ordered (Bayer) dither before
     *        quantising, trading colour banding for a fine pattern.
     */
    FrameBuffer reducedTo8Bit(bool useDithering) const;

    /**
     * @brief Save as a binary PPM (P6) image - the simplest true-colour format there is:
     *        a short text header followed by raw RGB bytes.
     * @return false if the file could not be opened.
     */
    bool savePpm(const std::string& filePath) const;

private:
    int width;
    int height;
    std::vector<Color> pixels;
};
