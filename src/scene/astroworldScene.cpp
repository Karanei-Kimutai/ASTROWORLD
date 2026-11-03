/**
 * @file astroworldScene.cpp
 * @brief Scene layout: where every ride and prop sits in the 1280x720 frame.
 */
#include "scene/astroworldScene.h"

#include "scene/astroHead.h"
#include "scene/backdrop.h"
#include "scene/carousel.h"
#include "scene/dropTower.h"
#include "scene/ferrisWheel.h"
#include "scene/foreground.h"
#include "scene/rollerCoaster.h"
#include "text/astroWordmark.h"

void renderAstroworldScene(FrameBuffer& frameBuffer) {
    const int width = frameBuffer.getWidth();
    const int horizonY = 600;

    // Path geometry shared by the path itself and the crowd walking on it.
    const double pathTopLeftX = 590.0;
    const double pathTopRightX = 690.0;
    const double pathBottomLeftX = 430.0;
    const double pathBottomRightX = 850.0;

    // ---- Far background -----------------------------------------------------------
    drawSkyGradient(frameBuffer, horizonY);
    drawStars(frameBuffer, horizonY, 7u);
    drawHorizonGlow(frameBuffer, {width * 0.5, double(horizonY)}, 560, 170);
    drawSearchlight(frameBuffer, {330.0, horizonY + 10.0}, 68.0, 6.0, astroPalette::neonCyan);
    drawSearchlight(frameBuffer, {960.0, horizonY + 10.0}, 114.0, 6.0, astroPalette::neonPink);
    drawSearchlight(frameBuffer, {1180.0, horizonY + 10.0}, 96.0, 5.0, astroPalette::goldLight);
    drawSkyline(frameBuffer, horizonY, 21u);

    // ---- Rides behind the head ------------------------------------------------------
    RollerCoaster coaster;
    coaster.controlPoints = {{820.0, 600.0},  {875.0, 470.0},  {935.0, 285.0}, {995.0, 345.0},
                             {1050.0, 500.0}, {1110.0, 540.0}, {1170.0, 400.0}, {1220.0, 320.0},
                             {1270.0, 380.0}, {1310.0, 520.0}};
    coaster.groundY = horizonY;
    coaster.trainDistance = 0.28;
    coaster.draw(frameBuffer);

    FerrisWheel ferrisWheel;
    ferrisWheel.hubPosition = {205.0, 335.0};
    ferrisWheel.radius = 172;
    ferrisWheel.gondolaCount = 12;
    ferrisWheel.rotationAngle = 0.12;
    ferrisWheel.groundY = horizonY;
    ferrisWheel.draw(frameBuffer);

    DropTower dropTower;
    dropTower.centerX = 405.0;
    dropTower.topY = 150.0;
    dropTower.groundY = horizonY;
    dropTower.seatY = 320.0;
    dropTower.draw(frameBuffer);

    // ---- Ground and the path leading to the head -----------------------------------
    drawGround(frameBuffer, horizonY);
    drawEntrancePath(frameBuffer, horizonY, pathTopLeftX, pathTopRightX, pathBottomLeftX, pathBottomRightX);

    // ---- The golden head (chin resting on the horizon line) -----------------------
    AstroHead head;
    head.scale = 400.0;
    head.center = {width * 0.5, horizonY - 0.52 * head.scale};
    head.draw(frameBuffer);

    // ---- Foreground ------------------------------------------------------------------
    Carousel carousel;
    carousel.baseCenter = {1010.0, 650.0};
    carousel.radius = 98.0;
    carousel.rotationAngle = 0.2;
    carousel.draw(frameBuffer);

    drawCrowd(frameBuffer, horizonY, pathTopLeftX, pathTopRightX, pathBottomLeftX, pathBottomRightX, 99u);
    drawFenceWithLights(frameBuffer, 650.0, 705.0, pathBottomLeftX - 20.0, pathBottomRightX + 20.0);
    drawBalloonVendor(frameBuffer, {345.0, 712.0});

    // ---- Title wordmark, sized to ~80% of the frame width and centred --------------
    WordmarkStyle style;
    style.letterHeight = 1.0;
    const double unitWidth = measureWordmark("ASTROWORLD", style);  // width per pixel of letter height
    style.letterHeight = width * 0.8 / unitWidth;
    const double wordmarkWidth = measureWordmark("ASTROWORLD", style);
    drawWordmark(frameBuffer, "ASTROWORLD", {(width - wordmarkWidth) * 0.5, 22.0}, style);
}
