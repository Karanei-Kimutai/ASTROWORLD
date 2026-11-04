/**
 * @file shaders.h
 * @brief GLSL 3.30 shader sources (kept in C++ strings so no files need to be found at runtime).
 *
 * Shaders are the PROGRAMMABLE stages of the GPU pipeline:
 *   - the VERTEX shader runs once per vertex: it applies the model, view and
 *     projection matrices (vertex processing);
 *   - the FRAGMENT shader runs once per covered pixel sample after
 *     rasterization: it computes the colour (fragment processing).
 *
 * All lighting is done in LINEAR, HIGH-DYNAMIC-RANGE colour. Colours authored
 * in sRGB (vertex colours, textures, palette) are converted to linear first
 * (pow 2.2); the post-processing pass tone-maps and converts back.
 */
#pragma once

#include <string>

namespace shaders {

/**
 * @brief Physically based shading (PBR), shared by every shading mode.
 *
 * The Phong reflection model's three parts survive, each made physically based:
 *   AMBIENT  -> light arriving from the whole sky and the ground (hemisphere
 *               irradiance) plus a blurred REFLECTION of the environment
 *   DIFFUSE  -> Lambert, reduced by what the Fresnel term reflects instead
 *   SPECULAR -> Cook-Torrance microfacet BRDF:  D * G * F / (4 (N.L)(N.V))
 *               D: GGX normal distribution - how many micro-facets face the half vector
 *               G: Smith-Schlick geometry  - facets shadowing/masking each other
 *               F: Schlick Fresnel         - grazing angles reflect more
 * METALLIC surfaces (the gold head, chrome) have coloured reflections and no
 * diffuse part; ROUGHNESS widens highlights and blurs reflections.
 */
inline const char* lighting = R"GLSL(
const float PI = 3.14159265;
const int maxPointLights = 16;

uniform vec3 cameraPosition;
uniform vec3 sunDirection;        // unit vector toward the sun / moon
uniform vec3 sunRadiance;         // linear HDR
uniform vec3 skyZenith;           // linear sky colour overhead
uniform vec3 skyHorizon;          // linear sky colour at the horizon
uniform vec3 groundColor;         // linear colour of light bounced up from the ground
uniform float ambientStrength;
uniform int pointLightCount;
uniform vec3 pointLightPositions[maxPointLights];
uniform vec3 pointLightRadiance[maxPointLights];
uniform sampler2DShadow shadowMap;
uniform mat4 lightViewProjection;
uniform float shadowTexelSize;
uniform int shadowsEnabled;

vec3 srgbToLinear(vec3 color) { return pow(color, vec3(2.2)); }

float distributionGGX(float NdotH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denominator = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denominator * denominator);
}
float geometrySchlickGGX(float NdotX, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}
// pow() of a negative base is undefined (NaN) in GLSL; rounding can push a cosine just
// past 1.0, so the base is clamped. (One NaN pixel would be smeared into a black blob by bloom.)
vec3 fresnelSchlick(float cosine, vec3 F0) { return F0 + (1.0 - F0) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0); }
vec3 fresnelSchlickRoughness(float cosine, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosine, 0.0, 1.0), 5.0);
}

/** Light reflected toward V from one light arriving along L with the given radiance. */
vec3 cookTorrance(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 1e-4);
    if (NdotL <= 0.0) return vec3(0.0);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = distributionGGX(max(dot(N, H), 0.0), roughness);
    float G = geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
    vec3 specular = D * G * F / (4.0 * NdotV * NdotL + 1e-4);
    vec3 diffuse = (1.0 - F) * (1.0 - metallic) * albedo / PI;
    return (diffuse + specular) * radiance * NdotL;
}

/** The sky and ground as seen along a direction - what a mirror would reflect. */
vec3 environment(vec3 direction, float roughness) {
    float up = direction.y;
    vec3 sky = mix(skyHorizon, skyZenith, pow(clamp(up, 0.0, 1.0), 0.6));
    vec3 ground = groundColor * 0.6;
    vec3 sharp = up >= 0.0 ? sky : mix(skyHorizon * 0.5, ground, clamp(-up * 4.0, 0.0, 1.0));
    // Rough surfaces reflect a blurred environment: fade toward its average.
    vec3 average = (skyZenith + skyHorizon + groundColor) / 3.0;
    return mix(sharp, average, roughness * roughness);
}

