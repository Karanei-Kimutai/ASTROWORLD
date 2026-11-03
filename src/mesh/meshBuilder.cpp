/**
 * @file meshBuilder.cpp
 * @brief Procedural mesh generators.
 */
#include "mesh/meshBuilder.h"

#include <algorithm>
#include <cmath>

#include "core/vec2.h"

namespace {

SegmentColorizer solid(Color color) {
    return [color](int) { return color; };
}

}  // namespace

MeshData makeBox(const Vec3& size, Color color) {
    MeshData mesh;
    const Vec3 half = size * 0.5;
    // Each face: outward normal plus two in-plane axes (chosen so the winding is counter-clockwise).
    const Vec3 faces[6][3] = {{{1, 0, 0}, {0, 0, -1}, {0, 1, 0}}, {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
                              {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}}, {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
                              {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},  {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}}};
    for (const auto& face : faces) {
        const Vec3& normal = face[0];
        const Vec3& axisU = face[1];
        const Vec3& axisV = face[2];
        const std::uint32_t first = mesh.nextIndex();
        for (int corner = 0; corner < 4; ++corner) {
            const double signU = (corner == 1 || corner == 2) ? 1.0 : -1.0;
            const double signV = corner >= 2 ? 1.0 : -1.0;
            const Vec3 point{normal.x * half.x + axisU.x * signU * half.x + axisV.x * signV * half.x,
                             normal.y * half.y + axisU.y * signU * half.y + axisV.y * signV * half.y,
                             normal.z * half.z + axisU.z * signU * half.z + axisV.z * signV * half.z};
            mesh.vertices.push_back(makeVertex(point, normal, color, (signU + 1) * 0.5, (signV + 1) * 0.5));
        }
        mesh.addTriangle(first, first + 1, first + 2);
        mesh.addTriangle(first, first + 2, first + 3);
    }
    return mesh;
}

MeshData makeCylinder(double radius, double height, int segments, const SegmentColorizer& colorizer, bool withCaps) {
    MeshData mesh;
    for (int segment = 0; segment < segments; ++segment) {
        const double angle0 = 2.0 * pi * segment / segments;
        const double angle1 = 2.0 * pi * (segment + 1) / segments;
        const Color color = colorizer(segment);
        const std::uint32_t first = mesh.nextIndex();
        // Side quad: smooth normals point straight out from the axis.
        for (double angle : {angle0, angle1}) {
            const Vec3 normal{std::cos(angle), 0.0, -std::sin(angle)};
            mesh.vertices.push_back(makeVertex(normal * radius, normal, color));
            mesh.vertices.push_back(makeVertex(normal * radius + Vec3(0.0, height, 0.0), normal, color));
        }
        mesh.addTriangle(first, first + 2, first + 3);
        mesh.addTriangle(first, first + 3, first + 1);
        if (withCaps) {
            // Top and bottom caps as fans around the centre.
            for (double capY : {0.0, height}) {
                const Vec3 normal{0.0, capY > 0.0 ? 1.0 : -1.0, 0.0};
                const std::uint32_t centre = mesh.nextIndex();
                mesh.vertices.push_back(makeVertex({0.0, capY, 0.0}, normal, color));
                mesh.vertices.push_back(makeVertex({std::cos(angle0) * radius, capY, -std::sin(angle0) * radius}, normal, color));
                mesh.vertices.push_back(makeVertex({std::cos(angle1) * radius, capY, -std::sin(angle1) * radius}, normal, color));
                if (capY > 0.0) mesh.addTriangle(centre, centre + 1, centre + 2);
                else mesh.addTriangle(centre, centre + 2, centre + 1);
            }
        }
    }
    return mesh;
}

MeshData makeCylinder(double radius, double height, int segments, Color color, bool withCaps) {
    return makeCylinder(radius, height, segments, solid(color), withCaps);
}

