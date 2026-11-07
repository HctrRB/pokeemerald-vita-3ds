#!/bin/bash
# Configures and builds build-vita/pokeemerald.vpk with VitaSDK (run from MSYS2, or through
# build_vita.ps1 on Windows).
set -e
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:/usr/bin:/bin:$PATH

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="build-vita"
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"
cd "$SCRIPT_DIR"

# The game's files are not in the repository: fetch_assets.sh gets them first
if [ ! -f .assets_ok ] || [ ! -f graphics/pokemon/bulbasaur/front.png ] || [ ! -f assets/ui2/ui_atlas.bin ]; then
    echo "ERROR: the game files are missing. Run ./fetch_assets.sh <your Pokemon Emerald .gba> first"
    echo "       (Windows: fetch_assets.ps1 -Rom <file>). See docs/BUILD_YOUR_OWN.md."
    exit 1
fi

# The intro's 16:9 margins are made from the converted intro graphics (make -f vita/assets.mk)
if [ ! -f assets/ui/intro_margins.bin ]; then
    if [ ! -f graphics/intro/scene_1/bg.4bpp ]; then
        echo "ERROR: convert the game graphics first: make -f vita/assets.mk (see docs/BUILD_YOUR_OWN.md)"
        exit 1
    fi
    PY=python3; command -v python3 >/dev/null || PY=python
    "$PY" tools/ui_gen/gen_intro_margins.py --output assets/ui/intro_margins.bin
fi

# A CMake cache only works for the source tree that created it. When the repository is
# copied or moved, drop the stale cache (the object files are rebuilt anyway) instead of
# letting CMake refuse to configure.
if [ -f "$CACHE_FILE" ]; then
    CACHED_SOURCE="$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$CACHE_FILE")"
    if [ "$CACHED_SOURCE" != "$SCRIPT_DIR" ]; then
        echo "=== Stale CMake cache (created in '$CACHED_SOURCE'), reconfiguring ==="
        rm -rf "$CACHE_FILE" "$BUILD_DIR/CMakeFiles"
    fi
fi

echo "=== Configuring CMake in $SCRIPT_DIR/$BUILD_DIR ==="
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

echo "=== Building pokeemerald.vpk ==="
cmake --build "$BUILD_DIR" -j4
