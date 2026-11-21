# Presentation guide – Taking a step further: photorealism

| | |
|---|---|
| **Branch** | [`photorealism`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/photorealism), built on top of `checkpoint-4` |
| **Course stage** | Taking a step further. It carries out the [Checkpoint 4 report's](checkpoint4.md) recommendations |
| **Presentation questions it answers** | "What did you do after Checkpoint 4?" and "How would you take this further?" |
| **Speaking time** | about 8 minutes, plus questions |
| **What you show** | The real-time OpenGL park, now photoreal: gold inflatable head, visitors walking through its mouth, new rides, real shadows, glow |

---

## 1. Before you present

```bash
git checkout photorealism
rm -rf build && cmake -B build && cmake --build build -j
./build/astroworldGl
```

Have these images ready (in `docs/screenshots/`) as slides or a fallback:
- `photoGateNight.png`, `photoGate.png`, `photoInside.png`
- `photoDay.png`, `photoNight.png`, `photoRides.png`, `photoAerial.png`
- `photoPathDay.png`, `photoRayDay.png`, `photoRayNight.png`

**Keys** (everything from the prototype still works, plus):

| Key | Action |
|---|---|
| `C` | 11 viewpoints, including **THE GATE**, **INSIDE THE PARK**, **SWING RIDE** and **PIRATE SHIP** |
| `X` | Sun shadows on/off (`--no-shadows` on the command line) |
| `B` | Bloom on/off (`--no-bloom`) |
| `1` / `2`, `Space`, arrows | Day / night, playback |
| `H` | Help |

> The frame rate is about 28 fps by day and 22 fps at night on this laptop's integrated GPU. That's smooth enough to demo. Close other programs first.

---

## 2. Why this stage exists

The [Checkpoint 4 report](checkpoint4.md) listed what didn't work and what we'd do next:

> *Shadow mapping in OpenGL · physically based materials and HDR in OpenGL · a sculpted head mesh with a real mouth opening.*

This branch does all of that. It also has a bigger goal: **make it look like a real ASTROWORLD theme park.** That means the giant inflatable head from the album cover as the entrance, more rides, and visitors who walk *into* the park through the mouth instead of vanishing.

---

## 3. The script

### 3.1 Opening: the gate at night (30 seconds)

**Do:** press `2` for night and `C` until **THE GATE**. Let it play.

> "In the Checkpoint 4 report I said the 3D head looked blobby, the mouth was just a glowing panel, and OpenGL had no shadows. This branch fixes those. What you're looking at is still running in real time, on the same OpenGL code base. The head is now the gold inflatable from the album cover. It's the park's entrance, and visitors walk *through its mouth* into the park."

### 3.2 Physically based shading (1½ minutes)

**Do:** press `1` for day. Orbit close to the gold head so the sky shows in it.

> "Until now we lit everything with the Phong model: ambient plus diffuse plus a specular highlight. It's a good approximation, but it isn't physical. Here we use **physically based rendering**, the **Cook–Torrance** model. It treats a surface as millions of tiny mirrors, called microfacets. Three terms decide how light reflects:
> - **D**, the GGX distribution: how many of those tiny mirrors point the right way to send light at your eye. A rough surface has them scattered, so the highlight is big and soft. A smooth one has them aligned, so the highlight is small and sharp.
> - **G**, the geometry term: how many are hidden in each other's shadows.
> - **F**, Fresnel: everything gets more mirror-like at grazing angles. Look at the ground far away.
>
> Each material just has two numbers, **metallic** and **roughness**. The head is gold vinyl, metallic 0.85 and roughness 0.3, so it *reflects the sky and the plaza* like the real inflatable in the photo."

*File:* [src/glScene/shaders.h](../../src/glScene/shaders.h): `distributionGGX`, `geometrySchlickGGX` and `fresnelSchlick`, plus the environment reflection.

### 3.3 Shadow mapping (1 minute)

**Do:** find a ride with a clear shadow. Press `X` off and on.

> "Shadows, in real time. Before drawing the frame, we draw the scene a second time *from the sun's point of view*, keeping only depth: how far each point is from the sun. That's the **shadow map**, a 4096 by 4096 depth image. It's orthographic, because sunlight is parallel.
>
> Then, for every pixel we draw, we ask: *is something between me and the sun?* If my distance to the sun is bigger than what the shadow map stored, something blocks me, so I'm in shadow. We compare a 3×3 neighbourhood so the edges are soft, and nudge the comparison slightly to avoid **shadow acne**, the same problem we had in the ray tracer."

*File:* `ShadowMap` in [src/gl/framePipeline.h](../../src/gl/framePipeline.h) / [framePipeline.cpp](../../src/gl/framePipeline.cpp); the depth comparison is in [src/glScene/shaders.h](../../src/glScene/shaders.h).

### 3.4 HDR, tone mapping and bloom (1 minute)

**Do:** press `2` for night. Point at the lamps and the mouth. Press `B` off and on.

> "Real light has a huge range: a bulb is thousands of times brighter than a wall at night. An 8-bit image can't hold that. So we light the scene into a **high-dynamic-range** buffer, 16-bit floating point, where brightness can go far beyond 1. Then:
> - **ACES filmic tone mapping** squeezes that range onto the screen the way film does, instead of clipping it to flat white.
> - **Gamma correction**: all the maths happens in *linear* light, and we convert to sRGB only at the very end.
> - **Bloom**: we take only the brightest pixels, blur them, and add them back. That's the glow you see around bright lights through a real camera lens. Press B and it's gone, and the night looks flat."

*File:* [src/gl/framePipeline.cpp](../../src/gl/framePipeline.cpp): `FramePipeline` creates the multisampled HDR buffer, the two half-size bloom buffers and the final buffer. The tone-mapping and blur shaders are in [src/glScene/shaders.h](../../src/glScene/shaders.h).

### 3.5 The inflatable head (1 minute)

**Do:** `C` to **THE GATE** by day, then orbit to see the head's side and the mouth.

> "The head is now **one sculpted surface** of about 130,000 triangles. Instead of sticking ellipsoids on, we start from a smooth head shape and *push the surface out along its normals*, making bumps for the nose, the pouting upper lip, the closed eyelids, the cheeks and the brow. So they flow into each other like inflated vinyl. There are even faint **seams** down the sides, like a real inflatable.
>
> The mouth is a **real opening**: the arch is cut out of the front and the back, with a glowing **tunnel** joining them, so you can walk through. The dreads are fat inflated tubes that sway."

*File:* [src/park3d/headModel.cpp](../../src/park3d/headModel.cpp): `buildHeadMesh`, `buildMouthTunnel` and `buildDreadMesh`.

### 3.6 A bigger, living park, with physics (1½ minutes)

**Do:** `C` to **SWING RIDE**, then **PIRATE SHIP**, then **INSIDE THE PARK**.

> "The new rides each move by real physics or a transformation hierarchy, which is our CP2 work again:
> - **Wave swinger.** The chairs swing out to the angle where the chain's pull balances gravity and the centripetal force: **tan θ = ω²r / g**. As they swing out, the radius grows, which changes the angle, so we solve it by repeating the formula until it settles. That's fixed-point iteration.
> - **Pirate ship.** A pendulum, with period **T = 2π√(L/g)**. The swing builds up and dies away over each ride.
> - **Teacups.** Three nested rotations: platform, turntables, cups.
> - **Bumper cars.** Each follows its own Lissajous curve and turns to face where it's going.
>
> And there are visitors, about 140 of them. They walk a closed spline: in through the mouth, around the rides, out of the exit gate. Their legs swing according to the *distance they've walked*, not the time, so their feet don't slide on the ground."

*Files:*
- [src/park3d/parkRides.cpp](../../src/park3d/parkRides.cpp): the new ride, stall, tree, lamp and wall meshes
- [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp): where everything stands, the swing angle, pendulum, bumper paths, the walking route and the night lights

### 3.7 Performance and the ray tracer (30 seconds)

> "About 1,400 objects are drawn every frame. With **GPU instancing**, all copies of one mesh go to the GPU in a single draw call with a list of matrices. That cut draw calls from about 2,750 to 300.
>
> And the ray tracer still draws the same scene, with the same physically based materials, so it serves as a 'ground truth' reference for the real-time image."

**Show** `photoPathDay.png` or `photoRayDay.png`.

*Files:* instancing in [src/gl/glResources.cpp](../../src/gl/glResources.cpp) and [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp); the ray tracer in [src/raytrace/rayTracer.cpp](../../src/raytrace/rayTracer.cpp).

### 3.8 Close (20 seconds)

> "So this branch turns the Checkpoint 4 report's 'what doesn't work' into 'what works': real-time shadows, physically based materials, HDR and bloom, and a sculpted walk-through head, all in a much bigger park that's alive. The next step, on the `blender` branch, takes this same park into Blender for film-quality rendering. Thank you."

---

## 4. Goal → what we did → where

| Goal (from the Checkpoint 4 report) | What we did | Where |
|---|---|---|
| Physically based materials | Cook–Torrance BRDF (GGX, Smith–Schlick, Schlick Fresnel) with metallic/roughness; sky/ground ambient and environment reflection | [src/glScene/shaders.h](../../src/glScene/shaders.h) |
| Real-time shadows | 4096² orthographic sun shadow map, 3×3 PCF, polygon and normal offset | `ShadowMap` in [src/gl/framePipeline.cpp](../../src/gl/framePipeline.cpp), [src/glScene/shaders.h](../../src/glScene/shaders.h) |
| HDR in OpenGL | 16-bit float 4× MSAA buffer, ACES tone mapping, sRGB gamma, exposure, vignette | `FramePipeline` in [src/gl/framePipeline.cpp](../../src/gl/framePipeline.cpp) |
| Glow from bright lights | Bright pass + separable Gaussian blur at half resolution (bloom) | [src/gl/framePipeline.cpp](../../src/gl/framePipeline.cpp), [src/glScene/shaders.h](../../src/glScene/shaders.h) |
| Sculpted head with a real mouth | One normal-displaced surface (~130k triangles), seams, cut-out mouth, tunnel, inflated dreads | [src/park3d/headModel.cpp](../../src/park3d/headModel.cpp) |
| A bigger, believable park | Palisade wall, album-cover plaza props, stalls, ~85 trees, lamps, meadow, exit gate | [src/park3d/parkRides.cpp](../../src/park3d/parkRides.cpp), [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp) |
| New rides moved by physics | Wave swinger (tan θ = ω²r/g), pirate ship (pendulum), teacups (nested rotations), bumper cars (Lissajous) | [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp), [src/park3d/parkRides.cpp](../../src/park3d/parkRides.cpp) |
| Life | ~140 visitors on a closed spline through the mouth; steps match distance walked; riders on rides | [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp) |
| Night lighting | Up to 16 nearest point lights (mouth, uplights, lamps, rides, stalls) | [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp), [src/glScene/shaders.h](../../src/glScene/shaders.h) |
| Keep it real-time | GPU instancing (`glDrawElementsInstanced`, `glVertexAttribDivisor`) | [src/gl/glResources.cpp](../../src/gl/glResources.cpp), [src/glScene/astroworldPark.cpp](../../src/glScene/astroworldPark.cpp), [src/gl/glFunctions.h](../../src/gl/glFunctions.h) |
| Reference renderer stays in sync | Ray tracer uses the same PBR materials, lights, sky and ACES | [src/raytrace/rayTracer.cpp](../../src/raytrace/rayTracer.cpp) |

---

## 5. A short tour of the new files

| File | In one sentence |
|---|---|
| [src/gl/framePipeline.h/.cpp](../../src/gl/framePipeline.cpp) | The off-screen buffers a frame passes through: the multisampled HDR scene, bloom, the final image, and the sun's shadow map. |
| [src/park3d/parkRides.h/.cpp](../../src/park3d/parkRides.cpp) | The meshes of the new rides and the park's furniture: swing tower and chairs, ship, teacups, bumper cars, stalls, trees, lamps, statue, wrecked car, gate and walls. |

---

## 6. Questions you may be asked

**"Why is PBR better than Phong?"**
> Phong's numbers are made up to look right, so they need retuning for every light. PBR follows how light really reflects and conserves energy, so one material looks right under sun, lamps or sky.

**"How does a shadow map work in one sentence?"**
> Draw the scene's depth from the light, then for each pixel check whether something nearer to the light was recorded there. If so, it's in shadow.

**"What's the point of HDR if the screen is 8-bit anyway?"**
> Lighting maths needs the real range. Bright things add up correctly and bloom picks out what's really bright. Only the last step, tone mapping, squeezes it down for the screen.

**"Why is the frame rate lower than before?"**
> Much more work per pixel: PBR, 16 lights, shadow lookups, bloom and a full-screen sky, on an integrated GPU behind WSL. Instancing fixed the CPU side, so the GPU's pixel work is now the limit.

**"What's still not realistic?"**
> The visitors and trees are built from simple shapes, and the procedurally sculpted face is softer than the real inflatable. That's what the Blender branch tackles.

**GitHub link:** https://github.com/Karanei-Kimutai/ASTROWORLD/tree/photorealism

---

## 7. The one-minute version

> "The photorealism branch carries out the Checkpoint 4 report's recommendations in real time. Phong lighting becomes physically based Cook–Torrance shading with metallic and roughness, so the gold head reflects the sky. A sun shadow map gives real shadows. Lighting is computed in HDR, then ACES tone mapped with bloom around bright lights. The head is a single sculpted inflatable with a real walk-through mouth. And the park is much bigger: a wave swinger obeying tan θ = ω²r/g, a pendulum pirate ship, teacups and bumper cars, about 140 visitors walking in through the mouth, and lamp-lit nights. Instancing keeps it running at over 20 frames per second."
