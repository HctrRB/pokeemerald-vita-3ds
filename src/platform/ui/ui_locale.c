// Catalog of the native UI's strings for a non-English build (ui_locale.h).
//
// app0:ui2/locale.bin, written by tools/locales/compile_ui.py (little endian):
//     char magic[4] = "UILC"
//     u16  version = 1, u16 reserved = 0
//     u32  count
//     count x { u32 hash, u32 offset }   sorted by hash; hash = FNV-1a 32 of the English UTF-8 key
//     blob of NUL-terminated UTF-8 translations; offset is from the start of the blob
// The compiler refuses two keys with the same hash, so a hash names one English string.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ui_locale.h"
#include "../vita/vita_platform.h"

#ifdef UI_LOCALE_CATALOG

typedef struct { uint32_t hash, offset; } UiLocaleEntry;

static uint8_t *sFile;                      // the whole file, kept: entries and blob point into it
static const UiLocaleEntry *sEntries;
static const char *sBlob;
static uint32_t sCount;

static uint32_t Rd32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

static uint32_t Fnv1a(const char *s)
{
    uint32_t h = 2166136261u;
    for (const uint8_t *p = (const uint8_t *)s; *p; p++)
        h = (h ^ *p) * 16777619u;
    return h;
}

// Every offset inside the blob, every string ending inside it, hashes strictly increasing
static bool Valid(const uint8_t *data, size_t size, uint32_t count)
{
    const size_t tableAt = 12, blobAt = tableAt + (size_t)count * 8;
    if (count > (size - tableAt) / 8 || blobAt >= size || data[size - 1] != 0)
        return false;
    const size_t blobSize = size - blobAt;
    for (uint32_t i = 0; i < count; i++)
    {
        const uint8_t *e = data + tableAt + (size_t)i * 8;
        if (Rd32(e + 4) >= blobSize || (i > 0 && Rd32(e) <= Rd32(e - 8)))
            return false;
    }
    return true;
}

bool UiLocale_Load(void)
{
    if (sFile != NULL)
        return true;
    size_t size = 0;
    uint8_t *data = VitaPlatform_LoadAsset("ui2/locale.bin", &size);
    if (data == NULL || size < 12 || memcmp(data, "UILC", 4) != 0 || data[4] != 1 || data[5] != 0)
    {
        VITA_LOG("[UI2] ui2/locale.bin missing or not version 1: the UI stays in English\n");
        free(data);
        return false;
    }
    const uint32_t count = Rd32(data + 8);
    if (!Valid(data, size, count))
    {
        VITA_LOG("[UI2] ui2/locale.bin damaged: the UI stays in English\n");
        free(data);
        return false;
    }
    // The table is 4-byte aligned in the file (offset 12) and malloc returns aligned memory
    sFile = data;
    sEntries = (const UiLocaleEntry *)(data + 12);
    sBlob = (const char *)(data + 12 + (size_t)count * 8);
    sCount = count;
    VITA_LOG("[UI2] UI catalog: %u strings\n", (unsigned)count);
    return true;
}

const char *UiLocale_Lookup(const char *english)
{
    if (english == NULL || sCount == 0)
        return english;
    const uint32_t h = Fnv1a(english);
    uint32_t lo = 0, hi = sCount;
    while (lo < hi)
    {
        const uint32_t mid = lo + (hi - lo) / 2;
        if (sEntries[mid].hash < h)
            lo = mid + 1;
        else
            hi = mid;
    }
    return (lo < sCount && sEntries[lo].hash == h) ? sBlob + sEntries[lo].offset : english;
}

#else

bool UiLocale_Load(void)
{
    return true;                            // English: the strings in the code are the UI's text
}

#endif
