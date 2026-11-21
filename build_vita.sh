#!/bin/bash
# Configures and builds build-vita/pokeemerald.vpk with VitaSDK (run from MSYS2, or through
# build_vita.ps1 on Windows).
set -e
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:/usr/bin:/bin:$PATH

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# BUILD_DIR and EXTRA_CMAKE_ARGS from the environment: a second build next to the normal one,
# so its CMake options never leak into it (build_vita.ps1 -Probe builds build-vita-probe with
# -DVITA_GPU_PROBE=ON, docs/guides/DEBUGGING.md)
# VITA_LOCALE from the environment: the build's language, a code of tools/locales/languages.json
# (build_vita.ps1 -Locale). Each language builds in its own folder, build-vita-<code>: the game's
# sources are preprocessed with the language's defines, which a folder never mixes.
VITA_LOCALE="${VITA_LOCALE:-en}"
if [ "$VITA_LOCALE" = "en" ]; then
    BUILD_DIR="${BUILD_DIR:-build-vita}"
else
    BUILD_DIR="${BUILD_DIR:-build-vita-$VITA_LOCALE}"
fi
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"
cd "$SCRIPT_DIR"

# The game's files are not in the repository: fetch_assets.sh gets them first
if [ ! -f .assets_ok ] || [ ! -f graphics/pokemon/bulbasaur/front.png ] || [ ! -f assets/ui2/ui_atlas.bin ]; then
    echo "ERROR: the game files are missing. Run ./fetch_assets.sh <your Pokemon Emerald .gba> first"
    echo "       (Windows: fetch_assets.ps1 -Rom <file>). See docs/guides/BUILD_YOUR_OWN.md."
    exit 1
fi

# The intro's 16:9 margins are made from the converted intro graphics (make -f vita/assets.mk)
if [ ! -f assets/ui/intro_margins.bin ]; then
    if [ ! -f graphics/intro/scene_1/bg.4bpp ]; then
        echo "ERROR: convert the game graphics first: make -f vita/assets.mk (see docs/guides/BUILD_YOUR_OWN.md)"
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

# A non-English build's UI atlas carries its language's letters (tools/ui2/build_atlas.py --locale)
if [ "$VITA_LOCALE" != "en" ]; then
    PYPIL=""
    for c in $(type -ap python3 python 2>/dev/null) /mingw64/bin/python3; do
        if [ -x "$c" ] && "$c" -c "import PIL" 2>/dev/null; then PYPIL="$c"; break; fi
    done
    if [ -z "$PYPIL" ]; then
        echo "ERROR: a non-English build needs Python 3 with Pillow (MSYS2: pacman -S mingw-w64-x86_64-python-pillow)"
        exit 1
    fi
    mkdir -p "$BUILD_DIR/ui2"
    "$PYPIL" tools/ui2/build_atlas.py --locale "$VITA_LOCALE" --out "$BUILD_DIR/ui2/ui_atlas.bin"
fi

echo "=== Configuring CMake in $SCRIPT_DIR/$BUILD_DIR ==="
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DVITA_LOCALE="$VITA_LOCALE" ${EXTRA_CMAKE_ARGS:-}

echo "=== Building pokeemerald.vpk ==="
cmake --build "$BUILD_DIR" -j4
