#!/bin/bash
# Gets the game's graphics, sounds and binary data, which this repository does not carry, and
# generates the port's files made from them. Run it once before the first build:
#
#     ./fetch_assets.sh /path/to/your/pokemon_emerald.gba
#
# 1. Checks that the file is a Pokémon Emerald cartridge dump (any language): size, title, game
#    code and header checksum. The ROM is only read for this check; nothing is copied from it,
#    stored or uploaded.
# 2. Downloads the files listed in tools/assets/manifest.txt from the public
#    pokeemerald-multiplatform repository (the pret/pokeemerald decompilation this port started
#    from), at a fixed commit, and checks each one against the hash in the manifest.
# 3. Generates the port's files built from them: the new UI atlas and the design mocks' images
#    (Python 3 with Pillow). The intro margins need the converted graphics and are made by
#    build_vita.sh.
#
# Needs: git, and Python 3 with Pillow (pip install pillow).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

SRC_URL="https://github.com/gradenGnostic/pokeemerald-multiplatform.git"
SRC_COMMIT="501c6e63ca24e38b91d904099413caecf1d9c7c0"
MANIFEST="tools/assets/manifest.txt"
WORK=".assets_src"
EMERALD_US_SHA1="f3ae088181bf583e55daf962a92bb46f4f1d07b7"

die() { echo "ERROR: $*" >&2; exit 1; }

ROM="${1:-}"
[ -n "$ROM" ] || die "usage: ./fetch_assets.sh <path to your Pokémon Emerald .gba dump>"
[ -f "$ROM" ] || die "'$ROM' does not exist"

# ---------------------------------------------------------------- 1. the ROM
echo "=== Checking the ROM ==="
size=$(wc -c < "$ROM" | tr -d ' ')
[ "$size" -eq 16777216 ] || die "not a Pokémon Emerald dump (size $size bytes, expected 16 MB)"
title=$(dd if="$ROM" bs=1 skip=160 count=12 2>/dev/null | tr -d '\000')
code=$(dd if="$ROM" bs=1 skip=172 count=4 2>/dev/null)
[ "$title" = "POKEMON EMER" ] || die "not a Pokémon Emerald dump (title '$title')"
case "$code" in BPE?) ;; *) die "not a Pokémon Emerald dump (game code '$code')" ;; esac
sum=0
for b in $(od -An -tu1 -j160 -N29 "$ROM"); do sum=$((sum + b)); done
expected=$(( (0 - (sum + 25)) & 255 ))
stored=$(od -An -tu1 -j189 -N1 "$ROM" | tr -d ' ')
[ "$expected" -eq "$stored" ] || die "the ROM header is damaged (checksum)"
sha=$( (sha1sum "$ROM" 2>/dev/null || shasum "$ROM") | cut -d' ' -f1)
if [ "$sha" = "$EMERALD_US_SHA1" ]; then
    echo "Pokémon Emerald ($code), USA/Europe 1.0: verified"
else
    echo "Pokémon Emerald ($code): header verified"
fi

# ---------------------------------------------------------------- 2. the decompilation's files
echo "=== Downloading the game files (pokeemerald-multiplatform ${SRC_COMMIT:0:7}) ==="
command -v git >/dev/null || die "git is needed (MSYS2: pacman -S git)"
if [ ! -d "$WORK/.git" ]; then
    git clone --quiet --filter=blob:none --no-checkout "$SRC_URL" "$WORK"
fi
if ! git -C "$WORK" cat-file -e "$SRC_COMMIT^{commit}" 2>/dev/null; then
    git -C "$WORK" fetch --quiet --filter=blob:none origin "$SRC_COMMIT"
fi
# (MSYS_NO_PATHCONV: MSYS2 / Git Bash would turn "/graphics/" into a Windows path)
MSYS_NO_PATHCONV=1 git -C "$WORK" sparse-checkout set --no-cone /graphics/ /sound/ /data/
git -C "$WORK" checkout --quiet --force "$SRC_COMMIT"

echo "=== Checking them against $MANIFEST ==="
git -C "$WORK" ls-tree -r "$SRC_COMMIT" -- graphics sound data \
    | awk -F'\t' '{ split($1, a, " "); print a[3] "  " $2 }' > "$WORK/.tree"
bad=$(awk 'NR == FNR { t[$2] = $1; next } t[$2] != $1 { print $2 }' "$WORK/.tree" "$MANIFEST" | head -5)
[ -z "$bad" ] || die "files missing or different in the download, e.g.: $bad"

echo "=== Copying $(wc -l < "$MANIFEST" | tr -d ' ') files ==="
awk '{ print $2 }' "$MANIFEST" > "$WORK/.list"
(cd "$WORK" && tar cf - -T .list) | tar xf -

# ---------------------------------------------------------------- 3. port files made from them
echo "=== Generating the port's files ==="
PY=""
for c in $(type -ap python3 python 2>/dev/null); do
    if "$c" -c "import PIL" 2>/dev/null; then PY="$c"; break; fi
done
[ -n "$PY" ] || die "Python 3 with Pillow is needed (MSYS2: pacman -S mingw-w64-x86_64-python-pillow, or pip install pillow)"
"$PY" design/stitch/_tools/export_assets.py > /dev/null
"$PY" tools/ui2/build_atlas.py

echo "$code" > .assets_ok
echo "=== Done: you can build now (build_vita.ps1 or build_vita.sh) ==="
