/**
 * @file goldShading.h
 * @brief Faux-lit "inflatable gold" surface shader shared by the head, ears and hair.
 *
 * CP1 has no 3D geometry yet, so a 2D shape is treated as if it were the
 * front of an ellipsoid: from a pixel's normalised position (u, v) inside the
 * ellipse we reconstruct a surface normal (u, v, sqrt(1 - u^2 - v^2)) and
 * light it. This is a preview of the lighting work in the final phase, where
 * the full Phong reflection model (ambient + diffuse + specular) is applied
 * to real 3D meshes.
 */
#pragma once

#include <algorithm>
#include <cmath>

#include "core/color.h"
#include "core/frameBuffer.h"
#include "core/vec2.h"

/**
 * @brief Colour of the gold surface at normalised ellipse coordinates (u, v).
 *
 * @param u,v        Position relative to the ellipse centre divided by its radii (|u|,|v| <= ~1).
 * @param brightness Overall multiplier (hair and features are slightly darker than skin).
 */
inline Color shadeGoldSurface(double u, double v, double brightness) {
    // Reconstruct a unit normal on an ellipsoid: (u, v, z) with z facing the viewer.
    const double radialSquared = u * u + v * v;
    const double normalZ = std::sqrt(std::max(0.0, 1.0 - radialSquared));
    const double normalLength = std::sqrt(radialSquared + normalZ * normalZ);
    const double nx = u / normalLength;
    const double ny = v / normalLength;
    const double nz = normalZ / normalLength;

    // Key light from the upper-left, slightly in front (normalised (-0.45, -0.55, 0.70)).
    const double lightX = -0.45, lightY = -0.55, lightZ = 0.70;
    const double lightLength = std::sqrt(lightX * lightX + lightY * lightY + lightZ * lightZ);
    const double diffuse = std::max(0.0, (nx * lightX + ny * lightY + nz * lightZ) / lightLength);

    // Specular via the half-vector between light and viewer (viewer = +z).
    const double halfX = lightX / lightLength, halfY = lightY / lightLength, halfZ = lightZ / lightLength + 1.0;
    const double halfLength = std::sqrt(halfX * halfX + halfY * halfY + halfZ * halfZ);
    const double specular = std::pow(std::max(0.0, (nx * halfX + ny * halfY + nz * halfZ) / halfLength), 36.0);

    // Rim term: surfaces turning away from the viewer pick up the pink neon of the park.
    const double rim = std::pow(1.0 - nz, 2.5);

    // Three-stop gold ramp driven by the diffuse term.
    Color base = diffuse < 0.5 ? lerpColor(astroPalette::goldShadow, astroPalette::gold, diffuse / 0.5)
                               : lerpColor(astroPalette::gold, astroPalette::goldLight, (diffuse - 0.5) / 0.5);
    base = scaleColor(base, brightness);
    base = lerpColor(base, astroPalette::neonPink, rim * 0.45);
    return lerpColor(base, astroPalette::white, specular * 0.85);
}

/**
 * @brief A PixelShader that applies shadeGoldSurface over an ellipse in screen space.
 */
inline PixelShader makeGoldShader(const Vec2& center, double radiusX, double radiusY, double brightness) {
    return [center, radiusX, radiusY, brightness](int x, int y) {
        return shadeGoldSurface((x - center.x) / radiusX, (y - center.y) / radiusY, brightness);
    };
}
