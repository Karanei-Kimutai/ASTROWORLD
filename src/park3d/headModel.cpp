/**
 * @file headModel.cpp
 * @brief Procedural 3D head.
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

constexpr double radiusU = 0.43;  // ellipsoid semi-axes in head units
constexpr double radiusV = 0.56;
constexpr double radiusDepth = 0.45;

/** @brief Depth (head units) of the face surface at (u, v), or -1 outside the silhouette. */
double surfaceDepth(double u, double v) {
    const double inside = 1.0 - (u / radiusU) * (u / radiusU) - (v / radiusV) * (v / radiusV);
    return inside > 0.0 ? radiusDepth * std::sqrt(inside) : -1.0;
}

/** @brief World-space point for head units, pushed 'lift' head units out from the surface along z. */
Vec3 facePoint(double u, double v, double lift) {
    const double depth = std::max(surfaceDepth(u, v), 0.0);
    return Vec3(u, 0.5 - v, depth + lift) * headUnitSize;
}

/** @brief Unit surface normal at (u, v): the ellipsoid gradient (x/a^2, y/b^2, z/c^2). */
Vec3 faceNormal(double u, double v) {
    const double depth = std::max(surfaceDepth(u, v), 0.05);
    return Vec3(u / (radiusU * radiusU), -v / (radiusV * radiusV), depth / (radiusDepth * radiusDepth)).normalized();
}

/**
 * @brief Place a feature mesh on the face: its local z axis is aligned with the
 *        surface normal (via Matrix4::fromBasis) and it sits 'lift' units out.
 */
void addFeature(MeshData& head, const MeshData& feature, double u, double v, double lift) {
    const Vec3 zAxis = faceNormal(u, v);
    const Vec3 xAxis = crossProduct({0.0, 1.0, 0.0}, zAxis).normalized();
    const Vec3 yAxis = crossProduct(zAxis, xAxis);
    head.append(feature, Matrix4::fromBasis(xAxis, yAxis, zAxis, facePoint(u, v, lift)));
}

MeshData ellipsoidUnits(double radiusX, double radiusY, double radiusZ, Color color) {
    return makeEllipsoid(Vec3(radiusX, radiusY, radiusZ) * headUnitSize, 24, 16, color);
}

}  // namespace

MeshData buildHeadMesh() {
    MeshData head;
    const Color brass = astroPalette::brass;

    // Face: one big ellipsoid, centred half a head unit above the ground (chin buried).
    head.append(makeEllipsoid(Vec3(radiusU, radiusV, radiusDepth) * headUnitSize, 72, 48, brass),
                Matrix4::translation({0.0, 0.5 * headUnitSize, 0.0}));

    for (double side : {-1.0, 1.0}) {
        // Ears on the sides.
        head.append(ellipsoidUnits(0.03, 0.10, 0.06, brass), Matrix4::translation(Vec3(side * 0.425, 0.54, 0.0) * headUnitSize));
        // Brow ridge, closed eyelid bulge and the cheekbones.
        addFeature(head, ellipsoidUnits(0.12, 0.03, 0.03, brass), side * 0.17, -0.165, -0.022);
        addFeature(head, ellipsoidUnits(0.10, 0.045, 0.035, brass), side * 0.155, -0.085, -0.022);
        addFeature(head, ellipsoidUnits(0.09, 0.07, 0.04, brass), side * 0.27, 0.06, -0.025);

        // Lash line of the closed eye: the same Bezier as the 2D head, wrapped onto the lid.
        std::vector<Vec3> lash;
        for (const Vec2& point : sampleCubicBezier({side * 0.065, -0.062}, {side * 0.11, -0.036}, {side * 0.20, -0.036},
                                                   {side * 0.255, -0.068}, 16)) {
            lash.push_back(facePoint(point.x, point.y, 0.01));
        }
        head.append(makeTube(lash, 0.008 * headUnitSize, 0.006 * headUnitSize, 8, astroPalette::brassShadow));

        // Nostril wings (alae) and dark nostrils.
        addFeature(head, ellipsoidUnits(0.05, 0.042, 0.04, brass), side * 0.085, 0.125, 0.0);
        addFeature(head, ellipsoidUnits(0.025, 0.012, 0.012, Color{52, 34, 12}), side * 0.047, 0.152, 0.045);
    }

    // Nose bridge and the rounded tip.
    addFeature(head, ellipsoidUnits(0.05, 0.13, 0.05, brass), 0.0, -0.01, -0.005);
    addFeature(head, ellipsoidUnits(0.07, 0.06, 0.06, brass), 0.0, 0.10, 0.02);

    // Lips: a low bulge around the mouth and a pouting upper lip, both kept
    // BEHIND the doorway panel so the glowing entrance reads clearly.
    addFeature(head, ellipsoidUnits(0.21, 0.13, 0.05, brass), 0.0, 0.42, -0.03);
    addFeature(head, ellipsoidUnits(0.17, 0.045, 0.045, brass), 0.0, 0.265, -0.005);
    return head;
}

