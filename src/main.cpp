/**
 * @file main.cpp
 * @brief Command-line renderer: still images, or an animation as a sequence of PPM frames.
 *
 * Still images (default):
 *   astroworld [outputDirectory]                 (default "output")
 *     astroworldDay.ppm / astroworldNight.ppm    park scene at t = 0, day and night
 *     astroworldDay8Bit(.ppm / Dither.ppm)       colour-depth reduction of the day scene
 *     algorithmShowcase.ppm                      CP1 algorithms in isolation
 *     colorShowcase.ppm                          colour models and theory
 *     transformShowcase.ppm                      CP2 transformations (3x3 and 4x4)
 *
 * Animation:
 *   astroworld --animate <seconds> <fps> <outputDirectory | ->
 *     Renders frames at t = 0, 1/fps, 2/fps, ... and either writes
 *     frame_00000.ppm, frame_00001.ppm, ... into the directory, or, with "-",
 *     streams them back to back on stdout so they can be piped into ffmpeg
 *     without touching the disk (see tools/makeVideo.sh).
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "core/frameBuffer.h"
#include "scene/astroworldScene.h"
#include "showcase/algorithmShowcase.h"
#include "showcase/colorShowcase.h"
#include "showcase/transformShowcase.h"

namespace {

/** @brief Save a frame buffer and report the result on the console. */
bool saveAndReport(const FrameBuffer& frameBuffer, const std::filesystem::path& filePath) {
    const bool saved = frameBuffer.savePpm(filePath.string());
    std::cout << (saved ? "  wrote  " : "  FAILED ") << filePath.string() << "\n";
    return saved;
}

int renderStills(const std::filesystem::path& outputDirectory) {
    std::filesystem::create_directories(outputDirectory);
    std::cout << "ASTROWORLD - Checkpoint 2 (pure C++ software rasterizer)\n";
    bool allSaved = true;

    FrameBuffer dayScene(sceneWidth, sceneHeight);
    renderAstroworldScene(dayScene, SceneSettings{0.0, 0.0});
    allSaved &= saveAndReport(dayScene, outputDirectory / "astroworldDay.ppm");
    allSaved &= saveAndReport(dayScene.reducedTo8Bit(false), outputDirectory / "astroworldDay8Bit.ppm");
    allSaved &= saveAndReport(dayScene.reducedTo8Bit(true), outputDirectory / "astroworldDay8BitDither.ppm");

    FrameBuffer nightScene(sceneWidth, sceneHeight);
    renderAstroworldScene(nightScene, SceneSettings{1.0, 0.0});
    allSaved &= saveAndReport(nightScene, outputDirectory / "astroworldNight.ppm");

    FrameBuffer showcase(sceneWidth, sceneHeight);
    renderAlgorithmShowcase(showcase);
    allSaved &= saveAndReport(showcase, outputDirectory / "algorithmShowcase.ppm");
    renderColorShowcase(showcase);
    allSaved &= saveAndReport(showcase, outputDirectory / "colorShowcase.ppm");
    renderTransformShowcase(showcase);
    allSaved &= saveAndReport(showcase, outputDirectory / "transformShowcase.ppm");
    return allSaved ? 0 : 1;
}

int renderAnimation(double durationSeconds, int framesPerSecond, const std::string& destination) {
    const bool toStdout = destination == "-";
    if (!toStdout) std::filesystem::create_directories(destination);
    // Progress goes to stderr so it never mixes with frames streamed on stdout.
    const int frameCount = static_cast<int>(durationSeconds * framesPerSecond + 0.5);
    std::cerr << "Rendering " << frameCount << " frames (" << durationSeconds << " s at " << framesPerSecond << " fps)\n";

    FrameBuffer frame(sceneWidth, sceneHeight);
    const auto start = std::chrono::steady_clock::now();
    for (int index = 0; index < frameCount; ++index) {
        // Each frame is a pure function of its time stamp.
        renderAstroworldScene(frame, sampleAnimation(static_cast<double>(index) / framesPerSecond));
        if (toStdout) {
            if (!frame.writePpm(std::cout)) return 1;
        } else {
            char fileName[32];
            std::snprintf(fileName, sizeof(fileName), "frame_%05d.ppm", index);
            if (!frame.savePpm((std::filesystem::path(destination) / fileName).string())) return 1;
        }
        if ((index + 1) % framesPerSecond == 0 || index + 1 == frameCount) {
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            std::cerr << "\r  " << index + 1 << "/" << frameCount << " frames, " << static_cast<int>(1000.0 * elapsed / (index + 1))
                      << " ms/frame" << std::flush;
        }
    }
    std::cerr << "\n";
    return 0;
}

}  // namespace

int main(int argumentCount, char** argumentValues) {
    std::ios::sync_with_stdio(false);  // faster binary output on stdout
    if (argumentCount > 1 && std::string(argumentValues[1]) == "--animate") {
        if (argumentCount < 5) {
            std::cerr << "usage: astroworld --animate <seconds> <fps> <outputDirectory | ->\n";
            return 2;
        }
        return renderAnimation(std::atof(argumentValues[2]), std::max(1, std::atoi(argumentValues[3])), argumentValues[4]);
    }
    return renderStills(argumentCount > 1 ? argumentValues[1] : "output");
}
