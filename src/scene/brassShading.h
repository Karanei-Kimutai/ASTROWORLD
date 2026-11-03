/**
 * @file goldShading.h
 * @brief Faux-lit matte-brass surface shader for the inflatable head and its dreads.
 *
 * CP1 has no 3D geometry yet, so a 2D shape is treated as if it were the
 * front of an ellipsoid: from a pixel's normalised position (u, v) inside the
 * ellipse we rebuild a surface normal (u, v, sqrt(1 - u^2 - v^2)) and light
 * it. This previews the lighting work in the final phase, where the full
 * Phong reflection model (ambient + diffuse + specular) is applied to real
 * 3D meshes.
 */
#pragma once

#include <algorithm>
#include <cmath>

#include "core/color.h"
#include "core/frameBuffer.h"
#include "core/vec2.h"

/**
 * @brief Colour of the brass surface at normalised ellipse coordinates (u, v).
 *
 * @param u,v        Position relative to the ellipse centre divided by its radii (|u|,|v| <= ~1).
 * @param brightness Overall multiplier (recessed areas are slightly darker).
 * @param rimColor   Environment colour reflected at grazing angles (sky blue by day, neon pink by night).
 */
inline Color shadeBrassSurface(double u, double v, double brightness, Color rimColor) {
    // Rebuild a unit normal on an ellipsoid: (u, v, z) with z facing the viewer.
    const double radialSquared = u * u + v * v;
    const double normalZ = std::sqrt(std::max(0.0, 1.0 - radialSquared));
    const double normalLength = std::sqrt(radialSquared + normalZ * normalZ);
    const double nx = u / normalLength;
    const double ny = v / normalLength;
    const double nz = normalZ / normalLength;

    // Key light from the upper-left, slightly in front (the sun on the album cover).
    const double lightX = -0.45, lightY = -0.55, lightZ = 0.70;
    const double lightLength = std::sqrt(lightX * lightX + lightY * lightY + lightZ * lightZ);
    const double diffuse = std::max(0.0, (nx * lightX + ny * lightY + nz * lightZ) / lightLength);

    // Broad, soft specular (matte vinyl, not polished metal) via the half-vector.
    const double halfX = lightX / lightLength, halfY = lightY / lightLength, halfZ = lightZ / lightLength + 1.0;
    const double halfLength = std::sqrt(halfX * halfX + halfY * halfY + halfZ * halfZ);
    const double specular = std::pow(std::max(0.0, (nx * halfX + ny * halfY + nz * halfZ) / halfLength), 14.0);

    // Rim term: surfaces turning away from the viewer pick up the environment colour.
    const double rim = std::pow(1.0 - nz, 3.0);

    // Three-stop brass ramp driven by the diffuse term.
    Color base = diffuse < 0.5 ? lerpColor(astroPalette::brassShadow, astroPalette::brass, diffuse / 0.5)
                               : lerpColor(astroPalette::brass, astroPalette::brassLight, (diffuse - 0.5) / 0.5);
    base = scaleColor(base, brightness);
    base = lerpColor(base, rimColor, rim * 0.35);
    return lerpColor(base, astroPalette::brassLight, specular * 0.45);
}

/** @brief A PixelShader that applies shadeBrassSurface over an ellipse in screen space. */
inline PixelShader makeBrassShader(const Vec2& center, double radiusX, double radiusY, double brightness,
                                   Color rimColor) {
    return [center, radiusX, radiusY, brightness, rimColor](int x, int y) {
        return shadeBrassSurface((x - center.x) / radiusX, (y - center.y) / radiusY, brightness, rimColor);
    };
}
