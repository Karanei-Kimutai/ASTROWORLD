# Presentation guide – Checkpoint 2: Transformation and animation

| | |
|---|---|
| **Branch** | [`checkpoint-2`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-2) (exact submitted state: tag `submitted/checkpoint-2`) |
| **Course stage** | *CP2 — Transformation and Animation* |
| **Presentation questions it answers** | Checkpoint 2: *What algorithms have been implemented? Screenshot of the current scene. GitHub link. What remains?* |
| **Speaking time** | about 7–9 minutes, plus questions |
| **What you show** | The park animating live, the transformation showcase, the exported video, and the matrix and animation code |

---

## 1. Before you present

```bash
git checkout checkpoint-2
rm -rf build && cmake -B build && cmake --build build -j
./build/astroworldWindow          # the live animation (needs SDL2)
./tools/makeVideo.sh              # optional, beforehand: output/astroworldAnimation.mp4 (needs ffmpeg)
```

Have open:
- the window: start it, then pause with `Space` so you begin on a calm frame
- `docs/screenshots/transformShowcase.png`
- the video `output/astroworldAnimation.mp4`, as a backup in case the live demo misbehaves
- in your editor: [src/math/matrix3.cpp](../../src/math/matrix3.cpp), [src/scene/ferrisWheel.cpp](../../src/scene/ferrisWheel.cpp) and [src/scene/astroworldScene.cpp](../../src/scene/astroworldScene.cpp)

**Window keys:**

| Key | Action |
|---|---|
| `Space` | Play / pause |
| `←` / `→` | Seek back / forward 1 s |
| `↑` / `↓` | Play faster / slower (0.25× to 4×) |
| `1` / `2` | Jump to day (t = 0 s) / night (t = 18 s) |
| `3` / `4` / `7` | Algorithm / colour / **transformation** showcase |
| `5` / `6` | 8-bit / dithered 8-bit, still animating |
| `S` | Save the current frame |
| `H` | Help overlay with live frames per second |

---

## 2. What the checkpoint asked for

> **CP2 — Transformation and Animation.** *Build on your CP1 components by implementing transformations and animation in Pure C++.*

And the Checkpoint 2 presentation asks:

1. What algorithms have been implemented?
2. A screenshot of the current scene
3. The GitHub link
4. What remains?

In plain words: **take the park we drew in Checkpoint 1 and make it move, using matrix transformations we wrote ourselves, still in plain C++.**

---

## 3. The script

### 3.1 Opening: the park comes alive (1 minute)

**Do:** press `Space` to play. Let it run 10–15 seconds without talking.

> "In Checkpoint 1 our ASTROWORLD park was a still picture. Now it's alive:
> - the Ferris wheel turns, with its gondolas hanging level
> - the coaster climbs the lift hill and races down
> - the drop tower lifts its riders, pauses, then drops and bounces on the brakes
> - visitors walk into the glowing mouth of the head
> - the head breathes and its dreads sway
> - clouds drift and searchlights sweep
> - day turns to dusk, then to a starry night, and back to dawn
>
> It's a 30-second loop, and every pixel and every movement still comes from our own C++ code."

**Do:** press `2` to jump to night, and let it play for a few seconds.

### 3.2 What did we implement? Transformations (about 3 minutes)

**Show** `transformShowcase.png`, or press `7` in the window.

> "To move things we need transformations: translation, rotation and scaling. We built them as matrices using **homogeneous coordinates**. A 2D point (x, y) is written as (x, y, 1). With that extra 1, *translation* also becomes a matrix multiplication, just like rotation and scaling. So any chain of moves can be multiplied together into **one matrix** and applied in one step."

Walk the eight panels:

1. **Translation:** "Sliding a shape. In the park: drifting clouds and walking visitors."
2. **Rotation about a pivot:** "To spin something around a point that isn't the origin, we move the pivot to the origin, rotate, and move it back: T(p)·R·T(−p). The dreads bend about their roots, legs swing about the hips, and the balloons sway about the vendor's hand."
3. **Scaling about a point:** "Same trick with scaling. The inflatable head 'breathes' by scaling about the middle of its base, so the mouth stays on the ground while it grows and shrinks."
4. **Shear and reflection:** "The other two affine transformations, shown on their own."
5. **Order matters: T·R vs R·T:** "Rotate then move is not the same as move then rotate. Matrix multiplication isn't commutative, and this panel shows both results side by side."
6. **Hierarchy:** "A transformation hierarchy, like the Ferris wheel: the wheel's matrix places the hub and rotates it, each spoke multiplies on its own angle, and each gondola takes only the spoke's *position*, not its rotation. That's why the gondolas stay level."
7. **4×4 rotation (orthographic):** "In 3D we use 4×4 matrices: (x, y, z, 1). Here's a cube rotating, projected with an orthographic projection."
8. **Perspective:** "And the same with a perspective projection. Far things get smaller because we divide by depth, the 'w' coordinate. That's the projection we'll hand to the GPU in Checkpoint 3."

