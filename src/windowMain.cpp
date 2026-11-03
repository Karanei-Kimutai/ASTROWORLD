/**
 * @file windowMain.cpp
 * @brief Live SDL2 viewer: plays the ASTROWORLD animation in real time.
 *
 * All drawing is still done by our own pure C++ rasterizer into a FrameBuffer.
 * SDL2 does only the last pipeline stage, DISPLAY: each finished frame is
 * uploaded to a streaming texture and presented in a window.
 *
 * Animation clock: every frame the real elapsed time (scaled by the playback
 * speed) is added to the animation time, and the scene is rendered for that
 * time. Because each frame is a pure function of time, the animation runs at
 * the correct speed whatever the frame rate, and seeking is instant.
 *
 * Controls:
 *   SPACE           play / pause
 *   LEFT / RIGHT    seek -1 s / +1 s
 *   UP / DOWN       playback speed x2 / /2   (0.25x .. 4x)
 *   1 / 2           jump to day (t = 0) / night (t = 18 s)
 *   5 / 6           show the scene reduced to 8-bit / 8-bit dithered
 *   3 / 4 / 7       algorithm / colour / transformation showcase
 *   S               save the current image to output/
 *   H               show / hide the help overlay
 *   ESC / Q         quit
 *
 * Usage: astroworldWindow [--time seconds] [--capture file.ppm] [--with-help]
 *   --time      starting animation time
 *   --capture   renders one frame, reads it BACK from the renderer
 *               (SDL_RenderReadPixels), saves it and exits - used for automated checks
 *   --with-help keeps the help overlay in the captured frame (hidden by default)
 */
#define SDL_MAIN_HANDLED  // we provide our own main(); no SDL2main shim needed
#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "core/frameBuffer.h"
#include "scene/astroworldScene.h"
#include "showcase/algorithmShowcase.h"
#include "showcase/colorShowcase.h"
#include "showcase/transformShowcase.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Which image the window is showing. */
enum class ViewMode { scene, scene8Bit, scene8BitDither, algorithms, colors, transforms };

/** @brief Everything the user can change from the keyboard. */
struct ViewerState {
    ViewMode mode = ViewMode::scene;
    double animationTime = 0.0;  // seconds on the animation clock
    double playbackSpeed = 1.0;
    bool playing = true;
    bool showHelp = true;
};

bool isSceneMode(ViewMode mode) {
    return mode == ViewMode::scene || mode == ViewMode::scene8Bit || mode == ViewMode::scene8BitDither;
}

/** @brief Render the image for the current state with the pure C++ renderer. */
void renderView(const ViewerState& state, FrameBuffer& image) {
    switch (state.mode) {
        case ViewMode::scene:
        case ViewMode::scene8Bit:
        case ViewMode::scene8BitDither:
            renderAstroworldScene(image, sampleAnimation(state.animationTime));
            if (state.mode == ViewMode::scene8Bit) image = image.reducedTo8Bit(false);
            if (state.mode == ViewMode::scene8BitDither) image = image.reducedTo8Bit(true);
            return;
        case ViewMode::algorithms: renderAlgorithmShowcase(image); return;
        case ViewMode::colors: renderColorShowcase(image); return;
        case ViewMode::transforms: renderTransformShowcase(image); return;
    }
}

/** @brief Short description of the view, for the title bar, overlay and saved file names. */
std::string describeView(const ViewerState& state, bool forFileName) {
    char timeText[32];
    std::snprintf(timeText, sizeof(timeText), forFileName ? "t%06.2f" : "T %5.1fS", state.animationTime);
    const std::string scenePart = std::string(forFileName ? "scene_" : "SCENE ") + timeText;
    switch (state.mode) {
        case ViewMode::scene: return scenePart;
        case ViewMode::scene8Bit: return scenePart + (forFileName ? "_8bit" : " 8-BIT");
        case ViewMode::scene8BitDither: return scenePart + (forFileName ? "_8bitDither" : " DITHER");
        case ViewMode::algorithms: return forFileName ? "algorithms" : "ALGORITHM SHOWCASE";
        case ViewMode::colors: return forFileName ? "colors" : "COLOUR SHOWCASE";
        case ViewMode::transforms: return forFileName ? "transforms" : "TRANSFORM SHOWCASE";
    }
    return "";
}

/** @brief Draw the controls panel: a translucent box (blended spans) with bitmap-font text. */
void drawHelpOverlay(FrameBuffer& frameBuffer, const ViewerState& state, double framesPerSecond) {
    char statusText[64];
    std::snprintf(statusText, sizeof(statusText), "%s X%.2f %2.0f FPS", state.playing ? "PLAY" : "PAUSE", state.playbackSpeed,
                  framesPerSecond);
    const std::vector<std::string> lines = {
        "SPACE PLAY/PAUSE",     "LEFT/RIGHT SEEK 1S", "UP/DOWN SPEED",      "1 DAY  2 NIGHT",
        "5 8-BIT  6 DITHER",    "3 ALGO 4 COLOUR 7 XFORM", "S SAVE H HELP ESC QUIT", "",
        describeView(state, false), statusText,
    };
    const int pixelScale = 2;
    const int lineHeight = 9 * pixelScale;
    const int boxLeft = 12;
    const int boxWidth = 320;
    const int boxHeight = static_cast<int>(lines.size()) * lineHeight + 16;
    const int boxTop = sceneHeight - 12 - boxHeight;
    for (int y = boxTop; y < boxTop + boxHeight; ++y) frameBuffer.blendSpan(y, boxLeft, boxLeft + boxWidth, astroPalette::black, 0.6);
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const Color textColor = index + 2 >= lines.size() ? astroPalette::letterYellow : astroPalette::white;
        drawBitmapText(frameBuffer, lines[index], boxLeft + 10, boxTop + 8 + static_cast<int>(index) * lineHeight, pixelScale, textColor);
    }
}

