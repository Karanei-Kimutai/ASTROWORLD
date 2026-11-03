/**
 * @file astroworldTimeline.cpp
 * @brief Keyframe tracks and ride timings shared by the software and OpenGL renderers.
 */
#include "animation/astroworldTimeline.h"

#include "animation/easing.h"
#include "animation/keyframeTrack.h"
#include "scene/rollerCoaster.h"

namespace {

/** @brief Day -> dusk -> night -> dawn over one loop. */
const KeyframeTrack& nightTrack() {
    static const KeyframeTrack track = KeyframeTrack()
                                           .addKey(0.0, 0.0)
                                           .addKey(8.0, 0.0)                          // day
                                           .addKey(13.0, 1.0, easing::smoothStep)     // dusk
                                           .addKey(25.0, 1.0)                         // night
                                           .addKey(animationLoopSeconds, 0.0, easing::smoothStep);  // dawn
    return track;
}

/** @brief Drop tower seat height over one 9-second ride cycle (0 = bottom, 1 = top). */
const KeyframeTrack& dropTowerTrack() {
    static const KeyframeTrack track = KeyframeTrack()
                                           .addKey(0.0, 0.0)
                                           .addKey(3.5, 1.0, easing::easeInOutCubic)  // climb
                                           .addKey(4.6, 1.0)                          // suspense at the top
                                           .addKey(6.0, 0.0, easing::easeOutBounce)   // DROP
                                           .addKey(9.0, 0.0);                         // unload / reload
    return track;
}

}  // namespace

double nightAmountAt(double timeSeconds) { return nightTrack().sample(wrapTime(timeSeconds, animationLoopSeconds)); }

double dropTowerLiftAt(double timeSeconds) {
    return dropTowerTrack().sample(wrapTime(timeSeconds, dropTowerTrack().duration()));
}

const std::vector<Vec2>& coasterProfilePoints() {
    static const std::vector<Vec2> points = {{830.0, 500.0},  {890.0, 390.0},  {955.0, 235.0},
                                             {1025.0, 300.0}, {1075.0, 440.0}, {1135.0, 470.0},
                                             {1190.0, 345.0}, {1240.0, 265.0}, {1300.0, 330.0}};
    return points;
}

double coasterTrainFractionAt(double timeSeconds) {
    RollerCoaster profile;
    profile.controlPoints = coasterProfilePoints();
    profile.groundY = coasterProfileGroundY;
    // +6 s: the train is already on the chain lift at t = 0.
    return profile.trainFractionAtTime(timeSeconds + 6.0);
}
