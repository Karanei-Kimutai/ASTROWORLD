# Presentation guide – Checkpoint 3: OpenGL integration

| | |
|---|---|
| **Branch** | [`checkpoint-3`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-3) (exact submitted state: tag `submitted/checkpoint-3`) |
| **Course stage** | *CP3 — OpenGL Integration* |
| **Presentation questions it answers** | Checkpoint 3: *Current OpenGL implementation. Animation/interaction evidence. GitHub link. What remains?* |
| **Speaking time** | about 8–10 minutes, plus questions |
| **What you show** | The 3D park running live in OpenGL, flying the camera around, the shaders and the reused CP1/CP2 code |

---

## 1. Before you present

```bash
git checkout checkpoint-3
rm -rf build && cmake -B build && cmake --build build -j
./build/astroworldGl                         # the 3D park (needs libglfw3-dev)
./tools/makeVideo.sh 30 30 opengl            # optional, beforehand: output/astroworldAnimationGl.mp4
```

Have open:
- the OpenGL window
- the backup video `output/astroworldAnimationGl.mp4`, or the GIF in the README
- in your editor:
  - [src/glScene/shaders.h](../../src/glScene/shaders.h)
  - [src/gl/glFunctions.h](../../src/gl/glFunctions.h)
  - [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp)
  - [src/mesh/meshBuilder.cpp](../../src/mesh/meshBuilder.cpp)

**Controls:**

| Input | Action |
|---|---|
| Mouse drag / scroll | Orbit the camera / zoom |
| `W` `A` `S` `D` / `Q` `E` | Move the camera's target / up and down |
| `C` | Cycle viewpoints: album cover, aerial, Ferris wheel, carousel |
| `O` | Automatic orbit on / off |
| `Space`, `←` / `→`, `↑` / `↓` | Play / pause, seek ±1 s, speed ×2 / ÷2 |
| `1` / `2` | Jump to day / night |
| `L` / `H` | Logo overlay / help panel |
| `P` | Save a screenshot to `output/` |

> **WSL note:** on this laptop the program uses the GPU through WSL's `d3d12` driver, and it selects that automatically. `--software` forces Mesa's CPU rasterizer instead, which is handy for the CPU-vs-GPU comparison below.

---

## 2. What the checkpoint asked for

> **CP3 — OpenGL Integration.** *Integrate your project components and previous work into an OpenGL-based implementation or your preferred environment.*

And the Checkpoint 3 presentation asks for:

1. The current OpenGL implementation
2. Animation/interaction evidence
3. The GitHub link
4. What remains?

In plain words: **move the park onto the graphics card with OpenGL, and reuse what we built before rather than throwing it away.** We went further than porting the 2D picture: we rebuilt the park as a real **3D world** you can fly around.

---

## 3. The script

### 3.1 Opening: stepping into the picture (1 minute)

**Do:** start `./build/astroworldGl`. Let it play on the album-cover view for a few seconds, then **drag the mouse** to swing the camera around the head.

> "Up to Checkpoint 2, ASTROWORLD was a flat painting. We could animate it, but we could never walk around it. In Checkpoint 3 we rebuilt it as a real 3D world running on the graphics card with OpenGL. This is the same park, the same rides, the same animation, the same day and night, but now I can fly the camera anywhere."

**Do:** press `C` to cycle viewpoints (aerial, Ferris wheel, carousel). Scroll to zoom into the head.

### 3.2 The OpenGL implementation (about 3 minutes)

> "Here's how it's built, layer by layer."

1. **The window and the context.**
   > "GLFW opens a window and creates an **OpenGL 3.3 core** context, the modern, shader-based OpenGL. OpenGL functions have to be *looked up* in the graphics driver while the program runs. Normally a library called GLAD does that. We couldn't install it offline, so we wrote our own loader. It's a list of every function we use, and each one is fetched from the driver at start-up. That showed us exactly what those libraries do."

   *Files:* [src/glMain.cpp](../../src/glMain.cpp), [src/gl/glFunctions.h](../../src/gl/glFunctions.h), [src/gl/glFunctions.cpp](../../src/gl/glFunctions.cpp).
2. **Shapes on the GPU.**
   > "There are no 3D model files. Every shape is built in code from simple primitives: boxes, cylinders, cones, ellipsoids, tori and tubes swept along a curve. The finished triangles go into GPU memory as **vertex buffers**. A vertex array object describes each vertex's position, normal, colour and texture coordinate."

   *Files:* [src/mesh/meshBuilder.cpp](../../src/mesh/meshBuilder.cpp) (`makeBox`, `makeCylinder`, `makeEllipsoid`, `makeTube`…), [src/mesh/meshData.h](../../src/mesh/meshData.h), [src/gl/glResources.cpp](../../src/gl/glResources.cpp) (`GpuMesh`, `ShaderProgram`, `Texture2D`).