/**
 * @brief Read the presented frame back from the renderer and save it as PPM.
 *
 * This copies pixels from the display side (possibly GPU memory) back to
 * system memory - the reverse of the upload done every frame.
 */
bool captureRenderer(SDL_Renderer* renderer, const std::string& filePath) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(sceneWidth) * sceneHeight * 3);
    SDL_Rect area{0, 0, sceneWidth, sceneHeight};
    if (SDL_RenderReadPixels(renderer, &area, SDL_PIXELFORMAT_RGB24, pixels.data(), sceneWidth * 3) != 0) {
        std::cerr << "SDL_RenderReadPixels failed: " << SDL_GetError() << "\n";
        return false;
    }
    std::ofstream file(filePath, std::ios::binary);
    if (!file) return false;
    file << "P6\n" << sceneWidth << " " << sceneHeight << "\n255\n";
    file.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    return static_cast<bool>(file);
}

}  // namespace

int main(int argumentCount, char** argumentValues) {
    std::string capturePath;
    bool captureWithHelp = false;
    ViewerState state;
    for (int index = 1; index < argumentCount; ++index) {
        const std::string argument = argumentValues[index];
        if (argument == "--capture" && index + 1 < argumentCount) capturePath = argumentValues[++index];
        if (argument == "--with-help") captureWithHelp = true;
        if (argument == "--time" && index + 1 < argumentCount) state.animationTime = std::max(0.0, std::atof(argumentValues[++index]));
    }
    if (!capturePath.empty()) state.showHelp = captureWithHelp;

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("ASTROWORLD - Checkpoint 2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          sceneWidth, sceneHeight, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }
    // Prefer a hardware renderer, fall back to SDL's software renderer.
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    // Keep the 16:9 image undistorted when the window is resized (letterboxing).
    SDL_RenderSetLogicalSize(renderer, sceneWidth, sceneHeight);
    // RGB24 = 3 bytes per pixel in R, G, B order - exactly our Color layout, so the
    // frame buffer can be uploaded without any conversion.
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, sceneWidth, sceneHeight);

    FrameBuffer currentImage(sceneWidth, sceneHeight);
    bool needsRender = true;
    bool running = true;
    double framesPerSecond = 0.0;
    Uint64 previousTicks = SDL_GetPerformanceCounter();
    const double ticksPerSecond = static_cast<double>(SDL_GetPerformanceFrequency());

    while (running) {
        // Static views (showcases, paused scene) sleep until an event arrives.
        const bool animating = isSceneMode(state.mode) && state.playing && capturePath.empty();
        if (!animating && !needsRender) SDL_WaitEvent(nullptr);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT) needsRender = true;
            if (event.type != SDL_KEYDOWN) continue;
            needsRender = true;
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q: running = false; break;
                case SDLK_SPACE: state.playing = !state.playing; break;
                case SDLK_LEFT: state.animationTime = std::max(0.0, state.animationTime - 1.0); break;
                case SDLK_RIGHT: state.animationTime += 1.0; break;
                case SDLK_UP: state.playbackSpeed = std::min(4.0, state.playbackSpeed * 2.0); break;
                case SDLK_DOWN: state.playbackSpeed = std::max(0.25, state.playbackSpeed * 0.5); break;
                case SDLK_1: state.mode = ViewMode::scene; state.animationTime = 0.0; break;
                case SDLK_2: state.mode = ViewMode::scene; state.animationTime = 18.0; break;
                case SDLK_3: state.mode = ViewMode::algorithms; break;
                case SDLK_4: state.mode = ViewMode::colors; break;
                case SDLK_5: state.mode = ViewMode::scene8Bit; break;
                case SDLK_6: state.mode = ViewMode::scene8BitDither; break;
                case SDLK_7: state.mode = ViewMode::transforms; break;
                case SDLK_h: state.showHelp = !state.showHelp; break;
                case SDLK_s: {
                    std::filesystem::create_directories("output");
                    const std::string filePath = "output/window_" + describeView(state, true) + ".ppm";
                    std::cout << (currentImage.savePpm(filePath) ? "saved " : "FAILED to save ") << filePath << "\n";
                    break;
                }
                default: break;
            }
        }
        if (!running) break;

        // Advance the animation clock by the real time since the last frame.
        const Uint64 nowTicks = SDL_GetPerformanceCounter();
        const double deltaSeconds = (nowTicks - previousTicks) / ticksPerSecond;
        previousTicks = nowTicks;
        if (animating) {
            state.animationTime += deltaSeconds * state.playbackSpeed;
            framesPerSecond = framesPerSecond * 0.9 + (deltaSeconds > 0.0 ? 0.1 / deltaSeconds : 0.0);  // smoothed
            needsRender = true;
        }
        if (!needsRender) continue;

        renderView(state, currentImage);
        // Overlay goes on a copy so saved images stay clean.
        FrameBuffer displayed = currentImage;
        if (state.showHelp) drawHelpOverlay(displayed, state, framesPerSecond);
        SDL_SetWindowTitle(window, ("ASTROWORLD - Checkpoint 2 - " + describeView(state, false)).c_str());

        // DISPLAY stage: upload the frame buffer and present it.
        SDL_UpdateTexture(texture, nullptr, displayed.data(), sceneWidth * static_cast<int>(sizeof(Color)));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        if (!capturePath.empty()) {
            const bool saved = captureRenderer(renderer, capturePath);
            std::cout << (saved ? "captured " : "FAILED to capture ") << capturePath << "\n";
            running = false;
        }
        SDL_RenderPresent(renderer);
        needsRender = false;
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
