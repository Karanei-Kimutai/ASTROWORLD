# ASTROWORLD

An animated **ASTROWORLD theme park** built for the Computer Graphics capstone. It recreates the album world: the giant brass head with its mouth open as the park entrance, the "Greetings from ASTROWORLD" postcard lettering, the blue-sky carnival of the cover, and the starfield and teal coaster of the night artwork. Every course algorithm is implemented by hand in C++.

![ASTROWORLD by day - Checkpoint 1](docs/screenshots/astroworldDay.png)
![ASTROWORLD by night - Checkpoint 1](docs/screenshots/astroworldNight.png)

---

## Contents

1. [Project roadmap and branches](#project-roadmap-and-branches)
2. [Building and running](#building-and-running)
3. [Project structure](#project-structure)
4. [Course concepts → where they live in the code](#course-concepts--where-they-live-in-the-code)
5. [How the software renderer maps to the rendering pipeline](#how-the-software-renderer-maps-to-the-rendering-pipeline)
6. [Coding conventions](#coding-conventions)
7. [Checkpoint log](#checkpoint-log)

---

## Project roadmap and branches

Each checkpoint has its own branch, created from the previous one, so every stage can be checked out and run exactly as it was submitted.

| Branch | Stage | Focus |
|---|---|---|
| `checkpoint-1` | **CP1 – Technical foundation** | Course algorithms in pure C++, used to build simplified park components. Output: PPM images. |
| `checkpoint-1-window` | CP1 (windowed variant) | The same renderer shown live in a window through a basic windowing library. |
| `checkpoint-2` | **CP2 – Transformation & animation** | 2D/3D transforms with homogeneous matrices; rides animate; frames combined into a video. |
| `checkpoint-3` | **CP3 – OpenGL integration** | Components ported to OpenGL (GLFW + GLAD); real-time camera and perspective projection. |
| `prototype` | Prototype | All components combined into one interactive park. |
| `final` | Final project | Phong lighting, textures, anti-aliasing, ray-traced still, polish. |

---

## Building and running

**Requirements:** a C++17 compiler (g++ 9+ / clang 10+ / MSVC 2019+) and, optionally, CMake 3.16+. CP1 uses **no external libraries**. `ffmpeg` is only used to convert screenshots for this README.

### With CMake

```bash
cmake -S . -B build
cmake --build build
./build/astroworld
```

### Without CMake (single command)

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Isrc $(find src -name '*.cpp') -o build/astroworld
```

### Output

Running `./build/astroworld [outputDirectory]` (default `output/`) writes:

| File | What it shows |
|---|---|
| `astroworldDay.ppm` | The park as on the album cover (24-bit colour) |
| `astroworldNight.ppm` | The same park at night: starfield, searchlights, brighter portal |
| `astroworldDay8Bit.ppm` | Day scene reduced to 256 colours (3-3-2 RGB) |
| `astroworldDay8BitDither.ppm` | 256 colours with 4×4 ordered (Bayer) dithering |
| `algorithmShowcase.ppm` | Every CP1 algorithm shown on its own, labelled |
| `colorShowcase.ppm` | HSV, tints/shades/tones, RGB vs CMYK, colour depth, palette |

PPM files open in GIMP, most Linux image viewers, and VS Code with a PPM extension. To regenerate the PNGs in `docs/screenshots/`:

```bash
./tools/exportScreenshots.sh
```

---

## Project structure

```
src/
├── main.cpp                    CP1 entry point: renders and saves all images
├── core/
│   ├── vec2.h                  2D vector maths, pi, rounding helper
│   ├── color.h/.cpp            RGB/HSV/CMYK conversions, tints/shades/tones, 8-bit reduction, palette
│   ├── frameBuffer.h/.cpp      Pixel memory, spans, alpha blending, dithering, PPM output
│   ├── coverageMask.h/.cpp     Off-screen stencil mask + supersampled (anti-aliased) compositing
│   └── simpleRandom.h          Deterministic xorshift random generator
├── raster/
│   ├── rasterTypes.h           Plot/span callbacks: rasterization kept separate from shading
│   ├── lines.h/.cpp            DDA and Bresenham lines
│   ├── circles.h/.cpp          Midpoint circle (8-way) and midpoint ellipse (4-way)
│   ├── clipping.h/.cpp         Cohen–Sutherland line clipping
│   └── strokes.h/.cpp          Thick and tapered strokes (scan-line quads + circle caps)
├── curves/
│   ├── bezier.h/.cpp           Cubic Bézier (Bernstein) and any-degree de Casteljau
│   └── spline.h/.cpp           Catmull–Rom splines (open and closed)
├── fill/
│   ├── scanLineFill.h/.cpp     Edge Table / Active Edge Table polygon fill (even-odd)
│   └── boundaryFill.h/.cpp     4- and 8-connected boundary fill (explicit stack)
├── text/
│   ├── bitmapFont.h/.cpp       5×7 raster font for labels
│   └── astroWordmark.h/.cpp    The ASTROWORLD logo: flared vector letters, globe O, ring O
├── scene/
│   ├── astroworldScene.h/.cpp  Composes everything into the park (painter's algorithm)
│   ├── backdrop.h/.cpp         Day/night sky, clouds, stars, searchlights, skyline, fence, plaza
│   ├── astroHead.h/.cpp        The brass head: dreads, closed eyes, nose, glowing mouth portal
│   ├── brassShading.h          Matte-brass surface shader (faux lighting)
│   ├── carnivalProps.h/.cpp    Rocket statue, big-top tent, park sign, bunting
│   ├── ferrisWheel.h/.cpp      Ferris wheel
│   ├── rollerCoaster.h/.cpp    Roller coaster track, supports and train
│   ├── dropTower.h/.cpp        Drop tower
│   ├── carousel.h/.cpp         Carousel
│   └── foreground.h/.cpp       Crowd walking into the mouth, balloon vendor
└── showcase/
    ├── algorithmShowcase.h/.cpp  Labelled 4×2 grid of algorithm demos
    └── colorShowcase.h/.cpp      Colour-theory demos
tools/exportScreenshots.sh      PPM → PNG for documentation
docs/screenshots/               Checkpoint screenshots
docs/references/                Reference artwork the scene is matched against (local only, not committed)
```

---

## Course concepts → where they live in the code

### Hardware and core fundamentals
| Concept | Implementation |
|---|---|
| **Raster vs. vector** | The bitmap font ([bitmapFont.cpp](src/text/bitmapFont.cpp)) is a *raster* font: it goes blocky when scaled (see the captions). The logo ([astroWordmark.cpp](src/text/astroWordmark.cpp)) is *vector*: its letters are described by curves and polygons, so it stays smooth at any size. |
| **Frame buffer** | [frameBuffer.h](src/core/frameBuffer.h): a row-major pixel array, the CPU-side equivalent of the VRAM frame buffer. "Display" means writing it to a PPM file. |
| **CPU vs. GPU** | CP1 rasterizes every pixel sequentially on the CPU. CP3 moves the same work to the GPU, where fragments are processed in parallel. |

### Colour theory and models
| Concept | Implementation |
|---|---|
| RGB / HSV / CMYK | `hsvToRgb`, `rgbToHsv`, `rgbToCmyk`, `cmykToRgb` in [color.cpp](src/core/color.cpp). Demonstrated in the colour showcase. |
| Hue, saturation, value; tints, shades, tones | `makeTint`, `makeShade`, `makeTone`. The Ferris wheel gondolas space their hues evenly around the HSV wheel. |
| Colour harmony | `astroPalette`, sampled from the album artwork: complementary blue sky against brass/gold for the day, navy against neon for the night, and a full-spectrum rainbow for the logo letters. |
| 24-bit → 8-bit reduction | `reduceTo8Bit` / `expandFrom8Bit` (3-3-2 RGB) plus Bayer dithering in `FrameBuffer::reducedTo8Bit`. |
| HSV interpolation | `lerpColorHsv`: the sky gradient goes the short way round the hue circle; `nightAmount` blends the day and night palettes. |

### Geometric primitives and drawing algorithms
| Algorithm | Where it is used in the park |
|---|---|
| **DDA line** ([lines.cpp](src/raster/lines.cpp)) | Searchlight rays, concrete joints in the plaza; the clipping demo |
| **Bresenham line** (integer decision parameter) | Ferris wheel spokes, the coaster's lattice truss, fence pickets and rails, star flares, bunting strings |
| **Midpoint circle** (8-way symmetry) ([circles.cpp](src/raster/circles.cpp)) | Ferris wheel rims and hub, light bulbs, the globe, round stroke caps |
| **Midpoint ellipse** (4-way symmetry, two regions) | Carousel, balloons, ears, lips, clouds, glows and soft shadows |
| **Line clipping** (Cohen–Sutherland) ([clipping.cpp](src/raster/clipping.cpp)) | Searchlight rays are 2200 px long and are clipped to the screen; plaza joints are clipped to the ground |

### Curves
| Concept | Where it is used |
|---|---|
| Parametric curves | Every curve is evaluated as a function of `t ∈ [0, 1]` |
| **Bézier curves** ([bezier.cpp](src/curves/bezier.cpp)) | Eyelids, nose creases, the mouth arch, the bowls of R and D, the A's outline, the rocket's nose and fins, the tent roof, sagging bunting, balloon strings |
| **Splines** (Catmull–Rom, [spline.cpp](src/curves/spline.cpp)) | Coaster track, head silhouette, the dreads, the S's spine, teardrop counters, the continents on the globe |

### Polygons and area filling
| Concept | Where it is used |
|---|---|
| **Scan-line fill** (ET + AET, even-odd) ([scanLineFill.cpp](src/fill/scanLineFill.cpp)) | Sky, ground, head, logo letters, skyline (highly concave), tent, rocket, pennants, gondolas, cars |
| **Boundary fill** (4- vs 8-connected) ([boundaryFill.cpp](src/fill/boundaryFill.cpp)) | Balloons. The showcase shows why 8-connected fill leaks through diagonal Bresenham outlines |
| Convex / concave / complex polygons | The showcase fills a concave star and a self-intersecting pentagram (hollow centre under the even-odd rule) |

### Rendering techniques
| Concept | Implementation |
|---|---|
| Painter's algorithm | Scene drawn far to near; carousel horses and crowd members are sorted by depth; outer dreads go behind the head, front dreads in front |
| Fragment shading | `PixelShader` callbacks colour each covered pixel (sky gradients, tent and canopy stripes, brass lighting, the portal glow) |
| Fragment discard | Rows below the ground line are discarded when filling the head and lips; continents are discarded outside the globe |
| Alpha blending | `FrameBuffer::blendPixel` and `blendSoftEllipse` (alpha falls off per pixel): clouds, glows, beams, sculpted shadows |
| Stencil mask | [coverageMask.h](src/core/coverageMask.h): letter pieces are *added* and counters *erased* off-screen, then composited |
| **Anti-aliasing** (SSAA) | The logo masks are rendered at 3×3 sub-samples per pixel and box-filtered down, so letter edges are smooth |
| Lighting (preview) | [brassShading.h](src/scene/brassShading.h) rebuilds an ellipsoid normal per pixel and applies diffuse, specular and rim terms. The dreads fake a lit cylinder with three offset tapered strokes. This is a preview of the full Phong model planned for the final phase. |

---

## How the software renderer maps to the rendering pipeline

| Pipeline stage | In this project (CP1) |
|---|---|
| 1. Application | `renderAstroworldScene` decides what to draw and in what order |
| 2. Vertex processing | Head-unit → screen mapping (`AstroHead::toScreen`), glyph shear, curve evaluation. Replaced by 4×4 matrices in CP2. |
| 3. Primitive assembly | Points become polylines, polygons and ellipses |
| 4. Clipping | Cohen–Sutherland for lines; span clipping and bounds checks in `FrameBuffer` |
| 5. Rasterization | DDA, Bresenham, midpoint circle/ellipse and scan-line fill emit pixels or spans through `PlotCallback` / `SpanCallback` |
| 6. Fragment processing | `PixelShader` functions choose each pixel's colour |
| 7. Output operations | Overwrite, alpha-blend, or stencil-mask into the buffer (SSAA compositing of the logo) |
| 8. Frame buffer | `FrameBuffer` |
| 9. Display | `savePpm` writes the image file (a live window in `checkpoint-1-window`) |

---

## Coding conventions

- **camelCase** for variables, functions, members and file names (`fillPolygonScanLine`, `astroHead.cpp`). Types use UpperCamelCase (`FrameBuffer`, `RollerCoaster`).
- **Descriptive names**, no single letters, except where the maths itself uses them (`t` for curve parameters, `x`/`y`/`u`/`v` for coordinates).
- **Comprehensive inline documentation**: every file starts with a header comment, every public function has a doc comment, and each algorithm explains its maths (decision parameters, symmetry, edge tables) where the code runs it.
- Screen space: origin at the top-left, x to the right, **y down**.
- Deterministic output: all randomness uses a seeded `SimpleRandom`, so the same frame is drawn every time.

---

## Checkpoint log

### Checkpoint 1 – Technical foundation (pure C++)

**What are we building?**
An animated ASTROWORLD theme park that recreates the album world. The giant brass head (eyes closed, dreads arching over the crown) is both the centrepiece and the entrance: its open mouth on the ground glows, and visitors walk in. Around it are a Ferris wheel, a teal lattice roller coaster, a drop tower, a carousel, a rocket statue, a striped big top, bunting and a black palisade fence. The scene has a day mood (the cover) and a night mood (the starfield artwork), with the "Greetings from ASTROWORLD / Wish you were here" postcard logo above.

**What graphics concepts are used?**
Frame buffer and raster vs. vector graphics, RGB/HSV/CMYK colour models, tints/shades/tones, 24 → 8-bit colour reduction, DDA, Bresenham, midpoint circle and ellipse, Cohen–Sutherland clipping, Bézier curves, Catmull–Rom splines, scan-line and boundary fill, painter's-algorithm layering, stencil masking and supersampling anti-aliasing. Details are in the [table above](#course-concepts--where-they-live-in-the-code).

**What has been completed?**
- A software rasterizer written from scratch: every algorithm above, with no graphics libraries
- Each park component as a self-contained, parameterised struct: `FerrisWheel`, `RollerCoaster`, `DropTower`, `Carousel`, `AstroHead`, plus props, backdrop and foreground
- The ASTROWORLD logo matched to the reference artwork: flared waisted letters with teardrop counters, the Earth-smiley O, the concentric-ring O, and flat per-letter colours, drawn with anti-aliasing
- Day and night renders controlled by one `nightAmount` value
- An algorithm showcase and a colour showcase for the checkpoint demo
- 24-bit, 8-bit and dithered 8-bit renders of the day scene

| Algorithm showcase | Colour showcase |
|---|---|
| ![Algorithms](docs/screenshots/algorithmShowcase.png) | ![Colour](docs/screenshots/colorShowcase.png) |

| 8-bit (3-3-2) | 8-bit dithered |
|---|---|
| ![8-bit](docs/screenshots/astroworldDay8Bit.png) | ![8-bit dither](docs/screenshots/astroworldDay8BitDither.png) |

**What are we doing next? (CP2)**
- A `Matrix3` / `Matrix4` class for homogeneous-coordinate translation, rotation and scaling
- Rotate the Ferris wheel and carousel, drop the tower seats, run the coaster train along the spline by arc length, bob the horses, sway the dreads and bunting, walk the crowd into the mouth, sweep the searchlights, and run a day → night cycle with `nightAmount`
- Render a numbered frame sequence and combine it into an MP4 with `ffmpeg`
- The components already expose `rotationAngle`, `seatY`, `trainDistance` and `nightAmount`, so CP2 only has to drive them over time
