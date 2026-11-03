/**
 * @file easing.h
 * @brief Easing (timing) functions and time helpers for animation.
 *
 * An easing function maps normalised time t in [0, 1] to progress in [0, 1].
 * Linear motion looks mechanical; easing adds acceleration and deceleration
 * so motion feels physical (a drop tower accelerates as it falls, a bouncing
 * landing overshoots and settles).
 */
#pragma once

#include <algorithm>
#include <cmath>

#include "core/vec2.h"

/** @brief Pointer to any easing function, so keyframes can store which one to use. */
using EasingFunction = double (*)(double);

namespace easing {

inline double linear(double t) { return t; }

/** @brief Hermite smoothstep 3t^2 - 2t^3: gentle start and stop. */
inline double smoothStep(double t) { return t * t * (3.0 - 2.0 * t); }

/** @brief Accelerate from rest (like free fall: distance grows with t^2). */
inline double easeInQuad(double t) { return t * t; }

/** @brief Decelerate to rest. */
inline double easeOutQuad(double t) { return 1.0 - (1.0 - t) * (1.0 - t); }

/** @brief Slow - fast - slow, stronger than smoothstep. */
inline double easeInOutCubic(double t) {
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - std::pow(-2.0 * t + 2.0, 3.0) / 2.0;
}

/** @brief Lands and bounces three times, each bounce smaller (piecewise parabolas). */
inline double easeOutBounce(double t) {
    const double strength = 7.5625;
    const double span = 2.75;
    if (t < 1.0 / span) return strength * t * t;
    if (t < 2.0 / span) {
        t -= 1.5 / span;
        return strength * t * t + 0.75;
    }
    if (t < 2.5 / span) {
        t -= 2.25 / span;
        return strength * t * t + 0.9375;
    }
    t -= 2.625 / span;
    return strength * t * t + 0.984375;
}

}  // namespace easing

/** @brief Time wrapped into [0, period) - for looping animations. */
inline double wrapTime(double time, double period) {
    const double wrapped = std::fmod(time, period);
    return wrapped < 0.0 ? wrapped + period : wrapped;
}

/** @brief Smooth oscillation between -amplitude and +amplitude. */
inline double oscillate(double time, double frequencyHz, double amplitude, double phase = 0.0) {
    return amplitude * std::sin(2.0 * pi * frequencyHz * time + phase);
}
