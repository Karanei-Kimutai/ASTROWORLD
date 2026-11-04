/**
 * @file headModel.cpp
 * @brief Sculpting the inflatable head.
 */
#include "park3d/headModel.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "curves/bezier.h"
#include "curves/spline.h"
#include "math/matrix3.h"
#include "mesh/meshBuilder.h"
#include "scene/astroHeadShape.h"

namespace {

// Ellipsoid semi-axes in head units, and its centre height (v = 0).
constexpr double radiusU = 0.43;
constexpr double radiusV = 0.56;
constexpr double radiusDepth = 0.45;

// The doorway: straight sides up to archStartV, then an elliptical arch peaking at archPeakV.
constexpr double doorwayHalfWidth = 0.15;
constexpr double archStartV = 0.38;
constexpr double archPeakV = 0.255;

/** @brief v of the doorway's upper edge at horizontal position u (inside |u| < half-width). */
double doorwayTopV(double u) {
    const double across = std::clamp(u / doorwayHalfWidth, -1.0, 1.0);
    return archStartV - (archStartV - archPeakV) * std::sqrt(1.0 - across * across);
}

bool insideDoorway(double u, double v) { return std::fabs(u) < doorwayHalfWidth && v > doorwayTopV(u); }

double smoothStep(double edge0, double edge1, double x) {
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

/**
 * @brief Half-width and half-depth of the head's horizontal cross-section at height v.
 *
 * Above the cheekbones it is an ellipsoid. Below them a pure ellipsoid would curve
 * in to a point, like a balloon resting on its tip; the album cover's head instead
 * has a broad jaw whose sides run almost straight down into the ground. So below
 * v = 0 the profile eases from the ellipsoid into nearly vertical sides.
 */
void crossSection(double v, double& halfWidth, double& halfDepth) {
    const double ellipse = std::sqrt(std::max(0.0, 1.0 - (v / radiusV) * (v / radiusV)));
    halfWidth = radiusU * ellipse;
    halfDepth = radiusDepth * ellipse;
    if (v > 0.0) {
        const double blend = smoothStep(0.0, 0.22, v);
        halfWidth = halfWidth + ((radiusU - 0.08 * v) - halfWidth) * blend;
        halfDepth = halfDepth + ((radiusDepth - 0.06 * v) - halfDepth) * blend;
    }
}

/** @brief Depth of the (un-sculpted) head surface at (u, v), or 0 outside its outline. */
double ellipsoidDepth(double u, double v) {
    double halfWidth, halfDepth;
    crossSection(v, halfWidth, halfDepth);
    const double inside = 1.0 - (u / halfWidth) * (u / halfWidth);
    return inside > 0.0 && halfWidth > 1e-6 ? halfDepth * std::sqrt(inside) : 0.0;
}

/** @brief A soft bump: amplitude at (centreU, centreV), falling off as a 2D Gaussian. */
double bump(double u, double v, double centreU, double centreV, double spreadU, double spreadV, double amplitude) {
    const double du = (u - centreU) / spreadU;
    const double dv = (v - centreV) / spreadV;
    return amplitude * std::exp(-0.5 * (du * du + dv * dv));
}

/** @brief A crease along a polyline: negative displacement by distance to the curve. */
double crease(double u, double v, const std::vector<Vec2>& curve, double width, double depth) {
    double nearest = 1e9;
    for (std::size_t index = 0; index + 1 < curve.size(); ++index) {
        const Vec2 a = curve[index], b = curve[index + 1];
        const Vec2 ab = b - a;
        const double t = std::clamp(dotProduct(Vec2(u, v) - a, ab) / std::max(dotProduct(ab, ab), 1e-12), 0.0, 1.0);
        nearest = std::min(nearest, (Vec2(u, v) - (a + ab * t)).length());
    }
    return -depth * std::exp(-0.5 * (nearest / width) * (nearest / width));
}

/**
 * @brief How far the face surface is pushed out along its normal at (u, v), head units.
 *        The same feature positions as the 2D head (astroHead.cpp), now as relief.
 */
double faceRelief(double u, double v) {
    static const std::vector<Vec2> leftLash = sampleCubicBezier({-0.065, -0.062}, {-0.11, -0.036}, {-0.20, -0.036}, {-0.255, -0.068}, 12);
    static const std::vector<Vec2> rightLash = sampleCubicBezier({0.065, -0.062}, {0.11, -0.036}, {0.20, -0.036}, {0.255, -0.068}, 12);
    double relief = 0.0;
    // Nose: bridge ridge, broad rounded tip, flared wings, dents for the nostrils.
    relief += bump(u, v, 0.0, -0.01, 0.04, 0.11, 0.045);
    relief += bump(u, v, 0.0, 0.10, 0.055, 0.045, 0.075);
    relief += bump(u, v, -0.078, 0.125, 0.032, 0.03, 0.045) + bump(u, v, 0.078, 0.125, 0.032, 0.03, 0.045);
    relief += bump(u, v, -0.045, 0.152, 0.014, 0.01, -0.025) + bump(u, v, 0.045, 0.152, 0.014, 0.01, -0.025);
    for (double side : {-1.0, 1.0}) {
        relief += bump(u, v, side * 0.17, -0.165, 0.09, 0.03, 0.022);   // brow ridge
        relief += bump(u, v, side * 0.16, -0.07, 0.085, 0.045, -0.028); // eye socket
        relief += bump(u, v, side * 0.155, -0.075, 0.075, 0.03, 0.03);  // closed upper eyelid
        relief += bump(u, v, side * 0.28, 0.07, 0.09, 0.09, 0.028);     // cheek
        relief += bump(u, v, side * 0.17, 0.33, 0.05, 0.07, 0.035);     // corners of the lips
    }
    relief += crease(u, v, leftLash, 0.007, 0.012) + crease(u, v, rightLash, 0.007, 0.012);
    // The pouting upper lip that overhangs the doorway (as on the album cover).
    relief += bump(u, v, 0.0, 0.232, 0.15, 0.03, 0.06);
    relief += bump(u, v, 0.0, 0.205, 0.016, 0.022, -0.012);  // philtrum groove
    relief += bump(u, v, 0.0, 0.40, 0.2, 0.1, 0.045);        // the mass of the open mouth
    return relief;
}

/** @brief Inflatable seams: shallow grooves running down the sides and back of the skull. */
double seamRelief(const Vec3& unit) {
    const double azimuth = std::atan2(unit.x, unit.z);  // 0 at the front of the face
    double relief = 0.0;
    for (double seamAngle : {pi / 2.0, -pi / 2.0, 0.78 * pi, -0.78 * pi}) {
        double difference = std::remainder(azimuth - seamAngle, 2.0 * pi);
        relief -= 0.006 * std::exp(-0.5 * (difference / 0.012) * (difference / 0.012));
    }
    return relief;
}

/** @brief The doorway outline, from the ground up one side, over the arch and down the other. */
std::vector<Vec2> doorwayOutline() {
    std::vector<Vec2> outline = {{-doorwayHalfWidth, 0.5}, {-doorwayHalfWidth, archStartV}};
    for (int step = 1; step < 40; ++step) {
        const double u = -doorwayHalfWidth + 2.0 * doorwayHalfWidth * step / 40.0;
        outline.push_back({u, doorwayTopV(u)});
    }
    outline.push_back({doorwayHalfWidth, archStartV});
    outline.push_back({doorwayHalfWidth, 0.5});
    return outline;
}

/** @brief Depth (head units) of the sculpted face surface at a doorway outline point. */
double openingDepth(double u, double v) { return ellipsoidDepth(u, v) + faceRelief(u, v) * smoothStep(0.05, 0.3, ellipsoidDepth(u, v) / radiusDepth); }

/** @brief The inflated lips around both ends of the tunnel (gold, lit normally). */
MeshData buildMouthRims() {
    MeshData rims;
    const std::vector<Vec2> outline = doorwayOutline();
    for (double side : {1.0, -1.0}) {
        std::vector<Vec3> path;
        for (const Vec2& point : outline) path.push_back(Vec3(point.x, 0.5 - point.y, side * (openingDepth(point.x, point.y) - 0.012)) * headUnitSize);
        rims.append(makeTube(path, 0.03 * headUnitSize, 0.03 * headUnitSize, 16, astroPalette::brass));
    }
    return rims;
}

}  // namespace

MeshData buildHeadMesh() {
    const int stacks = 220;
    const int slices = 300;
    const Color brass = astroPalette::brass;

    // 1. Sculpt: sample the ellipsoid on a latitude/longitude grid and displace each point along its normal.
    std::vector<Vec3> positions;
    std::vector<Vec2> faceCoordinates;  // (u, v) of each vertex, for cutting the doorway
    positions.reserve(static_cast<std::size_t>((stacks + 1) * (slices + 1)));
    for (int stack = 0; stack <= stacks; ++stack) {
        const double polar = pi * stack / stacks;
        const double v = -radiusV * std::cos(polar);  // head units, v down
        double halfWidth, halfDepth;
        crossSection(v, halfWidth, halfDepth);
        for (int slice = 0; slice <= slices; ++slice) {
            const double azimuth = 2.0 * pi * slice / slices;
            Vec3 point{halfWidth * std::sin(azimuth), -v, halfDepth * std::cos(azimuth)};
            // Approximate outward normal of the cross-section surface (exact normals are rebuilt below).
            const Vec3 normal = Vec3(point.x / (halfWidth * halfWidth + 1e-6), point.y / (radiusV * radiusV), point.z / (halfDepth * halfDepth + 1e-6)).normalized();
            const Vec3 unit{std::sin(polar) * std::sin(azimuth), std::cos(polar), std::sin(polar) * std::cos(azimuth)};
            const double u = point.x;
            const double frontWeight = smoothStep(0.05, 0.35, std::cos(azimuth));  // features only on the face
            const double displacement = faceRelief(u, v) * frontWeight + seamRelief(unit);
            point = point + normal * displacement;
            positions.push_back(Vec3(point.x, point.y + 0.5, point.z) * headUnitSize);
            faceCoordinates.push_back({u, v});
        }
    }

    // 2. Triangles - skipping those inside the doorway, which opens the tunnel front and back.
    MeshData head;
    const int rowLength = slices + 1;
    std::vector<Vec3> normals(positions.size(), Vec3(0, 0, 0));
    std::vector<std::uint32_t> indices;
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            const std::uint32_t a = static_cast<std::uint32_t>(stack * rowLength + slice);
            const std::uint32_t b = a + static_cast<std::uint32_t>(rowLength);
            const std::uint32_t corners[2][3] = {{a, b, b + 1}, {a, b + 1, a + 1}};
            for (const auto& corner : corners) {
                const Vec2 centre = (faceCoordinates[corner[0]] + faceCoordinates[corner[1]] + faceCoordinates[corner[2]]) * (1.0 / 3.0);
                if (insideDoorway(centre.x, centre.y)) continue;
                // Area-weighted face normal, accumulated into each corner for smooth vertex normals.
                const Vec3 faceNormal = crossProduct(positions[corner[1]] - positions[corner[0]], positions[corner[2]] - positions[corner[0]]);
                for (std::uint32_t index : corner) normals[index] += faceNormal;
                indices.insert(indices.end(), {corner[0], corner[1], corner[2]});
            }
        }
    }
    for (std::size_t index = 0; index < positions.size(); ++index) {
        head.vertices.push_back(makeVertex(positions[index], normals[index].normalized(), brass));
    }
    head.indices = std::move(indices);
    // Inflated lips around both ends of the tunnel hide the cut edge.
    head.append(buildMouthRims());
    return head;
}