**Do:** open [src/math/matrix3.cpp](../../src/math/matrix3.cpp).

> "Here's the actual matrix code. `rotationAbout` is literally translate × rotate × translate back. Nothing is hidden in a library."

**Do:** open [src/scene/ferrisWheel.cpp](../../src/scene/ferrisWheel.cpp).

> "And here the hierarchy is used for real: the wheel matrix, then each spoke, then the gondolas that only take a position."

> "One more 3D use: the carousel horses. Each horse is placed with a 4×4 rotation around the carousel, then tilted towards the viewer. The horses are sorted by depth, so the near ones are drawn last and cover the far ones."

*Files:* [src/math/matrix3.h](../../src/math/matrix3.h), [src/math/matrix4.h](../../src/math/matrix4.h), [src/math/vec3.h](../../src/math/vec3.h), [src/scene/carousel.cpp](../../src/scene/carousel.cpp), [src/showcase/transformShowcase.cpp](../../src/showcase/transformShowcase.cpp).

### 3.3 What did we implement? Animation (about 2 minutes)

**Do:** go back to the park (`1`) and play.

> "Animation is built in layers:
>
> 1. **Time drives everything.** Each frame is calculated from a single number, the time in seconds. Nothing depends on the previous frame. So the speed doesn't depend on how fast the computer is, and we can jump straight to any moment. Watch: I'll seek with the arrow keys."
>
> **Do:** press `→` a few times, then `↑` to play faster.
>
> 2. **Keyframes with easing.** For the day-night cycle we set keyframes: day until 8 s, dusk to night by 13 s, night until 25 s, then dawn at 30 s. Between keyframes we don't move in a straight line. We *ease*: a smooth S-curve, so the light changes gently. The drop tower uses the same system: an ease-in-out climb, a pause, then a fall with a bounce (the `easeOutBounce` curve).
>
> 3. **Physics.** The coaster isn't keyframed. It's pulled up the lift hill at a steady speed. After the top, its speed comes from **energy conservation**: v = √(v₀² + 2gΔh). It goes fastest at the bottom of the drop and slows on the climbs, like a real coaster. It moves along the spline by *distance*, not by curve parameter, so the speed is right.
>
> 4. **Procedural motion.** Sine waves give us the swinging gondolas, bobbing horses, the breathing head, swaying dreads, fluttering bunting, twinkling stars and sweeping searchlights. And the visitors have a walk cycle: their legs swing in opposite phase about the hip, with a small bounce each step."

*Files:*
- [src/animation/keyframeTrack.h](../../src/animation/keyframeTrack.h): the keyframe tracks
- [src/animation/easing.h](../../src/animation/easing.h): easing curves, `wrapTime`, `oscillate`
- [src/scene/astroworldScene.cpp](../../src/scene/astroworldScene.cpp): `sampleAnimation` turns a time into every ride's setting; the night and drop-tower keyframe tracks live here
- [src/scene/rollerCoaster.cpp](../../src/scene/rollerCoaster.cpp), `trainFractionAtTime`: the coaster physics
- [src/scene/foreground.cpp](../../src/scene/foreground.cpp): walking visitors; [src/scene/astroHead.cpp](../../src/scene/astroHead.cpp): breathing and dreads

### 3.4 Output: window and video (1 minute)

> "There are three ways to see it:
> - **Live:** the window plays it in real time, with play, pause, seek and speed controls.
> - **Frames:** `astroworld --animate` writes numbered frames.
> - **Video:** our `makeVideo.sh` script streams frames straight into ffmpeg and makes an MP4, without writing thousands of image files."

**Show** a few seconds of `output/astroworldAnimation.mp4`, or the GIF in the README.

> "We also made it faster. Caching the sky and ground colours per row, and inlining the colour blending, took a frame from about 100 milliseconds down to about 60."

*Files:* [src/main.cpp](../../src/main.cpp) (`--animate`), [tools/makeVideo.sh](../../tools/makeVideo.sh), [src/windowMain.cpp](../../src/windowMain.cpp).

### 3.5 Screenshot, GitHub, and what remains (1 minute)

**Show** a paused frame at dusk.

