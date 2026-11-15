# Presentation guide – Checkpoint 1: Technical foundation

| | |
|---|---|
| **Branch** | [`checkpoint-1`](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-1) (exact submitted state: tag `submitted/checkpoint-1`) |
| **Course stage** | *CP1 — Technical Foundation* |
| **Presentation questions it answers** | Checkpoint 1: *What are we building? What graphics concepts will we use? What have we completed? What are we doing next?* |
| **Speaking time** | about 6–8 minutes, plus questions |
| **What you show** | Two images of the park (day and night), the algorithm showcase, the colour showcase, the 8-bit versions, and a few source files |

---

## 1. Before you present (5 minutes of setup)

1. Switch to the branch and **build from scratch**. `build/` is never touched by git, so it may still hold programs from a later stage:
   ```bash
   git checkout checkpoint-1
   rm -rf build && cmake -B build && cmake --build build -j
   ./build/astroworld
   ```
   The program prints a line for every image it writes into `output/`.
2. Open these images, each in its own tab, in the order you'll show them. The PNG copies are in `docs/screenshots/`; the fresh PPMs are in `output/`.
   - `astroworldDay.png` and `astroworldNight.png`
   - `algorithmShowcase.png`
   - `colorShowcase.png`
   - `astroworldDay8Bit.png` and `astroworldDay8BitDither.png`
3. Open these files in your editor, ready to scroll to. Each one is a short code moment in the talk:
   - [src/raster/lines.cpp](../../src/raster/lines.cpp)
   - [src/raster/circles.cpp](../../src/raster/circles.cpp)
   - [src/fill/scanLineFill.cpp](../../src/fill/scanLineFill.cpp)
   - [src/scene/astroworldScene.cpp](../../src/scene/astroworldScene.cpp)

---

## 2. What the checkpoint asked for

The course brief says, for this stage:

> **CP1 — Technical Foundation.** *Implement the graphics techniques covered so far in Pure C++ and use them to create simplified graphical components relevant to your project.*

And the Checkpoint 1 presentation asks four questions:

1. What are we building?
2. What graphics concepts will we use?
3. What have we completed?
4. What are we doing next?

In plain words: **write the drawing algorithms ourselves, in plain C++ with no graphics library, and use them to draw the first pieces of our theme park.** That is exactly what this branch does. Every pixel in every image is placed by code we wrote.

---

## 3. The script

> Lines in quote blocks are what you **say**. Lines starting with **Show** or **Do** are what you do on screen.

### 3.1 Opening: the idea (about 1 minute)

**Show** `astroworldDay.png`, full screen.

> "This is ASTROWORLD: a theme park built from Travis Scott's 2018 album art. On the cover there's a giant golden head with its mouth open, and the mouth is the entrance to the park. Around it are a Ferris wheel, a roller coaster, a drop tower, a carousel and a big blue sky. On the night version of the art, the sky turns into a starfield with searchlights.
>
> Our capstone recreates that world as an animation. This first checkpoint is the foundation: before we can animate anything, we need to be able to *draw* it. And we draw it with algorithms we wrote ourselves."

**Show** `astroworldNight.png`.

> "Here's the same park at night. It isn't a second drawing. It's the same code with one setting changed: a number called `nightAmount`, which goes from 0 for day to 1 for night. The sky colours, the stars, the searchlights and the glow in the mouth all respond to that one number. That matters later: when we animate, we just move `nightAmount` over time."

### 3.2 What are we building? (30 seconds)

> "So, to answer the first question: we're building an **animated ASTROWORLD theme park**. The giant head is both the centrepiece and the gate. Visitors walk into the glowing mouth, and the rides around it move. The day mood matches the album cover, and the night mood matches the starfield artwork. Above it all is the 'Greetings from ASTROWORLD' postcard lettering."

### 3.3 What graphics concepts are we using? (about 3 minutes, the heart of the talk)

**Show** `algorithmShowcase.png`.

