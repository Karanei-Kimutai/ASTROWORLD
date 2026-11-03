/**
 * @file astroworldPark.cpp
 * @brief Building the GPU scene and drawing the animated park.
 *
 * World units are roughly metres: y up, the head stands at the origin with its
 * mouth facing +z, the fence runs along z = -9.5 and the rides stand behind it.
 *
 * Each frame:
 *   1. the sky triangle (gradient, stars, clouds) fills the background
 *   2. every object is drawn with its MODEL matrix; the shared VIEW (camera)
 *      and PROJECTION (perspective) matrices go to the vertex shader once
 *   3. the depth buffer (z-buffer) resolves visibility per sample - no
 *      painter's sorting needed any more
 *   4. overlays (logo, captions) are alpha-blended on top in screen space
 */
#include "glScene/astroworldPark.h"

#include <algorithm>
#include <cmath>

#include "animation/astroworldTimeline.h"
#include "animation/easing.h"
#include "core/simpleRandom.h"
#include "glScene/headModel.h"
#include "glScene/parkModels.h"
#include "glScene/shaders.h"
#include "mesh/meshBuilder.h"
#include "text/astroWordmark.h"
#include "text/bitmapFont.h"

namespace {

// Where each ride stands in the world.
const Vec3 ferrisPosition{-8.0, 0.0, -17.0};
const Vec3 dropTowerPosition{16.5, 0.0, -20.0};
const Vec3 carouselPosition{9.5, 0.0, 4.0};
const Vec3 rocketPosition{-6.5, 0.0, 3.5};
const Vec3 bigTopPosition{-14.0, 0.0, -13.5};
const Vec3 signPosition{-21.0, 0.0, -11.0};
const Vec3 vendorPosition{-9.5, 0.0, 8.5};
const double fenceZ = -9.5;

Vec3 lerpVector(const Vec3& a, const Vec3& b, double t) { return a + (b - a) * t; }

/** @brief Bitmap text with a drop shadow, for the software-rendered caption textures. */
void drawShadowedText(FrameBuffer& frameBuffer, const std::string& text, int left, int top, int scale) {
    drawBitmapText(frameBuffer, text, left + 2, top + 2, scale, astroPalette::black);
    drawBitmapText(frameBuffer, text, left, top, scale, astroPalette::white);
}

}  // namespace

bool AstroworldPark::initialise(std::string& errorOut) {
    if (!litShader.build(shaders::litVertex, shaders::litFragment, errorOut)) return false;
    if (!skyShader.build(shaders::skyVertex, shaders::skyFragment, errorOut)) return false;
    if (!overlayShader.build(shaders::overlayVertex, shaders::overlayFragment, errorOut)) return false;
    glGenVertexArrays(1, &emptyVertexArray);

    // ---- Static meshes: built once on the CPU, uploaded once to the GPU -------------
    ground.upload(makeGroundPlane(400.0, 400.0, astroPalette::white));
    fence.upload(buildFence(45.0));
    head.upload(buildHeadMesh());
    portal.upload(buildPortalMesh());
    ferrisStand.upload(buildFerrisStand());
    ferrisRotor.upload(buildFerrisRotor());
    ferrisBulbs.upload(buildFerrisBulbs());
    gondola.upload(buildGondola());
    coasterPath = buildCoasterTrackPath();
    coasterTrack.upload(buildCoasterTrack(coasterPath));
    coasterTruss.upload(buildCoasterTruss(coasterPath), GL_LINES);
    coasterCar.upload(buildCoasterCar());
    dropColumn.upload(buildDropTowerColumn());
    dropLights.upload(buildDropTowerLights());
    dropSeats.upload(buildDropTowerSeats());
    carouselBase.upload(buildCarouselBase());
    carouselRotor.upload(buildCarouselRotor());
    carouselBulbs.upload(buildCarouselBulbs());
    horse.upload(buildCarouselHorse());
    rocket.upload(buildRocket());
    bigTop.upload(buildBigTop());
    signFrame.upload(buildSignFrame());
    signFace.upload(buildSignFace());
    buntingFlags.upload(buildBuntingFlags());
    buntingStrings.upload(buildBuntingStrings(), GL_LINES);
    personTorso.upload(buildPersonTorso());
    personLeg.upload(buildPersonLeg());
    balloon.upload(buildBalloon());
    overlayQuad.upload(makeTexturedQuad(2.0, 2.0, astroPalette::white));

    // Arc length along the coaster, for placing cars by distance.
    coasterCumulativeLength.assign(coasterPath.size(), 0.0);
    for (std::size_t index = 1; index < coasterPath.size(); ++index) {
        coasterCumulativeLength[index] = coasterCumulativeLength[index - 1] + (coasterPath[index] - coasterPath[index - 1]).length();
    }

    // ---- Textures drawn by the CP1/CP2 software renderer ------------------------------
    // Park sign lettering (opaque).
    uploadImage(signTexture, renderOverlayImage(590, 115, [](FrameBuffer& canvas) {
                    canvas.clear(Color{232, 236, 244});
                    drawBitmapText(canvas, "ASTROWORLD", (590 - measureBitmapText("ASTROWORLD", 9)) / 2, (115 - 63) / 2, 9, Color{36, 40, 64});
                }));
    // The postcard logo: "GREETINGS FROM" + the anti-aliased ASTROWORLD wordmark.
    WordmarkStyle style;
    style.letterHeight = 66.0;
    const int logoWidth = static_cast<int>(measureWordmark(style)) + 24;
    uploadImage(logoTexture, renderOverlayImage(logoWidth, 112, [style](FrameBuffer& canvas) {
                    drawShadowedText(canvas, "GREETINGS FROM", 12, 2, 2);
                    drawWordmark(canvas, {10.0, 24.0}, style);
                }));
    uploadImage(footerTexture, renderOverlayImage(240, 20, [](FrameBuffer& canvas) {
                    drawShadowedText(canvas, "WISH YOU WERE HERE", (240 - measureBitmapText("WISH YOU WERE HERE", 2)) / 2, 2, 2);
                }));
    return true;
}

