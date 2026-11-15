# Presentation guide – Prototype: all components combined

| | |
|---|---|
| **Branch** | [`prototype`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/prototype) (exact submitted state: tag `submitted/prototype`) |
| **Course stage** | *Prototype — combine the developed components into a functional prototype of the project* |
| **Presentation questions it answers** | It's the "Render & Integrate" phase, so you show a working, interactive whole and how it covers the syllabus |
| **Speaking time** | about 8–10 minutes, plus questions |
| **What you show** | One interactive app: shading modes, every projection, textures and mipmaps, searchlights and a coaster ride |

---

## 1. Before you present

```bash
git checkout prototype
rm -rf build && cmake -B build && cmake --build build -j
./build/astroworldGl
```

Have open:
- the OpenGL window
- images: `docs/screenshots/shadingModes.png`, `projections.png`, `perspectivePoints.png` and `coasterRide.png`, as a fallback
- in your editor:
  - [src/park3d/parkScene.h](../../src/park3d/parkScene.h)
  - [src/glScene/shaders.h](../../src/glScene/shaders.h)
  - [src/park3d/orbitCamera.h](../../src/park3d/orbitCamera.h)
  - [src/math/matrix4.cpp](../../src/math/matrix4.cpp)

**Controls (practise these; the demo is all keys):**

| Key | Action |
|---|---|
| Mouse drag / scroll, `W` `A` `S` `D` `Q` `E` | Orbit, zoom, move |
| `C` | Viewpoints: album cover, aerial, Ferris wheel, carousel, **one-, two-, three-point perspective** |
| `V` | Projections: perspective → isometric → cavalier → cabinet → top view → front view |
| `G` / `F` | Shading: flat → Gouraud → Phong / wireframe |
| `T` / `M` | Texture mapping on/off / mipmapping on/off |
| `R` | Ride the roller coaster |
| `O` | Automatic orbit |
| `Space`, arrows, `1` / `2` | Playback; jump to day / night |
| `L` / `H` / `P` | Logo / help / screenshot |

---

## 2. What the stage asked for

> **Prototype.** *Combine the developed components into a functional prototype of the project.*

In plain words: **put everything together into one program that works, and works as a whole.** We also used the prototype to cover every remaining syllabus topic that needs a live demo: shading interpolation, every projection type, texture mapping, and an interactive camera path.

---

## 3. The script

### 3.1 Opening: one program, everything in it (1 minute)

**Do:** start the program on the album-cover view and let it play.

> "Up to now we had separate pieces: the 2D renderer, the showcases and the 3D OpenGL park. The prototype brings everything into **one functional, interactive application**. Everything a viewer of our project should be able to do, they can do from this window: look at the park from any angle, in any projection, with any shading model, at any time of day, or ride the coaster."

### 3.2 The big structural change: one scene, any renderer (1–2 minutes)

**Do:** open [src/park3d/parkScene.h](../../src/park3d/parkScene.h).

> "The most important change is invisible on screen. We separated *what the park is* from *how it's drawn*. `ParkScene` describes the whole park at any time *t* as a list of **instances**: a mesh (which shape), a model matrix (where it is) and a material (its colour and shine), plus the lighting. It contains no OpenGL code at all. The OpenGL renderer just walks through that list and draws it.
>
> Two good things follow. First, one mesh, like a gondola, a horse or a visitor, is drawn many times with different matrices, which is instancing. Second, any other renderer can draw the exact same park. That's what made our final ray tracer possible."

*Files:*
- [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp): where the park is described
- [src/park3d/headModel.cpp](../../src/park3d/headModel.cpp) and [src/park3d/parkModels.cpp](../../src/park3d/parkModels.cpp): the shapes
- [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp): the OpenGL side that draws a frame

### 3.3 Shading interpolation, live (2 minutes; a key syllabus topic)

**Do:** zoom in on the head or the carousel. Press `G` slowly through the three modes, then `F`.

