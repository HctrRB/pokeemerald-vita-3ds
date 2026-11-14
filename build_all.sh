#!/bin/bash
# One step from your Pokémon Emerald dump to an installable VPK (run in MSYS2, or through
# build_all.ps1 on Windows):
#
#     ./build_all.sh /path/to/your/emerald.gba
#
# Installs what is missing (MSYS2 packages with Python and Pillow, VitaSDK with SDL2), gets the game files
# (fetch_assets.sh, only the first time), builds the host tools, converts the game files, builds
# the VPK and copies it to ./EmeraldVita.vpk. Every step is skipped when it is already done, so
# running it again after `git pull` just rebuilds.
#
# Work in progress: please report the bugs you find as GitHub issues.
set -eo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
ROM="${1:-}"
# build_all.ps1 passes the ROM's Windows path in BUILD_ALL_ROM
if [ -z "$ROM" ] && [ -n "${BUILD_ALL_ROM:-}" ]; then
    ROM="$(cygpath -u "$BUILD_ALL_ROM" 2>/dev/null || echo "$BUILD_ALL_ROM")"
fi
JOBS=$(nproc 2>/dev/null || echo 4)

step() { echo; echo "=== $* ==="; }
die() { echo "ERROR: $*" >&2; exit 1; }

export VITASDK=/usr/local/vitasdk
export VITASDK_CHANNEL=2026.08
export TAR_OPTIONS=--force-local
export PATH="$VITASDK/bin:/mingw64/bin:$PATH"

# ---------------------------------------------------------------- 1. MSYS2 packages
command -v pacman >/dev/null || die "run this in MSYS2 (or use build_all.ps1 on Windows)"
PKGS="make git cmake tar bzip2 xz wget curl python patch pkgconf diffutils
      mingw-w64-x86_64-gcc mingw-w64-x86_64-libpng mingw-w64-x86_64-pkgconf
      mingw-w64-x86_64-python mingw-w64-x86_64-python-pillow"
missing=""
for p in $PKGS; do pacman -Q "$p" >/dev/null 2>&1 || missing="$missing $p"; done
if [ -n "$missing" ]; then
    step "Installing MSYS2 packages:$missing"
    # Refresh the package lists first: on a new or old MSYS2 they are out of date and the
    # downloads fail (404). -Syu also brings the installed packages up to the new lists.
    pacman -Syu --noconfirm || pacman -Syu --noconfirm
    pacman -S --needed --noconfirm $missing
fi

# ---------------------------------------------------------------- 2. VitaSDK with SDL2
if [ ! -x "$VITASDK/bin/arm-vita-eabi-gcc" ]; then
    step "Installing VitaSDK ($VITASDK_CHANNEL)"
    mkdir -p /c/vdpm-tmp
    export TMPDIR="$(cygpath -m /c/vdpm-tmp)"
    rm -rf /tmp/vdpm
    git clone --quiet --depth 1 https://github.com/vitasdk/vdpm /tmp/vdpm
    (cd /tmp/vdpm && ./bootstrap-vitasdk.sh)
fi
if [ ! -f "$VITASDK/arm-vita-eabi/lib/libSDL2.a" ]; then
    step "Installing SDL2 for the Vita"
    mkdir -p /c/vdpm-tmp
    export TMPDIR="$(cygpath -m /c/vdpm-tmp)"
    VDPM_NONINTERACTIVE=1 vdpm install sdl2 sdl2_image
fi
[ -x "$VITASDK/bin/arm-vita-eabi-gcc" ] || die "VitaSDK did not install; see docs/BUILD.md"

# ---------------------------------------------------------------- 3. the game files
if [ ! -f .assets_ok ] || [ ! -f graphics/pokemon/bulbasaur/front.png ] || [ ! -f assets/ui2/ui_atlas.bin ]; then
    [ -n "$ROM" ] || die "the game files are missing: give the path of your Pokémon Emerald dump"
    step "Getting the game files"
    ./fetch_assets.sh "$ROM"
fi

# ---------------------------------------------------------------- 4. host tools and conversion
if [ ! -x tools/gbagfx/gbagfx.exe ] && [ ! -x tools/gbagfx/gbagfx ]; then
    step "Building the host tools"
    make -f make_tools.mk -j"$JOBS" CC="gcc -static" CXX="g++ -static"
fi
step "Converting the game files"
make -f vita/assets.mk -j"$JOBS"

# ---------------------------------------------------------------- 5. the VPK
step "Building the VPK"
./build_vita.sh
cp build-vita/pokeemerald.vpk EmeraldVita.vpk
echo
echo "=== Done: $ROOT/EmeraldVita.vpk ==="
echo "Install it with VitaShell. This is a work in progress: please report the bugs you find"
echo "as GitHub issues (see docs/BUILD_YOUR_OWN.md, 'Reporting bugs')."
