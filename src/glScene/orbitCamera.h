/**
 * @file orbitCamera.h
 * @brief A perspective camera that orbits a target point (yaw, pitch, distance).
 *
 * The eye position is given in SPHERICAL coordinates around the target, then
 * Matrix4::lookAt builds the view matrix and Matrix4::perspective the
 * projection: the "center of projection" is the eye, and the viewing volume
 * is the frustum between the near and far planes.
 */
#pragma once

#include <algorithm>
#include <cmath>

#include "core/vec2.h"
#include "math/matrix4.h"

struct OrbitCamera {
    Vec3 target{0.0, 5.0, 0.0};
    double yaw = 0.0;       // radians around the vertical axis (0 = looking along -z from +z)
    double pitch = 0.15;    // radians above the horizon
    double distance = 30.0;
    double fieldOfViewDegrees = 45.0;

    Vec3 eyePosition() const {
        return target + Vec3(std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)) * distance;
    }

    Matrix4 viewMatrix() const { return Matrix4::lookAt(eyePosition(), target, {0.0, 1.0, 0.0}); }

    Matrix4 projectionMatrix(double aspectRatio) const {
        // Near and far planes bound the view frustum; anything outside is clipped.
        return Matrix4::perspective(fieldOfViewDegrees * pi / 180.0, aspectRatio, 0.1, 400.0);
    }

    /** @brief Orbit by mouse movement, keeping the camera above the ground. */
    void orbit(double deltaYaw, double deltaPitch) {
        yaw += deltaYaw;
        pitch = std::clamp(pitch + deltaPitch, -0.05, 1.45);
    }

    void zoom(double factor) { distance = std::clamp(distance * factor, 4.0, 120.0); }

    /** @brief Move the target sideways/forwards in the ground plane, relative to where the camera faces. */
    void pan(double right, double forward, double up) {
        const Vec3 forwardDirection{-std::sin(yaw), 0.0, -std::cos(yaw)};
        const Vec3 rightDirection{std::cos(yaw), 0.0, -std::sin(yaw)};
        target += rightDirection * right + forwardDirection * forward + Vec3(0.0, up, 0.0);
        target.y = std::max(target.y, 0.5);
    }

    /** @brief Named viewpoints, cycled with the C key. */
    static constexpr int presetCount = 4;
    static OrbitCamera preset(int index) {
        OrbitCamera camera;
        switch (((index % presetCount) + presetCount) % presetCount) {
            case 0: break;  // the album cover: straight at the head
            case 1: camera.target = {2.0, 3.0, -3.0}; camera.yaw = 0.6; camera.pitch = 0.55; camera.distance = 52.0; break;   // aerial
            case 2: camera.target = {-11.0, 7.0, -14.0}; camera.yaw = -0.35; camera.pitch = 0.1; camera.distance = 17.0; break;  // Ferris wheel
            case 3: camera.target = {9.5, 1.8, 4.0}; camera.yaw = 0.5; camera.pitch = 0.3; camera.distance = 10.0; break;   // carousel
        }
        return camera;
    }
};