void AstroworldPark::drawLit(const GpuMesh& mesh, const Matrix4& model, Color tint, double emissive, double specularStrength,
                             double shininess) {
    litShader.setMatrix("model", model);
    litShader.setColor("tint", tint);
    litShader.setFloat("emissive", emissive);
    litShader.setFloat("specularStrength", specularStrength);
    litShader.setFloat("shininess", shininess);
    mesh.draw();
}

void AstroworldPark::drawSky(const ParkFrameSettings& settings, const Matrix4& view, const Matrix4& projection) {
    const double night = settings.nightAmount;
    skyShader.use();
    skyShader.setMatrix("inverseViewProjection", (projection * view).inverse());
    skyShader.setVec3("cameraPosition", settings.camera.eyePosition());
    // Same day/night palette blend as the software sky.
    skyShader.setColor("skyTop", lerpColor(astroPalette::daySkyTop, astroPalette::nightSkyTop, night));
    skyShader.setColor("skyMid", lerpColor(astroPalette::daySkyMid, astroPalette::nightSkyMid, night));
    skyShader.setColor("skyHorizon", lerpColor(astroPalette::daySkyHorizon, astroPalette::nightSkyHorizon, night));
    skyShader.setFloat("nightAmount", night);
    skyShader.setFloat("timeSeconds", settings.timeSeconds);
    // The sky is infinitely far away: draw it first without touching the depth buffer.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindVertexArray(emptyVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void AstroworldPark::setLighting(const ParkFrameSettings& settings, const Matrix4& view, const Matrix4& projection) {
    const double night = settings.nightAmount;
    const double time = settings.timeSeconds;
    litShader.use();
    litShader.setMatrix("view", view);
    litShader.setMatrix("projection", projection);
    litShader.setVec3("cameraPosition", settings.camera.eyePosition());
    // The sun (from the upper left, as on the album cover) becomes a dim blue moon at night.
    litShader.setVec3("sunDirection", Vec3(-0.45, 0.75, 0.55).normalized());
    litShader.setVec3("sunColor", lerpVector({1.0, 0.95, 0.85}, {0.18, 0.2, 0.36}, night));
    litShader.setVec3("ambientColor", lerpVector({0.42, 0.47, 0.58}, {0.09, 0.08, 0.18}, night));
    // The mouth's light grows stronger as the sky darkens, and pulses gently.
    litShader.setVec3("portalPosition", portalLightPosition());
    litShader.setVec3("portalColor", {1.0, 0.68, 0.3});
    litShader.setFloat("portalIntensity", (0.6 + 2.6 * night) * (1.0 + oscillate(time, 0.8, 0.12)));
    litShader.setColor("fogColor", lerpColor(astroPalette::daySkyHorizon, astroPalette::nightSkyHorizon, night));
    litShader.setFloat("fogDensity", 0.009);
    litShader.setInt("useTexture", 0);
    litShader.setInt("diffuseTexture", 0);
}

