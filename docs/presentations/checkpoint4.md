# Presentation guide – Checkpoint 4

> This guide describes the `checkpoint-4` stage. Its file links open that branch on GitHub, because some files moved or changed in later stages.

| | |
|---|---|
| **Branch** | [`checkpoint-4`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-4) (exact submitted state: tag `submitted/checkpoint-4`) |
| **Course stage** | *Final — refine and complete the project* |
| **Presentation questions it answers** | **Checkpoint 4:** (1) final demonstration, (2) what works, (3) what doesn't, (4) final changes / recommendations |
| **Speaking time** | about 10 minutes, plus questions |
| **What you show** | The whole journey in one sitting: the 2D renderer, the OpenGL park, and the new CPU **ray tracer** and **path tracer** |

---

## 1. Before you present

```bash
git checkout checkpoint-4
rm -rf build && cmake -B build && cmake --build build -j
```

Three programs come out of that one build. Keep a terminal ready for each:

```bash
./build/astroworldWindow
```

```bash
./build/astroworldGl
```

```bash
./build/astroworldRay --width 640 --height 360
```

**Render the slow images the day before.** A path-traced frame takes minutes, so don't render it live:

```bash
./build/astroworldRay --mode path --samples 128
```

Have these images open as slides or a fallback:
- `docs/screenshots/rayTracedDay.png` and `rayTracedNight.png`
- `pathTracedDay.png` and `pathTracedNight.png`

**Ray tracer options** (all optional):

| Option | Meaning | Default |
|---|---|---|
| `--mode whitted` / `--mode path` | Classic ray tracing / path tracing | `whitted` |
| `--samples N` | Samples per pixel (anti-aliasing, or paths per pixel) | 9 (Whitted), 64 (path) |
| `--depth N` | Reflection depth / maximum bounces | 4 |
| `--time T` | Which second of the 30 s animation to render (`--time 18` is night) | 0 |
| `--camera N`, `--projection N` | The same viewpoints and projections as the OpenGL app | 0 |
| `--width`, `--height`, `--threads`, `--no-logo`, `--output` | Size, CPU threads, hide the postcard lettering, output file | 1280×720, all cores |

The image is written to `output/raytrace_whitted.ppm` or `output/raytrace_path.ppm`.

---

## 2. What the checkpoint asked for

> **Final.** *Refine and complete the project.*
>
> **Checkpoint 4 questions:** *Final demonstration · What works · What doesn't · Final changes or recommendations.*

In plain words: **finish it, show it, and be honest about it.** Show the finished project running. Say clearly what is done and what isn't, and what you'd do next. Checkpoint 4 also covers the last syllabus topics, **ray tracing**, **global illumination** and **anti-aliasing**, with a third renderer.

---

## 3. The script

### 3.1 Opening (30 seconds)

> "ASTROWORLD is a theme park built from the *Astroworld* album cover, with the giant golden head as its entrance. Over the checkpoints we built it three times. First a 2D renderer where we wrote every pixel ourselves. Then a real-time 3D park in OpenGL. And now, for Checkpoint 4, a **ray tracer** that draws the same 3D park the way light really behaves. I'll demonstrate all three, then tell you what works, what doesn't, and what I'd change."

### 3.2 Final demonstration, part 1: the 2D software renderer (1½ minutes)

**Do:** run `./build/astroworldWindow`. Press `3`, `4`, `7`, then `1` and `2`, and let the animation play.

> "This is Checkpoints 1 and 2. No graphics library draws anything here. Every pixel comes from our own C++ code:
> - lines with **DDA and Bresenham**
> - circles and ellipses with the **midpoint algorithm**
> - shapes filled with **scan-line** and **boundary fill**
> - the coaster track as a **spline**
>
> Press 3 for the algorithm showcase, 4 for colour theory, 7 for transformations. The park animates from day to night with **matrix transformations** and **keyframes**. The Ferris wheel is a transformation hierarchy, and the coaster is timed by energy conservation."

### 3.3 Final demonstration, part 2: the OpenGL park (2 minutes)

