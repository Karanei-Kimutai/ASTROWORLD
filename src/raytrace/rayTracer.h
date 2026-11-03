/**
 * @file rayTracer.h
 * @brief A CPU ray tracer for the 3D park: Whitted-style ray tracing and
 *        Monte-Carlo path tracing (global illumination), with supersampling anti-aliasing.
 *
 * RAY TRACING works backwards from the eye: for every pixel a ray is shot
 * into the scene; where it lands, more rays are sent to decide the colour.
 *
 *   WHITTED (1980): at each hit, SHADOW rays test whether each light is visible
 *   (hard shadows), and a REFLECTION ray is traced recursively for mirror-like
 *   surfaces. Indirect light is approximated by a constant ambient term.
 *
 *   PATH TRACING (Kajiya 1986): solves the rendering equation by Monte-Carlo
 *   integration. Each pixel averages many random light PATHS; at every bounce
 *   the path picks a random direction (cosine-weighted for diffuse surfaces),
 *   so light reflected off one surface onto another - colour bleeding, soft sky
 *   light, the portal's glow lighting the plaza - emerges naturally. This is
 *   GLOBAL ILLUMINATION; the ambient term is no longer needed.
 *
 *   ANTI-ALIASING: each pixel is sampled at several jittered (stratified)
 *   positions inside its area and the results averaged (supersampling), which
 *   smooths jagged edges and fine texture detail.
 */
#pragma once

#include <cstdint>

#include "core/frameBuffer.h"
#include "park3d/orbitCamera.h"
#include "raytrace/rayGeometry.h"

enum class TracingMode { whitted, pathTracing };

struct RayTraceSettings {
    TracingMode mode = TracingMode::whitted;
    int width = 1280;
    int height = 720;
    int samplesPerPixel = 9;    // anti-aliasing samples (Whitted) or paths (path tracing) per pixel
    int maxDepth = 4;           // reflection recursion depth / maximum path bounces
    int threadCount = 0;        // 0 = all hardware threads
    bool textures = true;
};

/** @brief Render statistics for the report. */
struct RayTraceStats {
    std::size_t triangles = 0;
    double buildSeconds = 0.0;
    double renderSeconds = 0.0;
    std::uint64_t raysCast = 0;
};

/**
 * @brief Ray trace one frame of the park as seen by 'camera'.
 *        Works for any projection: each pixel is UNPROJECTED through the inverse
 *        view-projection matrix to find its ray (parallel rays for orthographic views).
 */
FrameBuffer rayTraceFrame(const ParkScene& scene, const ParkFrame& frame, const CameraMatrices& camera,
                          const RayTraceSettings& settings, RayTraceStats& stats);
