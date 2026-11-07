// Game resets for the Vita (see vita_reset.h).
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vita_reset.h"

extern char __start_gamedata[], __stop_gamedata[];
extern char __start_gamebss[], __stop_gamebss[];

static char *sBootSnapshot;

static uint32_t DataSize(void) { return (uint32_t)(__stop_gamedata - __start_gamedata); }
static uint32_t BssSize(void)  { return (uint32_t)(__stop_gamebss - __start_gamebss); }

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
