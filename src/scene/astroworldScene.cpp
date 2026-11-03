/**
 * @file astroworldScene.cpp
 * @brief Scene layout: where every ride and prop sits in the 1280x720 frame.
 *
 * The composition follows the album cover: the head in the centre with its
 * mouth open on the ground, carnival rides and tents behind a black fence,
 * a rocket statue to the left, bunting strung out from the head, and the
 * "Greetings from ASTROWORLD" postcard logo across the top.
 */
#include "scene/astroworldScene.h"

#include <algorithm>
#include <string>

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

void renderAstroworldScene(FrameBuffer& frameBuffer, const SceneSettings& settings) {
    const int width = frameBuffer.getWidth();
    const double night = std::clamp(settings.nightAmount, 0.0, 1.0);
    const int horizonY = 500;  // base of the fence / far edge of the plaza

    // ---- Sky ------------------------------------------------------------------------
    drawSky(frameBuffer, horizonY, night);
    drawStars(frameBuffer, horizonY, 7u, night);
    drawClouds(frameBuffer, horizonY, 31u, 1.0 - night);
    drawSearchlight(frameBuffer, {420.0, horizonY + 10.0}, 74.0, 6.0, astroPalette::neonCyan, night);
    drawSearchlight(frameBuffer, {880.0, horizonY + 10.0}, 108.0, 6.0, astroPalette::neonPink, night);
    drawSkyline(frameBuffer, horizonY, 21u, lerpColor(astroPalette::daySkyline, astroPalette::nightSkyline, night));

    // ---- Behind the fence: rides, sign and big top ----------------------------------
    FerrisWheel ferrisWheel;
    ferrisWheel.hubPosition = {300.0, 330.0};
    ferrisWheel.radius = 132;
    ferrisWheel.gondolaCount = 12;
    ferrisWheel.rotationAngle = 0.12;
    ferrisWheel.groundY = horizonY;
    ferrisWheel.draw(frameBuffer);

    DropTower dropTower;
    dropTower.centerX = 1180.0;
    dropTower.topY = 130.0;
    dropTower.groundY = horizonY;
    dropTower.seatY = 300.0;
    dropTower.draw(frameBuffer);

    RollerCoaster coaster;
    coaster.controlPoints = {{830.0, 500.0}, {890.0, 390.0}, {955.0, 235.0}, {1025.0, 300.0}, {1075.0, 440.0},
                             {1135.0, 470.0}, {1190.0, 345.0}, {1240.0, 265.0}, {1300.0, 330.0}};
    coaster.groundY = horizonY;
    coaster.trainDistance = 0.3;
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
    head.glowStrength = 0.85 + 0.45 * night;

    drawBunting(frameBuffer, {head.center.x - 120.0, 300.0}, {40.0, 380.0}, 50.0, 14, 0);
    drawBunting(frameBuffer, {head.center.x - 100.0, 340.0}, {230.0, 455.0}, 30.0, 10, 2);
    drawBunting(frameBuffer, {head.center.x + 120.0, 300.0}, {1240.0, 380.0}, 50.0, 14, 1);
    drawBunting(frameBuffer, {head.center.x + 100.0, 340.0}, {1050.0, 455.0}, 30.0, 10, 3);
    head.draw(frameBuffer);

    // ---- Foreground -------------------------------------------------------------------
    drawRocket(frameBuffer, {330.0, 600.0}, 300.0);

    Carousel carousel;
    carousel.baseCenter = {1075.0, 655.0};
    carousel.radius = 92.0;
    carousel.rotationAngle = 0.2;
    carousel.draw(frameBuffer);

    drawCrowd(frameBuffer, {head.center.x, head.groundY()}, 99u);
    drawBalloonVendor(frameBuffer, {165.0, 712.0});

    // ---- "Greetings from ASTROWORLD - Wish you were here" -----------------------------
    WordmarkStyle style;
    style.letterHeight = 68.0;
    const double logoWidth = measureWordmark(style);
    const double logoLeft = (width - logoWidth) * 0.5;
    drawCaption(frameBuffer, "GREETINGS FROM", roundToInt(logoLeft + 72.0), 6, 2);
    drawWordmark(frameBuffer, {logoLeft, 24.0}, style);
    drawCaption(frameBuffer, "WISH YOU WERE HERE", width / 2, sceneHeight - 26, 2);
}
