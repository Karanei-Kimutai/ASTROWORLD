/**
 * @file shaders.h
 * @brief GLSL 3.30 shader sources (kept in C++ raw strings so no files need to be found at runtime).
 *
 * Shaders are the PROGRAMMABLE stages of the GPU pipeline:
 *   - the VERTEX shader runs once per vertex: it applies the model, view and
 *     projection matrices (vertex processing);
 *   - the FRAGMENT shader runs once per covered pixel sample after
 *     rasterization: it computes the colour (fragment processing).
 * Fixed-function hardware between them does primitive assembly, clipping,
 * rasterization, the depth test and blending.
 */
#pragma once

namespace shaders {

/** @brief Lit geometry: transforms vertices and passes world-space data to the fragment shader. */
inline const char* litVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTextureCoordinate;

uniform mat4 model;       // object -> world
uniform mat4 view;        // world  -> camera
uniform mat4 projection;  // camera -> clip space (perspective)

out vec3 worldPosition;
out vec3 worldNormal;
out vec3 vertexColor;
out vec2 textureCoordinate;

void main() {
    vec4 world = model * vec4(inPosition, 1.0);
    worldPosition = world.xyz;
    // Normals use the inverse-transpose so non-uniform scaling does not bend them.
    worldNormal = mat3(transpose(inverse(model))) * inNormal;
    vertexColor = inColor;
    textureCoordinate = inTextureCoordinate;
    gl_Position = projection * view * world;
}
)GLSL";

/**
 * @brief Blinn-Phong lighting: ambient + diffuse + specular from the sun (or
 *        moon), plus a warm POINT LIGHT pouring out of the head's mouth, with
 *        distance fog toward the horizon colour.
 */
inline const char* litFragment = R"GLSL(
#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec3 vertexColor;
in vec2 textureCoordinate;

uniform vec3 cameraPosition;
uniform vec3 sunDirection;      // unit vector TOWARD the sun
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 portalPosition;    // point light in the mouth
uniform vec3 portalColor;
uniform float portalIntensity;
uniform float specularStrength;
uniform float shininess;
uniform float emissive;         // 0 = lit normally, 1 = glows with its own colour (bulbs, portal)
uniform vec3 tint;              // per-draw colour multiplier
uniform vec3 fogColor;
uniform float fogDensity;
uniform sampler2D diffuseTexture;
uniform int useTexture;

out vec4 fragmentColor;

void main() {
    vec3 base = vertexColor * tint;
    if (useTexture == 1) base *= texture(diffuseTexture, textureCoordinate).rgb;

    vec3 normal = normalize(worldNormal);
    if (!gl_FrontFacing) normal = -normal;  // thin double-sided surfaces (flags) lit from both sides
    vec3 toCamera = normalize(cameraPosition - worldPosition);

    // Sun: ambient + Lambert diffuse + Blinn-Phong specular (half-vector).
    float diffuse = max(dot(normal, sunDirection), 0.0);
    vec3 halfVector = normalize(sunDirection + toCamera);
    float specular = diffuse > 0.0 ? pow(max(dot(normal, halfVector), 0.0), shininess) * specularStrength : 0.0;
    vec3 lit = base * (ambientColor + sunColor * diffuse) + sunColor * specular;

    // Portal point light, attenuated with distance: 1 / (1 + k1 d + k2 d^2).
    vec3 toPortal = portalPosition - worldPosition;
    float distanceToPortal = length(toPortal);
    toPortal /= distanceToPortal;
    float attenuation = portalIntensity / (1.0 + 0.12 * distanceToPortal + 0.035 * distanceToPortal * distanceToPortal);
    float portalDiffuse = max(dot(normal, toPortal), 0.0);
    float portalSpecular = pow(max(dot(normal, normalize(toPortal + toCamera)), 0.0), shininess) * specularStrength;
    lit += (base * portalDiffuse + portalSpecular) * portalColor * attenuation;

    lit = mix(lit, base, emissive);

    // Exponential fog: distant objects fade into the sky colour (aerial perspective).
    float fogAmount = 1.0 - exp(-fogDensity * length(cameraPosition - worldPosition));
    fragmentColor = vec4(mix(lit, fogColor, fogAmount), 1.0);
}
)GLSL";

