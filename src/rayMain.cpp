/**
 * @file rayMain.cpp
 * @brief Offline ray tracer for the ASTROWORLD park (final phase).
 *
 * Renders the same ParkFrame the OpenGL renderer draws, but by tracing rays on
 * the CPU: Whitted ray tracing (shadows + recursive reflections) or Monte-Carlo
 * path tracing (global illumination), both with supersampling anti-aliasing.
 *
 * Usage: astroworldRay [options]
 *   --mode whitted|path     algorithm (default whitted)
 *   --samples n             samples per pixel (default 9 for Whitted, 64 for path tracing)
 *   --depth n               reflection depth / maximum bounces (default 4)
 *   --width w --height h    image size (default 1280 x 720)
 *   --time s                animation time (default 0 = day; 18 = night)
 *   --camera n              viewpoint preset, as in astroworldGl (default 0)
 *   --projection n          projection mode, as in astroworldGl (default 0 = perspective)
 *   --threads n             worker threads (default: all cores)
 *   --no-logo               leave out the postcard logo
 *   --output file.ppm       where to save (default output/raytrace_<mode>.ppm)
 */
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "animation/astroworldTimeline.h"
#include "park3d/orbitCamera.h"
#include "park3d/parkScene.h"
#include "raytrace/rayTracer.h"
#include "text/astroWordmark.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Draw the postcard logo and captions straight onto the ray-traced image (CP1 software renderer). */
void drawPostcardOverlay(FrameBuffer& image) {
    const double scale = image.getWidth() / 1280.0;
    WordmarkStyle style;
    style.letterHeight = 66.0 * scale;
    const double logoWidth = measureWordmark(style);
    const double logoLeft = (image.getWidth() - logoWidth) * 0.5;
    const int textScale = std::max(1, static_cast<int>(2 * scale + 0.5));
    auto caption = [&](const std::string& text, int left, int top) {
        drawBitmapText(image, text, left + textScale, top + textScale, textScale, astroPalette::black);
        drawBitmapText(image, text, left, top, textScale, astroPalette::white);
    };
    caption("GREETINGS FROM", static_cast<int>(logoLeft + 2), static_cast<int>(8 * scale));
    drawWordmark(image, {logoLeft, 30.0 * scale}, style);
    const std::string footer = "WISH YOU WERE HERE";
    caption(footer, (image.getWidth() - measureBitmapText(footer, textScale)) / 2, image.getHeight() - static_cast<int>(26 * scale));
}

}  // namespace

int main(int argumentCount, char** argumentValues) {
    RayTraceSettings settings;
    double time = 0.0;
    int cameraPreset = 0;
    int projection = 0;
    bool showLogo = true;
    bool samplesGiven = false;
    std::string outputPath;
    for (int index = 1; index < argumentCount; ++index) {
        const std::string argument = argumentValues[index];
        const bool hasValue = index + 1 < argumentCount;
        if (argument == "--mode" && hasValue) {
            settings.mode = std::string(argumentValues[++index]) == "path" ? TracingMode::pathTracing : TracingMode::whitted;
        } else if (argument == "--samples" && hasValue) {
            settings.samplesPerPixel = std::max(1, std::atoi(argumentValues[++index]));
            samplesGiven = true;
        } else if (argument == "--depth" && hasValue) settings.maxDepth = std::max(0, std::atoi(argumentValues[++index]));
        else if (argument == "--width" && hasValue) settings.width = std::max(16, std::atoi(argumentValues[++index]));
        else if (argument == "--height" && hasValue) settings.height = std::max(16, std::atoi(argumentValues[++index]));
        else if (argument == "--time" && hasValue) time = std::atof(argumentValues[++index]);
        else if (argument == "--camera" && hasValue) cameraPreset = std::atoi(argumentValues[++index]);
        else if (argument == "--projection" && hasValue) projection = std::clamp(std::atoi(argumentValues[++index]), 0, static_cast<int>(ProjectionMode::count) - 1);
        else if (argument == "--threads" && hasValue) settings.threadCount = std::atoi(argumentValues[++index]);
        else if (argument == "--output" && hasValue) outputPath = argumentValues[++index];
        else if (argument == "--no-logo") showLogo = false;
    }
    if (!samplesGiven && settings.mode == TracingMode::pathTracing) settings.samplesPerPixel = 64;
    const std::string modeName = settings.mode == TracingMode::whitted ? "whitted" : "path";
    if (outputPath.empty()) {
        std::filesystem::create_directories("output");
        outputPath = "output/raytrace_" + modeName + ".ppm";
    }

    // The same scene and camera the OpenGL renderer uses.
    const ParkScene scene;
    const ParkFrame frame = scene.frameAt(time, nightAmountAt(time));
    const CameraMatrices camera = OrbitCamera::preset(cameraPreset).matrices(static_cast<ProjectionMode>(projection),
                                                                              static_cast<double>(settings.width) / settings.height);

    std::cerr << "Ray tracing (" << modeName << ", " << settings.samplesPerPixel << " samples/pixel, depth " << settings.maxDepth << ", "
              << settings.width << "x" << settings.height << ", t = " << time << " s)\n";
    RayTraceStats stats;
    FrameBuffer image = rayTraceFrame(scene, frame, camera, settings, stats);
    if (showLogo) drawPostcardOverlay(image);

    std::cerr << "  " << stats.triangles << " triangles, BVH built in " << stats.buildSeconds << " s\n"
              << "  rendered in " << stats.renderSeconds << " s, " << stats.raysCast << " rays ("
              << static_cast<long>(stats.raysCast / std::max(stats.renderSeconds, 1e-9) / 1e6) << " M rays/s)\n";
    const bool saved = image.savePpm(outputPath);
    std::cerr << (saved ? "  wrote  " : "  FAILED ") << outputPath << "\n";
    return saved ? 0 : 1;
}
