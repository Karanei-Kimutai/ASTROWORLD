/**
 * @file parkRides.h
 * @brief The expanded park: more rides, stalls, trees, lamps and the cover's props.
 *
 * Each builder returns a mesh in LOCAL coordinates (origin on the ground at the
 * object's centre, y up, front toward +z unless stated). Moving parts are
 * separate meshes so the scene can animate them with their own model matrices.
 */
#pragma once

#include <vector>

#include "mesh/meshData.h"

// ---- Wave swinger: chairs on chains flung outward by the spin -------------------------
inline constexpr double swingTowerHeight = 11.0;
inline constexpr double swingCanopyRadius = 5.0;
inline constexpr double swingChainLength = 5.0;
inline constexpr int swingChairCount = 16;
MeshData buildSwingTower();
MeshData buildSwingCanopy();   // rotates (and tilts) about the tower top; origin at the hub
MeshData buildSwingChair();    // chains + seat hanging DOWN from its origin (the canopy rim)

// ---- Pirate ship: a pendulum ----------------------------------------------------------
inline constexpr double shipPivotHeight = 10.0;
inline constexpr double shipArmLength = 7.5;
MeshData buildShipFrame();
MeshData buildShipHull();      // origin at the pivot, hull hanging below it; swings about x

// ---- Teacups: three levels of rotation --------------------------------------------------
inline constexpr double teacupPlatformRadius = 7.0;
inline constexpr double teacupTurntableOrbit = 4.0;
inline constexpr double teacupCupOrbit = 1.4;
MeshData buildTeacupPlatform();
MeshData buildTeacupTurntable();
MeshData buildTeacup();

// ---- Bumper cars ------------------------------------------------------------------------
inline constexpr double bumperArenaHalfWidth = 8.0;
inline constexpr double bumperArenaHalfDepth = 6.0;
MeshData buildBumperArena();
MeshData buildBumperCar();     // faces +z

// ---- Park furniture --------------------------------------------------------------------
MeshData buildFoodStall(Color stripeA, Color stripeB);
MeshData buildTree(int variant);
/** @brief A unit-radius disc of lawn, scaled per use (sits just above the paving). */
MeshData buildGrassPatch();
MeshData buildLampPost();
MeshData buildLampGlobe();     // the glowing part, drawn emissive at night
inline constexpr double lampGlobeHeight = 4.6;
MeshData buildStatue();        // gold figure on a plinth (album cover, left of the head)
MeshData buildWreckedCar();    // burnt-out car (album cover, right of the head)
MeshData buildExitGate();
/** @brief A flat paved ribbon along a closed path (the visitors' walkway). */
MeshData buildPathRibbon(const std::vector<Vec3>& path, double width);
MeshData buildEntranceWall(double fromX, double toX);