void AstroworldPark::drawRides(double time) {
    // ---- Ferris wheel: a transformation hierarchy ------------------------------------
    // rotor = T(hub) * Rz(-angle): negative so it turns clockwise as seen from the front, like the 2D wheel.
    const double wheelAngle = -(0.12 + 0.25 * time);
    const Matrix4 hubModel = Matrix4::translation(ferrisPosition + Vec3(0, ferrisHubHeight, 0));
    const Matrix4 rotorModel = hubModel * Matrix4::rotationZ(wheelAngle);
    drawLit(ferrisStand, Matrix4::translation(ferrisPosition));
    drawLit(ferrisRotor, rotorModel, astroPalette::white, 0.0, 0.5, 32.0);
    drawLit(ferrisBulbs, rotorModel, astroPalette::white, 1.0);
    const double swing = oscillate(time, 0.4, 0.08);
    for (int index = 0; index < ferrisGondolaCount; ++index) {
        const double spokeAngle = 2.0 * pi * index / ferrisGondolaCount;
        // The mount point rides on the rotor, but the gondola takes only its POSITION, so it hangs level.
        const Vec3 mount = rotorModel.transformPoint(Vec3(std::cos(spokeAngle), std::sin(spokeAngle), 0.0) * ferrisRadius);
        drawLit(gondola, Matrix4::translation(mount) * Matrix4::rotationZ(swing * std::cos(index * 1.3)),
                hsvToRgb(index * 360.0 / ferrisGondolaCount, 0.7, 0.95), 0.0, 0.4, 24.0);
    }

    // ---- Roller coaster: cars placed by arc length, oriented along the track ----------
    drawLit(coasterTrack, Matrix4::identity(), astroPalette::white, 0.0, 0.6, 40.0);
    drawLit(coasterTruss, Matrix4::identity(), astroPalette::white, 0.85);
    const double totalLength = coasterCumulativeLength.back();
    const double leadDistance = coasterTrainFractionAt(time) * totalLength;
    for (int car = 0; car < 3; ++car) {
        const double distance = leadDistance - car * 1.35;
        if (distance < 0.0) continue;
        const auto upper = std::lower_bound(coasterCumulativeLength.begin(), coasterCumulativeLength.end(), distance);
        const std::size_t index = std::clamp<std::size_t>(static_cast<std::size_t>(upper - coasterCumulativeLength.begin()), 1, coasterPath.size() - 1);
        const double segment = coasterCumulativeLength[index] - coasterCumulativeLength[index - 1];
        const double t = segment > 1e-9 ? (distance - coasterCumulativeLength[index - 1]) / segment : 0.0;
        const Vec3 position = lerpVec3(coasterPath[index - 1], coasterPath[index], t);
        // Car axes: forward along the tangent, up perpendicular to it, sideways from their cross product.
        const Vec3 forward = (coasterPath[index] - coasterPath[index - 1]).normalized();
        const Vec3 lateral = crossProduct(forward, {0, 1, 0}).normalized();
        const Vec3 up = crossProduct(lateral, forward);
        drawLit(coasterCar, Matrix4::fromBasis(forward, up, crossProduct(forward, up), position + up * 0.1),
                hsvToRgb(190.0 + car * 40.0, 0.8, 1.0), 0.0, 0.6, 48.0);
    }

    // ---- Drop tower: keyframed seat ring -----------------------------------------------
    drawLit(dropColumn, Matrix4::translation(dropTowerPosition), astroPalette::white, 0.0, 0.4, 32.0);
    drawLit(dropLights, Matrix4::translation(dropTowerPosition), astroPalette::white, 1.0);
    const double seatHeight = 1.4 + dropTowerLiftAt(time) * (dropTowerHeight - 3.0);
    drawLit(dropSeats, Matrix4::translation(dropTowerPosition + Vec3(0, seatHeight, 0)), astroPalette::white, 0.0, 0.5, 32.0);

    // ---- Carousel: 3D rotation about y, horses bobbing on their poles ------------------
    const double carouselAngle = 0.2 + 0.7 * time;
    const Matrix4 carouselModel = Matrix4::translation(carouselPosition) * Matrix4::rotationY(carouselAngle);
    drawLit(carouselBase, Matrix4::translation(carouselPosition));
    drawLit(carouselRotor, carouselModel, astroPalette::white, 0.0, 0.35, 24.0);
    drawLit(carouselBulbs, carouselModel, astroPalette::white, 1.0);
    for (int index = 0; index < 8; ++index) {
        const double bob = 0.18 * std::sin(2.0 * pi * 0.8 * time + index * 1.7);
        const Matrix4 horseModel = carouselModel * Matrix4::rotationY(index * 2.0 * pi / 8.0) *
                                   Matrix4::translation({carouselHorseRadius, 1.3 + bob, 0.0});
        drawLit(horse, horseModel, index % 2 == 0 ? astroPalette::cream : Color{255, 160, 205}, 0.0, 0.5, 32.0);
    }

    // ---- Static props -------------------------------------------------------------------
    drawLit(rocket, Matrix4::translation(rocketPosition), astroPalette::white, 0.0, 0.9, 64.0);
    drawLit(bigTop, Matrix4::translation(bigTopPosition));
    drawLit(signFrame, Matrix4::translation(signPosition));
    litShader.setInt("useTexture", 1);
    signTexture.bind(0);
    drawLit(signFace, Matrix4::translation(signPosition));
    litShader.setInt("useTexture", 0);
    drawLit(buntingFlags, Matrix4::identity());
    drawLit(buntingStrings, Matrix4::identity(), astroPalette::white, 1.0);
}