> "That's the current scene. The code is on GitHub at github.com/Karanei-Kimutai/ASTROWORLD, on the `checkpoint-2` branch.
>
> What remains: Checkpoint 3 moves to **OpenGL**. We'll rebuild the park as a real 3D world on the GPU, using the same 4×4 matrices we just wrote, in a vertex shader. We'll get a camera you can fly around, a depth buffer instead of drawing back to front, and Phong lighting on real 3D shapes. After that come shading modes, textures, and a ray-traced final image."

### 3.6 Close (10 seconds)

> "So in Checkpoint 2 we built homogeneous 2D and 3D matrices, and used them, together with keyframes, easing and real physics, to bring every Checkpoint 1 component to life, still in pure C++. Thank you."

---

## 4. Requirement → what we did → where

| What was asked | What we did | Where |
|---|---|---|
| **Build on CP1 components** | The same components, now drawn through transforms and driven by time | [src/scene/](../../src/scene/) |
| Translation, rotation, scaling | `Matrix3::translation`, `rotation`, `scaling`, plus shear and reflection | [src/math/matrix3.cpp](../../src/math/matrix3.cpp) |
| Homogeneous coordinates | Points (x, y, 1) and directions (x, y, 0), so translation is a matrix product | [src/math/matrix3.h](../../src/math/matrix3.h) |
| 4×4 matrices for 3D | `Matrix4`: axis rotations, orthographic and perspective projection with the divide by w | [src/math/matrix4.cpp](../../src/math/matrix4.cpp) |
| Composite and pivot transforms | `rotationAbout`, `scalingAbout`; the T·R vs R·T demonstration | [src/math/matrix3.cpp](../../src/math/matrix3.cpp), [src/showcase/transformShowcase.cpp](../../src/showcase/transformShowcase.cpp) |
| Hierarchical transforms | Ferris wheel → spokes → level gondolas | [src/scene/ferrisWheel.cpp](../../src/scene/ferrisWheel.cpp) |
| 3D placement | Carousel horses with 4×4 rotations, depth-sorted | [src/scene/carousel.cpp](../../src/scene/carousel.cpp) |
| **Animation in pure C++** | Time-based frames, keyframes with easing, physics, oscillators, walk cycles | [src/animation/](../../src/animation/), [src/scene/astroworldScene.cpp](../../src/scene/astroworldScene.cpp) |
| Physically based motion | Coaster speed from energy conservation along the spline | [src/scene/rollerCoaster.cpp](../../src/scene/rollerCoaster.cpp) |
| Evidence (screenshot / video) | Live window, frame export, MP4 through ffmpeg | [src/windowMain.cpp](../../src/windowMain.cpp), [src/main.cpp](../../src/main.cpp), [tools/makeVideo.sh](../../tools/makeVideo.sh) |
| *Algorithms implemented* (all so far) | Every CP1 algorithm plus the transformations above | [Checkpoint 1 guide](checkpoint1.md#4-requirement--what-we-did--where) and the table above |

---

## 5. Questions you may be asked

**"Why homogeneous coordinates?"**
> Without them, translation is an addition, not a multiplication, so it can't be combined with rotation and scaling into one matrix. With the extra coordinate, every transform is a matrix, and a whole chain becomes a single matrix.

**"Why do the gondolas stay level?"**
> They take only the *position* from the rotating spoke, not its rotation. In matrix terms, we use the spoke's transformed point, then draw the gondola with its own upright transform.

**"How is the animation frame-rate independent?"**
> Every frame is a pure function of the time in seconds. A slow computer shows fewer frames, but each one is at the correct moment.

**"Why easing instead of linear interpolation?"**
> Real things speed up and slow down. Linear movement starts and stops instantly and looks mechanical. Easing curves (smoothstep, cubic, bounce) give natural motion.

**"How does the coaster know its speed?"**
> The chain lift pulls at a constant speed to the highest point. After that, speed = √(v₀² + 2g × height dropped), which is energy conservation. We add up the time over each small piece of track, then look up where the train is at any given time.

**"Where's the GitHub link?"**
> https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-2

---

## 6. The one-minute version

> "Checkpoint 2 takes our Checkpoint 1 park and animates it in pure C++. We wrote 3×3 and 4×4 matrix classes using homogeneous coordinates: translation, rotation, scaling, shear, reflection, pivot transforms, hierarchies, and orthographic and perspective projection. The animation is time-based, with keyframes and easing for the day-night cycle and the drop tower, energy conservation for the coaster, and sine-wave motion for everything that sways. You can watch it live in a window with full playback controls, or export it as a video through ffmpeg. Next, we take the same matrices to the GPU with OpenGL."
