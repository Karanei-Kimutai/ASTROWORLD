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
Vec3 toVector(Color color) { return {color.r / 255.0, color.g / 255.0, color.b / 255.0}; }
double fractional(double value) { return value - std::floor(value); }
Vec3 mixVector(const Vec3& a, const Vec3& b, double t) { return a + (b - a) * t; }
double smoothStepRange(double edge0, double edge1, double x) {
    const double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

/**
 * @brief Extended Reinhard tone mapping, per channel: c (1 + c / w^2) / (1 + c).
 *
 * Path tracing produces physically based radiance that can exceed 1.0 (a white
 * surface in full sun PLUS bounced sky light). Clamping would flatten bright
 * areas; this curve compresses them smoothly, mapping 'w' to pure white.
 */
Vec3 toneMap(const Vec3& radiance) {
    const double whitePoint = 1.8;
    auto channel = [whitePoint](double c) { return c * (1.0 + c / (whitePoint * whitePoint)) / (1.0 + c); };
    return {channel(radiance.x) * 1.35, channel(radiance.y) * 1.35, channel(radiance.z) * 1.35};
}

/** @brief Per-thread random numbers in [0, 1). */
struct Sampler {
    SimpleRandom random;
    explicit Sampler(std::uint32_t seed) : random(seed) {}
    double next() { return random.nextDouble(); }
};

/**
 * @brief The sky seen along a direction: a C++ port of the OpenGL sky shader
 *        (gradient, value-noise clouds, stars), so both renderers share one sky.
 */
class SkyModel {
public:
    SkyModel(const SceneLighting& lighting, double timeSeconds)
        : top(toVector(lighting.skyTop)), mid(toVector(lighting.skyMid)), horizon(toVector(lighting.skyHorizon)),
          night(lighting.nightAmount), time(timeSeconds) {}

    Vec3 radiance(const Vec3& direction) const {
        const double elevation = std::max(direction.y, 0.0);
        Vec3 color = elevation < 0.25 ? mixVector(horizon, mid, elevation / 0.25)
                                      : mixVector(mid, top, std::clamp((elevation - 0.25) / 0.45, 0.0, 1.0));
        // Stars.
        const double angleX = std::atan2(direction.z, direction.x) * 120.0;
        const double angleY = std::asin(std::clamp(direction.y, -1.0, 1.0)) * 120.0;
        const double cellX = std::floor(angleX), cellY = std::floor(angleY);
        const double seed = hash(cellX, cellY);
        if (seed > 0.985 && direction.y > 0.03) {
            const double centreX = cellX + 0.5 + 0.3 * (hash(cellX + 7, cellY + 7) - 0.5);
            const double centreY = cellY + 0.5 + 0.3 * (hash(cellX + 13, cellY + 13) - 0.5);
            const double twinkle = 0.6 + 0.4 * std::sin(time * (1.0 + 3.0 * hash(cellX + 3, cellY + 3)) + seed * 40.0);
            const double distance = std::hypot(angleX - centreX, angleY - centreY);
            color += Vec3(1, 1, 1) * (smoothStepRange(0.35, 0.0, distance) * twinkle * night);
        }
        // Clouds.
        if (direction.y > 0.0) {
            const double planeX = direction.x / (direction.y + 0.08) * 1.2 + time * 0.03;
            const double planeY = direction.z / (direction.y + 0.08) * 1.2;
            const double density = smoothStepRange(0.48, 0.75, fractalNoise(planeX, planeY));
            const Vec3 cloudColor = mixVector({0.69, 0.76, 0.87}, {1, 1, 1}, smoothStepRange(0.5, 0.9, fractalNoise(planeX + 0.35, planeY + 0.35)));
            const double alpha = density * smoothStepRange(0.0, 0.12, direction.y) * (1.0 - night * 0.85);
            color = mixVector(color, cloudColor * (1.0 + (0.25 - 1.0) * night), alpha);
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
            x *= 2.03;
            y *= 2.03;
            amplitude *= 0.5;
        }
        return total;
    }

    Vec3 top, mid, horizon;
    double night, time;
};

/** @brief Everything about the surface a ray landed on. */
struct SurfacePoint {
    Vec3 position;
    Vec3 normal;      // shading normal, facing the incoming ray
    Vec3 baseColor;   // vertex colour x tint x texture
    const Material* material = nullptr;
};

/** @brief Renders one frame; holds the scene, lights and textures the rays need. */
class Tracer {
public:
    Tracer(const RayScene& sceneValue, const SceneLighting& lightingValue, double timeSeconds, const RayTraceSettings& settingsValue)
        : scene(sceneValue), lighting(lightingValue), sky(lightingValue, timeSeconds), settings(settingsValue),
          groundTexture(drawGroundTexture()), signTexture(drawSignTexture()) {}

    /** @brief Colour seen along a camera ray, by the chosen algorithm. */
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
        // Interpolate normals and colours with the barycentric weights (Phong-style smooth shading).
        surface.normal = (triangle.normal0 * w + triangle.normal1 * u + triangle.normal2 * v).normalized();
        if (dotProduct(surface.normal, ray.direction) > 0.0) surface.normal = -surface.normal;
        surface.baseColor = triangle.color0 * w + triangle.color1 * u + triangle.color2 * v;
        surface.material = &scene.material(triangle.materialIndex);
        if (settings.textures && surface.material->texture != TextureId::none) {
            const Vec2 uv = triangle.uv0 * w + triangle.uv1 * u + triangle.uv2 * v;
            const TextureSample texel = sampleTextureBilinear(surface.material->texture == TextureId::ground ? groundTexture : signTexture, uv.x, uv.y);
            surface.baseColor = multiply(surface.baseColor, {texel.r, texel.g, texel.b});
        }
        return true;
    }

    /** @brief Is the point lit along 'direction' (up to maxDistance)? One SHADOW ray. */
    bool visible(const Vec3& position, const Vec3& normal, const Vec3& direction, double maxDistance) {
        ++threadRayCount;
        // Start slightly off the surface so the ray does not hit the triangle it leaves ("shadow acne").
        return !scene.occluded({position + normal * 1e-3, direction}, maxDistance);
    }

    /**
     * @brief Direct light from the sun/moon and the portal, with shadow rays: the
     *        Phong diffuse and specular terms, multiplied by visibility (0 or 1).
     */
    void directLight(const SurfacePoint& surface, const Vec3& toEye, Vec3& diffuseLight, Vec3& specularLight) {
        const Material& material = *surface.material;
        diffuseLight = {0, 0, 0};
        specularLight = {0, 0, 0};
        const double sunCosine = dotProduct(surface.normal, lighting.sunDirection);
        if (sunCosine > 0.0 && visible(surface.position, surface.normal, lighting.sunDirection, 1e9)) {
            const Vec3 halfVector = (lighting.sunDirection + toEye).normalized();
            diffuseLight += lighting.sunColor * sunCosine;
            specularLight += lighting.sunColor * (std::pow(std::max(dotProduct(surface.normal, halfVector), 0.0), material.shininess) * material.specularStrength);
        }
        Vec3 toPortal = lighting.portalPosition - surface.position;
        const double portalDistance = toPortal.length();
        toPortal = toPortal * (1.0 / portalDistance);
        const double portalCosine = dotProduct(surface.normal, toPortal);
        if (portalCosine > 0.0 && visible(surface.position, surface.normal, toPortal, portalDistance)) {
            const double attenuation = lighting.portalIntensity / (1.0 + 0.12 * portalDistance + 0.035 * portalDistance * portalDistance);
            const Vec3 halfVector = (toPortal + toEye).normalized();
            diffuseLight += lighting.portalColor * (portalCosine * attenuation);
            specularLight += lighting.portalColor *
                             (std::pow(std::max(dotProduct(surface.normal, halfVector), 0.0), material.shininess) * material.specularStrength * attenuation);
        }
    }

    Vec3 applyFog(const Vec3& color, double distance) const {
        const double fog = 1.0 - std::exp(-lighting.fogDensity * distance);
        return mixVector(color, toVector(lighting.fogColor), fog);
    }

    /** @brief Mirror direction: d - 2 (d . n) n. */
    static Vec3 reflect(const Vec3& direction, const Vec3& normal) { return direction - normal * (2.0 * dotProduct(direction, normal)); }

    Vec3 traceWhitted(const Ray& ray, int depth) {
        SurfacePoint surface;
        double distance = 0.0;
        if (!hitSurface(ray, surface, distance)) return sky.radiance(ray.direction);
        const Material& material = *surface.material;
        if (material.emissive >= 1.0) return applyFog(surface.baseColor, distance);

        Vec3 diffuseLight, specularLight;
        directLight(surface, -ray.direction, diffuseLight, specularLight);
        Vec3 color = multiply(surface.baseColor, diffuseLight + lighting.ambientColor) + specularLight;
        color = mixVector(color, surface.baseColor, material.emissive);

        // Mirror reflection, traced recursively; metals tint what they reflect.
        if (material.reflectivity > 0.0 && depth < settings.maxDepth) {
            const Vec3 reflected = traceWhitted({surface.position + surface.normal * 1e-3, reflect(ray.direction, surface.normal)}, depth + 1);
            const Vec3 metalTint = mixVector({1, 1, 1}, surface.baseColor, 0.5);
            color = mixVector(color, multiply(reflected, metalTint), material.reflectivity);
        }
        return applyFog(color, distance);
    }

    /** @brief Random direction around the normal with probability proportional to cos(theta). */
    static Vec3 cosineWeightedDirection(const Vec3& normal, Sampler& sampler) {
        // Uniform point on a disc, lifted onto the hemisphere (Malley's method).
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
        Vec3 throughput{1, 1, 1};  // how much of the light found from here on reaches the eye
        double primaryDistance = -1.0;
        const double emissionStrength = 2.5;  // glowing surfaces emit more than they reflect
        const double skyLight = 0.45;  // the sky as a light source for bounced rays (its colour already includes scattered sunlight)

        for (int bounce = 0; bounce <= settings.maxDepth; ++bounce) {
            SurfacePoint surface;
            double distance = 0.0;
            if (!hitSurface(ray, surface, distance)) {
                radianceSum += multiply(throughput, sky.radiance(ray.direction) * (bounce == 0 ? 1.0 : skyLight));
                break;
            }
            if (bounce == 0) primaryDistance = distance;
            const Material& material = *surface.material;

            // Emission: bulbs and the doorway are light sources in their own right.
            if (material.emissive > 0.0) {
                radianceSum += multiply(throughput, surface.baseColor * (material.emissive * (bounce == 0 ? 1.0 : emissionStrength)));
                if (material.emissive >= 1.0) break;
            }

            // Direct light (next-event estimation): explicit shadow rays to the sun and the portal light.
            Vec3 diffuseLight, specularLight;
            directLight(surface, -ray.direction, diffuseLight, specularLight);
            radianceSum += multiply(throughput, multiply(surface.baseColor, diffuseLight) + specularLight);

            // Choose the next bounce: a mirror reflection with probability = reflectivity, else diffuse.
            Vec3 nextDirection;
            if (sampler.next() < material.reflectivity) {
                nextDirection = reflect(ray.direction, surface.normal);
                throughput = multiply(throughput, mixVector({1, 1, 1}, surface.baseColor, 0.5));
            } else {
                // Lambertian BRDF albedo/pi with a cosine-weighted pdf cos/pi: the weight is just the albedo.
                nextDirection = cosineWeightedDirection(surface.normal, sampler);
                throughput = multiply(throughput, surface.baseColor);
            }
            // Russian roulette: randomly end weak paths, boosting survivors so the average is unbiased.
            if (bounce >= 2) {
                const double survive = std::clamp(std::max({throughput.x, throughput.y, throughput.z}), 0.1, 0.95);
                if (sampler.next() > survive) break;
                throughput = throughput * (1.0 / survive);
            }
            ray = {surface.position + surface.normal * 1e-3, nextDirection};
        }
        return primaryDistance >= 0.0 ? applyFog(radianceSum, primaryDistance) : radianceSum;
    }

    const RayScene& scene;
    const SceneLighting& lighting;
    SkyModel sky;
    const RayTraceSettings& settings;
    FrameBuffer groundTexture;
    FrameBuffer signTexture;
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
                Vec3 average = sum * (1.0 / samplesPerPixel);
                if (settings.mode == TracingMode::pathTracing) average = toneMap(average);
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
