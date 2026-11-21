# ASTROWORLD: Checkpoint 1, windowed variant

## Summary

The windowed variant of Checkpoint 1 shows the same software-rendered park live in a window instead of only saving it as image files. SDL2 is used only for the final display stage: it opens the window and presents our finished frame buffer, while every pixel is still calculated by our own C++ code. Because our frame buffer uses the same byte layout as the window's texture, the image is copied across without conversion, and a captured frame matches the saved image byte for byte. The window lets us switch between day and night, slide the time of day, and view the 8-bit, dithered and showcase images, and by redrawing on demand it set up the continuous redraw that animation needed in Checkpoint 2.

## In detail

This branch takes the Checkpoint 1 renderer and shows its output live in a window instead of only saving it as image files. It answers the same checkpoint as `checkpoint-1`; the difference is in how the result reaches the screen.

When we planned the project, we wanted a version of Checkpoint 1 that could be demonstrated interactively. The rule for this checkpoint was that we could use a basic windowing library, but nothing that would draw for us, so we chose SDL2 and gave it a single job. SDL2 opens the window and puts our finished image on the screen. It does not draw a single line, circle or shape. Every pixel is still calculated by our own C++ code and written into our own frame buffer, exactly as on the main Checkpoint 1 branch.

In terms of the rendering pipeline, this is the very last stage, display. Each time something changes, our renderer produces a new image in the frame buffer, and we copy those pixels into an SDL texture, which SDL then presents in the window. Our frame buffer happens to use the same byte layout as the texture format, so the pixels are copied across without any conversion. We also checked that the window shows our image exactly. Capturing a frame from the window and comparing it with the image file produced by the renderer gives a byte-for-byte match.

The window makes the checkpoint's concepts easy to explore. It switches between the day and night scenes, and it can slide the time of day in small steps, so you can watch the sky blend through HSV colour space, the stars fade in and the searchlights come on. It also shows the 8-bit and dithered versions of the scene, as well as the algorithm and colour showcases.

This branch is small, but it set up something important for later. Because the window redraws whenever it is asked to, the next checkpoint could simply ask it to redraw continuously over time, which is exactly what animation needs.

## Where to find it in the code

The links open the `checkpoint-1-window` branch.

- The window player: [src/windowMain.cpp](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1-window/src/windowMain.cpp)
- The frame buffer it displays: [src/core/frameBuffer.h](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1-window/src/core/frameBuffer.h)
- The optional SDL2 build target: [CMakeLists.txt](https://github.com/Karanei-Kimutai/ASTROWORLD/blob/checkpoint-1-window/CMakeLists.txt)
- Everything else is the Checkpoint 1 renderer: see the [Checkpoint 1 guide](checkpoint1.md).