/** 0 = in shadow, 1 = lit. Nine filtered lookups smooth the shadow edge (PCF). */
float sunVisibility(vec3 position, vec3 normal) {
    if (shadowsEnabled == 0) return 1.0;
    vec4 lightSpace = lightViewProjection * vec4(position + normal * 0.06, 1.0);
    vec3 coordinates = lightSpace.xyz / lightSpace.w * 0.5 + 0.5;
    if (any(lessThan(coordinates, vec3(0.0))) || any(greaterThan(coordinates, vec3(1.0)))) return 1.0;
    float lit = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            lit += texture(shadowMap, vec3(coordinates.xy + vec2(x, y) * shadowTexelSize, coordinates.z - 0.0008));
        }
    }
    return lit / 9.0;
}

/** Total light leaving a surface point toward the camera. */
vec3 shadeSurface(vec3 position, vec3 N, vec3 albedo, float metallic, float roughness) {
    vec3 V = normalize(cameraPosition - position);
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 color = cookTorrance(N, V, sunDirection, sunRadiance * sunVisibility(position, N), albedo, metallic, roughness);
    for (int index = 0; index < maxPointLights; ++index) {
        if (index >= pointLightCount) break;
        vec3 toLight = pointLightPositions[index] - position;
        float distanceSquared = dot(toLight, toLight);
        // Skip lights too far away to matter: with inverse-square falloff a light of
        // strength I contributes under 2% of a unit here once d^2 > 50 I.
        vec3 lightRadiance = pointLightRadiance[index];
        if (distanceSquared > 50.0 * max(lightRadiance.r, max(lightRadiance.g, lightRadiance.b))) continue;
        // Inverse-square falloff, softened near the bulb.
        vec3 radiance = lightRadiance / (1.0 + distanceSquared);
        color += cookTorrance(N, V, toLight * inversesqrt(distanceSquared), radiance, albedo, metallic, roughness);
    }

    // Ambient: diffuse light from the sky above / ground below, plus environment reflection.
    float NdotV = max(dot(N, V), 1e-4);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 irradiance = mix(groundColor, mix(skyHorizon, skyZenith, 0.5), N.y * 0.5 + 0.5);
    vec3 diffuseAmbient = (1.0 - F) * (1.0 - metallic) * irradiance * albedo;
    vec3 specularAmbient = F * environment(reflect(-V, N), roughness);
    return color + (diffuseAmbient + specularAmbient) * ambientStrength;
}
)GLSL";

inline const char* litVertexMain = R"GLSL(
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTextureCoordinate;

// Per-INSTANCE attributes (GPU instancing): each copy brings its own model matrix and tint.
layout(location = 4) in mat4 instanceModel;  // object -> world (occupies locations 4-7)
layout(location = 8) in vec3 instanceTint;

uniform mat4 view;        // world  -> camera
uniform mat4 projection;  // camera -> clip space
uniform int shadingMode;  // 0 flat, 1 Gouraud, 2 Phong
uniform float metallic;
uniform float roughness;

out vec3 worldPosition;
out vec3 worldNormal;
out vec3 vertexColor;
out vec2 textureCoordinate;
out vec3 gouraudColor;    // Gouraud: lighting computed HERE, per vertex, then interpolated

