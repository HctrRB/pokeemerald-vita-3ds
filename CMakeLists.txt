# PS Vita build of pokeemerald (PLATFORM_VITA on top of the SDL2 backend).
#
# Prerequisites (from the repository root, see README.md):
#   make -f make_tools.mk
#   make -f vita/assets.mk -j8
# Configure and build:
#   cmake -S . -B build-vita -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-vita -j8

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(NOT DEFINED ENV{VITASDK})
        message(FATAL_ERROR "VITASDK is not set; see README.md")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
endif()

project(pokeemerald_vita C)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Emerald Vita")   # neutral name; change it for your own builds
set(VITA_TITLEID  "PKEM00001")
set(VITA_VERSION  "01.00")

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
if(CMAKE_HOST_WIN32)
    set(HOST_EXE ".exe")
else()
    set(HOST_EXE "")
endif()
set(PREPROC "${ROOT}/tools/preproc/preproc${HOST_EXE}")

if(NOT EXISTS "${PREPROC}" OR NOT EXISTS "${ROOT}/include/constants/map_groups.h")
    message(FATAL_ERROR "Host tools or generated assets are missing. Run:\n"
                        "  make -f make_tools.mk\n  make -f vita/assets.mk -j8")
endif()

find_program(VITA_CPP     arm-vita-eabi-cpp     HINTS "$ENV{VITASDK}/bin" REQUIRED)
find_program(VITA_AS      arm-vita-eabi-as      HINTS "$ENV{VITASDK}/bin" REQUIRED)
find_program(VITA_LD      arm-vita-eabi-ld      HINTS "$ENV{VITASDK}/bin" REQUIRED)
find_program(VITA_OBJCOPY arm-vita-eabi-objcopy HINTS "$ENV{VITASDK}/bin" REQUIRED)

set(GAME_DEFINES -DNONMATCHING -DPORTABLE -DPLATFORM_SDL2 -DPLATFORM_VITA -DRENDERER_FAST_DRAW -DVOXEL_SUPPORTED -DMODERN=1 -DUBFIX)
option(VITA_LOGGING "Write VITA_LOG output to ux0:data/POKEVITA/debug.log" OFF)
if(VITA_LOGGING)
    list(APPEND GAME_DEFINES -DVITA_LOGGING)
endif()
option(VITA_PARALLEL_DRAW "Draw each frame on three cores" ON)
if(VITA_PARALLEL_DRAW)
    list(APPEND GAME_DEFINES -DVITA_PARALLEL_DRAW)
endif()
option(VITA_DRAW_SELFCHECK "Periodically compare parallel drawing against sequential (testing)" OFF)
if(VITA_DRAW_SELFCHECK)
    list(APPEND GAME_DEFINES -DVITA_DRAW_SELFCHECK)
endif()
string(REPLACE ";" " " GAME_DEFINES_STR "${GAME_DEFINES}")

# --- C sources: cpp -> preproc (charmap strings, INCBIN) -> compiled below ---
file(GLOB_RECURSE GAME_C CONFIGURE_DEPENDS "${ROOT}/src/*.c" "${ROOT}/gflib/*.c")
list(FILTER GAME_C EXCLUDE REGEX "\\.inc\\.c$")

# Lets IMPLICIT_DEPENDS follow #include into these folders, so header edits
# regenerate the preprocessed sources that use them.
include_directories("${ROOT}/include" "${ROOT}/gflib" "${ROOT}/src")

