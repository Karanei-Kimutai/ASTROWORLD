/**
 * @file rayTracer.cpp
 * @brief Whitted ray tracing and path tracing of the park.
 */
#include "raytrace/rayTracer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

#include "core/simpleRandom.h"
#include "park3d/parkTextures.h"

namespace {

/** @brief Rays cast by the current thread (summed at the end - a shared counter would make threads contend). */
thread_local std::uint64_t threadRayCount = 0;

Vec3 multiply(const Vec3& a, const Vec3& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
double fractional(double value) { return value - std::floor(value); }
Vec3 mixVector(const Vec3& a, const Vec3& b, double t) { return a + (b - a) * t; }
double smoothStepRange(double edge0, double edge1, double x) {
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

/**
 * @brief Exposure, ACES filmic tone mapping and gamma - the same curve the OpenGL
 *        composite pass uses, so ray-traced and real-time images match in tone.
 */
Vec3 toneMap(const Vec3& radiance, double exposure) {
    auto aces = [](double x) { return std::clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0); };
    auto display = [&](double x) { return std::pow(aces(x * exposure), 1.0 / 2.2); };
    return {display(radiance.x), display(radiance.y), display(radiance.z)};
}

/** @brief Per-thread random numbers in [0, 1). */
struct Sampler {
    SimpleRandom random;
    explicit Sampler(std::uint32_t seed) : random(seed) {}
    double next() { return random.nextDouble(); }
};

/**
 * @brief The sky seen along a direction: a C++ port of the OpenGL sky shader
 *        (gradient, horizon glow, sun, clouds, stars), in LINEAR light.
 */
class SkyModel {
public:
    SkyModel(const SceneLighting& lighting, double timeSeconds)
        : top(linear(lighting.skyTop)), mid(linear(lighting.skyMid)), horizon(linear(lighting.skyHorizon)),
          glow(linear(lighting.horizonGlow)), sunDirection(lighting.sunDirection), sunColor(lighting.sunRadiance * 0.3),
          night(lighting.nightAmount), time(timeSeconds) {}

    static Vec3 linear(Color color) { return {std::pow(color.r / 255.0, 2.2), std::pow(color.g / 255.0, 2.2), std::pow(color.b / 255.0, 2.2)}; }

    Vec3 radiance(const Vec3& direction) const {
        const double elevation = std::max(direction.y, 0.0);
        Vec3 color = elevation < 0.2 ? mixVector(horizon, mid, elevation / 0.2) : mixVector(mid, top, std::clamp((elevation - 0.2) / 0.5, 0.0, 1.0));
        color += glow * std::exp(-elevation * 9.0);
        const double sunCosine = std::max(dotProduct(direction, sunDirection), 0.0);
        color += sunColor * (std::pow(sunCosine, 1200.0) * 40.0 + std::pow(sunCosine, 12.0) * 0.25);
        if (direction.y > 0.0) {
            const double planeX = direction.x / (direction.y + 0.12) * 1.1 + time * 0.02;
            const double planeY = direction.z / (direction.y + 0.12) * 1.1;
            const double noise = fractalNoise(planeX, planeY);
            const double density = smoothStepRange(0.5, 0.82, noise);
            const double thickness = smoothStepRange(0.62, 0.95, noise);
            const Vec3 litCloud = mixVector({1, 1, 1}, sunColor * 0.25 + Vec3(0.9, 0.9, 0.9), 0.3);
            const Vec3 shadedCloud = mixVector(mid, {0.55, 0.6, 0.7}, 0.6);
            Vec3 cloud = mixVector(litCloud, shadedCloud, thickness * 0.7);
            cloud = mixVector(cloud, mixVector({0.08, 0.05, 0.12}, glow * 0.6 + Vec3(0.06, 0.03, 0.1), 0.5), night);
            const double alpha = density * smoothStepRange(0.0, 0.15, direction.y) * (0.95 + (0.75 - 0.95) * night);
            color = mixVector(color, cloud, alpha);
        }
        return color;
    }

private:
    static double hash(double x, double y) { return fractional(std::sin(x * 127.1 + y * 311.7) * 43758.5453); }
    static double valueNoise(double x, double y) {
        const double cellX = std::floor(x), cellY = std::floor(y);
        double fx = x - cellX, fy = y - cellY;
        fx = fx * fx * (3.0 - 2.0 * fx);
        fy = fy * fy * (3.0 - 2.0 * fy);
        const double a = hash(cellX, cellY), b = hash(cellX + 1, cellY), c = hash(cellX, cellY + 1), d = hash(cellX + 1, cellY + 1);
        return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
    }
    static double fractalNoise(double x, double y) {
        double total = 0.0, amplitude = 0.5;
        for (int octave = 0; octave < 5; ++octave) {
            total += amplitude * valueNoise(x, y);
            x = x * 2.03 + 17.0;
            y = y * 2.03 + 9.0;
            amplitude *= 0.5;
        }
        return total;
    }

    Vec3 top, mid, horizon, glow, sunDirection, sunColor;
    double night, time;
};

/** @brief Everything about the surface a ray landed on. */
struct SurfacePoint {
    Vec3 position;
    Vec3 normal;      // shading normal, facing the incoming ray
    Vec3 albedo;      // linear base colour: vertex colour x tint x texture
    const Material* material = nullptr;
};

/** @brief The Cook-Torrance BRDF, the same maths as the OpenGL shader (see shaders.h). */
struct Brdf {
    static double distributionGGX(double NdotH, double roughness) {
        const double alpha2 = std::pow(roughness * roughness, 2.0);
        const double denominator = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
        return alpha2 / (pi * denominator * denominator);
    }
    static double geometrySchlick(double NdotX, double roughness) {
        const double k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        return NdotX / (NdotX * (1.0 - k) + k);
    }
    static Vec3 fresnel(double cosine, const Vec3& F0) { return F0 + (Vec3(1, 1, 1) - F0) * std::pow(1.0 - cosine, 5.0); }

    /** @brief Reflected radiance toward V from light arriving along L. */
    static Vec3 evaluate(const Vec3& N, const Vec3& V, const Vec3& L, const Vec3& radiance, const Vec3& albedo, double metallic, double roughness) {
        const double NdotL = dotProduct(N, L);
        if (NdotL <= 0.0) return {0, 0, 0};
        const double NdotV = std::max(dotProduct(N, V), 1e-4);
        const Vec3 H = (V + L).normalized();
        const Vec3 F0 = mixVector({0.04, 0.04, 0.04}, albedo, metallic);
        const Vec3 F = fresnel(std::max(dotProduct(H, V), 0.0), F0);
        const double D = distributionGGX(std::max(dotProduct(N, H), 0.0), roughness);
        const double G = geometrySchlick(NdotV, roughness) * geometrySchlick(NdotL, roughness);
        const Vec3 specular = F * (D * G / (4.0 * NdotV * NdotL + 1e-4));
        const Vec3 diffuse = multiply(Vec3(1, 1, 1) - F, albedo) * ((1.0 - metallic) / pi);
        return multiply(diffuse + specular, radiance) * NdotL;
    }
};

/** @brief Renders one frame; holds the scene, lights and textures the rays need. */
class Tracer {
public:
    Tracer(const RayScene& sceneValue, const SceneLighting& lightingValue, double timeSeconds, const RayTraceSettings& settingsValue)
        : scene(sceneValue), lighting(lightingValue), sky(lightingValue, timeSeconds), settings(settingsValue),
          groundTexture(drawGroundTexture()), signTexture(drawSignTexture()),
          skyZenith(SkyModel::linear(lightingValue.skyTop)), skyHorizon(SkyModel::linear(lightingValue.skyHorizon)),
          groundColor(SkyModel::linear(lightingValue.groundColor)), fogColor(SkyModel::linear(lightingValue.fogColor)) {}

    /** @brief Linear HDR radiance seen along a camera ray, by the chosen algorithm. */
    Vec3 radiance(const Ray& ray, Sampler& sampler) {
        return settings.mode == TracingMode::whitted ? traceWhitted(ray, 0) : tracePath(ray, sampler);
    }

private:
    bool hitSurface(const Ray& ray, SurfacePoint& surface, double& distance) {
        ++threadRayCount;
        RayHit hit;
        if (!scene.intersect(ray, 1e-4, 1e9, hit)) return false;
        const RayTriangle& triangle = scene.triangle(hit.triangle);
        const double w = 1.0 - hit.barycentricU - hit.barycentricV;
        const double u = hit.barycentricU, v = hit.barycentricV;
        distance = hit.distance;
        surface.position = ray.origin + ray.direction * hit.distance;
        surface.normal = (triangle.normal0 * w + triangle.normal1 * u + triangle.normal2 * v).normalized();
        if (dotProduct(surface.normal, ray.direction) > 0.0) surface.normal = -surface.normal;
        surface.albedo = triangle.color0 * w + triangle.color1 * u + triangle.color2 * v;
        surface.material = &scene.material(triangle.materialIndex);
        if (settings.textures && surface.material->texture != TextureId::none) {
            const Vec2 uv = triangle.uv0 * w + triangle.uv1 * u + triangle.uv2 * v;
            const TextureSample texel = sampleTextureBilinear(surface.material->texture == TextureId::ground ? groundTexture : signTexture, uv.x, uv.y);
            surface.albedo = multiply(surface.albedo, {std::pow(texel.r, 2.2), std::pow(texel.g, 2.2), std::pow(texel.b, 2.2)});
        }
        return true;
    }

    /** @brief Is the point lit along 'direction' (up to maxDistance)? One SHADOW ray. */
    bool visible(const Vec3& position, const Vec3& normal, const Vec3& direction, double maxDistance) {
        ++threadRayCount;
        return !scene.occluded({position + normal * 2e-3, direction}, maxDistance);
    }

    /** @brief Direct light from the sun / moon and every nearby point light, each with a shadow ray. */
    Vec3 directLight(const SurfacePoint& surface, const Vec3& toEye) {
        const Material& material = *surface.material;
        const double roughness = std::clamp(material.roughness, 0.04, 1.0);
        Vec3 color{0, 0, 0};
        if (dotProduct(surface.normal, lighting.sunDirection) > 0.0 && visible(surface.position, surface.normal, lighting.sunDirection, 1e9)) {
            color += Brdf::evaluate(surface.normal, toEye, lighting.sunDirection, lighting.sunRadiance, surface.albedo, material.metallic, roughness);
        }
        for (const PointLight& light : lighting.pointLights) {
            Vec3 toLight = light.position - surface.position;
            const double distanceSquared = dotProduct(toLight, toLight);
            // Same cut-off as the shader: skip lights whose contribution would be negligible.
            if (distanceSquared > 50.0 * std::max({light.radiance.x, light.radiance.y, light.radiance.z})) continue;
            const double distance = std::sqrt(distanceSquared);
            toLight = toLight * (1.0 / distance);
            if (dotProduct(surface.normal, toLight) <= 0.0 || !visible(surface.position, surface.normal, toLight, distance - 0.3)) continue;
            color += Brdf::evaluate(surface.normal, toEye, toLight, light.radiance * (1.0 / (1.0 + distanceSquared)), surface.albedo, material.metallic, roughness);
        }
        return color;
    }

    /** @brief The sky and ground as a mirror would see them, blurred by roughness (as in the shader). */
    Vec3 environment(const Vec3& direction, double roughness) const {
        const Vec3 skyColor = mixVector(skyHorizon, skyZenith, std::pow(std::clamp(direction.y, 0.0, 1.0), 0.6));
        const Vec3 sharp = direction.y >= 0.0 ? skyColor : mixVector(skyHorizon * 0.5, groundColor * 0.6, std::clamp(-direction.y * 4.0, 0.0, 1.0));
        const Vec3 average = (skyZenith + skyHorizon + groundColor) * (1.0 / 3.0);
        return mixVector(sharp, average, roughness * roughness);
    }

    Vec3 applyFog(const Vec3& color, double distance) const {
        return mixVector(color, fogColor, 1.0 - std::exp(-lighting.fogDensity * distance));
    }

    static Vec3 reflect(const Vec3& direction, const Vec3& normal) { return direction - normal * (2.0 * dotProduct(direction, normal)); }

    Vec3 traceWhitted(const Ray& ray, int depth) {
        SurfacePoint surface;
        double distance = 0.0;
        if (!hitSurface(ray, surface, distance)) return sky.radiance(ray.direction);
        const Material& material = *surface.material;
        const Vec3 emitted = surface.albedo * material.emissionStrength;
        if (material.emissive >= 1.0) return applyFog(emitted, distance);

        const Vec3 toEye = -ray.direction;
        const double roughness = std::clamp(material.roughness, 0.04, 1.0);
        Vec3 color = directLight(surface, toEye);

        // Ambient: sky/ground irradiance for the diffuse part, and a REFLECTION for the specular
        // part - a real recursive mirror ray for smooth surfaces, the blurred environment otherwise.
        const double NdotV = std::max(dotProduct(surface.normal, toEye), 1e-4);
        const Vec3 F0 = mixVector({0.04, 0.04, 0.04}, surface.albedo, material.metallic);
        const Vec3 F = F0 + (Vec3(std::max(1.0 - roughness, F0.x), std::max(1.0 - roughness, F0.y), std::max(1.0 - roughness, F0.z)) - F0) * std::pow(1.0 - NdotV, 5.0);
        const Vec3 irradiance = mixVector(groundColor, mixVector(skyHorizon, skyZenith, 0.5), surface.normal.y * 0.5 + 0.5);
        const Vec3 diffuseAmbient = multiply(multiply(Vec3(1, 1, 1) - F, irradiance), surface.albedo) * (1.0 - material.metallic);
        const Vec3 reflectedDirection = reflect(ray.direction, surface.normal);
        Vec3 reflected = environment(reflectedDirection, roughness);
        if (roughness < 0.5 && depth < settings.maxDepth) {
            const Vec3 traced = traceWhitted({surface.position + surface.normal * 2e-3, reflectedDirection}, depth + 1);
            reflected = mixVector(traced, reflected, roughness * 2.0);
        }
        color += (diffuseAmbient + multiply(F, reflected)) * lighting.ambientStrength;
        color = mixVector(color, emitted, material.emissive);
        return applyFog(color, distance);
    }

    /** @brief Random direction around the normal with probability proportional to cos(theta). */
    static Vec3 cosineWeightedDirection(const Vec3& normal, Sampler& sampler) {
        const double radius = std::sqrt(sampler.next());
        const double angle = 2.0 * pi * sampler.next();
        const double x = radius * std::cos(angle), y = radius * std::sin(angle);
        const double z = std::sqrt(std::max(0.0, 1.0 - x * x - y * y));
        const Vec3 tangent = crossProduct(std::fabs(normal.x) > 0.5 ? Vec3(0, 1, 0) : Vec3(1, 0, 0), normal).normalized();
        const Vec3 bitangent = crossProduct(normal, tangent);
        return (tangent * x + bitangent * y + normal * z).normalized();
    }

    Vec3 tracePath(Ray ray, Sampler& sampler) {
        Vec3 radianceSum{0, 0, 0};
        Vec3 throughput{1, 1, 1};
        double primaryDistance = -1.0;
        for (int bounce = 0; bounce <= settings.maxDepth; ++bounce) {
            SurfacePoint surface;
            double distance = 0.0;
            if (!hitSurface(ray, surface, distance)) {
                radianceSum += multiply(throughput, sky.radiance(ray.direction));
                break;
            }
            if (bounce == 0) primaryDistance = distance;
            const Material& material = *surface.material;
            // Emission: bulbs, lamps and the tunnel walls are light sources in their own right.
            if (material.emissive > 0.0) {
                radianceSum += multiply(throughput, surface.albedo * (material.emissive * material.emissionStrength));
                if (material.emissive >= 1.0) break;
            }
            // Direct light by next-event estimation (explicit shadow rays) - no ambient term:
            // indirect light comes from the bounces themselves.
            radianceSum += multiply(throughput, directLight(surface, -ray.direction));

            // Next bounce: metals reflect (blurred by roughness), everything else scatters diffusely.
            const double roughness = std::clamp(material.roughness, 0.04, 1.0);
            Vec3 nextDirection;
            if (sampler.next() < material.metallic) {
                const Vec3 mirror = reflect(ray.direction, surface.normal);
                nextDirection = (mirror + cosineWeightedDirection(surface.normal, sampler) * (roughness * roughness)).normalized();
                throughput = multiply(throughput, surface.albedo);
            } else {
                // Lambertian: BRDF albedo/pi with a cosine-weighted pdf cos/pi -> weight = albedo.
                nextDirection = cosineWeightedDirection(surface.normal, sampler);
                throughput = multiply(throughput, surface.albedo);
            }
            if (dotProduct(nextDirection, surface.normal) <= 0.0) break;
            // Russian roulette: randomly end weak paths, boosting survivors so the average is unbiased.
            if (bounce >= 2) {
                const double survive = std::clamp(std::max({throughput.x, throughput.y, throughput.z}), 0.1, 0.95);
                if (sampler.next() > survive) break;
                throughput = throughput * (1.0 / survive);
            }
            ray = {surface.position + surface.normal * 2e-3, nextDirection};
        }
        return primaryDistance >= 0.0 ? applyFog(radianceSum, primaryDistance) : radianceSum;
    }

    const RayScene& scene;
    const SceneLighting& lighting;
    SkyModel sky;
    const RayTraceSettings& settings;
    FrameBuffer groundTexture;
    FrameBuffer signTexture;
    Vec3 skyZenith, skyHorizon, groundColor, fogColor;
};

}  // namespace

FrameBuffer rayTraceFrame(const ParkScene& scene, const ParkFrame& frame, const CameraMatrices& camera,
                          const RayTraceSettings& settings, RayTraceStats& stats) {
    using Clock = std::chrono::steady_clock;
    const auto buildStart = Clock::now();
    RayScene rayScene;
    rayScene.build(scene, frame);
    stats.triangles = rayScene.triangleCount();
    stats.buildSeconds = std::chrono::duration<double>(Clock::now() - buildStart).count();

    Tracer tracer(rayScene, frame.lighting, frame.timeSeconds, settings);
    const Matrix4 inverseViewProjection = (camera.projection * camera.view).inverse();
    FrameBuffer image(settings.width, settings.height);

    // Stratified supersampling: split each pixel into a grid and jitter one sample in each cell.
    const int gridSize = std::max(1, static_cast<int>(std::lround(std::sqrt(settings.samplesPerPixel))));
    const int samplesPerPixel = std::max(settings.samplesPerPixel, 1);

    std::atomic<int> nextRow{0};  // threads take rows from a shared counter (dynamic load balancing)
    std::atomic<std::uint64_t> totalRays{0};
    auto worker = [&](unsigned threadIndex) {
        Sampler sampler(0x9E3779B9u * (threadIndex + 1));
        threadRayCount = 0;
        for (int row = nextRow++; row < settings.height; row = nextRow++) {
            for (int column = 0; column < settings.width; ++column) {
                Vec3 sum{0, 0, 0};
                for (int sample = 0; sample < samplesPerPixel; ++sample) {
                    const int cell = sample % (gridSize * gridSize);
                    const double offsetX = (cell % gridSize + sampler.next()) / gridSize;
                    const double offsetY = (cell / gridSize + sampler.next()) / gridSize;
                    // Pixel -> normalised device coordinates -> unproject the near and far points.
                    const double ndcX = 2.0 * (column + offsetX) / settings.width - 1.0;
                    const double ndcY = 1.0 - 2.0 * (row + offsetY) / settings.height;
                    const Vec3 nearPoint = inverseViewProjection.transformPoint({ndcX, ndcY, -1.0});
                    const Vec3 farPoint = inverseViewProjection.transformPoint({ndcX, ndcY, 1.0});
                    sum += tracer.radiance({nearPoint, (farPoint - nearPoint).normalized()}, sampler);
                }
                const Vec3 average = toneMap(sum * (1.0 / samplesPerPixel), frame.lighting.exposure);
                image.setPixel(column, row, Color{clampChannel(average.x * 255.0), clampChannel(average.y * 255.0), clampChannel(average.z * 255.0)});
            }
        }
        totalRays += threadRayCount;
    };

    const auto renderStart = Clock::now();
    const unsigned threadCount = settings.threadCount > 0 ? static_cast<unsigned>(settings.threadCount)
                                                          : std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    for (unsigned index = 0; index < threadCount; ++index) threads.emplace_back(worker, index);
    for (std::thread& thread : threads) thread.join();
    stats.renderSeconds = std::chrono::duration<double>(Clock::now() - renderStart).count();
    stats.raysCast = totalRays.load();
    return image;
}