> "This is our algorithm showcase. Every panel is one algorithm from the course, drawn on its own and labelled, so you can see each one working without the rest of the park around it."

Walk the panels left to right, top row then bottom row. For each, one sentence of *what it is* and one of *where it is in the park*:

1. **DDA line.**
   > "DDA, the Digital Differential Analyzer, steps along the line one pixel at a time and adds a fixed fraction to the other coordinate each step. It uses floating-point maths. In the park it draws the long searchlight rays and the joints in the plaza paving."

   *File:* [src/raster/lines.cpp](../../src/raster/lines.cpp), `rasterizeLineDda`.
2. **Bresenham line.**
   > "Bresenham draws the same lines using only whole numbers. It keeps a 'decision parameter' that says whether the next pixel steps up or stays level, so it never needs decimals. That's why it was the classic choice for hardware. We use it for the Ferris wheel spokes, the fence and the coaster's lattice."

   *File:* `rasterizeLineBresenham` in the same file.
3. **Midpoint circle and ellipse.**
   > "The midpoint algorithm works out one eighth of a circle and mirrors it into the other seven, which is 8-way symmetry. An ellipse is only symmetric four ways, and it has two regions where the slope changes, so the algorithm switches halfway. Circles give us the Ferris wheel rim and the light bulbs. Ellipses give us the carousel, the balloons, the clouds and the soft glows."

   *File:* [src/raster/circles.cpp](../../src/raster/circles.cpp): `rasterizeCircleOutline`, `rasterizeEllipseOutline` and their filled versions.
4. **Bézier curve (de Casteljau).**
   > "A Bézier curve is shaped by control points that pull on it without being on it. This panel shows the de Casteljau construction: you repeatedly take the point partway along each line, and the last point standing is on the curve. Bézier curves draw the eyelids, the mouth arch, the curves of the letters, the rocket's nose and the sagging bunting."

   *File:* [src/curves/bezier.cpp](../../src/curves/bezier.cpp): `evaluateCubicBezier`, `evaluateBezierDeCasteljau`, `sampleBezierCurve`.
5. **Catmull–Rom spline.**
   > "A spline is a chain of small curves joined smoothly. Unlike Bézier, a Catmull–Rom spline passes *through* every point you give it, and moving one point only changes the nearby part. That's 'local control'. It's perfect for the roller-coaster track, the outline of the head and the dreads."

   *File:* [src/curves/spline.cpp](../../src/curves/spline.cpp): `evaluateCatmullRom`, `sampleCatmullRomSpline`.
6. **Scan-line fill.**
   > "To fill a polygon, we go down the image row by row. For each row we find where it crosses the polygon's edges and fill between the pairs of crossings. We keep an Edge Table of all edges and an Active Edge Table of the edges crossing the current row. On the left is a concave star. On the right is a self-intersecting pentagram: its centre stays empty because of the even-odd rule. Almost every solid shape in the park is filled this way: the sky, the ground, the head, the letters, the tent."

   *File:* [src/fill/scanLineFill.cpp](../../src/fill/scanLineFill.cpp): `fillPolygonScanLine`.
7. **Boundary fill, 4- versus 8-connected.**
   > "Boundary fill works like a paint bucket: start inside a shape and spread until you hit the border colour. On the left, the 4-connected fill only moves up, down, left and right, so it stays inside. On the right, the 8-connected fill also moves diagonally, and it leaks out through the gaps in a diagonal Bresenham outline. That's the classic trap this panel shows. In the park, boundary fill colours the balloons."

   *File:* [src/fill/boundaryFill.cpp](../../src/fill/boundaryFill.cpp): `boundaryFill4`, `boundaryFill8`.
8. **Cohen–Sutherland line clipping.**
   > "Clipping removes the parts of a line that fall outside a window. Each end gets a 4-bit code saying whether it's left of, right of, above or below the window. If both ends are outside on the same side, we drop the line at once. Otherwise we cut it at the edge. The dark red parts are what gets thrown away. In the park, the searchlight rays are over two thousand pixels long and get clipped to the screen."

   *File:* [src/raster/clipping.cpp](../../src/raster/clipping.cpp): `computeOutCode`, `clipLineCohenSutherland`.