void main() {
    vec4 world = instanceModel * vec4(inPosition, 1.0);
    worldPosition = world.xyz;
    // Normals transform by the inverse-transpose of the model matrix in general. Every
    // instance here is a rotation + UNIFORM scale (non-uniform shapes are baked into the
    // meshes; lawns only stretch flat discs whose normal stays straight up), and for those
    // the inverse-transpose equals the matrix itself up to scale - so mat3(model) and a
    // normalize give the same result without inverting a matrix per vertex.
    worldNormal = normalize(mat3(instanceModel) * inNormal);
    vertexColor = srgbToLinear(inColor * instanceTint);
    textureCoordinate = inTextureCoordinate;
    gouraudColor = vec3(0.0);
    if (shadingMode == 1) gouraudColor = shadeSurface(worldPosition, worldNormal, vertexColor, metallic, roughness);
    gl_Position = projection * view * world;
}
)GLSL";

inline const char* litFragmentMain = R"GLSL(
in vec3 worldPosition;
in vec3 worldNormal;
in vec3 vertexColor;
in vec2 textureCoordinate;
in vec3 gouraudColor;

uniform int shadingMode;          // 0 flat, 1 Gouraud, 2 Phong
uniform float metallic;
uniform float roughness;
uniform float emissive;           // 0 = lit, 1 = glows with its own colour
uniform float emissionStrength;   // HDR brightness of the glow (> 1 blooms)
uniform vec3 fogColor;            // linear
uniform float fogDensity;
uniform sampler2D diffuseTexture;
uniform int useTexture;

out vec4 fragmentColor;

void main() {
    vec3 albedo = vertexColor;
    if (useTexture == 1) albedo *= srgbToLinear(texture(diffuseTexture, textureCoordinate).rgb);
    vec3 toCamera = normalize(cameraPosition - worldPosition);

    vec3 color;
    if (shadingMode == 1) {
        // GOURAUD: the colours lit at the three vertices, interpolated across the triangle.
        color = gouraudColor * (useTexture == 1 ? srgbToLinear(texture(diffuseTexture, textureCoordinate).rgb) : vec3(1.0));
    } else {
        vec3 normal;
        if (shadingMode == 0) {
            // FLAT: one normal per triangle from the screen-space derivatives of the position.
            normal = normalize(cross(dFdx(worldPosition), dFdy(worldPosition)));
            if (dot(normal, toCamera) < 0.0) normal = -normal;
        } else {
            // PHONG: interpolate the NORMAL and light every pixel.
            normal = normalize(worldNormal);
            if (!gl_FrontFacing) normal = -normal;
        }
        color = shadeSurface(worldPosition, normal, albedo, metallic, roughness);
    }
    color = mix(color, albedo * emissionStrength, emissive);

    // Aerial perspective: distant surfaces fade into the haze of the horizon.
    float fogAmount = 1.0 - exp(-fogDensity * length(cameraPosition - worldPosition));
    fragmentColor = vec4(mix(color, fogColor, fogAmount), 1.0);
}
)GLSL";

inline std::string litVertex() { return std::string("#version 330 core\n") + lighting + litVertexMain; }
inline std::string litFragment() { return std::string("#version 330 core\n") + lighting + litFragmentMain; }

/** @brief Shadow pass: only depth, seen from the sun. */
inline const char* depthVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 4) in mat4 instanceModel;
uniform mat4 lightViewProjection;
void main() { gl_Position = lightViewProjection * instanceModel * vec4(inPosition, 1.0); }
)GLSL";

inline const char* depthFragment = R"GLSL(
#version 330 core
void main() {}
)GLSL";

/** @brief Full-screen triangle, shared by the sky and every post-processing pass. */
inline const char* fullScreenVertex = R"GLSL(
#version 330 core
out vec2 ndc;
out vec2 textureCoordinate;
void main() {
    // Vertices (-1,-1), (3,-1), (-1,3): one triangle covering the [-1,1] square.
    vec2 corner = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2)) * 2.0 - 1.0;
    ndc = corner;
    textureCoordinate = corner * 0.5 + 0.5;
    gl_Position = vec4(corner, 0.0, 1.0);
}
)GLSL";

/**
 * @brief The sky: each pixel reconstructs its world-space view ray with the inverse
 *        view-projection matrix, then adds a gradient, the sun, clouds and stars.
 */
