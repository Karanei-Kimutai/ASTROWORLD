/**
 * @file glMain.cpp
 * @brief The ASTROWORLD park rendered in real time with OpenGL 3.3 core.
 *
 * GLFW opens the window and the OpenGL context; our own loader (gl/glFunctions.h)
 * fetches the GL functions; ParkScene describes the park at each moment and
 * AstroworldPark draws it into an off-screen multisampled frame buffer, which
 * is then shown in the window.
 *
 * Controls:
 *   mouse drag / scroll      orbit / zoom          W A S D / Q E    move the camera target
 *   C   cycle viewpoints (incl. 1-, 2-, 3-point perspective)   R   ride the roller coaster
 *   V   cycle projections: perspective, isometric, cavalier, cabinet, top, front
 *   G   cycle shading: flat, Gouraud, Phong                    F   wireframe
 *   T   textures on/off      M   mipmapping on/off             O   automatic orbit
 *   SPACE play/pause   LEFT/RIGHT seek   UP/DOWN speed   1/2 day/night
 *   L   logo   H   help   P   screenshot   ESC quit
 *
 * Usage:
 *   astroworldGl [options]                         interactive
 *   astroworldGl --capture file.ppm [options]      one frame, off-screen
 *   astroworldGl --animate seconds fps (dir | -)   frame sequence, off-screen
 * Options: --time s  --camera n  --projection n  --shading n  --wireframe  --ride
 *          --no-logo  --no-textures  --no-mipmaps  --software  --duration s
 */
#define GLFW_INCLUDE_NONE  // we include our own OpenGL declarations (gl/glFunctions.h)
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "animation/astroworldTimeline.h"
#include "gl/glFunctions.h"
#include "gl/glResources.h"
#include "glScene/astroworldPark.h"
#include "glScene/overlayImage.h"
#include "park3d/parkScene.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Everything the user changes with the keyboard and mouse. */
struct ViewerState {
    double animationTime = 0.0;
    double playbackSpeed = 1.0;
    bool playing = true;
    bool autoOrbit = false;
    bool showHelp = true;
    bool ridingCoaster = false;
    bool screenshotRequested = false;
    int cameraPreset = 0;
    OrbitCamera camera = OrbitCamera::preset(0);
    ProjectionMode projection = ProjectionMode::perspective;
    RenderOptions options;
    double scrollAccumulated = 0.0;
    double closeAfterSeconds = 0.0;  // 0 = run until closed; >0 = benchmark run (--duration)
};

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    auto& state = *static_cast<ViewerState*>(glfwGetWindowUserPointer(window));
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        case GLFW_KEY_SPACE: state.playing = !state.playing; break;
        case GLFW_KEY_LEFT: state.animationTime = std::max(0.0, state.animationTime - 1.0); break;
        case GLFW_KEY_RIGHT: state.animationTime += 1.0; break;
        case GLFW_KEY_UP: state.playbackSpeed = std::min(4.0, state.playbackSpeed * 2.0); break;
        case GLFW_KEY_DOWN: state.playbackSpeed = std::max(0.25, state.playbackSpeed * 0.5); break;
        case GLFW_KEY_1: state.animationTime = 0.0; break;
        case GLFW_KEY_2: state.animationTime = 18.0; break;
        case GLFW_KEY_C:
            state.cameraPreset = (state.cameraPreset + 1) % OrbitCamera::presetCount;
            state.camera = OrbitCamera::preset(state.cameraPreset);
            state.ridingCoaster = false;
            break;
        case GLFW_KEY_V:
            state.projection = static_cast<ProjectionMode>((static_cast<int>(state.projection) + 1) % static_cast<int>(ProjectionMode::count));
            break;
        case GLFW_KEY_G: state.options.shading = static_cast<ShadingMode>((static_cast<int>(state.options.shading) + 1) % 3); break;
        case GLFW_KEY_F: state.options.wireframe = !state.options.wireframe; break;
        case GLFW_KEY_T: state.options.textures = !state.options.textures; break;
        case GLFW_KEY_M: state.options.mipmapping = !state.options.mipmapping; break;
        case GLFW_KEY_R: state.ridingCoaster = !state.ridingCoaster; break;
        case GLFW_KEY_O: state.autoOrbit = !state.autoOrbit; break;
        case GLFW_KEY_L: state.options.showLogo = !state.options.showLogo; break;
        case GLFW_KEY_H: state.showHelp = !state.showHelp; break;
        case GLFW_KEY_P: state.screenshotRequested = true; break;
        default: break;
    }
}