MeshData buildPortalMesh() {
    using namespace astroHeadShape;
    // Outline in head units: up one side, over the Bezier arch, down the other side.
    std::vector<Vec2> outline = {{-doorwayHalfWidth, 0.5}, {-doorwayHalfWidth, doorwayArchStartV}};
    for (const Vec2& point : sampleCubicBezier({-doorwayHalfWidth, doorwayArchStartV}, {-doorwayHalfWidth, doorwayArchControlV},
                                               {doorwayHalfWidth, doorwayArchControlV}, {doorwayHalfWidth, doorwayArchStartV}, 24)) {
        outline.push_back(point);
    }
    outline.push_back({doorwayHalfWidth, 0.5});

    // Same colour ramp as the 2D portal shader: white-hot at the bottom centre, orange at the rim.
    const Vec2 lightSource{0.0, 0.5};
    auto portalColor = [&](const Vec2& point) {
        const double distance = std::min(1.0, (point - lightSource).length() / 0.2);
        return distance < 0.5 ? lerpColor(astroPalette::portalCore, astroPalette::portalGlow, distance / 0.5)
                              : lerpColor(astroPalette::portalGlow, astroPalette::portalEdge, (distance - 0.5) / 0.5);
    };

    // The doorway is a flat panel just in front of the lip bulge. Tessellate it as
    // rings shrinking toward a centre point so the colour gradient interpolates smoothly.
    const double depth = surfaceDepth(0.0, 0.40) + 0.05;
    const Vec2 centre{0.0, 0.47};
    const int ringCount = 4;
    MeshData portal;
    const Vec3 front{0.0, 0.0, 1.0};
    for (int ring = 0; ring <= ringCount; ++ring) {
        const double t = static_cast<double>(ring) / ringCount;
        for (const Vec2& point : outline) {
            const Vec2 ringPoint = lerpVec2(centre, point, t);
            portal.vertices.push_back(makeVertex(Vec3(ringPoint.x, 0.5 - ringPoint.y, depth) * headUnitSize, front, portalColor(ringPoint)));
        }
    }
    const std::uint32_t ringSize = static_cast<std::uint32_t>(outline.size());
    for (std::uint32_t ring = 0; ring < static_cast<std::uint32_t>(ringCount); ++ring) {
        for (std::uint32_t index = 0; index + 1 < ringSize; ++index) {
            const std::uint32_t inner = ring * ringSize + index;
            const std::uint32_t outer = inner + ringSize;
            portal.addTriangle(inner, inner + 1, outer + 1);
            portal.addTriangle(inner, outer + 1, outer);
        }
    }
    // Brass rim around the arch.
    std::vector<Vec3> rim;
    for (const Vec2& point : outline) rim.push_back(Vec3(point.x, 0.5 - point.y, depth + 0.004) * headUnitSize);
    portal.append(makeTube(rim, 0.012 * headUnitSize, 0.012 * headUnitSize, 8, astroPalette::brassDark));
    return portal;
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
        {outerDread, -1.0, outerDreadSway, -0.04},  {outerDread, 1.0, outerDreadSway, -0.04},
        {middleDread, -1.0, middleDreadSway, 0.10}, {middleDread, 1.0, middleDreadSway, 0.10},
        {frontDreadLeft, -1.0, frontDreadSway, 0.20}, {frontDreadRight, 1.0, frontDreadSway, 0.20},
    };
    const double rootRadius = 0.043;
    const double tipRadius = 0.024;

    MeshData mesh;
    for (const DreadSpec& dread : dreads) {
        // Bend in the wind exactly as the 2D renderer does: rotate each control point
        // about the root by an angle that grows toward the tip (Matrix3::rotationAbout).
        const Vec2 root{dread.side * dread.points[0].x, dread.points[0].y};
        std::vector<Vec3> controlPoints;
        for (int index = 0; index < dreadPointCount; ++index) {
            const double along = static_cast<double>(index) / (dreadPointCount - 1);
            const Vec2 unit = Matrix3::rotationAbout(root, swayAngle * dread.sway * along)
                                  .transformPoint({dread.side * dread.points[index].x, dread.points[index].y});
            // Lift the dread clear of the face where it passes in front of it.
            const double radius = rootRadius + (tipRadius - rootRadius) * along;
            const double depth = std::max(dread.depth, surfaceDepth(unit.x, unit.y) + radius * 1.3);
            controlPoints.push_back(Vec3(unit.x, 0.5 - unit.y, depth) * headUnitSize);
        }
        // Root the dread INSIDE the crown so it grows out of the head.
        controlPoints.front() = controlPoints.front() + Vec3(0.0, -0.03, -0.05) * headUnitSize;
        mesh.append(makeTube(sampleCatmullRomSpline3D(controlPoints, 10, false), rootRadius * headUnitSize, tipRadius * headUnitSize, 12,
                             astroPalette::brass));
    }
    return mesh;
}

Vec3 portalLightPosition() { return Vec3(0.0, 0.12, surfaceDepth(0.0, 0.40) + 0.2) * headUnitSize; }