> "Shading interpolation decides *where* lighting is calculated across a triangle.
> - **Flat shading:** one normal per triangle, so each facet is one colour. You can see every flat face. We get that normal from how the position changes across the screen, using the cross product of the screen-space derivatives.
> - **Gouraud shading:** the Phong lighting is calculated only at the *corners* of each triangle, in the vertex shader, and the colours are blended across. It's smooth and cheap, but look at the highlight: it smears, and on big triangles it can disappear completely.
> - **Phong shading:** we blend the *normal* across the triangle instead, and calculate lighting at *every pixel*. The highlight is sharp and in the right place.
>
> All three use the same lighting function in our shader. Only where it runs changes. And this is the wireframe (press `F`), showing the triangles underneath."

**Show** `shadingModes.png` if needed.

*File:* [src/glScene/shaders.h](../../src/glScene/shaders.h): the three modes and the shared lighting function. The `ShadingMode` enum is in [src/glScene/astroworldPark.h](../../src/glScene/astroworldPark.h).

### 3.4 Every projection in the syllabus (2 minutes)

**Do:** press `V` through the projection modes, pausing on each.

> "Projection turns the 3D world into a 2D image. Press V:
> - **Isometric**, a kind of axonometric orthographic projection. The camera looks down the diagonal of a cube, about 35° down and 45° around, so the three axes shrink equally. There's no perspective: parallel lines stay parallel.
> - **Cavalier**, an oblique projection. Depth is drawn at 45° at *full* length, so things look too deep.
> - **Cabinet**, also oblique, but depth is drawn at *half* length. That looks more natural.
> - **Top view and front view**, the orthographic multiview projections: a true plan and a true elevation, like engineering drawings.
> - **Back to perspective**, where far things are smaller."

**Do:** press `C` until the **one-point**, **two-point** and **three-point perspective** viewpoints come up.

> "And the three perspective cases:
> - **One-point:** the camera is square to the fence, so only the lines going *into* the scene meet at a vanishing point.
> - **Two-point:** turned 45°, so both sets of ground lines converge.
> - **Three-point:** tilted up at the drop tower, so even the vertical lines converge."

**Show** `projections.png` and `perspectivePoints.png` if needed.

*Files:*
- [src/park3d/orbitCamera.h](../../src/park3d/orbitCamera.h): the `ProjectionMode` options and how each matrix is built, plus the camera presets
- [src/math/matrix4.cpp](../../src/math/matrix4.cpp): `perspective`, `orthographic`, `obliqueShear` (shears depth onto a 45° diagonal before an orthographic projection) and `lookAt`

> "The frustum, the course's 'viewing volume', is the shape the perspective matrix maps into a cube. The GPU clips everything outside it, then divides by w."

### 3.5 Texture mapping and mipmaps (1 minute)

**Do:** go to the aerial or album-cover view and look at the far plaza. Press `T`, then `M`.

> "The plaza is **texture mapped**: a 4-metre concrete slab with joints and spilled popcorn, *drawn by our own CP1 software renderer*, uploaded as a texture and **tiled** a hundred times across the ground. The park sign's lettering is a texture too. `T` switches texturing off and on.
>
> `M` turns off **mipmapping**. Look at the distant slabs: they shimmer and sparkle. A far-away pixel covers many texture pixels, and picking just one causes aliasing. Mipmaps are pre-shrunk copies of the texture, so the GPU samples the right size, which is trilinear filtering. Turn them back on and it's calm again."

*Files:* `Texture2D` and `setMipmapFiltering` in [src/gl/glResources.cpp](../../src/gl/glResources.cpp); the texture is uploaded in [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp), drawn by the CP1 renderer through `renderOverlayImage`.

### 3.6 Night atmosphere and the coaster ride (1 minute)

**Do:** press `2` for night. Point at the sweeping searchlights.

> "At night, searchlight beams sweep the sky. They're cones drawn with **additive blending**: their colour adds to what's behind them, so they glow. They fade along their length, and they don't write to the depth buffer, so they never hide anything."

**Do:** press `R`.

> "And the ride. The camera sits on the front car of the coaster, which still uses our CP2 energy-conservation timing. It looks a few metres ahead along the spline track, tilted with the car. The coaster's position comes from the same scene description as everything else."

*Files:* `coasterRideView` in [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp), used by the camera in [src/glMain.cpp](../../src/glMain.cpp); the coaster timing `coasterTrainFractionAt` in [src/animation/astroworldTimeline.cpp](../../src/animation/astroworldTimeline.cpp); searchlight beams in [src/park3d/parkModels.cpp](../../src/park3d/parkModels.cpp) and [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp).