MeshData buildMouthTunnel() {
    MeshData tunnel;
    const std::vector<Vec2> outline = doorwayOutline();
    // Walls: the doorway outline extruded from the front opening to the back opening.
    // They GLOW (emissive), warm orange at the openings to white-hot deep inside,
    // like the light pouring out of the mouth on the album cover.
    const int depthSteps = 12;
    for (int step = 0; step <= depthSteps; ++step) {
        const double t = static_cast<double>(step) / depthSteps;  // 0 front .. 1 back
        const double heat = std::sin(t * pi);                      // hottest in the middle
        for (const Vec2& point : outline) {
            const double frontDepth = openingDepth(point.x, point.y) - 0.01;
            const double depth = frontDepth + (-frontDepth - frontDepth) * t;
            const Vec3 position = Vec3(point.x, 0.5 - point.y, depth) * headUnitSize;
            const Vec3 inward = Vec3(-point.x, -(0.5 - point.y) + 0.15, 0.0).normalized();
            const Color color = lerpColor(astroPalette::portalEdge, astroPalette::portalGlow, heat);
            tunnel.vertices.push_back(makeVertex(position, inward, color));
        }
    }
    const std::uint32_t ringSize = static_cast<std::uint32_t>(outline.size());
    for (std::uint32_t step = 0; step < static_cast<std::uint32_t>(depthSteps); ++step) {
        for (std::uint32_t index = 0; index + 1 < ringSize; ++index) {
            const std::uint32_t a = step * ringSize + index;
            const std::uint32_t b = a + ringSize;
            tunnel.addTriangle(a, a + 1, b + 1);
            tunnel.addTriangle(a, b + 1, b);
        }
    }
    return tunnel;
}

