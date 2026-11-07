#!/bin/bash
# Link launcher for the Vita ELF (CMake RULE_LAUNCH_LINK): runs the link command it is
# given, then makes sure the executable LOAD segment ends at least NEEDED bytes before
# the next page. vita-elf-create appends the SCE module info (~3 KB) to that segment,
# and without enough room the result can fail to load on real hardware.
set -e
NEEDED=4000   # proven on hardware

"$@"

# The output file is the argument after -o.
OUT=
prev=
for arg in "$@"; do
    [[ $prev == -o ]] && OUT=$arg
    prev=$arg
done
[[ -n $OUT ]] || { echo "vita_link: no -o in link command" >&2; exit 1; }

exec_segment_gap() {
    # LOAD  offset  vaddr  paddr  filesz  memsz  flags...  align
    # The flags column is split by spaces ("R E"), so match E anywhere after memsz.
    local vaddr memsz
    read -r vaddr memsz < <(arm-vita-eabi-readelf -lW "$OUT" |
        awk '$1 == "LOAD" { for (i = 7; i < NF; i++) if ($i == "E") { print $3, $6; exit } }')
    [[ -n $vaddr ]] || { echo "vita_link: no executable LOAD segment in $OUT" >&2; exit 1; }
    local next_vaddr
    read -r next_vaddr < <(arm-vita-eabi-readelf -lW "$OUT" |
        awk 'BEGIN { found=0 } $1 == "LOAD" { if (found) { print $3; exit } for (i = 7; i < NF; i++) if ($i == "E") found=1 }')
    local end=$(( vaddr + memsz ))
    if [[ -n $next_vaddr ]]; then
        echo $(( next_vaddr - end ))
    else
        echo $(( (4096 - end % 4096) % 4096 ))
    fi
}

GAP=$(exec_segment_gap)
if (( GAP < NEEDED )); then
    PAD=$(( GAP + 8 ))
    PADOBJ="$(dirname "$OUT")/vita_segment_pad.o"
    printf '\t.section .rodata.vita_segment_pad,"a"\n\t.global vita_segment_pad\nvita_segment_pad:\n\t.space %d\n' "$PAD" |
        arm-vita-eabi-as -o "$PADOBJ" -
    "$@" "$PADOBJ" -Wl,--undefined=vita_segment_pad
    echo "vita_link: padded exec segment by $PAD bytes (gap was $GAP, now $(exec_segment_gap))"
fi