MeshData makeCone(double radius, double height, int segments, const SegmentColorizer& colorizer) {
    MeshData mesh;
    // The side normal tilts up by the cone's slope: (cos a * h, r, -sin a * h), normalised.
    for (int segment = 0; segment < segments; ++segment) {
        const double angle0 = 2.0 * pi * segment / segments;
        const double angle1 = 2.0 * pi * (segment + 1) / segments;
        const double middle = (angle0 + angle1) * 0.5;
        const Color color = colorizer(segment);
        auto sideNormal = [&](double angle) { return Vec3(std::cos(angle) * height, radius, -std::sin(angle) * height).normalized(); };
        const std::uint32_t first = mesh.nextIndex();
        mesh.vertices.push_back(makeVertex({std::cos(angle0) * radius, 0.0, -std::sin(angle0) * radius}, sideNormal(angle0), color));
        mesh.vertices.push_back(makeVertex({std::cos(angle1) * radius, 0.0, -std::sin(angle1) * radius}, sideNormal(angle1), color));
        mesh.vertices.push_back(makeVertex({0.0, height, 0.0}, sideNormal(middle), color));
        mesh.addTriangle(first, first + 1, first + 2);
        // Base.
        const Vec3 down{0.0, -1.0, 0.0};
        const std::uint32_t base = mesh.nextIndex();
        mesh.vertices.push_back(makeVertex({0.0, 0.0, 0.0}, down, color));
        mesh.vertices.push_back(makeVertex({std::cos(angle0) * radius, 0.0, -std::sin(angle0) * radius}, down, color));
        mesh.vertices.push_back(makeVertex({std::cos(angle1) * radius, 0.0, -std::sin(angle1) * radius}, down, color));
        mesh.addTriangle(base, base + 2, base + 1);
    }
    return mesh;
}

MeshData makeCone(double radius, double height, int segments, Color color) {
    return makeCone(radius, height, segments, solid(color));
}

MeshData makeEllipsoid(const Vec3& radii, int slices, int stacks, Color color) {
    MeshData mesh;
    // Latitude/longitude grid: stack = angle down from the north pole, slice = around the y axis.
    for (int stack = 0; stack <= stacks; ++stack) {
        const double polar = pi * stack / stacks;
        for (int slice = 0; slice <= slices; ++slice) {
            const double azimuth = 2.0 * pi * slice / slices;
            const Vec3 unit{std::sin(polar) * std::cos(azimuth), std::cos(polar), -std::sin(polar) * std::sin(azimuth)};
            const Vec3 position{unit.x * radii.x, unit.y * radii.y, unit.z * radii.z};
            const Vec3 normal = Vec3(position.x / (radii.x * radii.x), position.y / (radii.y * radii.y),
                                     position.z / (radii.z * radii.z)).normalized();
            mesh.vertices.push_back(makeVertex(position, normal, color, static_cast<double>(slice) / slices,
                                               static_cast<double>(stack) / stacks));
        }
    }
    const int rowLength = slices + 1;
    for (int stack = 0; stack < stacks; ++stack) {
        for (int slice = 0; slice < slices; ++slice) {
            const std::uint32_t topLeft = static_cast<std::uint32_t>(stack * rowLength + slice);
            const std::uint32_t bottomLeft = topLeft + static_cast<std::uint32_t>(rowLength);
            mesh.addTriangle(topLeft, bottomLeft, bottomLeft + 1);
            mesh.addTriangle(topLeft, bottomLeft + 1, topLeft + 1);
        }
    }
    return mesh;
}

MeshData makeTorus(double majorRadius, double minorRadius, int majorSegments, int minorSegments, Color color) {
    MeshData mesh;
    for (int major = 0; major <= majorSegments; ++major) {
        const double around = 2.0 * pi * major / majorSegments;
        const Vec3 ringDirection{std::cos(around), std::sin(around), 0.0};
        for (int minor = 0; minor <= minorSegments; ++minor) {
            const double tubeAngle = 2.0 * pi * minor / minorSegments;
            // The tube's normal mixes the outward ring direction and the z axis.
            const Vec3 normal = ringDirection * std::cos(tubeAngle) + Vec3(0.0, 0.0, std::sin(tubeAngle));
            mesh.vertices.push_back(makeVertex(ringDirection * majorRadius + normal * minorRadius, normal, color));
        }
    }
    const int rowLength = minorSegments + 1;
    for (int major = 0; major < majorSegments; ++major) {
        for (int minor = 0; minor < minorSegments; ++minor) {
            const std::uint32_t a = static_cast<std::uint32_t>(major * rowLength + minor);
            const std::uint32_t b = a + static_cast<std::uint32_t>(rowLength);
            mesh.addTriangle(a, b, b + 1);
            mesh.addTriangle(a, b + 1, a + 1);
        }
    }
    return mesh;
}