**Do:** run `./build/astroworldGl`. Go through the keys quickly. You've shown these in detail before.

> "This is Checkpoint 3 and the prototype: the same park in 3D on the GPU, built from our own meshes and shaders. Let me show the syllabus topics live:
> - **`C`** goes through the viewpoints, including **one-, two- and three-point perspective**.
> - **`V`** goes through the **projections**: isometric, cavalier, cabinet, top and front views.
> - **`G`** switches **flat, Gouraud and Phong shading**, and **`F`** shows the wireframe.
> - **`T` / `M`** switch **texture mapping** and **mipmaps**. Watch the far plaza shimmer without them.
> - **`2`** jumps to night with the searchlights, and **`R`** rides the coaster."

*If asked for detail, use the [Checkpoint 3](checkpoint3.md) and [Prototype](prototype.md) guides.*

### 3.4 Final demonstration, part 3: the ray tracer (3 minutes; the new work)

**Do:** run `./build/astroworldRay --width 640 --height 360`. While it renders (a few seconds), talk:

> "This is new for Checkpoint 4. OpenGL works *object by object*: it takes each triangle and asks which pixels it covers. A ray tracer works *pixel by pixel*. For every pixel it shoots a ray from the eye into the scene and asks *what does this ray hit first?* Then, from that point, it shoots more rays: one towards the sun to check for **shadow**, and, for shiny things, one in the **mirror direction** to see what is **reflected**. That's called **Whitted ray tracing**.
>
> Importantly, it draws the *exact same park*. It reads the same `ParkScene` the OpenGL program draws, the same meshes, materials, textures, sky, camera and animation time."

**Do:** when it finishes, point at the printed statistics, then open the result (or `rayTracedDay.png`).

> "It reports about 92,000 triangles. To avoid testing every ray against every triangle, we first build a **bounding volume hierarchy**: boxes inside boxes around groups of triangles. A ray that misses a box skips everything inside it. That takes a quarter of a second to build, and it's what makes this practical.
>
> Look at the picture:
> - **Hard shadows** under the rides, from the shadow rays. Our OpenGL version doesn't have these.
> - The **chrome ball** sculpture, which we added for this stage, reflects the whole park, and it's curved, so the reflection bends.
> - **Smooth edges.** Each pixel is sampled 9 times at jittered positions inside it and the results averaged. That's **supersampling anti-aliasing**."

**Show** `pathTracedDay.png` next to `rayTracedDay.png`.

> "And this is the **path tracer**, which is global illumination. Whitted only follows mirror bounces. A path tracer also lets light bounce off *matte* surfaces in random directions, the way real light does. So light bounces off the ground into shadows, colours bleed onto their neighbours, and the sky lights everything softly. Compare the shadows: here they're soft and filled with bounced light.
>
> Random bounces mean noise, so we average many paths per pixel, 128 for this image, which took about 3 minutes on all CPU cores. Two tricks make it efficient:
> - **next-event estimation**: at each bounce, we aim a shadow ray straight at the sun instead of hoping a random bounce finds it
> - **Russian roulette**: randomly stop weak paths, and boost the ones that survive, so the average stays correct
>
> The result has brightness above 1.0, so we **tone map** it to fit the screen instead of clipping it."

**Show** the night images.

> "And at night the bulbs and the glowing mouth become the light sources."

### 3.5 Performance: CPU vs GPU (30 seconds)

> "Some numbers, because they tell a story:
> - our 2D software renderer: about 15–20 frames per second
> - OpenGL on the GPU: about 80
> - the *same* OpenGL forced onto the CPU: about 14
> - the ray tracer: about 11 seconds for a clean Whitted frame (16 samples) and nearly 3 minutes for a path-traced one
>
> That's why games rasterize and films ray trace."

### 3.6 What works (1 minute)

