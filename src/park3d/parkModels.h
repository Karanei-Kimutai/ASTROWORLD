/**
 * @file parkModels.h
 * @brief 3D meshes for every ride and prop, each in its own LOCAL coordinates
 *        (origin on the ground at the object's centre, y up).
 *
 * Parts that move are returned as separate meshes so the renderer can place
 * them with their own model matrices every frame (e.g. the Ferris wheel's
 * rotor, its gondolas, the drop tower's seat ring).
 */
#pragma once

#include <vector>

#include "mesh/meshData.h"

// ---- Ferris wheel --------------------------------------------------------------
inline constexpr double ferrisHubHeight = 7.5;
inline constexpr double ferrisRadius = 5.2;
inline constexpr int ferrisGondolaCount = 12;
MeshData buildFerrisStand();                    // A-frame legs (static)
MeshData buildFerrisRotor();                    // rims, spokes, hub - rotates about z at the hub
MeshData buildFerrisBulbs();                    // emissive bulbs on the rim - rotate with the rotor
MeshData buildGondola();                        // one cabin, hanging below its origin (tinted per gondola)

// ---- Roller coaster -------------------------------------------------------------
/** @brief The 3D track centre line, from the shared 2D profile (arc-length sampled). */
std::vector<Vec3> buildCoasterTrackPath();
MeshData buildCoasterTrack(const std::vector<Vec3>& path);       // rails, spine, supports
MeshData buildCoasterTruss(const std::vector<Vec3>& path);       // GL_LINES lattice
MeshData buildCoasterCar();                                       // local x = forward, y = up

// ---- Drop tower -----------------------------------------------------------------
inline constexpr double dropTowerHeight = 15.5;
MeshData buildDropTowerColumn();
MeshData buildDropTowerLights();                // emissive chase lights and beacon
MeshData buildDropTowerSeats();                 // ring of seats, origin at its centre

// ---- Carousel -------------------------------------------------------------------
inline constexpr double carouselHorseRadius = 2.2;
MeshData buildCarouselBase();                   // static plinth
MeshData buildCarouselRotor();                  // platform, column, poles, canopy - rotates about y
MeshData buildCarouselBulbs();                  // emissive bulbs on the canopy rim
MeshData buildCarouselHorse();                  // faces -z (the direction of travel)

// ---- Props ----------------------------------------------------------------------
MeshData buildRocket();
MeshData buildBigTop();
MeshData buildSignFrame();                      // posts and board
MeshData buildSignFace();                       // textured quad with the lettering
MeshData buildFence(double halfLength);
MeshData buildBuntingFlags();                   // triangles (double-sided) and their poles
MeshData buildBuntingStrings();                 // GL_LINES
MeshData buildPersonTorso();                    // torso + head, feet at origin, facing +z
MeshData buildPersonLeg();                      // hangs down from its origin (the hip)
MeshData buildBalloon();

// ---- Night effects ----------------------------------------------------------------
inline constexpr double searchlightBeamLength = 70.0;
/**
 * @brief An open cone pointing up +y from its apex at the origin. Texture
 *        coordinate v runs 0 at the lamp to 1 at the far end, so the beam
 *        shader can fade the light along its length.
 */
MeshData buildSearchlightBeam();