set(GENERATED_C)
# (-Wno-trigraphs: "??" in game text, strings.c, is never meant as a trigraph)
foreach(SOURCE IN LISTS GAME_C)
    file(RELATIVE_PATH RELATIVE_SOURCE "${ROOT}" "${SOURCE}")
    set(OUTPUT "${CMAKE_BINARY_DIR}/generated/${RELATIVE_SOURCE}")
    get_filename_component(OUTPUT_DIR "${OUTPUT}" DIRECTORY)
    set(EXTRA_PREPROC_FLAGS "")
    # Stage scenes (one whole GBA picture each): their VBlank callbacks are reported to
    # vita_stage.c through a forced include, so the platform knows a stage is on screen without
    # editing the scene code (the approach of Emerald 3DS, compat/ctr_gba_stage.h).
    if(RELATIVE_SOURCE MATCHES "src/(intro|title_screen|credits|cable_car|rayquaza_scene)\\.c$")
        set(EXTRA_PREPROC_FLAGS "-DCTR_GBA_STAGE -include '${ROOT}/src/platform/vita/compat/ctr_gba_stage.h'")
    endif()
    add_custom_command(
        OUTPUT "${OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
        COMMAND /bin/sh -c "cd '${ROOT}' && '${CMAKE_C_COMPILER}' -E -std=gnu99 -Wno-trigraphs -iquote include -iquote gflib ${GAME_DEFINES_STR} ${EXTRA_PREPROC_FLAGS} '${RELATIVE_SOURCE}' -o '${OUTPUT}.i' && '${PREPROC}' '${OUTPUT}.i' charmap.txt > '${OUTPUT}'"
        DEPENDS "${SOURCE}" "${PREPROC}" "${ROOT}/charmap.txt"
        IMPLICIT_DEPENDS C "${SOURCE}"
        VERBATIM)
    list(APPEND GENERATED_C "${OUTPUT}")
endforeach()

# --- Data and song assembly, relinked into one object ---
file(GLOB DATA_ASM CONFIGURE_DEPENDS "${ROOT}/data/*.s")
file(GLOB SONG_ASM CONFIGURE_DEPENDS "${ROOT}/sound/songs/*.s" "${ROOT}/sound/songs/midi/*.s")
set(SPECIAL_IDS "${CMAKE_BINARY_DIR}/generated/special_ids.inc")
add_custom_command(
    OUTPUT "${SPECIAL_IDS}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/generated"
    COMMAND /bin/sh -c "awk '/^[[:space:]]+def_special[[:space:]]/ { printf \".set SPECIAL_%s, %d\\n\", $2, id++ }' '${ROOT}/data/specials.inc' > '${SPECIAL_IDS}'"
    DEPENDS "${ROOT}/data/specials.inc"
    VERBATIM)

set(GENERATED_ASM)
foreach(SOURCE IN LISTS DATA_ASM SONG_ASM)
    file(RELATIVE_PATH RELATIVE_SOURCE "${ROOT}" "${SOURCE}")
    set(OUTPUT "${CMAKE_BINARY_DIR}/generated/${RELATIVE_SOURCE}.o")
    get_filename_component(OUTPUT_DIR "${OUTPUT}" DIRECTORY)
    if(SOURCE MATCHES "/sound/songs/")
        add_custom_command(
            OUTPUT "${OUTPUT}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
            COMMAND /bin/sh -c "cd '${ROOT}' && sed -e 's/\\.4byte/\\.word/g;s/\\.2byte/\\.short/g' '${RELATIVE_SOURCE}' > '${OUTPUT}.s' && '${VITA_AS}' -march=armv7-a -I sound -o '${OUTPUT}' '${OUTPUT}.s'"
            DEPENDS "${SOURCE}"
            VERBATIM)
    else()
        add_custom_command(
            OUTPUT "${OUTPUT}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
            # special_ids.inc is found through -I: an absolute path written inside the .s
            # would not be translated for a native assembler under MSYS2.
            COMMAND /bin/sh -c "cd '${ROOT}' && { printf '.include \"special_ids.inc\"\\n'; '${PREPROC}' '${RELATIVE_SOURCE}' charmap.txt | '${VITA_CPP}' -I include -DMODERN=1 -DPORTABLE -DUBFIX | '${PREPROC}' -ie '${RELATIVE_SOURCE}' charmap.txt; } > '${OUTPUT}.s' && '${VITA_AS}' -march=armv7-a -I . -I sound -I '${CMAKE_BINARY_DIR}/generated' -o '${OUTPUT}' '${OUTPUT}.s'"
            DEPENDS "${SOURCE}" "${PREPROC}" "${ROOT}/charmap.txt" "${SPECIAL_IDS}"
            VERBATIM)
    endif()
    list(APPEND GENERATED_ASM "${OUTPUT}")