inline const char* skyFragment = R"GLSL(
#version 330 core
in vec2 ndc;
uniform mat4 inverseViewProjection;
uniform vec3 skyTop;           // linear
uniform vec3 skyMid;
uniform vec3 skyHorizon;
uniform vec3 horizonGlow;      // warm band low on the horizon (strongest at night, as on the night cover)
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform float nightAmount;
uniform float timeSeconds;
out vec4 fragmentColor;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float valueNoise(vec2 p) {
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(cell), hash(cell + vec2(1, 0)), u.x), mix(hash(cell + vec2(0, 1)), hash(cell + vec2(1, 1)), u.x), u.y);
}
float fractalNoise(vec2 p) {
    float total = 0.0;
    float amplitude = 0.5;
    for (int octave = 0; octave < 5; ++octave) {
        total += amplitude * valueNoise(p);
        p = p * 2.03 + vec2(17.0, 9.0);
        amplitude *= 0.5;
    }
    return total;
}

void main() {
    vec4 nearPoint = inverseViewProjection * vec4(ndc, -1.0, 1.0);
    vec4 farPoint = inverseViewProjection * vec4(ndc, 1.0, 1.0);
    vec3 direction = normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);
    float elevation = max(direction.y, 0.0);

    vec3 color = elevation < 0.2 ? mix(skyHorizon, skyMid, elevation / 0.2)
                                 : mix(skyMid, skyTop, clamp((elevation - 0.2) / 0.5, 0.0, 1.0));
    color += horizonGlow * exp(-elevation * 9.0);

    // The sun (or moon): a bright disc plus a wide glow (forward scattering in the air).
    float sunCosine = max(dot(direction, sunDirection), 0.0);
    color += sunColor * (pow(sunCosine, 1200.0) * 40.0 + pow(sunCosine, 12.0) * 0.25);

    // Clouds: fractal noise on a sky ceiling, lit from the sun side, drifting with time.
    if (direction.y > 0.0) {
        vec2 plane = direction.xz / (direction.y + 0.12) * 1.1 + vec2(timeSeconds * 0.02, 0.0);
        // One noise evaluation per pixel: denser parts of a cloud are also its thicker, shaded parts.
        float noise = fractalNoise(plane);
        float density = smoothstep(0.5, 0.82, noise);
        float thickness = smoothstep(0.62, 0.95, noise);
        vec3 litCloud = mix(vec3(1.0), sunColor * 0.25 + vec3(0.9), 0.3);
        vec3 shadedCloud = mix(skyMid, vec3(0.55, 0.6, 0.7), 0.6);
        vec3 cloudColor = mix(litCloud, shadedCloud, thickness * 0.7);
        cloudColor = mix(cloudColor, mix(vec3(0.08, 0.05, 0.12), horizonGlow * 0.6 + vec3(0.06, 0.03, 0.1), 0.5), nightAmount);
        float alpha = density * smoothstep(0.0, 0.15, direction.y);
        color = mix(color, cloudColor, alpha * mix(0.95, 0.75, nightAmount));
    }

    // Stars at night, twinkling.
    vec2 angles = vec2(atan(direction.z, direction.x), asin(clamp(direction.y, -1.0, 1.0))) * 160.0;
    vec2 cell = floor(angles);
    float starSeed = hash(cell);
    if (starSeed > 0.986 && direction.y > 0.05) {
        vec2 starCentre = cell + 0.5 + 0.3 * vec2(hash(cell + 7.0) - 0.5, hash(cell + 13.0) - 0.5);
        float twinkle = 0.6 + 0.4 * sin(timeSeconds * (1.0 + 3.0 * hash(cell + 3.0)) + starSeed * 40.0);
        // (smoothstep needs edge0 < edge1 - reversed edges are undefined in GLSL - so fade with 1 - smoothstep.)
        color += vec3(2.0) * (1.0 - smoothstep(0.0, 0.3, length(angles - starCentre))) * twinkle * nightAmount;
    }
    fragmentColor = vec4(color, 1.0);
}
)GLSL";

/** @brief Searchlight beams: additive light in the night air, fading along the beam. */
inline const char* beamVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 3) in vec2 inTextureCoordinate;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out float along;
void main() {
    along = inTextureCoordinate.y;
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
}
)GLSL";

