#!/usr/bin/env bash
# Export the park from our engine and build / render it in Blender (side project, branch 'blender').
#
# Usage:
#   tools/blender/render.sh scene                 # export + build output/blender/astroworld.blend (open it in Blender)
#   tools/blender/render.sh still [frame]         # + render one frame with EEVEE  -> output/blender/still.png
#   tools/blender/render.sh stills 1,241,433 [w h samples]  # + preview several frames -> output/blender/stills/
#   tools/blender/render.sh cycles [frame]        # + render one frame with Cycles -> output/blender/still_cycles.png
#   tools/blender/render.sh animation             # + render every frame with EEVEE -> output/blender/frames/, then MP4
#
# Blender is looked for on Windows (the recommended install) and then on the PATH;
# set BLENDER=/path/to/blender to override.
set -euo pipefail

repoRoot="$(cd "$(dirname "$0")/../.." && pwd)"
mode="${1:-scene}"
frame="${2:-240}"
dataDirectory="$repoRoot/output/blender"

# ---- Find Blender ---------------------------------------------------------------------------
blender="${BLENDER:-}"
if [ -z "$blender" ]; then
    for candidate in /mnt/c/Program\ Files/Blender\ Foundation/*/blender.exe; do
        [ -e "$candidate" ] && blender="$candidate"
    done
fi
if [ -z "$blender" ] && command -v blender >/dev/null; then blender="$(command -v blender)"; fi
[ -n "$blender" ] || { echo "Blender not found: install it from blender.org (Windows) or set BLENDER=..."; exit 1; }

# A Windows blender.exe needs Windows paths (\\wsl.localhost\...) to reach files inside WSL.
toBlenderPath() {
    case "$blender" in
        *.exe) wslpath -w "$1" ;;
        *) echo "$1" ;;
    esac
}

# ---- Export from our engine -------------------------------------------------------------------
cmake --build "$repoRoot/build" --target astroworldExport >/dev/null
"$repoRoot/build/astroworldExport" --fps 24 --seconds 30 --out "$dataDirectory"

script="$(toBlenderPath "$repoRoot/tools/blender/buildAstroworld.py")"
data="$(toBlenderPath "$dataDirectory")"
blend="$(toBlenderPath "$dataDirectory/astroworld.blend")"

case "$mode" in
    scene)
        "$blender" -b -P "$script" -- --data "$data" --save "$blend"
        echo "Open in Blender: $(wslpath -w "$dataDirectory/astroworld.blend")  (allow its Python script so the animation plays)"
        ;;
    still)
        "$blender" -b -P "$script" -- --data "$data" --render still --frame "$frame" --output "$(toBlenderPath "$dataDirectory/still.png")"
        ;;
    stills)
        # Preview sheet: tools/blender/render.sh stills 1,145,241,313,433,553,649,720 [width height samples]
        mkdir -p "$dataDirectory/stills"
        "$blender" -b -P "$script" -- --data "$data" --render stills --frames "$frame" \
            --width "${3:-640}" --height "${4:-360}" --samples "${5:-16}" --output "$(toBlenderPath "$dataDirectory/stills")"
        ;;
    cycles)
        "$blender" -b -P "$script" -- --data "$data" --engine cycles --samples 128 --render still --frame "$frame" \
            --output "$(toBlenderPath "$dataDirectory/still_cycles.png")"
        ;;
    animation)
        mkdir -p "$dataDirectory/frames"
        "$blender" -b -P "$script" -- --data "$data" --render animation --output "$(toBlenderPath "$dataDirectory/frames")\\frame_"
        ffmpeg -loglevel error -y -framerate 24 -i "$dataDirectory/frames/frame_%04d.png" -c:v libx264 -pix_fmt yuv420p -crf 18 \
            "$repoRoot/output/astroworldBlender.mp4"
        echo "  wrote  output/astroworldBlender.mp4"
        ;;
    *) echo "unknown mode: $mode"; exit 2 ;;
esac