**Do:** open [src/raster/lines.cpp](../../src/raster/lines.cpp) for 20 seconds.

> "And this is real code, not a library call. Here's Bresenham: the decision parameter, the integer steps. Every algorithm is written out like this, with comments explaining each step."

**Show** `colorShowcase.png`.

> "Colour theory gets the same treatment. Top row, left to right:
> - the HSV colour wheel
> - tints, shades and tones: mixing a colour with white, with black, or with grey
> - RGB, the additive model screens use
> - CMYK, the subtractive model printers use
>
> Bottom left is colour-depth reduction. Bottom right is our ASTROWORLD palette, picked from the album art."

*File:* [src/core/color.cpp](../../src/core/color.cpp): `hsvToRgb`, `rgbToHsv`, `rgbToCmyk`, `cmykToRgb`, `makeTint`, `makeShade`, `makeTone`, `reduceTo8Bit`, and the `astroPalette`.

**Show** `astroworldDay8Bit.png`, then `astroworldDay8BitDither.png`.

> "This is the park squeezed from 24-bit colour, about 16 million colours, down to 8-bit, which is only 256. We keep 3 bits for red, 3 for green and 2 for blue. Blue gets fewer because our eyes are least sensitive to it. Notice the bands in the sky. The second image adds ordered dithering, a Bayer pattern of tiny offsets that trades the bands for fine noise. Your eye blends that noise back into a smooth gradient."

*File:* [src/core/frameBuffer.cpp](../../src/core/frameBuffer.cpp), `FrameBuffer::reducedTo8Bit`.

**Then** the hardware ideas, briefly:

> "A few hardware concepts are in here too:
> - **Frame buffer:** ours is the class `FrameBuffer`, a block of memory holding one colour per pixel. It's our version of the frame buffer in a graphics card's video memory. 'Display' here means saving it as a PPM image file.
> - **Raster versus vector:** the small captions use a bitmap font, which is raster: it's stored as pixels, so it gets blocky when you scale it up. The big ASTROWORLD logo is vector: the letters are made of curves and polygons, so it stays smooth at any size.
> - **Anti-aliasing:** we draw the logo at three times the resolution in each direction and average it down, so the letter edges are smooth instead of jagged. That's supersampling."

*Files:*
- [src/core/frameBuffer.h](../../src/core/frameBuffer.h)
- [src/text/bitmapFont.cpp](../../src/text/bitmapFont.cpp)
- [src/text/astroWordmark.cpp](../../src/text/astroWordmark.cpp)
- [src/core/coverageMask.cpp](../../src/core/coverageMask.cpp): supersampling and the stencil mask

### 3.4 What have we completed? (about 1 minute)

**Show** `astroworldDay.png` again, and point as you talk.

> "With those algorithms we built every component of the park as its own self-contained piece of code, each with settings you can change:
> - **The head:** filled with scan-line fill, outlined with splines, eyelids and lips from Bézier curves, and lit with a brass shader that fakes light per pixel.
> - **The Ferris wheel:** a midpoint-circle rim, Bresenham spokes and gondolas coloured evenly around the HSV wheel.
> - **The roller coaster:** a Catmull–Rom track with a Bresenham lattice.
> - **The drop tower and the carousel.** The carousel is built from ellipses.
> - **Props:** the rocket, the striped big top, the bunting and the park sign.
> - **The backdrop:** a sky gradient, clouds, stars, searchlights, the skyline and the fence.
> - **The crowd and the balloon vendor** in the foreground.
> - **The logo,** matched to the album lettering: flared letters, a globe for the first O and rings for the second.
>
> The scene puts them all together back to front, which is the painter's algorithm. Things drawn later cover things drawn earlier, so we draw the far sky first and the crowd last."