inline const char* beamFragment = R"GLSL(
#version 330 core
in float along;
uniform vec3 tint;
out vec4 fragmentColor;
void main() {
    // 'along' is interpolated and can overshoot 1.0 at the far rim: clamp before pow (negative base = NaN).
    fragmentColor = vec4(pow(tint, vec3(2.2)) * pow(clamp(1.0 - along, 0.0, 1.0), 1.6) * 0.35, 1.0);
}
)GLSL";

/** @brief Bloom step 1: keep only what is brighter than a threshold (at half resolution). */
inline const char* brightPassFragment = R"GLSL(
#version 330 core
in vec2 textureCoordinate;
uniform sampler2D sceneTexture;
uniform float threshold;
out vec4 fragmentColor;
void main() {
    vec3 color = texture(sceneTexture, textureCoordinate).rgb;
    if (any(isnan(color)) || any(isinf(color))) color = vec3(0.0);  // never let a bad sample spread
    float brightness = max(color.r, max(color.g, color.b));
    // Soft knee: fade in above the threshold instead of a hard cut.
    float contribution = clamp((brightness - threshold) / max(brightness, 1e-4), 0.0, 1.0);
    fragmentColor = vec4(color * contribution, 1.0);
}
)GLSL";

/** @brief Bloom step 2: separable Gaussian blur (run horizontally, then vertically). */
inline const char* blurFragment = R"GLSL(
#version 330 core
in vec2 textureCoordinate;
uniform sampler2D sourceTexture;
uniform vec2 texelStep;   // one texel along the blur direction
out vec4 fragmentColor;
void main() {
    const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 sum = texture(sourceTexture, textureCoordinate).rgb * weights[0];
    for (int offset = 1; offset < 5; ++offset) {
        sum += texture(sourceTexture, textureCoordinate + texelStep * float(offset) * 1.5).rgb * weights[offset];
        sum += texture(sourceTexture, textureCoordinate - texelStep * float(offset) * 1.5).rgb * weights[offset];
    }
    fragmentColor = vec4(sum, 1.0);
}
)GLSL";

/**
 * @brief Final composite: bloom + exposure, ACES filmic tone mapping, gamma, vignette.
 *
 * Tone mapping squeezes the unbounded HDR range into 0..1 the way film does:
 * shadows and mid-tones stay almost linear, highlights roll off smoothly
 * instead of clipping to flat white.
 */
inline const char* compositeFragment = R"GLSL(
#version 330 core
in vec2 textureCoordinate;
uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float exposure;
uniform float bloomStrength;
out vec4 fragmentColor;

vec3 acesFilmic(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(sceneTexture, textureCoordinate).rgb + texture(bloomTexture, textureCoordinate).rgb * bloomStrength;
    if (any(isnan(hdr)) || any(isinf(hdr))) hdr = vec3(0.0);
    vec3 mapped = acesFilmic(hdr * exposure);
    vec3 display = pow(mapped, vec3(1.0 / 2.2));          // linear -> sRGB (gamma correction)
    vec2 fromCentre = textureCoordinate - 0.5;
    display *= 1.0 - 0.35 * dot(fromCentre, fromCentre);  // gentle lens vignette
    fragmentColor = vec4(display, 1.0);
}
)GLSL";

/** @brief Screen-space textured quads (logo, captions, help) blended over the final image. */
inline const char* overlayVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 3) in vec2 inTextureCoordinate;
uniform mat4 model;
out vec2 textureCoordinate;
void main() {
    textureCoordinate = inTextureCoordinate;
    gl_Position = model * vec4(inPosition, 1.0);
}
)GLSL";

inline const char* overlayFragment = R"GLSL(
#version 330 core
in vec2 textureCoordinate;
uniform sampler2D overlayTexture;
out vec4 fragmentColor;
void main() { fragmentColor = texture(overlayTexture, textureCoordinate); }
)GLSL";

}  // namespace shaders
