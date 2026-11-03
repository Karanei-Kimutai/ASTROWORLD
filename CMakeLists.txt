# ASTROWORLD - Computer Graphics capstone
# Checkpoint 1: pure C++ software rasterizer (no external libraries).

cmake_minimum_required(VERSION 3.16)
project(ASTROWORLD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(astroworld
    src/main.cpp
    # core: colour, frame buffer
    src/core/color.cpp
    src/core/frameBuffer.cpp
    src/core/coverageMask.cpp
    # raster: lines, circles/ellipses, clipping, thick strokes
    src/raster/lines.cpp
    src/raster/circles.cpp
    src/raster/clipping.cpp
    src/raster/strokes.cpp
    # curves: Bezier, Catmull-Rom spline
    src/curves/bezier.cpp
    src/curves/spline.cpp
    # fill: scan-line, boundary
    src/fill/scanLineFill.cpp
    src/fill/boundaryFill.cpp
    # text: bitmap font, vector wordmark
    src/text/bitmapFont.cpp
    src/text/astroWordmark.cpp
    # scene components
    src/scene/backdrop.cpp
    src/scene/ferrisWheel.cpp
    src/scene/dropTower.cpp
    src/scene/rollerCoaster.cpp
    src/scene/carousel.cpp
    src/scene/astroHead.cpp
    src/scene/foreground.cpp
    src/scene/carnivalProps.cpp
    src/scene/astroworldScene.cpp
    # showcases
    src/showcase/algorithmShowcase.cpp
    src/showcase/colorShowcase.cpp
)

target_include_directories(astroworld PRIVATE src)

if(MSVC)
    target_compile_options(astroworld PRIVATE /W4)
else()
    target_compile_options(astroworld PRIVATE -Wall -Wextra -Wpedantic)
endif()