*Files:*
- [src/scene/astroworldScene.cpp](../../src/scene/astroworldScene.cpp), `renderAstroworldScene`: puts the park together, back to front
- [src/scene/astroHead.cpp](../../src/scene/astroHead.cpp) and [src/scene/brassShading.h](../../src/scene/brassShading.h)
- [src/scene/ferrisWheel.cpp](../../src/scene/ferrisWheel.cpp)
- [src/scene/rollerCoaster.cpp](../../src/scene/rollerCoaster.cpp)
- [src/scene/dropTower.cpp](../../src/scene/dropTower.cpp) and [src/scene/carousel.cpp](../../src/scene/carousel.cpp)
- [src/scene/carnivalProps.cpp](../../src/scene/carnivalProps.cpp)
- [src/scene/backdrop.cpp](../../src/scene/backdrop.cpp) and [src/scene/foreground.cpp](../../src/scene/foreground.cpp)

### 3.5 What are we doing next? (30 seconds)

> "Next is Checkpoint 2: transformation and animation. We'll write matrix classes using homogeneous coordinates, so translating, rotating and scaling all become matrix multiplications. Then we'll animate everything: the Ferris wheel turns, the coaster runs, the drop tower drops, visitors walk into the mouth, and the sky moves from day to night. We prepared for this. Each component already has the right setting exposed, like the wheel's `rotationAngle` and the coaster's `trainDistance`, so animation just means changing those numbers over time."

### 3.6 Close (10 seconds)

> "So Checkpoint 1 is a complete software renderer written from scratch, every course algorithm visible on its own and inside the park, and a foundation built to be animated. Thank you. Happy to take questions."

---

## 4. Requirement → what we did → where

| What was asked | What we did | Where |
|---|---|---|
| Implement the techniques covered so far **in pure C++** | A software rasterizer with no graphics libraries. Only the C++ standard library is used. | [CMakeLists.txt](../../CMakeLists.txt) (no dependencies), [src/core/frameBuffer.cpp](../../src/core/frameBuffer.cpp) |
| DDA and Bresenham lines | Both algorithms, kept separate from colouring through plot callbacks | [src/raster/lines.cpp](../../src/raster/lines.cpp), [src/raster/rasterTypes.h](../../src/raster/rasterTypes.h) |
| Midpoint circle and ellipse (8-way / 4-way symmetry) | Outline and filled versions, plus soft blended ellipses | [src/raster/circles.cpp](../../src/raster/circles.cpp) |
| Line clipping | Cohen–Sutherland with outcodes | [src/raster/clipping.cpp](../../src/raster/clipping.cpp) |
| Bézier curves, control points | Cubic Bernstein form and the any-degree de Casteljau construction | [src/curves/bezier.cpp](../../src/curves/bezier.cpp) |
| Splines with local control | Catmull–Rom, open and closed | [src/curves/spline.cpp](../../src/curves/spline.cpp) |
| Polygon types; scan-line and boundary fill | Edge Table / Active Edge Table scan-line fill (even-odd); 4- and 8-connected boundary fill | [src/fill/scanLineFill.cpp](../../src/fill/scanLineFill.cpp), [src/fill/boundaryFill.cpp](../../src/fill/boundaryFill.cpp) |
| RGB, HSV, CMYK; tints, shades, tones; harmony | Conversions, tint/shade/tone and a palette taken from the album art | [src/core/color.cpp](../../src/core/color.cpp), [src/showcase/colorShowcase.cpp](../../src/showcase/colorShowcase.cpp) |
| 24-bit → 8-bit reduction | 3-3-2 reduction, with and without Bayer dithering | `FrameBuffer::reducedTo8Bit` in [src/core/frameBuffer.cpp](../../src/core/frameBuffer.cpp) |
| Frame buffer; raster vs vector | Our `FrameBuffer` class; raster bitmap font vs vector logo | [src/core/frameBuffer.h](../../src/core/frameBuffer.h), [src/text/bitmapFont.cpp](../../src/text/bitmapFont.cpp), [src/text/astroWordmark.cpp](../../src/text/astroWordmark.cpp) |
| Anti-aliasing (preview) | 3×3 supersampling of the logo through a coverage mask | [src/core/coverageMask.cpp](../../src/core/coverageMask.cpp) |
| *Simplified graphical components relevant to the project* | Head, Ferris wheel, coaster, drop tower, carousel, props, backdrop, crowd and logo | [src/scene/](../../src/scene/) |
| Show each technique clearly | Algorithm and colour showcases | [src/showcase/algorithmShowcase.cpp](../../src/showcase/algorithmShowcase.cpp), [src/showcase/colorShowcase.cpp](../../src/showcase/colorShowcase.cpp) |
| Output | PPM images: day, night, 8-bit, dithered and both showcases | [src/main.cpp](../../src/main.cpp) |

