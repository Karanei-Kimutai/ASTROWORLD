/**
 * @file astroworldScene.cpp
 * @brief Scene layout and animation: where every ride sits, and how it moves over time.
 *
 * The composition follows the album cover: the head in the centre with its
 * mouth open on the ground, carnival rides and tents behind a black fence,
 * a rocket statue to the left, bunting strung out from the head, and the
 * "Greetings from ASTROWORLD" postcard logo across the top.
 *
 * Animation summary (t = seconds):
 *   Ferris wheel     rotation = 0.25 rad/s * t, gondolas swing +-0.08 rad
 *   Carousel         rotation = 0.7 rad/s * t about its vertical axis (4x4), horses bob
 *   Roller coaster   chain lift then energy-conserving speed along the spline
 *   Drop tower       keyframes: slow climb (ease in-out), pause, free-fall drop (bounce)
 *   Head             breathing scale about its base, dreads bend in the wind, glow pulses
 *   Crowd            walk cycles toward the mouth, shrink into the light
 *   Clouds / stars   drift with parallax / twinkle
 *   Bunting          pennants flutter
 *   Searchlights     sweep back and forth (night)
 *   Sky              day -> dusk -> night -> dawn over a 30 s loop (keyframes)
 *
 * The keyframed and physics-driven parts come from animation/astroworldTimeline.h,
 * which the OpenGL renderer shares.
 */
#include "scene/astroworldScene.h"

#include <algorithm>
#include <string>

#include "animation/easing.h"
#include "scene/astroHead.h"
#include "scene/backdrop.h"
#include "scene/carnivalProps.h"
#include "scene/carousel.h"
#include "scene/dropTower.h"
#include "scene/ferrisWheel.h"
#include "scene/foreground.h"
#include "scene/rollerCoaster.h"
#include "text/astroWordmark.h"
#include "text/bitmapFont.h"

namespace {

/** @brief Bitmap-font caption centred on centerX with a 2 px drop shadow. */
void drawCaption(FrameBuffer& frameBuffer, const std::string& text, int centerX, int top, int pixelScale) {
    const int left = centerX - measureBitmapText(text, pixelScale) / 2;
    drawBitmapText(frameBuffer, text, left + 2, top + 2, pixelScale, astroPalette::black);
    drawBitmapText(frameBuffer, text, left, top, pixelScale, astroPalette::white);
}

}  // namespace

SceneSettings sampleAnimation(double timeSeconds) {
    return SceneSettings{nightAmountAt(timeSeconds), timeSeconds};
}

void renderAstroworldScene(FrameBuffer& frameBuffer, const SceneSettings& settings) {
    const int width = frameBuffer.getWidth();
    const double night = std::clamp(settings.nightAmount, 0.0, 1.0);
    const double time = settings.timeSeconds;
    const int horizonY = 500;  // base of the fence / far edge of the plaza

    // ---- Sky ------------------------------------------------------------------------
    drawSky(frameBuffer, horizonY, night);
    drawStars(frameBuffer, horizonY, 7u, night, time);
    drawClouds(frameBuffer, horizonY, 31u, 1.0 - night, time);
    drawSearchlight(frameBuffer, {420.0, horizonY + 10.0}, 74.0 + oscillate(time, 0.07, 22.0), 6.0, astroPalette::neonCyan, night);
    drawSearchlight(frameBuffer, {880.0, horizonY + 10.0}, 108.0 + oscillate(time, 0.09, 20.0, 1.3), 6.0, astroPalette::neonPink, night);
    drawSkyline(frameBuffer, horizonY, 21u, lerpColor(astroPalette::daySkyline, astroPalette::nightSkyline, night));

    // ---- Behind the fence: rides, sign and big top ----------------------------------
    FerrisWheel ferrisWheel;
    ferrisWheel.hubPosition = {300.0, 330.0};
    ferrisWheel.radius = 132;
    ferrisWheel.gondolaCount = 12;
    ferrisWheel.rotationAngle = 0.12 + 0.25 * time;
    ferrisWheel.swingAngle = oscillate(time, 0.4, 0.08);
    ferrisWheel.groundY = horizonY;
    ferrisWheel.draw(frameBuffer);

    DropTower dropTower;
    dropTower.centerX = 1180.0;
    dropTower.topY = 130.0;
    dropTower.groundY = horizonY;
    const double seatBottom = horizonY - 40.0;
    const double seatTop = dropTower.topY + 40.0;
    dropTower.seatY = seatBottom + (seatTop - seatBottom) * dropTowerLiftAt(time);
    dropTower.draw(frameBuffer);

    RollerCoaster coaster;
    coaster.controlPoints = coasterProfilePoints();
    coaster.groundY = coasterProfileGroundY;
    coaster.trainDistance = coasterTrainFractionAt(time);
    coaster.draw(frameBuffer);

    drawAstroworldSign(frameBuffer, {26.0, 236.0}, 190.0, 42.0, horizonY);
    drawBigTopTent(frameBuffer, {128.0, double(horizonY)}, 250.0, 160.0);

    // ---- Fence and plaza --------------------------------------------------------------
    drawHorizonFence(frameBuffer, horizonY, 46);
    drawGround(frameBuffer, horizonY, night, 57u);

    // ---- The head, with bunting strung out behind it ---------------------------------
    AstroHead head;
    head.scale = 372.0;
    head.center = {width * 0.5, 560.0 - 0.5 * head.scale};
    head.rimColor = lerpColor(Color{120, 170, 230}, astroPalette::neonPink, night);
    head.glowStrength = (0.85 + 0.45 * night) * (1.0 + oscillate(time, 0.8, 0.12));
    head.breathingScale = 1.0 + oscillate(time, 0.25, 0.012);
    head.dreadSwayAngle = oscillate(time, 0.3, 0.035) + oscillate(time, 0.71, 0.012);

    drawBunting(frameBuffer, {head.center.x - 120.0, 300.0}, {40.0, 380.0}, 50.0, 14, 0, time);
    drawBunting(frameBuffer, {head.center.x - 100.0, 340.0}, {230.0, 455.0}, 30.0, 10, 2, time);
    drawBunting(frameBuffer, {head.center.x + 120.0, 300.0}, {1240.0, 380.0}, 50.0, 14, 1, time);
    drawBunting(frameBuffer, {head.center.x + 100.0, 340.0}, {1050.0, 455.0}, 30.0, 10, 3, time);
    head.draw(frameBuffer);

    // ---- Foreground -------------------------------------------------------------------
    drawRocket(frameBuffer, {330.0, 600.0}, 300.0);

    Carousel carousel;
    carousel.baseCenter = {1075.0, 655.0};
    carousel.radius = 92.0;
    carousel.rotationAngle = 0.2 + 0.7 * time;
    carousel.bobPhase = 2.0 * pi * 0.8 * time;
    carousel.draw(frameBuffer);

    drawCrowd(frameBuffer, {head.center.x, head.groundY()}, time, 99u);
    drawBalloonVendor(frameBuffer, {165.0, 712.0}, time);

    // ---- "Greetings from ASTROWORLD - Wish you were here" -----------------------------
    WordmarkStyle style;
    style.letterHeight = 68.0;
    const double logoWidth = measureWordmark(style);
    const double logoLeft = (width - logoWidth) * 0.5;
    drawCaption(frameBuffer, "GREETINGS FROM", roundToInt(logoLeft + 72.0), 6, 2);
    drawWordmark(frameBuffer, {logoLeft, 24.0}, style);
    drawCaption(frameBuffer, "WISH YOU WERE HERE", width / 2, sceneHeight - 26, 2);
}
