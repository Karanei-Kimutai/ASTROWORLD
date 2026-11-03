# ASTROWORLD - Computer Graphics capstone
# Checkpoint 1 (windowed variant): the pure C++ software rasterizer, plus an
# SDL2 window that displays its frame buffer live.
#
# Targets:
#   astroworldCore    static library - every algorithm and scene component (no dependencies)
#   astroworld        writes all CP1 images as PPM files (no dependencies)
#   astroworldWindow  shows the images in a window (needs SDL2; skipped if not found)

cmake_minimum_required(VERSION 3.16)
project(ASTROWORLD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_library(astroworldCore STATIC
    # core: colour, frame buffer, coverage mask
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
target_include_directories(astroworldCore PUBLIC src)

if(MSVC)
    target_compile_options(astroworldCore PUBLIC /W4)
else()
    target_compile_options(astroworldCore PUBLIC -Wall -Wextra -Wpedantic)
endif()

# File output (pure C++).
add_executable(astroworld src/main.cpp)
target_link_libraries(astroworld PRIVATE astroworldCore)

# Live window (SDL2 is only used for the final DISPLAY stage).
find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND)
    add_executable(astroworldWindow src/windowMain.cpp)
    if(TARGET SDL2::SDL2)
        target_link_libraries(astroworldWindow PRIVATE astroworldCore SDL2::SDL2)
    else()
        target_include_directories(astroworldWindow PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(astroworldWindow PRIVATE astroworldCore ${SDL2_LIBRARIES})
    endif()
else()
    message(STATUS "SDL2 not found - skipping astroworldWindow (install libsdl2-dev to build it)")
endif()
