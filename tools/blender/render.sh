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

# A sculpted head (head.blend, made in Blender) replaces ours when it exists: by default the one in the
# Windows copy folder. HEAD=/path/to/head.blend or HEAD=none to override; HEAD_PARTS=head,dreads,tunnel.
headFile="${HEAD:-}"
if [ -z "$headFile" ] && [[ "$blender" == *.exe ]]; then
    windowsProfile="$(cd /mnt/c && cmd.exe /c "echo %USERPROFILE%" 2>/dev/null | tr -d '\r')"
    headFile="$(wslpath -u "$windowsProfile")/Documents/ASTROWORLD-Blender/head.blend"
fi
headArguments=()
if [ "$headFile" != "none" ] && [ -f "$headFile" ]; then
    headArguments=(--head-file "$(toBlenderPath "$headFile")" --head-parts "${HEAD_PARTS:-head,dreads,tunnel}")
fi
script="$(toBlenderPath "$repoRoot/tools/blender/buildAstroworld.py")"
data="$(toBlenderPath "$dataDirectory")"
blend="$(toBlenderPath "$dataDirectory/astroworld.blend")"

case "$mode" in
    scene)
        "$blender" -b -P "$script" -- --data "$data" "${headArguments[@]}" --save "$blend"
        # Windows Blender can refuse to open a .blend from the \\wsl.localhost share ("Permission denied"),
        # so for a Windows install the scene and its data files (the player reads frames.bin next to the
        # .blend) are also copied to Documents\ASTROWORLD-Blender on the Windows drive.
        case "$blender" in
            *.exe)
                windowsProfile="$(cd /mnt/c && cmd.exe /c "echo %USERPROFILE%" 2>/dev/null | tr -d '\r')"
                windowsCopy="$(wslpath -u "$windowsProfile")/Documents/ASTROWORLD-Blender"
                mkdir -p "$windowsCopy"
                cp "$dataDirectory"/{astroworld.blend,frames.bin,meshes.bin,meshes.json,objects.json,lighting.json,ground.bmp,sign.bmp} "$windowsCopy/"
                echo "Open in Blender: $(wslpath -w "$windowsCopy/astroworld.blend")  (allow its Python script so the animation plays)"
                ;;
            *) echo "Open in Blender: $dataDirectory/astroworld.blend  (allow its Python script so the animation plays)" ;;
        esac
        ;;
    still)
        "$blender" -b -P "$script" -- --data "$data" "${headArguments[@]}" --render still --frame "$frame" --output "$(toBlenderPath "$dataDirectory/still.png")"
        ;;
    stills)
        # Preview sheet: tools/blender/render.sh stills 1,145,241,313,433,553,649,720 [width height samples]
        mkdir -p "$dataDirectory/stills"
        "$blender" -b -P "$script" -- --data "$data" "${headArguments[@]}" --render stills --frames "$frame" \
            --width "${3:-640}" --height "${4:-360}" --samples "${5:-16}" --output "$(toBlenderPath "$dataDirectory/stills")"
        ;;
    cycles)
        "$blender" -b -P "$script" -- --data "$data" "${headArguments[@]}" --engine cycles --samples 128 --render still --frame "$frame" \
            --output "$(toBlenderPath "$dataDirectory/still_cycles.png")"
        ;;
    animation)
        mkdir -p "$dataDirectory/frames"
        "$blender" -b -P "$script" -- --data "$data" "${headArguments[@]}" --render animation --output "$(toBlenderPath "$dataDirectory/frames")\\frame_"
        ffmpeg -loglevel error -y -framerate 24 -i "$dataDirectory/frames/frame_%04d.png" -c:v libx264 -pix_fmt yuv420p -crf 18 \
            "$repoRoot/output/astroworldBlender.mp4"
        echo "  wrote  output/astroworldBlender.mp4"
        ;;
    *) echo "unknown mode: $mode"; exit 2 ;;
esac
