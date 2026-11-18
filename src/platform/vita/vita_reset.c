// Game resets for the Vita (see vita_reset.h).
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vita_reset.h"

extern char __start_gamedata[], __stop_gamedata[];
extern char __start_gamebss[], __stop_gamebss[];

// Keep at least 4 bytes in .noinit (placed after gamebss in the linker script) so
// __stop_gamebss stays strictly below the RW segment's p_vaddr + p_memsz; otherwise
// vita-elf-create drops the relocation for __stop_gamebss from .sce.rel.
__attribute__((section(".noinit"), used)) static uint32_t sSegmentEndPad;

static char *sBootSnapshot;

static uint32_t DataSize(void) { return (uint32_t)((&__stop_gamedata[-1] + 1) - __start_gamedata); }
static uint32_t BssSize(void)  { return (uint32_t)((&__stop_gamebss[-1] + 1) - __start_gamebss); }

void VitaReset_TakeBootSnapshot(void)
{
    if (sBootSnapshot == NULL)
        sBootSnapshot = malloc(DataSize() + BssSize());
    if (sBootSnapshot != NULL)
    {
        memcpy(sBootSnapshot, __start_gamedata, DataSize());
        memcpy(sBootSnapshot + DataSize(), __start_gamebss, BssSize());
    }
}

bool VitaReset_RestoreBootSnapshot(void)
{
    if (sBootSnapshot == NULL)
        return false;
    memcpy(__start_gamedata, sBootSnapshot, DataSize());
    memcpy(__start_gamebss, sBootSnapshot + DataSize(), BssSize());
    return true;
}
