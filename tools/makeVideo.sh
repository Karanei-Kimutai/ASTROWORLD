#!/usr/bin/env bash
# Render the ASTROWORLD animation and encode it as an MP4 (plus a small GIF preview).
#
# The renderer streams raw PPM frames on stdout straight into ffmpeg, so no
# frame files are written to disk. ffmpeg only ENCODES the video; every pixel
# is still drawn by the pure C++ renderer.
#
# Usage: tools/makeVideo.sh [seconds] [fps]      (defaults: one 30 s loop at 30 fps)
set -euo pipefail

repoRoot="$(cd "$(dirname "$0")/.." && pwd)"
seconds="${1:-30}"
framesPerSecond="${2:-30}"
renderer="$repoRoot/build/astroworld"
videoFile="$repoRoot/output/astroworldAnimation.mp4"
previewFile="$repoRoot/docs/screenshots/animationPreview.gif"

[ -x "$renderer" ] || { echo "Build first: cmake -S . -B build && cmake --build build"; exit 1; }
mkdir -p "$repoRoot/output" "$(dirname "$previewFile")"

"$renderer" --animate "$seconds" "$framesPerSecond" - |
    ffmpeg -loglevel error -y -f image2pipe -framerate "$framesPerSecond" -c:v ppm -i - \
        -c:v libx264 -pix_fmt yuv420p -crf 20 -movflags +faststart "$videoFile"
echo "  wrote  ${videoFile#"$repoRoot"/}"

# README preview: 12 s from the start, 480 px wide, 12 fps, optimised 128-colour palette.
ffmpeg -loglevel error -y -t 12 -i "$videoFile" \
    -vf "fps=12,scale=480:-2:flags=lanczos,split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=bayer" \
    "$previewFile"
echo "  wrote  ${previewFile#"$repoRoot"/}"