void scrollCallback(GLFWwindow* window, double, double offsetY) {
    static_cast<ViewerState*>(glfwGetWindowUserPointer(window))->scrollAccumulated += offsetY;
}

/**
 * @brief The camera for this frame: the orbit camera through the chosen projection,
 *        or - when riding - a first-person perspective view from the front coaster car.
 */
CameraMatrices cameraFor(const ViewerState& state, const ParkScene& scene) {
    const double aspect = static_cast<double>(AstroworldPark::renderWidth) / AstroworldPark::renderHeight;
    Vec3 eye, forward, up;
    if (state.ridingCoaster && scene.coasterRideView(state.animationTime, eye, forward, up)) {
        CameraMatrices ride;
        ride.eye = eye;
        ride.view = Matrix4::lookAt(eye, eye + forward, up);  // the camera banks with the car
        ride.projection = Matrix4::perspective(75.0 * pi / 180.0, aspect, 0.05, 400.0);
        return ride;
    }
    return state.camera.matrices(state.projection, aspect);
}

/** @brief The help panel, drawn by the software renderer's bitmap font into a texture. */
RgbaImage buildHelpImage(const ViewerState& state, double framesPerSecond, const std::string& rendererName) {
    char status[96];
    std::snprintf(status, sizeof(status), "T %5.1fS  %s X%.2f  %2.0f FPS", state.animationTime, state.playing ? "PLAY" : "PAUSE",
                  state.playbackSpeed, framesPerSecond);
    const std::string view = state.ridingCoaster ? "COASTER RIDE" : OrbitCamera::presetName(state.cameraPreset);
    const std::string modes = std::string(shadingModeName(state.options.shading)) + (state.options.wireframe ? " WIREFRAME" : "") +
                              (state.options.textures ? "" : " NO-TEX") + (state.options.mipmapping ? "" : " NO-MIP");
    const std::vector<std::string> lines = {
        "DRAG ORBIT  SCROLL ZOOM  WASDQE",
        "C VIEW  R RIDE  V PROJECTION",
        "G SHADING  F WIREFRAME",
        "T TEXTURES  M MIPMAPS  O ORBIT",
        "SPACE PLAY  ARROWS SEEK/SPEED",
        "1 DAY 2 NIGHT  L LOGO  P SHOT",
        "",
        view,
        state.ridingCoaster ? std::string("PERSPECTIVE") : projectionModeName(state.projection),
        modes,
        status,
        rendererName,
    };
    const int lineHeight = 18;
    const int width = 380;
    const int height = static_cast<int>(lines.size()) * lineHeight + 16;
    return renderOverlayImage(width, height, [&](FrameBuffer& canvas) {
        for (int y = 0; y < height; ++y) canvas.blendSpan(y, 0, width - 1, astroPalette::black, 0.6);
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const Color color = index >= 7 ? astroPalette::letterYellow : astroPalette::white;
            drawBitmapText(canvas, lines[index], 10, 8 + static_cast<int>(index) * lineHeight, 2, color);
        }
    });
}

/** @brief Upper-case, length-limited renderer name for the bitmap font (it only has capitals). */
std::string shortRendererName() {
    std::string name = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return name.substr(0, 28);
}

}  // namespace

/**
 * @brief Everything that owns OpenGL objects: off-screen capture/animation, or the interactive loop.
 *        Keeping them inside this function means their destructors (glDelete*) run while
 *        the context still exists - before glfwTerminate.
 * @return Process exit code.
 */