endforeach()

# Game data holds pointers, so it goes to a writable section.
# The object list goes through a response file with relative paths: the full command line
# exceeds the Windows limit, and paths inside the file are not translated under MSYS2.
set(GAME_DATA_OBJECT "${CMAKE_BINARY_DIR}/generated/game_data.o")
set(GAME_DATA_RSP "${CMAKE_BINARY_DIR}/generated/game_data.rsp")
set(GAME_DATA_RSP_CONTENT "")
foreach(OBJECT IN LISTS GENERATED_ASM)
    file(RELATIVE_PATH RELATIVE_OBJECT "${CMAKE_BINARY_DIR}/generated" "${OBJECT}")
    string(APPEND GAME_DATA_RSP_CONTENT "${RELATIVE_OBJECT}\n")
endforeach()
file(WRITE "${GAME_DATA_RSP}" "${GAME_DATA_RSP_CONTENT}")
add_custom_command(
    OUTPUT "${GAME_DATA_OBJECT}"
    COMMAND "${VITA_LD}" -r -o game_data.o.tmp @game_data.rsp
    COMMAND "${VITA_OBJCOPY}" --rename-section .rodata=.data,alloc,load,data,contents game_data.o.tmp game_data.o
    DEPENDS ${GENERATED_ASM} "${GAME_DATA_RSP}"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/generated"
    VERBATIM)
set_source_files_properties("${GAME_DATA_OBJECT}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

# --- Game side and platform side ---
# All game state (RAM, emulated VRAM/OAM/palette/IO registers in bios.c, sound, the save in
# flash) is linked first into one object whose .data/.bss become "gamedata"/"gamebss", so the
# platform layer can copy it through the linker's __start_/__stop_ symbols: its power-on state
# restarts the game (src/platform/vita/vita_reset.c). The platform side (SDL, the parallel
# renderer with its threads, widescreen, the port menu) stays outside, as does game_data.o: ROM
# data, read only.
set(PLATFORM_SIDE_REGEX "/src/platform/(sdl2|widescreen|gba_fast_draw|gba_fast_draw_b|gba_fast_draw_c)\\.c$|/src/platform/(vita|voxel|ui)/")
set(GAME_SIDE_C)
set(PLATFORM_SIDE_C)
foreach(OUTPUT IN LISTS GENERATED_C)
    if(OUTPUT MATCHES "${PLATFORM_SIDE_REGEX}")
        list(APPEND PLATFORM_SIDE_C "${OUTPUT}")
    else()
        list(APPEND GAME_SIDE_C "${OUTPUT}")
    endif()
endforeach()

# -fno-strict-aliasing / -fwrapv: the decomp relies on type punning and signed overflow
# that the GBA tolerates; without them GCC 15 may optimise that code away.
set(VITA_CPU_TUNING_OPTIONS
    -mcpu=cortex-a9
    -mtune=cortex-a9
    -mfpu=neon
    -mfloat-abi=hard)

# Options shared by both sides.
set(COMMON_COMPILE_OPTIONS
    -std=gnu99
    -O2
    ${VITA_CPU_TUNING_OPTIONS}
    -fno-strict-aliasing
    -fwrapv
    -fno-builtin
    -fcommon
    "SHELL:-iquote '${ROOT}/include'"
    "SHELL:-iquote '${ROOT}/gflib'")

# The decompiled game keeps the leniency it was written for.
set(GAME_COMPILE_OPTIONS
    ${COMMON_COMPILE_OPTIONS}
    -Wno-implicit-function-declaration
    -Wno-error=incompatible-pointer-types
    -Wno-error=int-conversion
    -Wno-error=implicit-int)

# The port's own code (src/platform) builds with the warnings on.
set(PLATFORM_COMPILE_OPTIONS
    ${COMMON_COMPILE_OPTIONS}
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-missing-field-initializers
    -fno-math-errno)

add_library(game_side OBJECT ${GAME_SIDE_C})
target_include_directories(game_side PRIVATE "${ROOT}" "${ROOT}/sound")
target_compile_options(game_side PRIVATE ${GAME_COMPILE_OPTIONS})

# ld -r -d gives common symbols (-fcommon) their space in .bss. The object list goes through a
# response file with paths relative to the build folder (see game_data.o above).
set(GAME_COMBINED_OBJECT "${CMAKE_BINARY_DIR}/game_combined.o")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/game_side.abs.rsp"
     CONTENT "$<JOIN:$<TARGET_OBJECTS:game_side>,\n>\n")