3. **Shaders, the programmable pipeline.**

   **Do:** open [src/glScene/shaders.h](../../src/glScene/shaders.h).

   > "These are our shaders: small programs that run *on the GPU*, in parallel.
   > - The **vertex shader** runs once per corner of every triangle. It multiplies the position by projection × view × model, our Checkpoint 2 matrices, uploaded straight to the GPU.
   > - The **fragment shader** runs once per pixel and lights it with the **Phong reflection model**: ambient light, plus diffuse (brighter when the surface faces the sun), plus a specular highlight (the shine). At night the sun becomes the moon, and a warm **point light** inside the mouth lights the face and the crowd, fading with distance.
   > - **Fog** fades distant things toward the horizon colour, the way real air does."
4. **Depth and anti-aliasing.**
   > "In 2D we drew back to front, the painter's algorithm. Now the GPU keeps a **depth buffer**: for every pixel it remembers how far away the nearest surface is, and hides anything behind it. We also render into an off-screen buffer with **4× MSAA**, four samples per pixel, so edges are smooth."
5. **The sky.**
   > "The sky is one big triangle covering the screen. For each pixel, the shader works out which direction you're looking in. From that it paints the gradient, noise clouds and twinkling stars."

### 3.3 Reusing our earlier work (1–2 minutes, a strong point to make)

> "The brief says *integrate* the previous work, and that's what we did. A lot of this 3D park is CP1 and CP2 code running in a new place:
> - **Our CP2 `Matrix4`** builds every model, view and projection matrix. We added `lookAt` for the camera. We don't use GLM.
> - **Our CP1 Catmull–Rom splines**, extended to 3D, shape the coaster track and the dreads.
> - **Our CP1 Bézier curves** shape the closed-eye lash lines, the doorway arch and the bunting.
> - **Our CP2 timeline** (day/night keyframes, drop-tower keyframes, coaster physics) is now shared, so the 2D and 3D versions animate in perfect sync.
> - **Our CP1 software renderer** still draws the ASTROWORLD logo, the captions and the park sign's lettering. Those images become textures on the GPU. A trick called *difference matting* recovers their transparency.
> - **Our CP1 colour maths and palette** colour every vertex."

*Files:*
- [src/math/matrix4.cpp](../../src/math/matrix4.cpp): `lookAt` added
- [src/curves/spline.cpp](../../src/curves/spline.cpp): `sampleCatmullRomSpline3D` added
- [src/animation/astroworldTimeline.cpp](../../src/animation/astroworldTimeline.cpp): the shared timeline
- [src/glScene/overlayImage.cpp](../../src/glScene/overlayImage.cpp): `renderOverlayImage`, difference matting
- [src/scene/astroHeadShape.h](../../src/scene/astroHeadShape.h): head shape data shared by the 2D and 3D heads

### 3.4 The park in 3D (1 minute)

**Do:** press `C` to the album cover view, then press `2` for night.

> "Every component was rebuilt in 3D:
> - **The brass head:** its features are placed on an ellipsoid, the dreads are tubes swept along splines, and the doorway glows.
> - **The rides:** the Ferris wheel with gondolas hanging level, the teal truss coaster, the drop tower and the carousel.
> - **Props:** the rocket, the big top and the textured park sign.
> - **People and decoration:** visitors, balloons, bunting and the palisade fence.
>
> At night, bulbs and the portal glow, and the point light in the mouth lights up the faces of the people walking in."

*Files:*
- [src/glScene/headModel.cpp](../../src/glScene/headModel.cpp): the 3D head
- [src/glScene/parkModels.cpp](../../src/glScene/parkModels.cpp): the rides and props
- [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp): uploads the meshes and draws the park each frame (`drawRides`, `drawHead`, `drawPeople`, `drawSky`)

### 3.5 Animation and interaction evidence (1 minute)

**Do:** press `Space` to pause, use `←`/`→` to scrub, press `↑` to speed up, press `O` for the automatic orbit.

> "The whole 30-second loop runs here: the turning wheel, the physics-timed coaster, the drop tower, the breathing head with bending dreads, walking visitors, and day to dusk to night to dawn. For interaction, you can orbit, zoom and pan the camera, jump between viewpoints, play, pause, seek and change speed, and toggle the logo and help. Every frame can also be captured. That's how we made the video and the GIF in the README."

**CPU vs GPU, a nice concrete number:**

> "This also shows the CPU-vs-GPU idea from the course. On the graphics card, this 3D park runs at about **83 frames per second**. Force the *same* OpenGL code onto the CPU with `--software` and it drops to about **14**. The GPU wins because it runs thousands of vertices and pixels in parallel."

(`./build/astroworldGl --duration 6` prints the average frame rate. Add `--software` for the CPU run.)

### 3.6 GitHub and what remains (30 seconds)

