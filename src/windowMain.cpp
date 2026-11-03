/**
 * @file windowMain.cpp
 * @brief Checkpoint 1 (windowed variant): shows the software renderer live in an SDL2 window.
 *
 * All drawing is still done by our own pure C++ rasterizer into a FrameBuffer.
 * SDL2 does only the last pipeline stage, DISPLAY: each finished frame is
 * uploaded to a streaming texture and presented in a window, replacing the
 * "write a PPM file" step of the file-based build.
 *
 * Controls:
 *   1 / 2           day / night scene
 *   LEFT / RIGHT    move the time of day (nightAmount -/+ 0.1)
 *   3 / 4           algorithm showcase / colour showcase
 *   5 / 6           current scene reduced to 8-bit / 8-bit dithered
 *   S               save the current image to output/
 *   H               show / hide the help overlay
 *   ESC / Q         quit
 *
 * Usage: astroworldWindow [--night amount] [--capture file.ppm] [--with-help]
 *   --night     starting time of day, 0 (day) .. 1 (night)
 *   --capture   renders one frame, reads it BACK from the renderer
 *               (SDL_RenderReadPixels), saves it and exits - used for automated checks.
 *   --with-help keeps the help overlay in the captured frame (hidden by default).
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
#include "text/bitmapFont.h"

namespace {

/** @brief Which image the window is showing. */
enum class ViewMode { scene, scene8Bit, scene8BitDither, algorithms, colors };

/** @brief Everything the user can change from the keyboard. */
struct ViewerState {
    ViewMode mode = ViewMode::scene;
    double nightAmount = 0.0;
    bool showHelp = true;
};

/** @brief Render the image for the current state with the pure C++ renderer. */
FrameBuffer renderView(const ViewerState& state) {
    FrameBuffer image(sceneWidth, sceneHeight);
    switch (state.mode) {
        case ViewMode::scene:
        case ViewMode::scene8Bit:
        case ViewMode::scene8BitDither:
            renderAstroworldScene(image, SceneSettings{state.nightAmount});
            if (state.mode == ViewMode::scene8Bit) return image.reducedTo8Bit(false);
            if (state.mode == ViewMode::scene8BitDither) return image.reducedTo8Bit(true);
            return image;
        case ViewMode::algorithms:
            renderAlgorithmShowcase(image);
            return image;
        case ViewMode::colors:
            renderColorShowcase(image);
            return image;
    }
    return image;
}

/** @brief Short description of the view, used in the title bar, overlay and saved file names. */
std::string describeView(const ViewerState& state, bool forFileName) {
    char nightText[16];
    std::snprintf(nightText, sizeof(nightText), "%.1f", state.nightAmount);
    std::string scenePart = std::string(forFileName ? "scene_night" : "SCENE  NIGHT ") + nightText;
    switch (state.mode) {
        case ViewMode::scene: return scenePart;
        case ViewMode::scene8Bit: return scenePart + (forFileName ? "_8bit" : "  8-BIT");
        case ViewMode::scene8BitDither: return scenePart + (forFileName ? "_8bitDither" : "  8-BIT DITHER");
        case ViewMode::algorithms: return forFileName ? "algorithms" : "ALGORITHM SHOWCASE";
        case ViewMode::colors: return forFileName ? "colors" : "COLOUR SHOWCASE";
    }
    return "";
}