add_custom_command(
    OUTPUT "${GAME_COMBINED_OBJECT}"
    COMMAND /bin/sh -c "sed -e 's|^${CMAKE_BINARY_DIR}/||' game_side.abs.rsp > game_side.rsp"
    COMMAND "${VITA_LD}" -r -d -o game_combined.o.tmp @game_side.rsp
    COMMAND "${VITA_OBJCOPY}" --rename-section .data=gamedata --rename-section .bss=gamebss game_combined.o.tmp game_combined.o
    DEPENDS game_side $<TARGET_OBJECTS:game_side> "${CMAKE_BINARY_DIR}/game_side.abs.rsp"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Linking the game side (game_combined.o)"
    VERBATIM)
set_source_files_properties("${GAME_COMBINED_OBJECT}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

# --- Executable ---
add_executable(pokeemerald ${PLATFORM_SIDE_C} "${GAME_COMBINED_OBJECT}" "${GAME_DATA_OBJECT}")
target_include_directories(pokeemerald PRIVATE "${ROOT}" "${ROOT}/sound")
target_compile_options(pokeemerald PRIVATE ${PLATFORM_COMPILE_OPTIONS})
# SDL2's own config carries its stub libraries, which differ between VitaSDK builds
# (e.g. SDL2 built with the vitaGL backend also needs vitaGL and its dependencies).
# SDL2 renders through GXM directly (VitaSDK 2026.08); no vitaGL. SDL2_image loads the border art.
find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_image REQUIRED CONFIG)
target_link_libraries(pokeemerald PRIVATE
    SDL2_image::SDL2_image-static
    SDL2::SDL2-static
    vitaGL
    vitashark
    mathneon
    SceShaccCgExt
    SceShaccCg_stub
    SceKernelDmacMgr_stub
    taihen_stub
    SceGxm_stub
    SceDisplay_stub
    ScePower_stub
    SceAudio_stub
    SceCtrl_stub
    SceTouch_stub
    SceRtc_stub
    stdc++
    m)
target_link_options(pokeemerald PRIVATE -Wl,-z,max-page-size=0x10000 -Wl,-z,common-page-size=0x10000)
# libwebp (pulled in by SDL2_image) needs pthread. CMake appends transitive dependencies after
# the target's own libraries, so it has to go at the very end of the link line.
set(CMAKE_C_STANDARD_LIBRARIES "-lpthread -lm")

# Leaves room after the code segment for the module info vita-elf-create appends.
set_property(TARGET pokeemerald PROPERTY RULE_LAUNCH_LINK "bash '${ROOT}/vita/vita_link.sh'")

# UNSAFE: the game reads and writes ux0:data/POKEVITA/.
vita_create_self(eboot.bin pokeemerald UNSAFE)
# Equivalent of vita_create_vpk, which joins its -a arguments into one string and splits it
# on spaces, so it breaks when the checkout path has one (e.g. "Pokemon Esmeralda").
# LiveArea images are 8-bit indexed PNGs made by vita/make_livearea.sh.
set(SCE_SYS "${ROOT}/vita/sce_sys")
set(SCE_SYS_FILES
    icon0.png
    pic0.png
    livearea/contents/bg.png
    livearea/contents/startup.png
    livearea/contents/template.xml)