> "The code is on GitHub at github.com/Karanei-Kimutai/ASTROWORLD, on the `checkpoint-3` branch.
>
> What remains for the prototype and Checkpoint 4:
> - **Shading modes:** flat, Gouraud and Phong shading, side by side.
> - **Every projection** in the syllabus, not just perspective.
> - **Texture mapping** with mipmaps.
> - **A ray tracer and path tracer** for photoreal images in Checkpoint 4.
> - **More interaction,** like riding the roller coaster."

### 3.7 Close (10 seconds)

> "So in Checkpoint 3 we moved ASTROWORLD onto the GPU: a real 3D world with shaders, Phong lighting, a depth buffer and anti-aliasing, built on our own matrices, curves, timeline and renderer from the earlier checkpoints. Thank you."

---

## 4. Requirement → what we did → where

| What was asked | What we did | Where |
|---|---|---|
| **OpenGL-based implementation** | OpenGL 3.3 core through GLFW, with our own function loader | [src/glMain.cpp](../../src/glMain.cpp), [src/gl/glFunctions.cpp](../../src/gl/glFunctions.cpp) |
| GPU resources | Shader programs, VBO/EBO/VAO meshes, textures, an off-screen MSAA frame buffer, all cleaned up automatically (RAII) | [src/gl/glResources.cpp](../../src/gl/glResources.cpp) |
| Rendering pipeline on the GPU | Vertex shader (model–view–projection), fragment shader (lighting), depth test, MSAA resolve | [src/glScene/shaders.h](../../src/glScene/shaders.h), [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp) |
| Phong reflection model | Ambient + diffuse + specular (Blinn-Phong) from the sun or moon, plus a point light in the mouth, and fog | [src/glScene/shaders.h](../../src/glScene/shaders.h) |
| **Integrate project components** | Every park component rebuilt as a 3D mesh | [src/glScene/headModel.cpp](../../src/glScene/headModel.cpp), [src/glScene/parkModels.cpp](../../src/glScene/parkModels.cpp), [src/mesh/meshBuilder.cpp](../../src/mesh/meshBuilder.cpp) |
| **…and previous work** | CP2 matrices, CP1 splines and Béziers, CP2 timeline, CP1 renderer for textures and overlays, CP1 palette | [src/math/matrix4.cpp](../../src/math/matrix4.cpp), [src/curves/spline.cpp](../../src/curves/spline.cpp), [src/animation/astroworldTimeline.cpp](../../src/animation/astroworldTimeline.cpp), [src/glScene/overlayImage.cpp](../../src/glScene/overlayImage.cpp) |
| Perspective projection, camera | `Matrix4::perspective`, `Matrix4::lookAt`, orbit camera with presets | [src/math/matrix4.cpp](../../src/math/matrix4.cpp), [src/glScene/orbitCamera.h](../../src/glScene/orbitCamera.h) |
| **Animation evidence** | The full 30 s loop running live; MP4 and GIF capture | [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp), [tools/makeVideo.sh](../../tools/makeVideo.sh) (`opengl` mode) |
| **Interaction evidence** | Orbit, zoom, pan, viewpoints, playback controls, toggles, screenshots | [src/glMain.cpp](../../src/glMain.cpp) |
| CPU vs GPU | ~83 fps on the GPU vs ~14 fps on Mesa's CPU rasterizer, same code | `astroworldGl --duration 6 [--software]` |

---

## 5. Questions you may be asked

**"Why didn't you use GLAD or GLEW?"**
> GLAD's generator wasn't available offline, so we wrote the loader ourselves. It's a list of the GL functions we need, each looked up in the driver by name at start-up. It's what GLAD generates, written by hand.

**"Why OpenGL 3.3 core and not the old fixed-function OpenGL?"**
> Core profile has no built-in lighting or matrices. You must write shaders and supply your own matrices. That's exactly the pipeline taught in the course, and it lets us use our own `Matrix4`.

**"How do normals stay correct when you scale shapes unevenly?"**
> Normals are transformed by the inverse-transpose of the model matrix, not the model matrix itself. Otherwise squashed shapes would be lit wrongly.

**"What replaced the painter's algorithm?"**
> The depth buffer (z-buffer). For every pixel, the GPU keeps the nearest depth so far and rejects anything farther away. Draw order no longer matters for solid objects.

**"Is the animation the same as Checkpoint 2?"**
> Yes, by design. Both renderers read the same timeline functions: night amount, drop-tower lift and coaster position.

**"Where's the GitHub link?"**
> https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-3

---

## 6. The one-minute version

> "In Checkpoint 3 we rebuilt ASTROWORLD as a real 3D world in OpenGL 3.3 core. GLFW gives us the window, our own loader finds the GL functions, every shape is generated in code and stored in GPU buffers, and our shaders do the model-view-projection transform and Phong lighting per pixel, with a point light in the mouth at night, fog, a procedural sky, a depth buffer and 4× MSAA. It reuses our CP2 matrices, CP1 splines and Béziers, the CP2 animation timeline, and even the CP1 renderer for the logo and text textures. You can fly around it, scrub through time and capture video. And it runs about six times faster on the GPU than the same code on the CPU."