MeshData buildDreadMesh(double swayAngle) {
    using namespace astroHeadShape;
    struct DreadSpec {
        const Vec2* points;
        double side;
        double sway;
        double depth;  // how far forward (head units) the hanging part sits
    };
    const DreadSpec dreads[] = {
        {outerDread, -1.0, outerDreadSway, -0.12},   {outerDread, 1.0, outerDreadSway, -0.12},
        {middleDread, -1.0, middleDreadSway, 0.05},  {middleDread, 1.0, middleDreadSway, 0.05},
        {frontDreadLeft, -1.0, frontDreadSway, 0.2}, {frontDreadRight, 1.0, frontDreadSway, 0.2},
    };
    // Inflated tubes: fat, only gently tapering, with round ends.
    const double rootRadius = 0.05;
    const double tipRadius = 0.038;

    MeshData mesh;
    for (const DreadSpec& dread : dreads) {
        const Vec2 root{dread.side * dread.points[0].x, dread.points[0].y};
        std::vector<Vec3> controlPoints;
        for (int index = 0; index < dreadPointCount; ++index) {
            const double along = static_cast<double>(index) / (dreadPointCount - 1);
            const Vec2 unit = Matrix3::rotationAbout(root, swayAngle * dread.sway * along)
                                  .transformPoint({dread.side * dread.points[index].x, dread.points[index].y});
            const double radius = rootRadius + (tipRadius - rootRadius) * along;
            // Keep the tube clear of the face where it hangs in front of it.
            const double surface = ellipsoidDepth(unit.x, unit.y) + faceRelief(unit.x, unit.y) * 0.5;
            const double depth = std::max(dread.depth, surface + radius * 1.2);
            controlPoints.push_back(Vec3(unit.x, 0.5 - unit.y, depth) * headUnitSize);
        }
        // Root the dread inside the crown so it grows out of the head.
        controlPoints.front() = controlPoints.front() + Vec3(0.0, -0.04, -0.06) * headUnitSize;
        mesh.append(makeTube(sampleCatmullRomSpline3D(controlPoints, 12, false), rootRadius * headUnitSize, tipRadius * headUnitSize, 20,
                             astroPalette::brass));
    }
    // Two more dreads hanging down the back of the head.
    for (double side : {-1.0, 1.0}) {
        std::vector<Vec3> back = {Vec3(side * 0.12, 1.02, -0.18), Vec3(side * 0.24, 1.10, -0.32), Vec3(side * 0.36, 0.98, -0.40),
                                  Vec3(side * 0.42, 0.70, -0.40), Vec3(side * 0.44, 0.42, -0.36), Vec3(side * 0.48, 0.30, -0.34)};
        for (Vec3& point : back) point = point * headUnitSize;
        mesh.append(makeTube(sampleCatmullRomSpline3D(back, 12, false), rootRadius * headUnitSize, tipRadius * headUnitSize, 20, astroPalette::brass));
    }
    return mesh;
}

Vec3 portalLightPosition() { return {0.0, 2.2, (openingDepth(0.0, 0.42) - 0.08) * headUnitSize}; }

double mouthTunnelFrontZ() { return openingDepth(0.0, 0.48) * headUnitSize; }
double mouthTunnelBackZ() { return -openingDepth(0.0, 0.48) * headUnitSize; }
double mouthDoorwayHalfWidth() { return doorwayHalfWidth * headUnitSize; }