/** @brief Draw the controls panel: a translucent box (blended spans) with bitmap-font text. */
void drawHelpOverlay(FrameBuffer& frameBuffer, const ViewerState& state) {
    const std::vector<std::string> lines = {
        "1 DAY   2 NIGHT",        "LEFT/RIGHT  TIME OF DAY", "3 ALGORITHMS  4 COLOUR",
        "5 8-BIT   6 DITHER",     "S SAVE  H HELP  ESC QUIT", "",
        describeView(state, false),
    };
    const int pixelScale = 2;
    const int lineHeight = 9 * pixelScale;
    const int boxLeft = 12;
    const int boxTop = sceneHeight - 12 - static_cast<int>(lines.size()) * lineHeight - 16;
    const int boxWidth = 320;
    const int boxHeight = static_cast<int>(lines.size()) * lineHeight + 16;
    for (int y = boxTop; y < boxTop + boxHeight; ++y) frameBuffer.blendSpan(y, boxLeft, boxLeft + boxWidth, astroPalette::black, 0.6);
    for (std::size_t index = 0; index < lines.size(); ++index) {
        const Color textColor = index + 1 == lines.size() ? astroPalette::letterYellow : astroPalette::white;
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
    double startNightAmount = 0.0;
    for (int index = 1; index < argumentCount; ++index) {
        const std::string argument = argumentValues[index];
        if (argument == "--capture" && index + 1 < argumentCount) capturePath = argumentValues[++index];
        if (argument == "--with-help") captureWithHelp = true;
        if (argument == "--night" && index + 1 < argumentCount) {
            startNightAmount = std::clamp(std::atof(argumentValues[++index]), 0.0, 1.0);
        }
    }

    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("ASTROWORLD - Checkpoint 1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          sceneWidth, sceneHeight, SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }
    // Prefer a hardware renderer, fall back to SDL's software renderer.
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

    ViewerState state;
    state.nightAmount = startNightAmount;
    if (!capturePath.empty()) state.showHelp = captureWithHelp;
    FrameBuffer currentImage = renderView(state);
    bool needsRender = false;   // the pure C++ renderer must redraw the image
    bool needsPresent = true;   // the window must be refreshed
    bool running = true;

    while (running) {
        // Sleep until something happens: the scene is static, so there is no need to redraw every frame.
        // (SDL_WaitEvent with nullptr blocks without removing the event from the queue.)
        if (!needsRender && !needsPresent) SDL_WaitEvent(nullptr);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT) needsPresent = true;
            if (event.type != SDL_KEYDOWN) continue;

            const ViewerState previous = state;
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q: running = false; break;
                case SDLK_1: state.mode = ViewMode::scene; state.nightAmount = 0.0; break;
                case SDLK_2: state.mode = ViewMode::scene; state.nightAmount = 1.0; break;
                case SDLK_3: state.mode = ViewMode::algorithms; break;
                case SDLK_4: state.mode = ViewMode::colors; break;
                case SDLK_5: state.mode = ViewMode::scene8Bit; break;
                case SDLK_6: state.mode = ViewMode::scene8BitDither; break;
                case SDLK_LEFT:
                case SDLK_RIGHT: {
                    const double step = event.key.keysym.sym == SDLK_RIGHT ? 0.1 : -0.1;
                    state.nightAmount = std::clamp(std::round((state.nightAmount + step) * 10.0) / 10.0, 0.0, 1.0);
                    if (state.mode == ViewMode::algorithms || state.mode == ViewMode::colors) state.mode = ViewMode::scene;
                    break;
                }
                case SDLK_h: state.showHelp = !state.showHelp; needsPresent = true; break;
                case SDLK_s: {
                    std::filesystem::create_directories("output");
                    const std::string filePath = "output/window_" + describeView(state, true) + ".ppm";
                    std::cout << (currentImage.savePpm(filePath) ? "saved " : "FAILED to save ") << filePath << "\n";
                    break;
                }
                default: break;
            }
            if (state.mode != previous.mode || state.nightAmount != previous.nightAmount) needsRender = true;
        }

        if (needsRender) {
            currentImage = renderView(state);
            needsRender = false;
            needsPresent = true;
        }
        if (needsPresent) {
            // Overlay goes on a copy so saved images stay clean.
            FrameBuffer displayed = currentImage;
            if (state.showHelp) drawHelpOverlay(displayed, state);
            SDL_SetWindowTitle(window, ("ASTROWORLD - Checkpoint 1 - " + describeView(state, false)).c_str());

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
            needsPresent = false;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
