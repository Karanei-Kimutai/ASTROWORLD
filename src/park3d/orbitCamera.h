/**
 * @file orbitCamera.h
 * @brief The viewer's camera: an orbit around a target, seen through any of the
 *        course's projections.
 *
 * The eye position is given in SPHERICAL coordinates around the target, then
 * Matrix4::lookAt builds the view matrix. The projection is one of:
 *
 *   PERSPECTIVE                 projectors meet at the eye (the centre of projection);
 *                               the frustum between near and far planes is the view volume
 *   PARALLEL, ORTHOGRAPHIC:
 *     isometric (axonometric)   looking down the cube diagonal: the three axes are
 *                               foreshortened equally and 120 degrees apart
 *     top / front (multiview)   looking straight down an axis: true shape of one face
 *   PARALLEL, OBLIQUE:
 *     cavalier / cabinet        front face true shape, depth drawn on a 45 degree slant
 *                               at full / half length
 */
#pragma once

#include <algorithm>
#include <cmath>

#include "core/vec2.h"
#include "math/matrix4.h"

enum class ProjectionMode { perspective, isometric, cavalier, cabinet, topView, frontView, count };

inline const char* projectionModeName(ProjectionMode mode) {
    switch (mode) {
        case ProjectionMode::perspective: return "PERSPECTIVE";
        case ProjectionMode::isometric: return "ISOMETRIC";
        case ProjectionMode::cavalier: return "OBLIQUE CAVALIER";
        case ProjectionMode::cabinet: return "OBLIQUE CABINET";
        case ProjectionMode::topView: return "MULTIVIEW TOP";
        case ProjectionMode::frontView: return "MULTIVIEW FRONT";
        default: return "";
    }
}

/** @brief The finished camera for one frame. */
struct CameraMatrices {
    Matrix4 view;
    Matrix4 projection;
    Vec3 eye;
};

struct OrbitCamera {
    Vec3 target{0.0, 5.0, 0.0};
    double yaw = 0.0;       // radians around the vertical axis (0 = looking along -z from +z)
    double pitch = 0.15;    // radians above the horizon
    double distance = 30.0;
    double fieldOfViewDegrees = 45.0;

    Vec3 eyePosition() const {
        return target + Vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)) * distance;
    }

    /** @brief View and projection for the chosen projection type. */
    CameraMatrices matrices(ProjectionMode mode, double aspectRatio) const {
        CameraMatrices result;
        // Parallel projections show a box whose height follows the zoom distance.
        const double halfHeight = distance * 0.42;
        const double halfWidth = halfHeight * aspectRatio;
        const Matrix4 parallelBox = Matrix4::orthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, -400.0, 400.0);
        Vec3 viewDirection;  // from target toward the eye
        Vec3 up{0.0, 1.0, 0.0};
        switch (mode) {
            case ProjectionMode::perspective:
                result.eye = eyePosition();
                result.view = Matrix4::lookAt(result.eye, target, up);
                result.projection = Matrix4::perspective(fieldOfViewDegrees * pi / 180.0, aspectRatio, 0.1, 400.0);
                return result;
            case ProjectionMode::isometric: {
                // Elevation atan(1/sqrt 2) = 35.26 degrees and 45 degrees round: the cube-diagonal view.
                const double elevation = std::atan(1.0 / std::sqrt(2.0));
                const double azimuth = yaw + pi / 4.0;
                viewDirection = {std::sin(azimuth) * std::cos(elevation), std::sin(elevation), std::cos(azimuth) * std::cos(elevation)};
                break;
            }
            case ProjectionMode::topView:
                viewDirection = {0.0, 1.0, 0.0};
                up = {0.0, 0.0, -1.0};  // north is up on the plan
                break;
            default:  // front view and the obliques look straight at the front face
                viewDirection = {0.0, 0.0, 1.0};
                break;
        }
        result.eye = target + viewDirection * distance;
        result.view = Matrix4::lookAt(result.eye, target, up);
        result.projection = parallelBox;
        if (mode == ProjectionMode::cavalier || mode == ProjectionMode::cabinet) {
            const double receding = mode == ProjectionMode::cavalier ? 1.0 : 0.5;
            result.projection = parallelBox * Matrix4::obliqueShear(receding, pi / 4.0, -distance);
        }
        return result;
    }

    /** @brief Orbit by mouse movement, keeping the camera above the ground. */
    void orbit(double deltaYaw, double deltaPitch) {
        yaw += deltaYaw;
        pitch = std::clamp(pitch + deltaPitch, -0.05, 1.45);
    }

    void zoom(double factor) { distance = std::clamp(distance * factor, 4.0, 160.0); }

    /** @brief Move the target sideways/forwards in the ground plane, relative to where the camera faces. */
    void pan(double right, double forward, double up) {
        const Vec3 forwardDirection{-std::sin(yaw), 0.0, -std::cos(yaw)};
        const Vec3 rightDirection{std::cos(yaw), 0.0, -std::sin(yaw)};
        target += rightDirection * right + forwardDirection * forward + Vec3(0.0, up, 0.0);
        target.y = std::max(target.y, 0.5);
    }

    /** @brief Named viewpoints, cycled with the C key. */
    static constexpr int presetCount = 7;
    static const char* presetName(int index) {
        static const char* names[presetCount] = {"ALBUM COVER", "AERIAL", "FERRIS WHEEL", "CAROUSEL",
                                                 "1-POINT PERSPECTIVE", "2-POINT PERSPECTIVE", "3-POINT PERSPECTIVE"};
        return names[((index % presetCount) + presetCount) % presetCount];
    }
    static OrbitCamera preset(int index) {
        OrbitCamera camera;
        switch (((index % presetCount) + presetCount) % presetCount) {
            case 0: break;  // the album cover: straight at the head
            case 1: camera.target = {2.0, 3.0, -3.0}; camera.yaw = 0.6; camera.pitch = 0.55; camera.distance = 52.0; break;
            case 2: camera.target = {-11.0, 7.0, -14.0}; camera.yaw = -0.35; camera.pitch = 0.1; camera.distance = 17.0; break;
            case 3: camera.target = {9.5, 1.8, 4.0}; camera.yaw = 0.5; camera.pitch = 0.3; camera.distance = 10.0; break;
            case 4:  // ONE vanishing point: the view direction is parallel to the ground and square to the fence,
                     // so only lines running away from the viewer (z) converge.
                camera.target = {0.0, 3.0, -9.5}; camera.yaw = 0.0; camera.pitch = 0.0; camera.distance = 34.0; camera.fieldOfViewDegrees = 60.0;
                break;
            case 5:  // TWO vanishing points: turned 45 degrees, so both horizontal axes (x and z) converge;
                     // verticals stay vertical because the camera is level.
                camera.target = {0.0, 4.0, 0.0}; camera.yaw = pi / 4.0; camera.pitch = 0.0; camera.distance = 36.0; camera.fieldOfViewDegrees = 55.0;
                break;
            case 6:  // THREE vanishing points: also tilted to look up at the drop tower, so verticals converge too.
                camera.target = {16.5, 14.0, -20.0}; camera.yaw = 0.6; camera.pitch = -0.75; camera.distance = 16.0; camera.fieldOfViewDegrees = 75.0;
                break;
        }
        return camera;
    }
};
