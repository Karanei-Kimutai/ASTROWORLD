# Presentation guide – Taking a step further: Blender (latest work)

| | |
|---|---|
| **Branch** | [`blender`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/blender), also on `main`; built on top of `photorealism` |
| **Course stage** | Taking a step further. The most recent work |
| **Presentation questions it answers** | "Where did you take the project in the end?" and "How does your engine fit into a real production pipeline?" |
| **Speaking time** | about 8 minutes, plus questions |
| **What you show** | The 30-second film `output/astroworldBlender.mp4`, the `.blend` file playing inside Blender, and the new closed-circuit roller coaster |

---

## 1. Before you present

**Play the film, don't render it.** The full animation took about 2½ hours to render (720 frames at ~16–34 s each), so render it the day before.

```bash
git checkout blender
rm -rf build && cmake -B build && cmake --build build -j
```

> Don't delete `output/`. That's where the Blender data and the rendered film live, and they take hours to make.

Have ready:
- **The film:** `output/astroworldBlender.mp4`. If it's missing, render it with `tools/blender/render.sh animation`, which takes hours.
- **The `.blend` open in Blender** (Windows), so you can scrub the timeline live:

  ```bash
  tools/blender/render.sh scene
  ```

  Then open `Documents\ASTROWORLD-Blender\astroworld.blend` and click **Allow Execution** when Blender asks, so the park animates.
- **The OpenGL park** for the coaster: `./build/astroworldGl`.
- **Images** in `docs/screenshots/`: `blenderGate.png`, `blenderMouth.png`, `blenderTunnel.png`, `blenderNight.png`, `blenderSwing.png`, `blenderAerial.png` and `coasterCircuit.png`.

**All the ways to render** (each one exports from our engine first):

| Command | What you get | How long |
|---|---|---|
| `tools/blender/render.sh scene` | The `.blend` file to open in Blender | about a minute |
| `tools/blender/render.sh stills 1,145,313,720` | Small preview frames in `output/blender/stills/` | a few minutes |
| `tools/blender/render.sh still 313` | One full-quality EEVEE frame | under a minute |
| `tools/blender/render.sh cycles 313` | One path-traced Cycles frame | several minutes |
| `tools/blender/render.sh animation` | All 720 frames → `output/astroworldBlender.mp4` | about 2½ hours |

---

## 2. Why this stage exists

The photorealism branch made the park look as real as our own real-time renderer can on a laptop GPU. Two things still held it back:
- the procedurally sculpted face was too soft
- real-time tricks can't match film-quality light: soft volumetric beams, depth of field, motion blur

The idea of this stage: **keep our C++ engine as the "brain" of the park, and let Blender be the "camera".** Our code still decides every shape, every position at every moment, the physics of every ride and the lighting timeline. Blender only does the final, highest-quality rendering. That's how real studios work: one tool for animation, another for rendering.

---

## 3. The script

### 3.1 Opening: play the film (1 minute)

**Do:** play `output/astroworldBlender.mp4` full screen. Let it run for its 30 seconds without talking for the first few.

> "This is ASTROWORLD rendered in Blender: a 30-second loop from day through sunset into night. Everything that *moves* in this film, every visitor, every ride, the coaster train, the swaying dreads, the sun and the searchlights, was computed by **our own C++ engine**, the same code from the checkpoints. Blender only made it beautiful."

### 3.2 How the pipeline works: export → build → play (2 minutes)

**Show** the "How it works" table in the README, or draw three boxes on the board: *our engine → files → Blender*.

> "There are three steps.
>
> **1. Export.** A new program, `astroworldExport`, runs our `ParkScene`, the same scene description the OpenGL program and the ray tracer use. It samples the park 24 times a second for 30 seconds and writes:
> - every **mesh** once: the head, a visitor's torso, a gondola, and so on
> - for every frame, every object's **model matrix**, its tint and how much it glows (`frames.bin`)
> - the **lighting** for every frame: where the sun is, the sky colours, the point lights and the exposure
> - the **textures**
>
> **2. Build.** A Python script inside Blender reads those files and builds the scene. It shares each mesh between its copies, so the 140 visitors all use one torso mesh, just like our GPU instancing. Then it adds what Blender does best:
> - physically based materials: gold vinyl with a clear coat, chrome, paving
> - a physical sky with a real sunset
> - searchlights as spotlights inside glowing haze
> - a camera fly-through, bloom, motion blur and filmic colour
>
> **3. Play.** Another small script runs every time the frame changes. It looks up that frame in the exported data and moves every object. It's stored *inside* the `.blend` file, so when you open it in Blender and press play, the park moves."

