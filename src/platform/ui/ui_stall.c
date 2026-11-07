// Stall detector of the new UI (user report: the menu freezes, then jumps to where the presses
// took the cursor). Writes ux0:data/POKEVITA/ui_stall.log when, with the UI open:
//  - Core 1 did not run Ui_Tick for more than 100 ms (the game thread stalled), or
//  - Core 0 did not render for more than 100 ms, or
//  - Core 0 kept drawing the same list for more than 100 ms while Core 1 was ticking
//    (the hand-off failed).
// Each line carries the ages of the other side, so the log says which core held the frame.

#include <stdio.h>
#include <stdarg.h>
#include "ui_internal.h"
#include "../vita/vita_perf.h"

#define STALL_MS 100.0

static uint32_t sTickMs;     // Core 1: time of the last Ui_Tick with the UI open (ms, written atomically)
static uint32_t sRenderMs;   // Core 0: time of the last Ui_Render
static uint32_t sPubSeq;     // Core 1: seq of the last published list
static const char *sScreen = "-";
static int sLines;

static void Log(const char *fmt, ...)
{
    if (sLines >= 200)
        return;
    FILE *f = fopen("ux0:data/POKEVITA/ui_stall.log", sLines == 0 ? "w" : "a");
    if (f == NULL)
        return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fclose(f);
    sLines++;
}

static const char *ScreenName(const UiScreen *s)
{
    if (s == &gUiHubScreen) return "HUB";
    if (s == &gUiBagScreen) return "BAG";
    if (s == &gUiPartyScreen) return "PARTY";
    if (s == &gUiTargetScreen) return "TARGET";
    if (s == &gUiCardScreen) return "CARD";
    if (s == &gUiOptionsScreen) return "OPTIONS";
    if (s == &gUiDexScreen) return "DEX";
    if (s == &gUiDexDetailScreen) return "DEX DETAIL";
    if (s == &gUiNuzlockeScreen) return "NUZLOCKE";
    if (s == &gUiNavScreen) return "POKENAV";
    if (s == &gUiFlyScreen) return "FLY";
    if (s == &gUiMartScreen) return "MART";
    if (s == &gUiChooseScreen) return "CHOOSE";
    return "?";
}

static uint32_t NowMs(void)
{
    return (uint32_t)VitaPerf_NowMs();
}

// Core 1, every Ui_Tick with a screen open, before the screen update
void UiStall_Tick(const UiScreen *top, float lastTickMs)
{
    const uint32_t now = NowMs();
    const uint32_t prev = __atomic_load_n(&sTickMs, __ATOMIC_RELAXED);
    sScreen = ScreenName(top);
    if (prev != 0 && now - prev > STALL_MS)
        Log("[%u] CORE 1 gap %u ms (no Ui_Tick) screen=%s lastTick=%.2f ms lastRender %u ms ago\n",
            now, now - prev, sScreen, (double)lastTickMs, now - __atomic_load_n(&sRenderMs, __ATOMIC_RELAXED));
    __atomic_store_n(&sTickMs, now, __ATOMIC_RELAXED);
}

void UiStall_Published(uint32_t seq)
{
    __atomic_store_n(&sPubSeq, seq, __ATOMIC_RELAXED);
}

void UiStall_Closed(void)
{
    __atomic_store_n(&sTickMs, 0u, __ATOMIC_RELAXED);
}

// Core 0, every Ui_Render call
void UiStall_Render(uint32_t drawnSeq)
{
    static uint32_t sSeenSeq, sSeqSince, sLastRender;
    const uint32_t now = NowMs();
    const uint32_t tick = __atomic_load_n(&sTickMs, __ATOMIC_RELAXED);
    __atomic_store_n(&sRenderMs, now, __ATOMIC_RELAXED);
    if (tick == 0)
    {
        sLastRender = now;
        sSeenSeq = drawnSeq;
        sSeqSince = now;
        return;                                           // UI closed: nothing to watch
    }
    if (sLastRender != 0 && now - sLastRender > STALL_MS)
        Log("[%u] CORE 0 gap %u ms (no frame) screen=%s lastTick %u ms ago\n", now, now - sLastRender, sScreen, now - tick);
    sLastRender = now;
    if (drawnSeq != sSeenSeq)
    {
        if (now - sSeqSince > STALL_MS)
            Log("[%u] SAME LIST %u ms (Core 0 drew seq %u, Core 1 published %u, lastTick %u ms ago) screen=%s\n",
                now, now - sSeqSince, sSeenSeq, __atomic_load_n(&sPubSeq, __ATOMIC_RELAXED), now - tick, sScreen);
        sSeenSeq = drawnSeq;
        sSeqSince = now;
    }
}
