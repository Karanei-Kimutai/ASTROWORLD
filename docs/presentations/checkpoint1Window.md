# Presentation guide – Checkpoint 1, windowed variant

| | |
|---|---|
| **Branch** | [`checkpoint-1-window`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-1-window) (exact submitted state: tag `submitted/checkpoint-1-window`) |
| **Course stage** | *CP1 — Technical Foundation*, shown live instead of as image files |
| **Presentation questions it answers** | The same Checkpoint 1 questions. This variant is the live demo version of them. |
| **Speaking time** | 2–3 minutes as an add-on to the [Checkpoint 1 guide](checkpoint1.md), or 6–8 minutes on its own using that guide's script |
| **What you show** | The park and the showcases live in a window, switched with the keyboard |

---

## 1. Before you present

```bash
git checkout checkpoint-1-window
rm -rf build && cmake -B build && cmake --build build -j
./build/astroworldWindow
```

The window needs SDL2 (`sudo apt install libsdl2-dev`). If CMake can't find SDL2, it skips this program and builds the image-file version only, and a plain `./build/astroworld` still works.

**Practise the keys once before the session:**

| Key | What it shows |
|---|---|
| `1` / `2` | Day / night |
| `←` / `→` | Slide the time of day (night amount −/+ 0.1): dusk is a blend of both moods |
| `3` / `4` | Algorithm showcase / colour showcase |
| `5` / `6` | The current scene in 8-bit / 8-bit dithered |
| `S` | Save what's on screen to `output/window_<view>.ppm` |
| `H` | Show / hide the help overlay |
| `Esc` / `Q` | Quit |

---

## 2. What the checkpoint asked for

> **CP1 — Technical Foundation.** *Implement the graphics techniques covered so far in Pure C++ and use them to create simplified graphical components relevant to your project.*

The main `checkpoint-1` branch answers this with image files. When we planned the project, we decided on a second branch that shows the *same* renderer live in a window, using a basic windowing library. That makes the demo interactive, and it shows the last stage of the rendering pipeline, **Display**, actually happening on screen.

---

## 3. The script

### 3.1 Why a window (30 seconds)

**Do:** run `./build/astroworldWindow`. The day scene appears.

> "Checkpoint 1's renderer writes image files. This variant shows the same images live. The important part is *what the window library does and doesn't do*. SDL2 only opens a window and puts our finished picture on the screen. It doesn't draw a single line or circle. Every pixel is still calculated by our own C++ code into our own frame buffer. SDL2 is just the last step: **display**."

### 3.2 Live tour (1–2 minutes)

**Press `2`, then hold `←` / `→` slowly.**

> "Day, night, and everything in between. As I slide the time of day, the sky colours blend through HSV space. The stars fade in, the searchlights switch on and the mouth glows brighter. It's all driven by one number, `nightAmount`. That's the hook we'll use to animate the day-night cycle in Checkpoint 2."

**Press `3`.**

> "The algorithm showcase, live: DDA and Bresenham lines, midpoint circles and ellipses, Bézier with the de Casteljau construction, a Catmull–Rom spline, scan-line fill, boundary fill (4- versus 8-connected), and Cohen–Sutherland clipping."
>
> (Use the [Checkpoint 1 guide, section 3.3](checkpoint1.md#33-what-graphics-concepts-are-we-using-about-3-minutes-the-heart-of-the-talk) if you want to explain each panel.)

**Press `4`.**

> "Colour theory: the HSV wheel, tints, shades and tones, RGB against CMYK, colour depth, and our palette from the album art."

**Press `1`, then `5`, then `6`.**

> "And here's 24-bit colour reduced to 8-bit: 3 bits red, 3 green, 2 blue. You can see bands in the sky. With dithering, the bands turn into fine noise that the eye smooths out."

**Press `S`.**

> "Pressing S saves exactly what's on screen as an image, straight from our frame buffer."

### 3.3 How it works (30 seconds)

> "Each time the window needs a new picture, we render the scene into our `FrameBuffer` on the CPU. We then copy those pixels into an SDL texture and SDL shows it. Our frame buffer plays the part of the video memory frame buffer from the lectures, and this upload is the 'display' stage of the nine-stage pipeline."

---

## 4. Requirement → what we did → where

| What was asked / planned | What we did | Where |
|---|---|---|
| Same CP1 renderer, shown live | A window player that renders our scene and showcases on demand | [src/windowMain.cpp](../../src/windowMain.cpp) |
| A *basic windowing library* only for display | SDL2 opens the window and shows a texture of our pixels; it draws nothing itself | [src/windowMain.cpp](../../src/windowMain.cpp), [CMakeLists.txt](../../CMakeLists.txt) (`astroworldWindow`, optional) |
| Interaction | Keys for day/night, time-of-day slider, showcases, 8-bit modes, saving | [src/windowMain.cpp](../../src/windowMain.cpp) |
| Frame buffer → display | Our `FrameBuffer` exposes its raw pixel data for upload | [src/core/frameBuffer.h](../../src/core/frameBuffer.h) |
| Every CP1 technique | Unchanged from `checkpoint-1` | See the [Checkpoint 1 guide, section 4](checkpoint1.md#4-requirement--what-we-did--where) |

---

## 5. Questions you may be asked

**"Isn't using SDL2 cheating on 'pure C++'?"**
> No. SDL2 is only the display step: it's given a finished image. Every pixel is computed by our code. You can check: the `checkpoint-1` branch makes the identical images with no library at all.

**"What happens if SDL2 isn't installed?"**
> CMake skips the window program and builds the file-based renderer, which needs no libraries.

**"Why is it fast enough to be interactive?"**
> One 1280×720 frame of the scene takes a fraction of a second on the CPU. That's enough for switching views. Checkpoint 2 speeds it up further for smooth animation.

**GitHub link:** https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-1-window