---

## 5. A tour of the files

```text
src/main.cpp                  Entry point: renders every image and saves it as PPM
src/core/                     The basics: colour, the frame buffer, masks, random numbers, 2D vectors
src/raster/                   Turning shapes into pixels: lines, circles, ellipses, clipping, thick strokes
src/curves/                   Bézier curves and Catmull–Rom splines
src/fill/                     Scan-line and boundary fill
src/text/                     The bitmap font (raster) and the ASTROWORLD logo (vector)
src/scene/                    Each park component, and astroworldScene.cpp that puts them together
src/showcase/                 The algorithm and colour showcase images
tools/exportScreenshots.sh    Converts the PPM images to PNG for the README
```

**One design idea worth mentioning:** the rasterizers don't colour pixels themselves. They call back with "this pixel is covered" (`PlotCallback`) or "this row segment is covered" (`SpanCallback`), and a separate *shader* chooses the colour. That's the same split a GPU makes between **rasterization** and **fragment processing**. See [src/raster/rasterTypes.h](../../src/raster/rasterTypes.h).

---

## 6. Questions you may be asked

**"Why write the algorithms yourselves instead of using a library?"**
> The brief asks for pure C++ at this stage, and writing them is how you understand them. For example, Bresenham only makes sense once you've written the decision parameter yourself.

**"What's the difference between DDA and Bresenham?"**
> DDA uses floating-point steps and rounds each pixel. Bresenham uses only integer additions and a decision parameter. It's faster and has no rounding drift, which is why hardware used it.

**"Why does the 8-connected fill leak?"**
> A Bresenham diagonal line only touches pixels at their corners. An 8-connected fill can step diagonally between two corner-touching pixels, straight through the line. A 4-connected fill can't.

**"Why does the pentagram have a hole?"**
> Under the even-odd rule, a point is inside if a ray from it crosses the edges an odd number of times. The centre of a pentagram is crossed an even number of times, so it counts as outside.

**"What does 'pure C++' cover? Is anything external used?"**
> Only the C++ standard library. The image format, PPM, is plain text plus pixel bytes that we write ourselves. `ffmpeg` is only used outside the program, to convert screenshots to PNG for the README.

**"How do you get the right order when shapes overlap?"**
> The painter's algorithm: we draw from back to front, so nearer things cover farther ones. Real depth testing comes later, with OpenGL.

**"Where's the GitHub link?"**
> https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-1

---

## 7. The one-minute version

> "We're building an animated ASTROWORLD theme park from the album art. For Checkpoint 1, we wrote a software renderer in pure C++ with no graphics libraries: DDA and Bresenham lines, midpoint circles and ellipses, Cohen–Sutherland clipping, Bézier curves, Catmull–Rom splines, scan-line and boundary fill, plus RGB, HSV and CMYK colour, tints, shades and tones, and 8-bit reduction with dithering. We used them to build every park component: the head, the rides, the props and the logo. The park renders by day and by night from one setting. The showcases show each algorithm on its own. Next, we add matrices and animate it all."