void AstroworldPark::drawHead(double time, double night) {
    // Inflatable breathing: scale about the base point on the ground. In local
    // coordinates the base IS the origin, so T(base) * S * T(-base) reduces to S.
    const double breathing = 1.0 + oscillate(time, 0.25, 0.012);
    const Matrix4 headModel = Matrix4::scaling({breathing, breathing, breathing});
    drawLit(head, headModel, astroPalette::white, 0.0, 0.45, 18.0);
    dreads.upload(buildDreadMesh(oscillate(time, 0.3, 0.035) + oscillate(time, 0.71, 0.012)));
    drawLit(dreads, headModel, astroPalette::white, 0.0, 0.45, 18.0);
    // The doorway glows: emissive, a little brighter at night.
    drawLit(portal, headModel, scaleColor(astroPalette::white, 0.9 + 0.1 * night), 1.0);
}

void AstroworldPark::drawPeople(double time) {
    // ---- Visitors walking into the mouth (same idea as the 2D crowd, now in 3D) ---------
    // Visitors aim for the doorway panel, which stands a little behind the portal light.
    const double doorwayZ = portalLightPosition().z - 1.2;
    SimpleRandom random(99u);
    for (int index = 0; index < 14; ++index) {
        const Vec3 start{random.nextRange(-13.0, 13.0), 0.0, random.nextRange(15.0, 19.0)};
        const Vec3 end{random.nextRange(-0.6, 0.6), 0.0, doorwayZ};
        const double walkDuration = random.nextRange(12.0, 18.0);
        const double phase = random.nextDouble();
        const Color clothes = hsvToRgb(random.nextRange(0.0, 360.0), 0.55, 0.75);

        const double progress = wrapTime(time / walkDuration + phase, 1.0);
        const Vec3 feet = lerpVec3(start, end, progress);
        const double heading = std::atan2(end.x - start.x, end.z - start.z);
        // Shrink into the light over the last 6% of the walk (scaling about the feet).
        const double size = progress > 0.94 ? (1.0 - progress) / 0.06 : 1.0;
        const double stride = 2.0 * pi * 1.4 * time + index;
        const Matrix4 bodyModel = Matrix4::translation(feet + Vec3(0, std::fabs(std::sin(stride)) * 0.04, 0)) *
                                  Matrix4::rotationY(heading) * Matrix4::scaling({size, size, size});
        drawLit(personTorso, bodyModel, clothes);
        // Legs: children of the body, swinging about the hip in opposite phase.
        for (double side : {-1.0, 1.0}) {
            drawLit(personLeg, bodyModel * Matrix4::translation({side * 0.1, 0.88, 0}) * Matrix4::rotationX(side * 0.4 * std::sin(stride)));
        }
    }

    // ---- Balloon vendor ------------------------------------------------------------------
    const Matrix4 vendorModel = Matrix4::translation(vendorPosition) * Matrix4::rotationY(0.4);
    drawLit(personTorso, vendorModel, Color{200, 60, 60});
    for (double side : {-1.0, 1.0}) drawLit(personLeg, vendorModel * Matrix4::translation({side * 0.1, 0.88, 0}));
    const Vec3 hand = vendorPosition + Vec3(0.3, 1.45, 0.1);
    const Vec3 offsets[5] = {{-0.6, 2.3, 0.0}, {0.1, 2.65, 0.2}, {0.75, 2.2, -0.1}, {-0.25, 1.7, 0.3}, {0.5, 1.6, 0.25}};
    const Color colors[5] = {astroPalette::neonPink, astroPalette::gold, astroPalette::neonCyan, astroPalette::neonPurple, astroPalette::neonOrange};
    std::vector<Vec3> stringEnds;
    for (int index = 0; index < 5; ++index) {
        // The bunch sways about the hand (rotation about a pivot, now around the z axis in 3D).
        const double sway = oscillate(time, 0.25, 0.08) + oscillate(time, 0.6, 0.03, index * 1.9);
        const Vec3 center = hand + Matrix4::rotationZ(sway).transformPoint(offsets[index]);
        drawLit(balloon, Matrix4::translation(center), colors[index], 0.0, 0.8, 48.0);
        stringEnds.insert(stringEnds.end(), {hand, center + Vec3(0, -0.45, 0)});
    }
    balloonStrings.upload(makeLines(stringEnds, Color{230, 225, 235}), GL_LINES);
    drawLit(balloonStrings, Matrix4::identity(), astroPalette::white, 1.0);
}

