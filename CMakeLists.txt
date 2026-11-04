# ASTROWORLD - Computer Graphics capstone
#
# Targets:
#   astroworldCore    static library - every algorithm and scene component (no dependencies)
#   astroworld        CP1/CP2 software renderer: stills and animation frames as PPM (no dependencies)
#   astroworldWindow  CP2 live player for the software renderer (needs SDL2; skipped if not found)
#   astroworldGl      CP3+ real-time 3D park in OpenGL 3.3 core (needs GLFW; skipped if not found)
#   astroworldRay     final phase: CPU ray tracer / path tracer of the same 3D park (no dependencies)

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
    # math: 2D (3x3) and 3D (4x4) homogeneous transformation matrices
    src/math/matrix3.cpp
    src/math/matrix4.cpp
    # animation timeline shared by the software and OpenGL renderers
    src/animation/astroworldTimeline.cpp
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
    src/showcase/transformShowcase.cpp
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

# Checkpoint 3: OpenGL 3.3 core renderer (needs GLFW; GL functions are loaded at runtime by our own loader).
find_package(glfw3 3.3 QUIET)
if(glfw3_FOUND)
    add_executable(astroworldGl
        src/glMain.cpp
        src/gl/glFunctions.cpp
        src/gl/glResources.cpp
        src/gl/framePipeline.cpp
        src/mesh/meshBuilder.cpp
        src/glScene/overlayImage.cpp
        src/glScene/astroworldPark.cpp
        src/park3d/headModel.cpp
        src/park3d/parkModels.cpp
        src/park3d/parkScene.cpp
        src/park3d/parkRides.cpp
        src/park3d/parkTextures.cpp
    )
    target_link_libraries(astroworldGl PRIVATE astroworldCore glfw ${CMAKE_DL_LIBS})
else()
    message(STATUS "GLFW not found - skipping astroworldGl (install libglfw3-dev to build it)")
endif()

# Final phase: offline CPU ray tracer / path tracer for the 3D park (no external libraries, uses threads).
find_package(Threads REQUIRED)
add_executable(astroworldRay
    src/rayMain.cpp
    src/mesh/meshBuilder.cpp
    src/park3d/headModel.cpp
    src/park3d/parkModels.cpp
    src/park3d/parkScene.cpp
    src/park3d/parkRides.cpp
    src/park3d/parkTextures.cpp
    src/raytrace/rayGeometry.cpp
    src/raytrace/rayTracer.cpp
)
target_link_libraries(astroworldRay PRIVATE astroworldCore Threads::Threads)

# Side project (branch 'blender'): export the 3D park and its animation for Blender.
add_executable(astroworldExport
    src/exportMain.cpp
    src/mesh/meshBuilder.cpp
    src/park3d/headModel.cpp
    src/park3d/parkModels.cpp
    src/park3d/parkScene.cpp
    src/park3d/parkRides.cpp
    src/park3d/parkTextures.cpp
)
target_link_libraries(astroworldExport PRIVATE astroworldCore)
