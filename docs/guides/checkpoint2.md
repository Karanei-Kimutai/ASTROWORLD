# ASTROWORLD: Checkpoint 2, transformation and animation

## Summary

Checkpoint 2 brought the park to life with transformations and animation, still drawn by our own software renderer. We built 3×3 and 4×4 matrix classes using homogeneous coordinates to translate, rotate, scale, shear and reflect shapes, including rotation and scaling about a pivot, and showed why the order of transformations matters. The Ferris wheel became a transformation hierarchy with level gondolas, and the carousel's horses were placed in 3D and drawn back to front. Every frame is a pure function of time within a 30-second loop that moves from day through dusk and night to dawn, using easing curves and keyframes, with the roller coaster's speed coming from conservation of energy and the smaller movements from oscillators. The animation plays in the window or streams straight into ffmpeg as a video, and its limit of about 15 to 20 frames per second on the CPU is what led us to OpenGL.

## In detail

Checkpoint 2 is where the park came to life. The task was to implement transformations and animation, still in pure C++, so we kept the software renderer from Checkpoint 1 and taught every part of the park to move.

The foundation for this was a small matrix library. Transformations such as moving, rotating and scaling a shape can each be written as a matrix, and by using homogeneous coordinates we can treat all of them the same way. A 2D point becomes three numbers, (x, y, 1), and that extra one is what allows a translation to be written as a matrix multiplication just like a rotation. We built a `Matrix3` class for 2D transformations and a `Matrix4` class for 3D ones, covering translation, rotation, scaling, shearing and reflection. Because they are all matrices, any sequence of transformations can be combined into one.

Many objects need to turn or grow about a particular point rather than about the origin. To rotate around a pivot, we move the pivot to the origin, rotate, and move it back. The dreads bend about their roots this way, the balloons sway about the vendor's hand, and the visitors' legs swing about their hips. Scaling about a point works the same way: the head gently "breathes" by scaling about its base, so its mouth stays on the ground. We also built a transformation showcase that demonstrates each operation on its own, including an important detail: the order matters, and translating then rotating gives a different result from rotating then translating.

The Ferris wheel is a transformation hierarchy. The wheel rotates about its hub, each spoke is placed relative to the wheel, and each gondola is placed at the end of its spoke. The gondolas take only their position from the spoke and not its rotation, so they stay level and hang straight down as the wheel turns, the way real gondolas do. We also used 4×4 matrices to place the carousel's horses in 3D around the carousel. They are then projected onto the screen and drawn from back to front, so the nearer horses correctly cover the ones behind them.

For animation, we made every frame a pure function of time. Given a moment in the 30-second loop, the renderer can draw exactly what the park looks like at that moment, without depending on the frames before it. This means the animation plays at the correct speed at any frame rate, and we can jump straight to any point in the loop. On top of that we built easing curves and keyframe tracks. The drop tower uses them to climb smoothly, pause at the top for suspense, then fall with a bounce at the bottom, and the whole park moves through day, dusk, night and dawn along keyframes.

Some motion is driven by physics. The roller coaster's train is pulled up the lift at a steady speed, and from then on its speed comes from conservation of energy: the further it has dropped, the faster it goes. We move it along the spline by distance travelled rather than by the curve's parameter, so its speed is physically correct. Other movements, such as the swinging gondolas, the bobbing horses, the swaying dreads and bunting, the twinkling stars and the sweeping searchlights, use simple oscillators, and the visitors walk into the mouth with their legs swinging in opposite phase.

The result can be watched live in the window, with play, pause, seek and speed controls, or exported. The renderer can output every frame of the loop, and we stream those frames straight into ffmpeg to produce an MP4 video without ever writing gigabytes of raw images to disk. We also made the renderer faster, cutting a frame from about 100 milliseconds to about 60 by caching the colours of the sky and ground for each row.

By the end of Checkpoint 2 the park was fully animated, but it was still a flat, 2D picture drawn on the CPU at around 15 to 20 frames per second. That limit is what led us to the graphics card and OpenGL in Checkpoint 3.

## Where to find it in the code

The links open the `checkpoint-2` branch.

- 2D and 3D matrices: [src/math/matrix3.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/math/matrix3.h), [src/math/matrix4.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/math/matrix4.h)
- Easing curves and keyframes: [src/animation/easing.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/animation/easing.h), [src/animation/keyframeTrack.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/animation/keyframeTrack.h)
- The animated scene and timeline: [src/scene/astroworldScene.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/scene/astroworldScene.cpp)
- Ferris wheel hierarchy: [src/scene/ferrisWheel.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/scene/ferrisWheel.cpp)
- Coaster physics: [src/scene/rollerCoaster.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/scene/rollerCoaster.cpp)
- Carousel in 3D: [src/scene/carousel.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/scene/carousel.cpp)
- Transformation showcase: [src/showcase/transformShowcase.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/src/showcase/transformShowcase.cpp)
- Video export: [tools/makeVideo.sh](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-2/tools/makeVideo.sh)