void AstroworldPark::render(const ParkFrameSettings& settings) {
    const double aspect = static_cast<double>(renderWidth) / renderHeight;
    const Matrix4 view = settings.camera.viewMatrix();
    const Matrix4 projection = settings.camera.projectionMatrix(aspect);
    const double night = settings.nightAmount;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    drawSky(settings, view, projection);
    setLighting(settings, view, projection);

    // Ground: sandy concrete by day, dark plaza by night (tint), lit by sun and portal.
    drawLit(ground, Matrix4::identity(), lerpColor(astroPalette::dayGroundNear, Color{70, 60, 80}, night), 0.0, 0.05, 8.0);
    drawLit(fence, Matrix4::translation({0, 0, fenceZ}), astroPalette::white, 0.0, 0.3, 16.0);

    drawRides(settings.timeSeconds);
    drawHead(settings.timeSeconds, night);
    drawPeople(settings.timeSeconds);

    if (settings.showLogo) {
        drawOverlay(logoTexture, (renderWidth - logoTexture.getWidth()) / 2, 6);
        drawOverlay(footerTexture, (renderWidth - footerTexture.getWidth()) / 2, renderHeight - 28);
    }
}

void AstroworldPark::drawOverlay(const Texture2D& texture, int left, int top) {
    if (!texture.isValid()) return;
    // Map the pixel rectangle to normalised device coordinates (y up) and stretch the 2x2 quad over it.
    const double width = texture.getWidth() * 2.0 / renderWidth;
    const double height = texture.getHeight() * 2.0 / renderHeight;
    const double centerX = -1.0 + (left + texture.getWidth() * 0.5) * 2.0 / renderWidth;
    const double centerY = 1.0 - (top + texture.getHeight() * 0.5) * 2.0 / renderHeight;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // straight alpha "over" compositing
    overlayShader.use();
    overlayShader.setMatrix("model", Matrix4::translation({centerX, centerY, 0.0}) * Matrix4::scaling({width * 0.5, height * 0.5, 1.0}));
    overlayShader.setInt("overlayTexture", 0);
    texture.bind(0);
    overlayQuad.draw();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}
