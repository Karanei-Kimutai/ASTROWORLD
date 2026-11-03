/**
 * @file main.cpp
 * @brief Checkpoint 1 entry point: renders every CP1 image to PPM files in output/.
 *
 * Outputs:
 *   output/astroworldDay.ppm              park scene as on the album cover, 24-bit colour
 *   output/astroworldNight.ppm            the same park at night (starfield, searchlights)
 *   output/astroworldDay8Bit.ppm          day scene reduced to 256 colours (3:3:2)
 *   output/astroworldDay8BitDither.ppm    reduced with ordered dithering
 *   output/algorithmShowcase.ppm          each algorithm in isolation
 *   output/colorShowcase.ppm              colour models and theory
 *
 * Usage: astroworld [outputDirectory]   (default "output")
 */
#include <filesystem>
#include <iostream>
#include <string>

#include "core/frameBuffer.h"
#include "scene/astroworldScene.h"
#include "showcase/algorithmShowcase.h"
#include "showcase/colorShowcase.h"

namespace {

/** @brief Save a frame buffer and report the result on the console. */
bool saveAndReport(const FrameBuffer& frameBuffer, const std::filesystem::path& filePath) {
    const bool saved = frameBuffer.savePpm(filePath.string());
    std::cout << (saved ? "  wrote  " : "  FAILED ") << filePath.string() << "\n";
    return saved;
}

}  // namespace

int main(int argumentCount, char** argumentValues) {
    const std::filesystem::path outputDirectory = argumentCount > 1 ? argumentValues[1] : "output";
    std::filesystem::create_directories(outputDirectory);

    std::cout << "ASTROWORLD - Checkpoint 1 (pure C++ software rasterizer)\n";
    bool allSaved = true;

    FrameBuffer dayScene(sceneWidth, sceneHeight);
    renderAstroworldScene(dayScene, SceneSettings{0.0});
    allSaved &= saveAndReport(dayScene, outputDirectory / "astroworldDay.ppm");
    allSaved &= saveAndReport(dayScene.reducedTo8Bit(false), outputDirectory / "astroworldDay8Bit.ppm");
    allSaved &= saveAndReport(dayScene.reducedTo8Bit(true), outputDirectory / "astroworldDay8BitDither.ppm");

    FrameBuffer nightScene(sceneWidth, sceneHeight);
    renderAstroworldScene(nightScene, SceneSettings{1.0});
    allSaved &= saveAndReport(nightScene, outputDirectory / "astroworldNight.ppm");

    FrameBuffer algorithms(sceneWidth, sceneHeight);
    renderAlgorithmShowcase(algorithms);
    allSaved &= saveAndReport(algorithms, outputDirectory / "algorithmShowcase.ppm");

    FrameBuffer colors(sceneWidth, sceneHeight);
    renderColorShowcase(colors);
    allSaved &= saveAndReport(colors, outputDirectory / "colorShowcase.ppm");

    return allSaved ? 0 : 1;
}