set(VPK_FILE_ARGS)
set(VPK_FILE_DEPS)
# vita/sce_sys holds neutral images; your own LiveArea art can go in vita/sce_sys_custom (same
# names, not committed) and is used instead.
foreach(FILE_NAME IN LISTS SCE_SYS_FILES)
    set(SCE_FILE "${SCE_SYS}/${FILE_NAME}")
    if(EXISTS "${ROOT}/vita/sce_sys_custom/${FILE_NAME}")
        set(SCE_FILE "${ROOT}/vita/sce_sys_custom/${FILE_NAME}")
    endif()
    list(APPEND VPK_FILE_ARGS -a "${SCE_FILE}=sce_sys/${FILE_NAME}")
    list(APPEND VPK_FILE_DEPS "${SCE_FILE}")
endforeach()
# Voxel 3D diorama assets read from app0:voxel/
file(GLOB VOXEL_ASSETS CONFIGURE_DEPENDS "${ROOT}/assets/voxel/*.*")
foreach(VA IN LISTS VOXEL_ASSETS)
    get_filename_component(VA_NAME "${VA}" NAME)
    list(APPEND VPK_FILE_ARGS -a "${VA}=voxel/${VA_NAME}")
    list(APPEND VPK_FILE_DEPS "${VA}")
endforeach()

# Follower Pokemon HGSS assets read from app0:followers/
if(EXISTS "${ROOT}/assets/followers")
    list(APPEND VPK_FILE_ARGS -a "${ROOT}/assets/followers=followers")
    list(APPEND VPK_FILE_DEPS "${ROOT}/assets/followers/sources.txt")
endif()

# Procedural Emerald 3DS UI & HUD binary assets read from app0:ui/
file(GLOB UI_ASSETS CONFIGURE_DEPENDS "${ROOT}/assets/ui/*.bin")
foreach(UA IN LISTS UI_ASSETS)
    get_filename_component(UA_NAME "${UA}" NAME)
    list(APPEND VPK_FILE_ARGS -a "${UA}=ui/${UA_NAME}")
    list(APPEND VPK_FILE_DEPS "${UA}")
endforeach()

# New UI atlas read from app0:ui2/ (built by tools/ui2/build_atlas.py)
file(GLOB UI2_ASSETS CONFIGURE_DEPENDS "${ROOT}/assets/ui2/*.bin")
foreach(UA IN LISTS UI2_ASSETS)
    get_filename_component(UA_NAME "${UA}" NAME)
    list(APPEND VPK_FILE_ARGS -a "${UA}=ui2/${UA_NAME}")
    list(APPEND VPK_FILE_DEPS "${UA}")
endforeach()

set(PARAM_SFO "${CMAKE_BINARY_DIR}/param.sfo")
add_custom_command(
    OUTPUT "${PARAM_SFO}"
    COMMAND "${VITA_MKSFOEX}" -d ATTRIBUTE2=12 -s APP_VER=${VITA_VERSION} -s TITLE_ID=${VITA_TITLEID} "${VITA_APP_NAME}" "${PARAM_SFO}"
    DEPENDS "${CMAKE_BINARY_DIR}/eboot.bin"
    COMMENT "Generating param.sfo"
    VERBATIM)
add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/pokeemerald.vpk"
    COMMAND "${VITA_PACK_VPK}" ${VPK_FILE_ARGS} -s "${PARAM_SFO}" -b "${CMAKE_BINARY_DIR}/eboot.bin" "${CMAKE_BINARY_DIR}/pokeemerald.vpk"
    DEPENDS "${PARAM_SFO}" "${CMAKE_BINARY_DIR}/eboot.bin" ${VPK_FILE_DEPS}
    COMMENT "Building pokeemerald.vpk"
    VERBATIM)
add_custom_target(pokeemerald-vpk ALL DEPENDS "${CMAKE_BINARY_DIR}/pokeemerald.vpk")
add_dependencies(pokeemerald-vpk eboot.bin-self)
