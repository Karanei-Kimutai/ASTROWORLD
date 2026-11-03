/**
 * @file keyframeTrack.h
 * @brief Keyframe animation: values pinned at moments in time, eased in between.
 *
 * Animators set KEYFRAMES ("at 4 s the seats are at the top", "at 4.8 s they
 * are at the bottom") and the computer fills the IN-BETWEEN frames by
 * interpolation. Each keyframe stores the easing used for the segment that
 * arrives at it, so one track can mix a slow climb, a free-fall drop and a
 * bouncing landing.
 */
#pragma once

#include <algorithm>
#include <vector>

#include "animation/easing.h"

class KeyframeTrack {
public:
    /**
     * @brief Add a keyframe. Keys must be added in increasing time order.
     * @param easingIn Easing applied on the way INTO this key from the previous one.
     */
    KeyframeTrack& addKey(double time, double value, EasingFunction easingIn = easing::smoothStep) {
        keys.push_back({time, value, easingIn});
        return *this;
    }

    /** @brief Time of the last key (the length of one loop). */
    double duration() const { return keys.empty() ? 0.0 : keys.back().time; }

    /**
     * @brief Value at a given time. Before the first key / after the last the
     *        end values are held; use wrapTime(time, duration()) to loop.
     */
    double sample(double time) const {
        if (keys.empty()) return 0.0;
        if (time <= keys.front().time) return keys.front().value;
        if (time >= keys.back().time) return keys.back().value;
        // Find the segment [previous, next] that contains 'time'.
        const auto next = std::upper_bound(keys.begin(), keys.end(), time,
                                           [](double value, const Key& key) { return value < key.time; });
        const auto previous = next - 1;
        const double segmentLength = next->time - previous->time;
        const double t = segmentLength > 0.0 ? (time - previous->time) / segmentLength : 1.0;
        return previous->value + (next->value - previous->value) * next->easingIn(t);
    }

private:
    struct Key {
        double time;
        double value;
        EasingFunction easingIn;
    };
    std::vector<Key> keys;
};
