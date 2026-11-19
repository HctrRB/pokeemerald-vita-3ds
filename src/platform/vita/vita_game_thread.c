// The game-thread tripwire (include/platform/game_thread.h).
#include <stdint.h>
#include <stdio.h>
#include <psp2/kernel/threadmgr.h>

#include "platform/game_thread.h"
#include "vita_platform.h"

// Outside gamebss: a soft reset restores the game's memory, not the thread that runs it.
volatile uintptr_t gGameThreadStackTop;
static uint32_t sGameThreadId;

#define MAX_REPORTED_SITES 64

static const void *volatile sReported[MAX_REPORTED_SITES];

// Called first thing on the game thread (DoMain), near the top of its stack.
void GameThread_Register(void)
{
    sGameThreadId = (uint32_t)sceKernelGetThreadId();
    __atomic_store_n(&gGameThreadStackTop, GameThread_StackPointer(), __ATOMIC_RELEASE);
}

void GameThread_Violation(const void *caller, const char *what)
{
    // Once per call site: claim a free slot, or find the site already there.
    for (uint32_t i = 0; i < MAX_REPORTED_SITES; i++)
    {
        const void *seen = __atomic_load_n(&sReported[i], __ATOMIC_ACQUIRE);
        if (seen == caller)
            return;
        if (seen == NULL)
        {
            const void *expected = NULL;
            if (__atomic_compare_exchange_n(&sReported[i], &expected, caller, false,
                                            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
                break;
            if (expected == caller)
                return;
        }
    }

    char path[128];
    VitaPlatform_BuildPath(path, sizeof(path), "thread_violations.log");
    FILE *f = fopen(path, "a");
    if (f != NULL)
    {
        // The caller is an ELF address: arm-vita-eabi-addr2line -f -e build-vita/pokeemerald <lr>
        fprintf(f, "%s: lr=%p thread=0x%08x game=0x%08x\n",
                what, caller, (unsigned)sceKernelGetThreadId(), (unsigned)sGameThreadId);
        fclose(f);
    }
    VITA_LOG("[thread] %s: lr=%p\n", what, caller);
}