int runWithContext(GLFWwindow* window, ViewerState& state, const std::string& capturePath, const std::string& animateDestination,
                   double animateSeconds, int animateFps) {
    std::string error;
    const ParkScene scene;
    RenderTarget target;
    AstroworldPark park;
    if (!target.create(AstroworldPark::renderWidth, AstroworldPark::renderHeight, 4, error) || !park.initialise(scene, error)) {
        std::cerr << "Initialisation failed: " << error << "\n";
        return 1;
    }

    auto renderFrame = [&](double time) {
        state.animationTime = time;
        target.bindForDrawing();
        park.render(scene.frameAt(time, nightAmountAt(time)), cameraFor(state, scene), state.options);
    };

    // ---- Off-screen modes: one screenshot, or a whole animation ---------------------------
    if (!capturePath.empty()) {
        renderFrame(state.animationTime);
        target.resolve();
        const bool saved = target.readBack().savePpm(capturePath);
        std::cerr << (saved ? "captured " : "FAILED to capture ") << capturePath << "\n";
        return saved ? 0 : 1;
    }
    if (!animateDestination.empty()) {
        const bool toStdout = animateDestination == "-";
        if (!toStdout) std::filesystem::create_directories(animateDestination);
        std::ios::sync_with_stdio(false);
        const double startTime = state.animationTime;
        const int frameCount = static_cast<int>(animateSeconds * animateFps + 0.5);
        for (int index = 0; index < frameCount; ++index) {
            renderFrame(startTime + static_cast<double>(index) / animateFps);
            target.resolve();
            const FrameBuffer frame = target.readBack();
            if (toStdout) {
                frame.writePpm(std::cout);
            } else {
                char fileName[32];
                std::snprintf(fileName, sizeof(fileName), "frame_%05d.ppm", index);
                frame.savePpm((std::filesystem::path(animateDestination) / fileName).string());
            }
            if ((index + 1) % animateFps == 0) std::cerr << "\r  " << index + 1 << "/" << frameCount << " frames" << std::flush;
        }
        std::cerr << "\n";
        return 0;
    }

    // ---- Interactive mode -------------------------------------------------------------------
    glfwSetWindowUserPointer(window, &state);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetScrollCallback(window, scrollCallback);

    const std::string rendererName = shortRendererName();
    Texture2D helpTexture;
    double framesPerSecond = 0.0;
    double lastHelpUpdate = -1.0;
    double previousTime = glfwGetTime();
    double previousCursorX = 0.0, previousCursorY = 0.0;
    glfwGetCursorPos(window, &previousCursorX, &previousCursorY);
    const double sessionStart = glfwGetTime();
    long framesDrawn = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        if (state.closeAfterSeconds > 0.0 && now - sessionStart > state.closeAfterSeconds) glfwSetWindowShouldClose(window, GLFW_TRUE);
        const double deltaSeconds = now - previousTime;
        previousTime = now;
        if (deltaSeconds > 0.0) framesPerSecond = framesPerSecond * 0.9 + 0.1 / deltaSeconds;
        if (state.playing) state.animationTime += deltaSeconds * state.playbackSpeed;

        // Mouse drag orbits; scroll zooms; keys pan.
        double cursorX = 0.0, cursorY = 0.0;
        glfwGetCursorPos(window, &cursorX, &cursorY);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            state.camera.orbit(-(cursorX - previousCursorX) * 0.006, (cursorY - previousCursorY) * 0.006);
        }
        previousCursorX = cursorX;
        previousCursorY = cursorY;
        if (state.scrollAccumulated != 0.0) {
            state.camera.zoom(std::pow(0.9, state.scrollAccumulated));
            state.scrollAccumulated = 0.0;
        }
        const double moveStep = 10.0 * deltaSeconds;
        auto held = [&](int key) { return glfwGetKey(window, key) == GLFW_PRESS ? 1.0 : 0.0; };
        state.camera.pan((held(GLFW_KEY_D) - held(GLFW_KEY_A)) * moveStep, (held(GLFW_KEY_W) - held(GLFW_KEY_S)) * moveStep,
                         (held(GLFW_KEY_E) - held(GLFW_KEY_Q)) * moveStep);
        if (state.autoOrbit) state.camera.orbit(0.15 * deltaSeconds, 0.0);

        renderFrame(state.animationTime);
        if (state.showHelp) {
            // Rebuild the help texture four times a second (it shows the clock and FPS).
            if (now - lastHelpUpdate > 0.25) {
                AstroworldPark::uploadImage(helpTexture, buildHelpImage(state, framesPerSecond, rendererName));
                lastHelpUpdate = now;
            }
            park.drawOverlay(helpTexture, 12, AstroworldPark::renderHeight - helpTexture.getHeight() - 12);
        }
        target.resolve();
        if (state.screenshotRequested) {
            std::filesystem::create_directories("output");
            char fileName[64];
            std::snprintf(fileName, sizeof(fileName), "output/gl_t%06.2f.ppm", state.animationTime);
            std::cout << (target.readBack().savePpm(fileName) ? "saved " : "FAILED to save ") << fileName << "\n";
            state.screenshotRequested = false;
        }
        int windowWidth = 0, windowHeight = 0;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        target.presentToWindow(windowWidth, windowHeight);
        glfwSwapBuffers(window);
        ++framesDrawn;
    }
    // Performance evidence for the checkpoint: average frame rate over the session (vsync-capped).
    const double sessionSeconds = glfwGetTime() - sessionStart;
    if (sessionSeconds > 0.0) std::cerr << "average " << framesDrawn / sessionSeconds << " fps over " << sessionSeconds << " s\n";
    return 0;
}

