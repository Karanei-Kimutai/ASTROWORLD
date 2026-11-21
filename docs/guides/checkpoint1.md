# ASTROWORLD: Checkpoint 1, technical foundation

## Summary

Checkpoint 1 laid the technical foundation of ASTROWORLD, a theme park based on the album art of Travis Scott's *Astroworld*, drawn entirely by a software renderer we wrote from scratch in C++ with no graphics library. Every pixel is written into our own frame buffer and saved as an image. We implemented the course's raster algorithms and used them to build the park: DDA and Bresenham lines, midpoint circles and ellipses, Cohen–Sutherland clipping, Bézier curves and Catmull–Rom splines, and scan-line and boundary fills. We also covered colour, with the RGB, HSV and CMYK models, tints, shades and tones, an HSV day-to-night sky blend, and 8-bit colour reduction with ordered dithering. The logo contrasts raster and vector graphics and uses supersampling anti-aliasing. Built as separate, adjustable components with showcase images for every technique, Checkpoint 1 produced a complete still image of the park and the building blocks that every later stage reused.

## In detail

Checkpoint 1 is where ASTROWORLD began. The task was to take the graphics techniques covered in the course so far, implement them in pure C++, and use them to build simplified versions of the components our project would need.

We decided to build a theme park based on the album art of Travis Scott's *Astroworld*. The giant golden head is the entrance, with its open, glowing mouth as the gate, and around it sit a Ferris wheel, a roller coaster, a drop tower, a carousel, a rocket statue, a striped big top and the "Greetings from ASTROWORLD" postcard lettering. The scene has two moods: the blue-sky carnival of the album cover by day, and the starfield of the night artwork.

The most important decision at this stage was that no graphics library would draw anything for us. We wrote our own software renderer from scratch. At its centre is a frame buffer, a block of memory holding the colour of every pixel, which plays the same role as the video memory on a graphics card. Every line, circle, curve and filled shape is written into that buffer by our own code, and the finished image is saved as a PPM file.

For lines we implemented two classic algorithms. DDA steps along a line in small increments and rounds each point to the nearest pixel, while Bresenham's algorithm makes the same decisions using only whole numbers, which is faster and more exact. We use them throughout the park: Bresenham draws the Ferris wheel's spokes, the coaster's lattice and the fence, and DDA draws the searchlight rays. For circles and ellipses we used the midpoint algorithm. It works out one eighth of a circle, or one quarter of an ellipse, and mirrors it to complete the shape, which is how we draw the Ferris wheel's rims, the light bulbs, the balloons and the clouds.

Some of those lines reach far beyond the edge of the image; each searchlight ray is about 2,200 pixels long. Rather than drawing pixels that would never be seen, we clip every line to the screen with the Cohen–Sutherland algorithm. It gives each end of a line a code describing where it lies relative to the screen, which makes it quick to accept a line that is fully visible, reject one that is fully hidden, and trim one that crosses the edge.

Curves were the next step. Bézier curves let us draw smooth shapes from a handful of control points, and we use them for the eyelids, the mouth's arch, the rocket's nose and the sagging bunting. Catmull–Rom splines pass through every one of their points, which suits shapes that need to follow a path exactly, such as the roller coaster's track, the outline of the head and the dreads.

To colour in shapes we implemented two filling methods. The scan-line fill works row by row: it keeps a table of the polygon's edges, finds where each row crosses them, and fills between the crossings. It handles even very complicated outlines, so it fills the sky, the ground, the head and the letters of the logo. Boundary fill instead starts from one point and spreads outwards until it meets a border. We implemented both the 4-connected and the 8-connected versions, and the showcase shows why the difference matters: the 8-connected version leaks out through the diagonal gaps in a line drawn with Bresenham's algorithm.

Colour was the other half of this checkpoint. We implemented the RGB, HSV and CMYK colour models and conversions between them, as well as tints, shades and tones. The sky blends from day to night through HSV space, so the colours pass through natural hues instead of turning grey. We also reduce the image from 24-bit to 8-bit colour, keeping three bits for red, three for green and two for blue, which produces visible bands in the sky. Ordered dithering then breaks those bands into a fine pattern that the eye blends back together.

The ASTROWORLD logo shows the difference between raster and vector graphics. The captions use a small bitmap font, which turns blocky when it is enlarged, while the logo's letters are described by curves and polygons, so they stay smooth at any size. To keep their edges clean, each letter is rendered at three times three samples per pixel and averaged down, a technique called supersampling anti-aliasing.

Each part of the park is its own component with adjustable settings, and a single value called `nightAmount` moves the whole scene from day to night: the sky darkens, stars appear, the searchlights switch on and the mouth glows brighter. Alongside the park we built two showcase images, one presenting every algorithm on its own and one presenting the colour concepts, so each technique can be seen in isolation.

By the end of Checkpoint 1 we had a complete, still image of the park, drawn entirely by our own code, and a set of components already designed to be animated. The next step was to bring them to life with transformations.

## Where to find it in the code

The links open the `checkpoint-1` branch.

- Frame buffer and PPM output: [src/core/frameBuffer.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/core/frameBuffer.h)
- Lines (DDA, Bresenham): [src/raster/lines.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/raster/lines.cpp)
- Circles and ellipses (midpoint): [src/raster/circles.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/raster/circles.cpp)
- Clipping (Cohen–Sutherland): [src/raster/clipping.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/raster/clipping.cpp)
- Bézier curves and splines: [src/curves/bezier.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/curves/bezier.cpp), [src/curves/spline.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/curves/spline.cpp)
- Scan-line and boundary fill: [src/fill/scanLineFill.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/fill/scanLineFill.cpp), [src/fill/boundaryFill.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/fill/boundaryFill.cpp)
- Colour models and 8-bit reduction: [src/core/color.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/core/color.cpp)
- Logo and anti-aliasing: [src/text/astroWordmark.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/text/astroWordmark.cpp), [src/core/coverageMask.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1/src/core/coverageMask.cpp)
- The park and the showcases: [src/scene/](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-1/src/scene), [src/showcase/](https://github.com/Karanei-Kimutai/ASTROWORLD/tree/checkpoint-1/src/showcase)
