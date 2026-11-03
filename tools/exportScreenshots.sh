#!/usr/bin/env bash
# Convert every PPM render in output/ to a PNG in docs/screenshots/ for the README.
# PPM is what the pure C++ renderer writes; PNG is only for documentation, so
# ffmpeg is a convenience here, not a dependency of the program itself.
set -euo pipefail

repoRoot="$(cd "$(dirname "$0")/.." && pwd)"
outputDirectory="$repoRoot/output"
screenshotDirectory="$repoRoot/docs/screenshots"
mkdir -p "$screenshotDirectory"

for ppmFile in "$outputDirectory"/*.ppm; do
    pngFile="$screenshotDirectory/$(basename "${ppmFile%.ppm}").png"
    ffmpeg -loglevel error -y -i "$ppmFile" "$pngFile"
    echo "  wrote  ${pngFile#"$repoRoot"/}"
done