### 3.7 Close (20 seconds)

> "So the prototype is the whole project in one working program. A renderer-independent scene. Live flat, Gouraud and Phong shading. Every projection: perspective with one, two and three vanishing points, isometric, cavalier, cabinet and multiview. Mipmapped texture mapping, searchlights, and a coaster ride. Next, the final stage adds ray tracing and path tracing. Thank you."

---

## 4. Requirement → what we did → where

| What was asked / needed | What we did | Where |
|---|---|---|
| **Combine the components into one functional prototype** | A single interactive app containing the whole park and every mode | [src/glMain.cpp](../../src/glMain.cpp) |
| A clean way to combine them | `ParkScene`: the park at time t as instances + lighting, with no API code | [src/park3d/parkScene.h](../../src/park3d/parkScene.h), [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp) |
| Shading interpolation: flat, Gouraud, Phong | Three live modes sharing one lighting function; wireframe view | [src/glScene/shaders.h](../../src/glScene/shaders.h), [src/glScene/astroworldPark.h](../../src/glScene/astroworldPark.h) |
| Orthographic: multiview, axonometric/isometric | Top and front views; isometric camera | [src/park3d/orbitCamera.h](../../src/park3d/orbitCamera.h), `Matrix4::orthographic` |
| Oblique: cavalier, cabinet | Depth sheared onto a 45° diagonal, full or half length | `Matrix4::obliqueShear` in [src/math/matrix4.cpp](../../src/math/matrix4.cpp) |
| One-, two-, three-point perspective; COP; frustum | Camera presets for each; `Matrix4::perspective` with the eye as centre of projection | [src/park3d/orbitCamera.h](../../src/park3d/orbitCamera.h), [src/math/matrix4.cpp](../../src/math/matrix4.cpp) |
| Texture mapping | Tiled plaza texture drawn by our CP1 renderer; UV-mapped sign | [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp), [src/gl/glResources.cpp](../../src/gl/glResources.cpp) |
| Anti-aliasing of textures | Mipmaps with trilinear filtering, toggled live | `Texture2D::setMipmapFiltering` in [src/gl/glResources.cpp](../../src/gl/glResources.cpp) |
| Interaction | Full keyboard and mouse control; a first-person coaster ride | [src/glMain.cpp](../../src/glMain.cpp), `coasterRideView` |
| Atmosphere | Additive-blended searchlight beams | [src/park3d/parkModels.cpp](../../src/park3d/parkModels.cpp) |

---

## 5. Questions you may be asked

**"Why does Gouraud lose the highlight?"**
> Lighting is only calculated at the vertices. If the shiny spot falls in the middle of a large triangle, no vertex sees it, so the blended colours can't contain it.

**"Cavalier vs cabinet: what's the difference?"**
> Both draw depth at an angle (45° here). Cavalier keeps the full depth length, cabinet halves it, so cabinet looks less stretched.

**"What makes a view one-, two- or three-point perspective?"**
> How many of the world's main axes aren't parallel to the image. Each one gets a vanishing point. Square-on gives one, turned gives two, and turned and tilted gives three.

**"What problem do mipmaps solve?"**
> Texture aliasing in the distance. Many texels fall into one screen pixel, and sampling just one causes shimmer. Mipmaps pre-average the texture at smaller sizes.

**"Why separate the scene description from OpenGL?"**
> So the same park can be drawn by any renderer. The final ray tracer reads exactly the same `ParkScene`, which guarantees the two images show the same thing.

**GitHub link:** https://github.com/Karanei-Kimutai/ASTROWORLD/tree/prototype

---

## 6. The one-minute version

> "The prototype combines everything into one interactive OpenGL program. The park is described once, as a renderer-independent scene of instances and lights. From the keyboard you can switch flat, Gouraud and Phong shading. You can step through every projection: isometric, cavalier, cabinet, top and front multiview, and perspective with one, two and three vanishing points. You can toggle texture mapping and mipmapping on a plaza texture drawn by our own CP1 renderer, watch additive searchlights at night, and ride the physics-timed coaster in first person."