**Do:** switch to Blender and scrub the timeline. The rides turn and the visitors walk.

*Files:*
- [src/exportMain.cpp](../../src/exportMain.cpp): the exporter
- [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py): builds the Blender scene (meshes, materials, the sculpted head, lights, sky, camera, render settings)
- [tools/blender/astroworldPlayer.py](../../tools/blender/astroworldPlayer.py): the per-frame player
- [tools/blender/render.sh](../../tools/blender/render.sh): one command for every step

### 3.3 The hand-sculpted head (1 minute)

**Show** `blenderMouth.png` and `blenderTunnel.png`.

> "The one thing our engine couldn't do well was the face. So the head, the dreads and the mouth tunnel were **sculpted by hand in Blender**, using Blender's sculpting tools, and saved in the repository as `head.blend`. The sculpt had to respect our engine's rules. The mouth had to be big enough for the visitors' walking path to pass through, because that path is computed in our C++ code.
>
> When the scene is built, the script **swaps the sculpted parts into the existing objects**. They keep their material, their glow and their animation. The dreads even get a wind-sway shape so they move in the breeze."

*Files:* [assets/blender/head.blend](../../assets/blender/head.blend); the swap and `add_sculpted_sway` are in [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py).

### 3.4 Light: day, sunset, night (1 minute)

**Show** `blenderGate.png` then `blenderNight.png` and `blenderSwing.png`.

> "By day it's a **physical sky model**: Blender simulates how sunlight scatters in the atmosphere, so as our timeline lowers the sun, the sky really turns orange. At night it switches to our own purple starry sky from the album cover. The searchlights are spotlights inside cones of glowing haze. That's **volumetric light**, the thing our ray tracer couldn't do. Bulbs, lamps and the glowing mouth get **bloom**, and moving things get **motion blur**, like a real camera's shutter."

### 3.5 The closed-circuit roller coaster (1½ minutes)

**Do:** in `./build/astroworldGl`, press `C` to **AERIAL**, or show `coasterCircuit.png`. Then press `R` to ride.

> "One change in our engine came with this branch. The old coaster was our 2D side view lifted into 3D: the train rose out of the ground and vanished at the far end. Now it's a **closed circuit**:
> - station → chain lift to 24 metres → first drop into a climbing turn
> - camelback hills → a sweeping turn back → brakes → station
>
> The track is **15 control points** joined into a **closed Catmull–Rom spline**, the same spline from Checkpoint 1, but closed, so the end flows smoothly into the start.
>
> The train's speed is **physics**:
> - In the station it moves at a fixed 2.5 m/s. The chain pulls it up at 4 m/s.
> - After the top, **energy conservation**: half v-squared equals the crest energy plus g times the height dropped, minus a small friction term. So it's fastest at the bottom, about 21.5 m/s, 77 km/h, and slows on every hill.
> - On the brake run, the speed is capped so it glides gently back into the station.
>
> And one lap takes exactly 30 seconds, one animation loop, so the film loops seamlessly."

*Files:*
- `buildCoasterTrackPath` in [src/park3d/parkModels.cpp](../../src/park3d/parkModels.cpp): the circuit's control points
- `coasterLeadDistanceAt` in [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp): the speed integration

### 3.6 What we learned (30 seconds)

> "Three lessons:
> - **Per-object modifiers are expensive.** Smoothing each of 500 visitors separately made every frame take 150 seconds. Smoothing the shared meshes *once* brought it down to 16–34 seconds.
> - **A fog over the whole world** turned the night completely black. Fog only where we need it, in the beams and the mouth, gives the look cheaply.
> - **Reading files across Windows and Linux is slow**, so the player reads the frame data in one go."

### 3.7 Close (20 seconds)

> "So the whole journey ends here. From drawing single pixels with Bresenham in Checkpoint 1, to our own engine driving a film-quality render in Blender. And the engine is still the single source of truth: change a ride in C++, re-export, and the film changes. Thank you."

---

## 4. Goal → what we did → where