> "**What works:**
> - **Every course topic is implemented and visible.** The README has a table mapping each one to its code. That includes raster and vector, colour models, all the line, circle, curve and fill algorithms, clipping, 2D and 3D transforms, every projection, the Phong model with three shading modes, texture mapping with mipmaps, ray tracing, path tracing, and anti-aliasing (supersampling and MSAA).
> - **Three renderers, one park.** They share the palette, curves, matrices and timeline. The OpenGL and ray-traced versions draw the identical scene.
> - **A 30-second animated loop**: day, dusk, night, dawn, with every ride moving.
> - **Full interaction**: camera, viewpoints, projections, shading, toggles, the coaster ride.
> - **Output**: images, MP4 videos and ray-traced stills, all scriptable from the command line. That's how every screenshot in the README was made."

### 3.7 What doesn't (1 minute; be honest, it scores well)

> "**What doesn't work, or isn't finished:**
> - **The 3D head is too smooth.** The nose, lips and eyelids are ellipsoids stuck onto the face, so close-ups look blobby, and the mouth is a glowing panel, not a real opening.
> - **No real-time shadows.** Only the ray tracer has shadows.
> - **The ray tracer is offline**: seconds to minutes a frame, with no denoiser, so low sample counts look grainy.
> - **The searchlight beams exist only in OpenGL**. The ray tracer has no volumetric light.
> - **Small things**: captions use our 5×7 bitmap font, not the album's lettering. OpenGL lines are 1 pixel wide, so the coaster lattice thins out in the distance. And it's only been tested on WSL2 / Ubuntu."

### 3.8 Recommendations (30 seconds)

> "**What I'd do next:**
> - **Shadow mapping** in OpenGL, so real-time shadows match the ray tracer.
> - A **faster ray tracer**: a smarter BVH, SIMD, a denoiser, and progressive rendering in the window.
> - A **sculpted head** with a real mouth.
> - **Physically based materials** and **HDR** in OpenGL too.
>
> And I've since done most of that list on two further branches, taking the project a step further. `photorealism` adds shadow maps, HDR, bloom, PBR materials and a sculpted head. `blender` exports the park to Blender for a film-quality render. Thank you."

