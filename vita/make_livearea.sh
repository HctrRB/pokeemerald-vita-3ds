#!/bin/bash
# Builds the LiveArea images in vita/sce_sys/ from full-colour artwork.
# LiveArea needs 8-bit indexed PNGs on real hardware, so everything is quantised here.
#
# Usage (MINGW64 shell with ImageMagick, from the repository root):
#   vita/make_livearea.sh <icon0 128x128> <pic0 960x544> <background 840x500>
set -euo pipefail

if [[ $# -ne 3 ]]; then
    echo "usage: $0 icon0.png pic0.png background.png" >&2
    exit 2
fi

OUT=vita/sce_sys
mkdir -p "$OUT/livearea/contents"

png8() { magick "$1" ${2:+-resize "$2"} -strip -colors 256 "PNG8:$3"; }

png8 "$1" ""         "$OUT/icon0.png"
png8 "$2" ""         "$OUT/pic0.png"
png8 "$3" ""         "$OUT/livearea/contents/bg.png"
png8 "$3" "280x158!" "$OUT/livearea/contents/startup.png"

magick identify "$OUT/icon0.png" "$OUT/pic0.png" "$OUT/livearea/contents/"*.png