| Goal | What we did | Where |
|---|---|---|
| Keep our engine as the source of truth | Exporter samples `ParkScene` at 24 fps × 30 s: meshes once, matrices/tints/emission per frame, lighting per frame, textures | [src/exportMain.cpp](../../src/exportMain.cpp), [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp) |
| Build the scene in Blender quickly | Bulk mesh creation with shared meshes; subdivision baked into shared meshes | `build_mesh`, `bake_subdivision` in [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py) |
| Film-quality materials | Principled BSDF: gold vinyl with clear coat, chrome, textured paving | `principled_material`, `material_for` in [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py) |
| Realistic light | Sun with a real sunset, moon, park point lights; physical sky by day, starry sky at night; volumetric searchlight beams | Lights and World sections of [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py) |
| Camera work | A designed fly-through; motion blur, bloom, AgX colour | Camera and Render-settings sections of [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py) |
| Motion inside Blender | Frame-change handler applying exported frames (with sub-frame interpolation), embedded in the `.blend` | `Player`, `on_frame_change` in [tools/blender/astroworldPlayer.py](../../tools/blender/astroworldPlayer.py) |
| A better head | Hand-sculpted head, dreads and tunnel swapped into existing objects; wind-sway shape key | [assets/blender/head.blend](../../assets/blender/head.blend), `add_sculpted_sway` in [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py) |
| One-command workflow | `scene`, `stills`, `still`, `cycles`, `animation` modes; finds Windows Blender from WSL; `HEAD` / `HEAD_PARTS` overrides | [tools/blender/render.sh](../../tools/blender/render.sh) |
| A believable coaster | Closed Catmull–Rom circuit of 15 control points | `buildCoasterTrackPath` in [src/park3d/parkModels.cpp](../../src/park3d/parkModels.cpp) |
| Physics-timed laps | Fixed station/lift speeds, energy conservation with rolling friction after the crest, capped brake run, lap scaled to the 30 s loop | `coasterLeadDistanceAt` in [src/park3d/parkScene.cpp](../../src/park3d/parkScene.cpp) |

---

## 5. A short tour of the new files

| File | In one sentence |
|---|---|
| [src/exportMain.cpp](../../src/exportMain.cpp) | Runs our park for 30 seconds and writes everything Blender needs into `output/blender/`. |
| [tools/blender/buildAstroworld.py](../../tools/blender/buildAstroworld.py) | Runs inside Blender: builds meshes, materials, the sculpted head, lights, sky, camera and render settings, then saves or renders. |
| [tools/blender/astroworldPlayer.py](../../tools/blender/astroworldPlayer.py) | Runs on every frame change: moves, tints and lights everything to match that frame of our export. |
| [tools/blender/render.sh](../../tools/blender/render.sh) | The one command you run from WSL: export, then build or render with Windows Blender. |
| [assets/blender/head.blend](../../assets/blender/head.blend) | The hand-sculpted head, dreads and mouth tunnel. |

---

## 6. Questions you may be asked

**"Isn't Blender doing all the work?"**
> Blender does the *rendering*. Everything about *what* is rendered comes from our engine: every mesh except the sculpted head, every position on every frame, the ride physics, the visitors' walking, and the lighting timeline. Delete our export and Blender has an empty scene.

**"Why not just animate it in Blender?"**
> Then there'd be two versions of the park to keep in sync. With our engine as the single source of truth, the OpenGL app, the ray tracer and the Blender film always show the same park.

**"EEVEE vs Cycles?"**
> EEVEE is Blender's fast rasterizer, conceptually like our OpenGL renderer. Cycles is its path tracer, like our `astroworldRay --mode path`. We rendered the film with EEVEE for speed. `render.sh cycles` renders a single Cycles frame for comparison.

**"How does the coaster know how fast to go?"**
> Conservation of energy. Speed at any point comes from the height it has dropped since the crest, minus a small friction loss along the distance travelled. We step along the track by distance to find where the train is at each moment.

**"What's next?"**
> Animated characters (for example, rigged Mixamo-style walkers) to replace the simple visitors.

**GitHub link:** https://github.com/Karanei-Kimutai/ASTROWORLD/tree/blender

---

## 7. The one-minute version

> "In the latest stage, our C++ engine drives a Blender render. An exporter samples our park 24 times a second for 30 seconds: meshes once, every object's matrix and glow per frame, and the lighting timeline. A Python script rebuilds that in Blender with physically based materials, a physical sky with a real sunset, volumetric searchlights, a hand-sculpted head, motion blur and a camera fly-through. A player script embedded in the file moves everything frame by frame. We also rebuilt the roller coaster as a closed Catmull–Rom circuit whose speed comes from energy conservation, with one lap in exactly 30 seconds, so the film loops seamlessly."