/**
 * @brief The sky, drawn as one triangle that covers the whole screen.
 *        Each pixel reconstructs its WORLD-space view direction with the inverse
 *        view-projection matrix, then colours itself from that direction.
 */
inline const char* skyVertex = R"GLSL(
#version 330 core
out vec2 ndc;
void main() {
    // Vertices (-1,-1), (3,-1), (-1,3): one triangle covering the [-1,1] square.
    vec2 corner = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2)) * 2.0 - 1.0;
    ndc = corner;
    gl_Position = vec4(corner, 0.0, 1.0);
}
)GLSL";

inline const char* skyFragment = R"GLSL(
#version 330 core
in vec2 ndc;
uniform mat4 inverseViewProjection;
uniform vec3 cameraPosition;
uniform vec3 skyTop;
uniform vec3 skyMid;
uniform vec3 skyHorizon;
uniform float nightAmount;
uniform float timeSeconds;
out vec4 fragmentColor;

// Cheap hash and value noise for clouds and stars.
float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float valueNoise(vec2 p) {
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);  // smoothstep interpolation between lattice points
    return mix(mix(hash(cell), hash(cell + vec2(1, 0)), u.x), mix(hash(cell + vec2(0, 1)), hash(cell + vec2(1, 1)), u.x), u.y);
}
float fractalNoise(vec2 p) {
    float total = 0.0;
    float amplitude = 0.5;
    for (int octave = 0; octave < 5; ++octave) {
        total += amplitude * valueNoise(p);
        p *= 2.03;
        amplitude *= 0.5;
    }
    return total;
}

void main() {
    vec4 farPoint = inverseViewProjection * vec4(ndc, 1.0, 1.0);
    vec3 direction = normalize(farPoint.xyz / farPoint.w - cameraPosition);
    float elevation = max(direction.y, 0.0);

    // Same three-stop gradient as the software renderer, by elevation instead of screen row.
    vec3 color = elevation < 0.25 ? mix(skyHorizon, skyMid, elevation / 0.25)
                                  : mix(skyMid, skyTop, clamp((elevation - 0.25) / 0.45, 0.0, 1.0));

    // Stars: one candidate per cell of a (longitude, latitude) grid; twinkle with time.
    vec2 angles = vec2(atan(direction.z, direction.x), asin(clamp(direction.y, -1.0, 1.0))) * 120.0;
    vec2 cell = floor(angles);
    float starSeed = hash(cell);
    if (starSeed > 0.985 && direction.y > 0.03) {
        vec2 starCentre = cell + 0.5 + 0.3 * vec2(hash(cell + 7.0) - 0.5, hash(cell + 13.0) - 0.5);
        float twinkle = 0.6 + 0.4 * sin(timeSeconds * (1.0 + 3.0 * hash(cell + 3.0)) + starSeed * 40.0);
        float brightness = smoothstep(0.35, 0.0, length(angles - starCentre)) * twinkle;
        color += vec3(brightness) * nightAmount;
    }

    // Clouds: fractal noise projected onto a flat sky ceiling, drifting with time.
    if (direction.y > 0.0) {
        vec2 cloudPlane = direction.xz / (direction.y + 0.08) * 1.2 + vec2(timeSeconds * 0.03, 0.0);
        float density = smoothstep(0.48, 0.75, fractalNoise(cloudPlane));
        vec3 cloudColor = mix(vec3(0.69, 0.76, 0.87), vec3(1.0), smoothstep(0.5, 0.9, fractalNoise(cloudPlane + 0.35)));
        float cloudAlpha = density * smoothstep(0.0, 0.12, direction.y) * (1.0 - nightAmount * 0.85);
        color = mix(color, cloudColor * mix(1.0, 0.25, nightAmount), cloudAlpha);
    }
    fragmentColor = vec4(color, 1.0);
}
)GLSL";

/** @brief Screen-space textured quads (logo, captions, help) blended over the 3D view. */
inline const char* overlayVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 3) in vec2 inTextureCoordinate;
uniform mat4 model;  // places the unit quad on screen in normalised device coordinates
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