*(Only say the last part if you're allowed to mention work after the deadline. Otherwise stop at "Thank you".)*

---

## 4. Requirement → what we did → where

| What was asked / needed | What we did | Where |
|---|---|---|
| **Refine and complete** | Three finished renderers of one park, plus the Checkpoint 4 report | [src/main.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/main.cpp), [src/windowMain.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/windowMain.cpp), [src/glMain.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/glMain.cpp), [src/rayMain.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/rayMain.cpp) |
| **CP4: final demonstration** | The walkthrough above; the up-to-date version is in the project report | [README on `main`](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/main/README.md), section *Project report* |
| **CP4: what works / what doesn't / recommendations** | Written up honestly; this branch's limitations close its README | [README on `main`](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/main/README.md), section *Project report*; [this branch's README](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/README.md#limitations), section *Limitations* |
| Ray tracing (Whitted): primary, shadow and reflection rays | `traceWhitted`: recursive mirror reflections, shadow rays, fog | [src/raytrace/rayTracer.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.cpp) |
| Global illumination | `tracePath`: cosine-weighted diffuse bounces, next-event estimation, Russian roulette, emission | [src/raytrace/rayTracer.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.cpp) |
| Ray–object intersection | Möller–Trumbore ray–triangle test, ray–box slab test | `intersectTriangle`, `rayHitsBox` in [src/raytrace/rayGeometry.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayGeometry.cpp) |
| Making it fast enough | Median-split BVH; all CPU cores share rows of the image | `RayScene::build` / `buildNode` / `traverse` in [src/raytrace/rayGeometry.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayGeometry.cpp); `rayTraceFrame` in [src/raytrace/rayTracer.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.cpp) |
| Anti-aliasing | Stratified, jittered supersampling (SSAA) in the ray tracer; MSAA in OpenGL | `rayTraceFrame` in [src/raytrace/rayTracer.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.cpp); [src/gl/glResources.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/gl/glResources.cpp) |
| HDR → display | Extended Reinhard tone mapping | `toneMap` in [src/raytrace/rayTracer.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.cpp) |
| Same park in every 3D renderer | Ray tracer reads `ParkScene`; the sky shader is ported to C++ | [src/park3d/parkScene.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/park3d/parkScene.cpp), [src/raytrace/rayTracer.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.cpp) |
| Same textures in both 3D renderers | Textures drawn once on the CPU; bilinear sampling for rays | `drawGroundTexture`, `drawSignTexture`, `sampleTextureBilinear` in [src/park3d/parkTextures.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/park3d/parkTextures.cpp) |
| Something that shows off reflection | A chrome mirror-ball sculpture on a plinth | `buildMirrorBall` in [src/park3d/parkModels.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/park3d/parkModels.cpp); placed in [src/park3d/parkScene.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/park3d/parkScene.cpp) |
| Any projection, ray traced | Each pixel is unprojected through the inverse view-projection matrix | [src/raytrace/rayTracer.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.h), [src/park3d/orbitCamera.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/park3d/orbitCamera.h) |

---

## 5. A short tour of the new files

| File | In one sentence |
|---|---|
| [src/rayMain.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/rayMain.cpp) | The command-line program: reads the options, builds the scene at time *t*, ray traces it, adds the postcard lettering and saves the image. |
| [src/raytrace/rayGeometry.h/.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayGeometry.cpp) | Turns the park's meshes into world-space triangles, builds the BVH, and answers "what does this ray hit?" and "is this point in shadow?". |
| [src/raytrace/rayTracer.h/.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/raytrace/rayTracer.cpp) | The two lighting methods (Whitted and path tracing), the sky, fog, tone mapping, supersampling and the multi-threaded frame loop. |
| [src/park3d/parkTextures.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-4/src/park3d/parkTextures.cpp) | Draws the ground and sign textures once, so OpenGL and the ray tracer use the same pixels. |

---

## 6. Questions you may be asked

**"What's the difference between ray tracing and rasterization?"**
> Rasterization goes object → pixels: for each triangle, find the pixels it covers. It's fast and it's what GPUs do. Ray tracing goes pixel → objects: for each pixel, find what the ray hits. It's slower, but shadows and reflections come naturally.

**"Whitted vs path tracing?"**
> Whitted follows only perfect mirror reflections and shadow rays to the lights, so it gives sharp shadows and no bounced light. Path tracing follows random diffuse bounces too, so it captures indirect light, soft shadows and colour bleeding. That's global illumination.

**"Why is the path-traced image noisy at low samples?"**
> Each pixel's value is an average of random light paths. Fewer paths give a noisier average. Noise falls with the square root of the sample count, so halving it needs four times the samples.

**"What's a BVH and why do you need it?"**
> A tree of boxes around groups of triangles. Without it, each ray would test all 92,000 triangles. With it, a ray skips any box it misses, so it tests only a handful.

**"What's shadow acne and how did you avoid it?"**
> When a shadow ray starts exactly on a surface, rounding errors make it hit that same surface, which speckles it with false shadow. We start the ray a tiny distance off the surface.

**"Why doesn't the ray-traced image have the searchlight beams?"**
> They're a blending trick in OpenGL. The ray tracer would need volumetric scattering, which is listed under what doesn't work.

**GitHub link:** https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-4

---

## 7. The one-minute version

> "At Checkpoint 4, ASTROWORLD has three renderers of one theme park. A 2D renderer where we wrote every line, circle, curve and fill algorithm ourselves. A real-time OpenGL park with every projection, flat, Gouraud and Phong shading, mipmapped textures and a coaster ride. And a new CPU ray tracer that draws the identical 3D scene. Whitted mode gives hard shadows and mirror reflections. Path-tracing mode gives global illumination. A BVH makes it practical, and supersampling smooths the edges. What works: every syllabus topic, visible and mapped to its code. What doesn't: a blobby 3D head, no real-time shadows, and an offline ray tracer. Next: shadow mapping, physically based materials, a sculpted head and a faster ray tracer."