MeshData makeTube(const std::vector<Vec3>& path, double startRadius, double endRadius, int sides, Color color) {
    MeshData mesh;
    const std::size_t pointCount = path.size();
    if (pointCount < 2) return mesh;

    // Initial frame: any vector perpendicular to the first tangent.
    Vec3 tangent = (path[1] - path[0]).normalized();
    Vec3 normal = crossProduct(tangent, std::fabs(tangent.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)).normalized();

    for (std::size_t index = 0; index < pointCount; ++index) {
        const Vec3& previous = path[index == 0 ? 0 : index - 1];
        const Vec3& next = path[std::min(index + 1, pointCount - 1)];
        const Vec3 newTangent = (next - previous).normalized();
        // Parallel transport: remove the part of the old normal along the new tangent.
        normal = (normal - newTangent * dotProduct(normal, newTangent)).normalized();
        tangent = newTangent;
        const Vec3 binormal = crossProduct(tangent, normal);

        const double t = static_cast<double>(index) / (pointCount - 1);
        const double radius = startRadius + (endRadius - startRadius) * t;
        for (int side = 0; side <= sides; ++side) {
            const double angle = 2.0 * pi * side / sides;
            const Vec3 direction = normal * std::cos(angle) + binormal * std::sin(angle);
            mesh.vertices.push_back(makeVertex(path[index] + direction * radius, direction, color));
        }
    }
    const int rowLength = sides + 1;
    for (std::size_t ring = 0; ring + 1 < pointCount; ++ring) {
        for (int side = 0; side < sides; ++side) {
            const std::uint32_t a = static_cast<std::uint32_t>(ring * rowLength + side);
            const std::uint32_t b = a + static_cast<std::uint32_t>(rowLength);
            mesh.addTriangle(a, a + 1, b + 1);
            mesh.addTriangle(a, b + 1, b);
        }
    }
    // Round the end: a small sphere-like cap made from an ellipsoid at the tip.
    MeshData tip = makeEllipsoid({endRadius, endRadius, endRadius}, 8, 6, color);
    mesh.append(tip, Matrix4::translation(path.back()));
    return mesh;
}

MeshData makeCylinderBetween(const Vec3& start, const Vec3& end, double radius, int segments, Color color) {
    const Vec3 axis = end - start;
    const double length = axis.length();
    const Vec3 yAxis = axis.normalized();
    const Vec3 xAxis = crossProduct(yAxis, std::fabs(yAxis.y) < 0.9 ? Vec3(0, 1, 0) : Vec3(1, 0, 0)).normalized();
    const Vec3 zAxis = crossProduct(xAxis, yAxis);
    MeshData mesh;
    mesh.append(makeCylinder(radius, length, segments, color, false), Matrix4::fromBasis(xAxis, yAxis, zAxis, start));
    return mesh;
}

MeshData makeGroundPlane(double width, double depth, Color color, double textureRepeats) {
    MeshData mesh;
    const Vec3 up{0.0, 1.0, 0.0};
    const double r = textureRepeats;
    mesh.vertices = {makeVertex({-width / 2, 0, -depth / 2}, up, color, 0, 0), makeVertex({width / 2, 0, -depth / 2}, up, color, r, 0),
                     makeVertex({width / 2, 0, depth / 2}, up, color, r, r), makeVertex({-width / 2, 0, depth / 2}, up, color, 0, r)};
    mesh.addTriangle(0, 2, 1);
    mesh.addTriangle(0, 3, 2);
    return mesh;
}

MeshData makeTexturedQuad(double width, double height, Color color) {
    MeshData mesh;
    const Vec3 front{0.0, 0.0, 1.0};
    // v = 0 at the TOP so images stored top row first appear the right way up.
    mesh.vertices = {makeVertex({-width / 2, -height / 2, 0}, front, color, 0, 1), makeVertex({width / 2, -height / 2, 0}, front, color, 1, 1),
                     makeVertex({width / 2, height / 2, 0}, front, color, 1, 0), makeVertex({-width / 2, height / 2, 0}, front, color, 0, 0)};
    mesh.addTriangle(0, 1, 2);
    mesh.addTriangle(0, 2, 3);
    return mesh;
}

MeshData makeLines(const std::vector<Vec3>& endpoints, Color color) {
    MeshData mesh;
    for (std::size_t index = 0; index + 1 < endpoints.size(); index += 2) {
        const std::uint32_t first = mesh.nextIndex();
        mesh.vertices.push_back(makeVertex(endpoints[index], {0, 1, 0}, color));
        mesh.vertices.push_back(makeVertex(endpoints[index + 1], {0, 1, 0}, color));
        mesh.addLine(first, first + 1);
    }
    return mesh;
}
