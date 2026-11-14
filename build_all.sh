#!/bin/bash
# One step from your Pokémon Emerald dump to an installable VPK (run in MSYS2, or through
# build_all.ps1 on Windows):
#
#     ./build_all.sh /path/to/your/emerald.gba
#
# Installs what is missing (MSYS2 packages with Python and Pillow, VitaSDK with vitaGL and
# SDL2), gets the game files (fetch_assets.sh: the first time with the dump, and again without
# it when a new version's manifest asks for other files), rebuilds the interface atlas, builds
# the host tools, converts the game files, builds the VPK and copies it to ./EmeraldVita.vpk.
# Every step only does what is not done yet, so running it again after `git pull` just updates.
#
#     ./build_all.sh --missing-packages   exits 1 when MSYS2 packages are missing (build_all.ps1)
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
if [ "$ROM" = "--missing-packages" ]; then
    [ -n "$missing" ] && echo "Missing MSYS2 packages:$missing"
    [ -z "$missing" ]; exit $?
fi
if [ -n "$missing" ]; then
    step "Installing MSYS2 packages:$missing"
    # Refresh the package lists first: on a new or old MSYS2 they are out of date and the
    # downloads fail (404). -Syu also brings the installed packages up to the new lists.
    pacman -Syu --noconfirm || pacman -Syu --noconfirm
    pacman -S --needed --noconfirm $missing
fi

# ---------------------------------------------------------------- 2. VitaSDK and its libraries
if [ ! -x "$VITASDK/bin/arm-vita-eabi-gcc" ]; then
    step "Installing VitaSDK ($VITASDK_CHANNEL)"
    mkdir -p /c/vdpm-tmp
    export TMPDIR="$(cygpath -m /c/vdpm-tmp)"
    rm -rf /tmp/vdpm
    git clone --quiet --depth 1 https://github.com/vitasdk/vdpm /tmp/vdpm
    (cd /tmp/vdpm && ./bootstrap-vitasdk.sh)
fi
[ -x "$VITASDK/bin/arm-vita-eabi-gcc" ] || die "VitaSDK did not install; see docs/BUILD.md"
# The base VitaSDK has none of these. vdpm installs their dependencies too (vitaGL brings
# vitaShaRK, libmathneon, SceShaccCgExt and taihen; sdl2_image brings libpng, libjpeg-turbo,
# libwebp and zlib). Every file CMakeLists.txt links is checked, so a VitaSDK installed
# earlier without one of them gets it here.
VITA_LIB="$VITASDK/arm-vita-eabi/lib"
VITA_INC="$VITASDK/arm-vita-eabi/include"
need_gl=""; need_sdl=""
for f in "$VITA_INC/vitaGL.h" "$VITA_LIB/libvitaGL.a" "$VITA_LIB/libvitashark.a" \
         "$VITA_LIB/libmathneon.a" "$VITA_LIB/libSceShaccCgExt.a" "$VITA_LIB/libtaihen_stub.a"; do
    [ -f "$f" ] || need_gl=" vitaGL"
done
for f in "$VITA_LIB/libSDL2.a" "$VITA_LIB/libSDL2_image.a"; do
    [ -f "$f" ] || need_sdl=" sdl2 sdl2_image"
done
need_pkgs="$need_gl$need_sdl"
if [ -n "$need_pkgs" ]; then
    step "Installing the Vita libraries:$need_pkgs"
    mkdir -p /c/vdpm-tmp
    export TMPDIR="$(cygpath -m /c/vdpm-tmp)"
    VDPM_NONINTERACTIVE=1 vdpm install $need_pkgs
    for f in "$VITA_INC/vitaGL.h" "$VITA_LIB/libvitaGL.a" "$VITA_LIB/libSDL2.a" "$VITA_LIB/libSDL2_image.a"; do
        [ -f "$f" ] || die "$(basename "$f") is still missing after vdpm install $need_pkgs; see docs/BUILD.md"
    done
fi
# libpng's unversioned names are symbolic links in the package, and Windows without Developer
# Mode cannot create them ("Can't create ... libpng.a" while installing): copy the targets.
for pair in "lib/libpng16.a lib/libpng.a" "lib/pkgconfig/libpng16.pc lib/pkgconfig/libpng.pc" \
            "bin/libpng16-config bin/libpng-config"; do
    set -- $pair
    src="$VITASDK/arm-vita-eabi/$1"; dst="$VITASDK/arm-vita-eabi/$2"
    if [ -f "$src" ] && [ ! -e "$dst" ]; then cp "$src" "$dst"; fi
done

# ---------------------------------------------------------------- 3. the game files
manifest_sha=$(sha1sum tools/assets/manifest.txt | cut -d' ' -f1)
if [ ! -s .assets_ok ] || [ ! -f graphics/pokemon/bulbasaur/front.png ]; then
    # First run (or the files were removed): the dump is checked
    [ -n "$ROM" ] || die "the game files are missing: give the path of your Pokémon Emerald dump"
    step "Getting the game files"
    ./fetch_assets.sh "$ROM"
elif [ "$(cat .assets_manifest 2>/dev/null)" != "$manifest_sha" ]; then
    # A new version needs other game files; the dump was already checked
    step "Updating the game files"
    ./fetch_assets.sh --update
fi

# ---------------------------------------------------------------- 4. port files made from them
# The interface atlas and the mock images it reads are built from files in the repository
# (sprite lists, fonts, theme art) that an update can change, while the sprite ids the game
# uses (ui_sprite_ids.h) are committed: rebuild them on every run so the two always match.
step "Building the interface atlas"
PY=""
for c in $(type -ap python3 python 2>/dev/null); do
    if "$c" -c "import PIL" 2>/dev/null; then PY="$c"; break; fi
done
[ -n "$PY" ] || die "Python 3 with Pillow is missing (pacman -S mingw-w64-x86_64-python-pillow)"
"$PY" design/stitch/_tools/export_assets.py > /dev/null
"$PY" tools/ui2/build_atlas.py

# ---------------------------------------------------------------- 5. host tools and conversion
# Always run: make rebuilds only what an update changed
step "Building the host tools"
make -f make_tools.mk -j"$JOBS" CC="gcc -static" CXX="g++ -static"
step "Converting the game files"
make -f vita/assets.mk -j"$JOBS"

# ---------------------------------------------------------------- 6. the VPK
step "Building the VPK"
./build_vita.sh
cp build-vita/pokeemerald.vpk EmeraldVita.vpk
echo
echo "=== Done: $ROOT/EmeraldVita.vpk ==="
echo "Install it with VitaShell. This is a work in progress: please report the bugs you find"
echo "as GitHub issues (see docs/BUILD_YOUR_OWN.md, 'Reporting bugs')."
