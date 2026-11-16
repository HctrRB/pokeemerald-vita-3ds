#include "platform/vita_pak.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VITA_PAK_MAGIC "PKF1"
#define VITA_PAK_VERSION 1u
#define VITA_PAK_MAX_ENTRIES 8192u
#define VITA_PAK_KEY_LEN 24u

struct VitaPakHeader
{
    char magic[4];
    uint32_t version;
    uint32_t entryCount;
    uint32_t reserved;
};

struct VitaPakEntry
{
    char key[VITA_PAK_KEY_LEN];
    uint32_t offset;
    uint32_t size;
};

_Static_assert(sizeof(struct VitaPakHeader) == 16, "PKF1 header must be 16 bytes");
_Static_assert(sizeof(struct VitaPakEntry) == 32, "PKF1 entry must be 32 bytes");

static FILE *sPakFile = NULL;
static struct VitaPakEntry *sEntries = NULL;
static uint32_t sEntryCount = 0;
static bool sPakAttempted = false;

static const char *NormalizeFollowerKey(const char *const path)
{
    if (path == NULL || path[0] == '\0')
        return NULL;

    static const char prefix[] = "followers/";
    const size_t prefixLen = sizeof(prefix) - 1;
    if (strncmp(path, prefix, prefixLen) == 0)
        return path + prefixLen;

    return path;
}

static FILE *OpenFollowerPakFile(void)
{
    static const char *const candidates[] = {
        "app0:followers/followers.pak",
        "ux0:data/POKEVITA/followers/followers.pak",
        "assets/followers/followers.pak",
        "followers/followers.pak",
    };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++)
    {
        FILE *const f = fopen(candidates[i], "rb");
        if (f != NULL)
            return f;
    }
    return NULL;
}

static bool ValidatePakEntries(const struct VitaPakEntry *const entries, const uint32_t count,
                             const uint32_t dataStart, const uint32_t fileSize)
{
    for (uint32_t i = 0; i < count; i++)
    {
        if (memchr(entries[i].key, '\0', VITA_PAK_KEY_LEN) == NULL)
            return false;
        if (entries[i].offset < dataStart || entries[i].offset > fileSize)
            return false;
        if (entries[i].size > fileSize - entries[i].offset)
            return false;
        if (i > 0 && strcmp(entries[i - 1].key, entries[i].key) >= 0)
            return false;
    }
    return true;
}

static bool EnsureFollowerPakLoaded(void)
{
    if (sPakFile != NULL && sEntries != NULL)
        return true;
    if (sPakAttempted)
        return false;
    sPakAttempted = true;

    FILE *const f = OpenFollowerPakFile();
    if (f == NULL)
        return false;

    if (fseek(f, 0L, SEEK_END) != 0)
    {
        fclose(f);
        return false;
    }
    const long rawFileSize = ftell(f);
    if (rawFileSize < (long)sizeof(struct VitaPakHeader) || fseek(f, 0L, SEEK_SET) != 0)
    {
        fclose(f);
        return false;
    }
    const uint32_t fileSize = (uint32_t)rawFileSize;

    struct VitaPakHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1
     || memcmp(hdr.magic, VITA_PAK_MAGIC, 4) != 0
     || hdr.version != VITA_PAK_VERSION
     || hdr.entryCount == 0
     || hdr.entryCount > VITA_PAK_MAX_ENTRIES)
    {
        fclose(f);
        return false;
    }

    const uint32_t tableBytes = hdr.entryCount * (uint32_t)sizeof(struct VitaPakEntry);
    const uint32_t dataStart = (uint32_t)sizeof(struct VitaPakHeader) + tableBytes;
    if (dataStart > fileSize)
    {
        fclose(f);
        return false;
    }

    struct VitaPakEntry *const entries = malloc(tableBytes);
    if (entries == NULL)
    {
        fclose(f);
        return false;
    }

    if (fread(entries, sizeof(struct VitaPakEntry), hdr.entryCount, f) != hdr.entryCount
     || !ValidatePakEntries(entries, hdr.entryCount, dataStart, fileSize))
    {
        free(entries);
        fclose(f);
        return false;
    }

    sPakFile = f;
    sEntries = entries;
    sEntryCount = hdr.entryCount;
    return true;
}

static int ComparePakKey(const void *const keyPtr, const void *const entryPtr)
{
    const char *const key = (const char *)keyPtr;
    const struct VitaPakEntry *const entry = (const struct VitaPakEntry *)entryPtr;
    return strcmp(key, entry->key);
}

static const struct VitaPakEntry *FindEntry(const char *const path)
{
    const char *const key = NormalizeFollowerKey(path);
    if (key == NULL || strlen(key) >= VITA_PAK_KEY_LEN)
        return NULL;
    if (!EnsureFollowerPakLoaded())
        return NULL;

    return (const struct VitaPakEntry *)bsearch(key, sEntries, sEntryCount,
                                               sizeof(struct VitaPakEntry), ComparePakKey);
}

bool VitaPak_FollowerExists(const char *const path)
{
    return FindEntry(path) != NULL;
}

void *VitaPak_LoadFollower(const char *const path, uint32_t *const outSize)
{
    if (outSize != NULL)
        *outSize = 0;

    const struct VitaPakEntry *const entry = FindEntry(path);
    if (entry == NULL)
        return NULL;

    uint8_t *const buf = malloc((size_t)entry->size + 1u);
    if (buf == NULL)
        return NULL;

    if (fseek(sPakFile, (long)entry->offset, SEEK_SET) != 0
     || (entry->size > 0 && fread(buf, 1, entry->size, sPakFile) != entry->size))
    {
        free(buf);
        return NULL;
    }

    buf[entry->size] = 0;
    if (outSize != NULL)
        *outSize = entry->size;
    return buf;
}

void VitaPak_Shutdown(void)
{
    if (sPakFile != NULL)
    {
        fclose(sPakFile);
        sPakFile = NULL;
    }
    free(sEntries);
    sEntries = NULL;
    sEntryCount = 0;
    sPakAttempted = false;
}