int main(int argumentCount, char** argumentValues) {
    std::string capturePath;
    std::string animateDestination;
    double animateSeconds = 0.0;
    int animateFps = 30;
    bool useSoftwareGl = false;
    ViewerState state;
    for (int index = 1; index < argumentCount; ++index) {
        const std::string argument = argumentValues[index];
        const bool hasValue = index + 1 < argumentCount;
        if (argument == "--capture" && hasValue) capturePath = argumentValues[++index];
        else if (argument == "--time" && hasValue) state.animationTime = std::atof(argumentValues[++index]);
        else if (argument == "--camera" && hasValue) {
            state.cameraPreset = std::atoi(argumentValues[++index]);
            state.camera = OrbitCamera::preset(state.cameraPreset);
        } else if (argument == "--projection" && hasValue) {
            state.projection = static_cast<ProjectionMode>(std::clamp(std::atoi(argumentValues[++index]), 0, static_cast<int>(ProjectionMode::count) - 1));
        } else if (argument == "--shading" && hasValue) {
            state.options.shading = static_cast<ShadingMode>(std::clamp(std::atoi(argumentValues[++index]), 0, 2));
        } else if (argument == "--animate" && index + 3 < argumentCount) {
            animateSeconds = std::atof(argumentValues[++index]);
            animateFps = std::max(1, std::atoi(argumentValues[++index]));
            animateDestination = argumentValues[++index];
        } else if (argument == "--duration" && hasValue) {
            state.closeAfterSeconds = std::atof(argumentValues[++index]);
        } else if (argument == "--wireframe") state.options.wireframe = true;
        else if (argument == "--ride") state.ridingCoaster = true;
        else if (argument == "--no-logo") state.options.showLogo = false;
        else if (argument == "--no-textures") state.options.textures = false;
        else if (argument == "--no-mipmaps") state.options.mipmapping = false;
        else if (argument == "--software") useSoftwareGl = true;
    }
    const bool offscreen = !capturePath.empty() || !animateDestination.empty();

    // Under WSL, Mesa defaults to its CPU rasterizer (llvmpipe). The d3d12 driver
    // forwards OpenGL to the real GPU through /dev/dxg, so prefer it when present.
    if (!useSoftwareGl && std::getenv("GALLIUM_DRIVER") == nullptr && access("/dev/dxg", F_OK) == 0) {
        setenv("GALLIUM_DRIVER", "d3d12", 0);
    }

    if (!glfwInit()) {
        std::cerr << "glfwInit failed\n";
        return 1;
    }
    // Ask for a modern OpenGL 3.3 CORE profile context (no deprecated fixed-function API).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, offscreen ? GLFW_FALSE : GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(AstroworldPark::renderWidth, AstroworldPark::renderHeight, "ASTROWORLD (OpenGL)", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Could not create an OpenGL 3.3 core window\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // vsync

    std::string missing;
    const GlProcLookup lookup = [](const char* name) { return reinterpret_cast<void*>(glfwGetProcAddress(name)); };
    if (!loadOpenGlFunctions(lookup, missing)) {
        std::cerr << "Missing OpenGL functions: " << missing << "\n";
        glfwTerminate();
        return 1;
    }
    std::cerr << "OpenGL " << glGetString(GL_VERSION) << " on " << glGetString(GL_RENDERER) << "\n";

    const int exitCode = runWithContext(window, state, capturePath, animateDestination, animateSeconds, animateFps);
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
