#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "platform/vita_ui_art.h"
#include "platform/vita_ui.h"
#include "platform/vita_extras.h"

#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "data.h"
#include "decompress.h"
#include "graphics.h"
#include "item_icon.h"
#include "pokemon.h"
#include "pokemon_icon.h"
#include "string_util.h"
#include "constants/characters.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "3ds_bottom_art.h"

static const u16 sPokenavPal[] = INCBIN_U16("graphics/pokenav/nav_icon.gbapal");
static const u8 sPokenavGfx[] = INCBIN_U8("graphics/pokenav/nav_icon.4bpp");

extern const u16 gFontSmallNarrowLatinGlyphs[];
extern const u8 gFontSmallNarrowLatinGlyphWidths[];
extern const u16 gFontSmallLatinGlyphs[];
extern const u8 gFontSmallLatinGlyphWidths[];
extern const u16 gFontNarrowLatinGlyphs[];
extern const u8 gFontNarrowLatinGlyphWidths[];
extern const u16 gFontNormalLatinGlyphs[];
extern const u8 gFontNormalLatinGlyphWidths[];
extern const u8 gMoveCategories[];

enum
{
    FRONT_PIC_SLOTS   = 16,
    ITEM_ICON_SLOTS   = 24,
    SMOOTH_TEXT_SLOTS = 32,
    SMOOTH_KEY_CAP    = 48,
    SMOOTH_PAD        = 3,
    CAT_BADGE_W       = 32,
    CAT_BADGE_H       = 12
};

typedef struct
{
    const uint16_t *glyphs;
    const uint8_t *widths;
    int32_t height;
    int32_t lineHeight;
} FontDesc;

typedef struct
{
    uint32_t age;
    uint16_t species;
    bool shiny;
    bool loaded;
    uint8_t tiles[64 * 64 / 2];
    uint16_t pal565[16];
} FrontPicSlot;

typedef struct
{
    uint32_t age;
    uint16_t itemId;
    bool loaded;
    uint8_t tiles[24 * 24 / 2];
    uint16_t pal565[16];
} ItemIconSlot;

typedef struct
{
    bool loaded;
    bool female;
    uint8_t frame;
    uint8_t tiles[64 * 64 / 2];
    uint16_t pal565[16];
} BagPicCache;

typedef struct
{
    uint32_t age;
    uint8_t fontId;
    uint8_t scaleQ;
    bool dropShadow;
    VitaUiRgb fg;
    VitaUiRgb outline;
    uint8_t key[SMOOTH_KEY_CAP];
    uint16_t w;
    uint16_t h;
    uint16_t *p; // Premultiplied RGB565
    uint8_t *a;  // Coverage 0..255
} SmoothSlot;

static bool sArtInit = false;
static FontDesc sFonts[VITA_FONT_COUNT];
static FrontPicSlot sFrontPics[FRONT_PIC_SLOTS];
static ItemIconSlot sItemIcons[ITEM_ICON_SLOTS];
static BagPicCache sBagCache;
static SmoothSlot sSmoothCache[SMOOTH_TEXT_SLOTS];
static uint32_t sArtClock = 0U;

static uint8_t sTrainerPicTiles[2][64 * 64 / 2];
static uint16_t sTrainerPicPal565[2][16];
static bool sTrainerPicValid[2];

static uint8_t *sMoveTypeTiles = NULL;
static uint16_t sMoveTypePals565[3][16];
static VitaUiRgb sTypeColours[NUMBER_OF_MON_TYPES];
static uint16_t sCategoryBadgePx[3][CAT_BADGE_W * CAT_BADGE_H];
static uint8_t sCategoryBadgeMask[3][CAT_BADGE_W * CAT_BADGE_H];

static const uint8_t sTypeToPalIdx[NUMBER_OF_MON_TYPES] = {
    [TYPE_NORMAL]   = 0,
    [TYPE_FIGHTING] = 0,
    [TYPE_FLYING]   = 1,
    [TYPE_POISON]   = 1,
    [TYPE_GROUND]   = 0,
    [TYPE_ROCK]     = 0,
    [TYPE_BUG]      = 2,
    [TYPE_GHOST]    = 1,
    [TYPE_STEEL]    = 0,
    [TYPE_MYSTERY]  = 2,
    [TYPE_FIRE]     = 0,
    [TYPE_WATER]    = 1,
    [TYPE_GRASS]    = 2,
    [TYPE_ELECTRIC] = 0,
    [TYPE_PSYCHIC]  = 1,
    [TYPE_ICE]      = 1,
    [TYPE_DRAGON]   = 2,
    [TYPE_DARK]     = 0
};

static inline int32_t Div255(const int32_t x)
{
    return (x + 1 + (x >> 8)) >> 8;
}

static void ConvertPal16(const uint16_t *const srcBgr, uint16_t *const dst565)
{
    for (int32_t i = 0; i < 16; ++i)
    {
        dst565[i] = VitaUi_Bgr555ToRgb565(srcBgr[i]);
    }
}

static void InitFonts(void)
{
    sFonts[VITA_FONT_SMALL] = (FontDesc){
        .glyphs = gFontSmallLatinGlyphs,
        .widths = gFontSmallLatinGlyphWidths,
        .height = 13,
        .lineHeight = 13
    };
    sFonts[VITA_FONT_NORMAL] = (FontDesc){
        .glyphs = gFontNormalLatinGlyphs,
        .widths = gFontNormalLatinGlyphWidths,
        .height = 16,
        .lineHeight = 16
    };
    sFonts[VITA_FONT_SMALL_NARROW] = (FontDesc){
        .glyphs = gFontSmallNarrowLatinGlyphs,
        .widths = gFontSmallNarrowLatinGlyphWidths,
        .height = 13,
        .lineHeight = 13
    };
    sFonts[VITA_FONT_NARROW] = (FontDesc){
        .glyphs = gFontNarrowLatinGlyphs,
        .widths = gFontNarrowLatinGlyphWidths,
        .height = 16,
        .lineHeight = 16
    };
}

static void InitTrainerPics(void)
{
    for (uint8_t gender = MALE; gender <= FEMALE; ++gender)
    {
        const uint16_t picIdx = gFacilityClassToPicIndex[gender == FEMALE ? FACILITY_CLASS_MAY : FACILITY_CLASS_BRENDAN];
        uint16_t rawPal[16];
        LZ77UnCompWram(gTrainerFrontPicTable[picIdx].data, sTrainerPicTiles[gender]);
        LZ77UnCompWram(gTrainerFrontPicPaletteTable[picIdx].data, rawPal);
        ConvertPal16(rawPal, sTrainerPicPal565[gender]);
        sTrainerPicValid[gender] = true;
    }
}

static void InitTypeArtAndColours(void)
{
    const size_t sheetBytes = (size_t)(NUMBER_OF_MON_TYPES + 5) * 8U * 32U;
    sMoveTypeTiles = (uint8_t *)malloc(sheetBytes);
    if (sMoveTypeTiles != NULL)
    {
        LZ77UnCompWram(gMoveTypes_Gfx, sMoveTypeTiles);
    }

    uint16_t rawTypePals[48];
    LZ77UnCompWram(gMoveTypes_Pal, rawTypePals);
    for (int32_t p = 0; p < 3; ++p)
    {
        ConvertPal16(&rawTypePals[p * 16], sMoveTypePals565[p]);
    }

    for (int32_t type = 0; type < NUMBER_OF_MON_TYPES; ++type)
    {
        const uint16_t *const pal = sMoveTypePals565[sTypeToPalIdx[type]];
        int32_t count[16] = {0};
        int32_t best = -1;
        sTypeColours[type] = (VitaUiRgb){150, 150, 150};
        if (sMoveTypeTiles == NULL)
        {
            continue;
        }
        for (int32_t i = 0; i < 8 * 32; ++i)
        {
            const uint8_t byte = sMoveTypeTiles[type * 8 * 32 + i];
            count[byte & 15U]++;
            count[byte >> 4]++;
        }
        for (int32_t v = 1; v < 16; ++v)
        {
            const VitaUiRgb c = VitaUi_UnpackRgb565(pal[v]);
            if ((c.r > 230 && c.g > 230 && c.b > 230) || ((int32_t)c.r + c.g + c.b <= 120))
            {
                continue;
            }
            if (best < 0 || count[v] > count[best])
            {
                best = v;
            }
        }
        if (best >= 0)
        {
            sTypeColours[type] = VitaUi_UnpackRgb565(pal[best]);
        }
    }
}

static void BuildProceduralCategoryBadges(void)
{
    const VitaUiRgb bgCols[3] = {
        {214, 62, 42},   // Physical: crimson-orange
        {64, 92, 188},   // Special: deep indigo-blue
        {126, 134, 142}  // Status: cool slate
    };
    const VitaUiRgb emblemCols[3] = {
        {255, 224, 92},  // Physical: golden starburst
        {178, 226, 255}, // Special: cyan concentric ovals
        {244, 246, 248}  // Status: crisp white split droplet
    };
    const uint16_t border565 = VitaUi_PackRgb565(24, 30, 38);

    for (int32_t cat = 0; cat < 3; ++cat)
    {
        for (int32_t y = 0; y < CAT_BADGE_H; ++y)
        {
            for (int32_t x = 0; x < CAT_BADGE_W; ++x)
            {
                const size_t idx = (size_t)y * CAT_BADGE_W + (size_t)x;
                const bool cornerCut = ((x == 0 || x == CAT_BADGE_W - 1) && (y == 0 || y == CAT_BADGE_H - 1));
                if (cornerCut)
                {
                    sCategoryBadgeMask[cat][idx] = 0U;
                    sCategoryBadgePx[cat][idx] = 0U;
                    continue;
                }
                sCategoryBadgeMask[cat][idx] = 1U;
                const bool isBorder = (x == 0 || x == CAT_BADGE_W - 1 || y == 0 || y == CAT_BADGE_H - 1
                                    || ((x == 1 || x == CAT_BADGE_W - 2) && (y == 1 || y == CAT_BADGE_H - 2)));
                if (isBorder)
                {
                    sCategoryBadgePx[cat][idx] = border565;
                    continue;
                }

                const float dx = ((float)x - 15.5f);
                const float dy = ((float)y - 5.5f) * 1.45f;
                const float r = sqrtf(dx * dx + dy * dy);
                bool hitEmblem = false;
                if (cat == 0)
                {
                    const float ang = atan2f(dy, dx);
                    const float spikes = 3.6f + 2.6f * fabsf(cosf(ang * 4.0f));
                    hitEmblem = (r <= spikes);
                }
                else if (cat == 1)
                {
                    hitEmblem = (r <= 1.8f) || (r >= 3.3f && r <= 4.6f) || (r >= 6.0f && r <= 7.1f);
                }
                else
                {
                    hitEmblem = (r <= 5.4f && (dx + dy * 0.6f >= -0.4f || r <= 2.1f) && !(dx > 1.2f && r < 2.2f));
                }

                const VitaUiRgb c = hitEmblem
                    ? emblemCols[cat]
                    : (y < CAT_BADGE_H / 2 ? bgCols[cat] : VitaUi_ShadeRgb(bgCols[cat], 84));
                sCategoryBadgePx[cat][idx] = VitaUi_PackRgb565(c.r, c.g, c.b);
            }
        }
    }
}

void VitaUiArt_Init(void)
{
    if (sArtInit)
    {
        return;
    }
    sArtInit = true;
    memset(sFrontPics, 0, sizeof(sFrontPics));
    memset(sItemIcons, 0, sizeof(sItemIcons));
    memset(&sBagCache, 0, sizeof(sBagCache));
    memset(sSmoothCache, 0, sizeof(sSmoothCache));

    InitFonts();
    InitTrainerPics();
    InitTypeArtAndColours();
    BuildProceduralCategoryBadges();
}

void VitaUiArt_Shutdown(void)
{
    for (int32_t i = 0; i < SMOOTH_TEXT_SLOTS; ++i)
    {
        free(sSmoothCache[i].p);
        sSmoothCache[i].p = NULL;
    }
    free(sMoveTypeTiles);
    sMoveTypeTiles = NULL;
    sArtInit = false;
}

VitaUiRgb VitaUiArt_GetTypeRgb(const uint8_t moveType)
{
    if (!sArtInit) VitaUiArt_Init();
    if (moveType >= NUMBER_OF_MON_TYPES)
    {
        return (VitaUiRgb){150, 150, 150};
    }
    return sTypeColours[moveType];
}

static uint8_t MapAsciiCharToGba(const uint8_t ch)
{
    if (ch >= 'A' && ch <= 'Z') return (uint8_t)(CHAR_A + (ch - 'A'));
    if (ch >= 'a' && ch <= 'z') return (uint8_t)(CHAR_a + (ch - 'a'));
    if (ch >= '0' && ch <= '9') return (uint8_t)(CHAR_0 + (ch - '0'));
    switch (ch)
    {
    case ' ':  return CHAR_SPACE;
    case '!':  return CHAR_EXCL_MARK;
    case '?':  return CHAR_QUESTION_MARK;
    case '.':  return CHAR_PERIOD;
    case '-':  return CHAR_HYPHEN;
    case '/':  return CHAR_SLASH;
    case ',':  return CHAR_COMMA;
    case ':':  return CHAR_COLON;
    case '\'': return CHAR_SGL_QUOTE_RIGHT;
    case '"':  return CHAR_DBL_QUOTE_RIGHT;
    case '%':  return CHAR_PERCENT;
    case '+':  return CHAR_PLUS;
    case '(':  return CHAR_LEFT_PAREN;
    case ')':  return CHAR_RIGHT_PAREN;
    case '*':  return CHAR_MULT_SIGN;
    case '&':  return CHAR_AMPERSAND;
    case '=':  return CHAR_EQUALS;
    case ';':  return CHAR_SEMICOLON;
    case '_':  return CHAR_SPACE;
    case '[':  return CHAR_LEFT_PAREN;
    case ']':  return CHAR_RIGHT_PAREN;
    case '<':  return CHAR_LESS_THAN;
    case '>':  return CHAR_GREATER_THAN;
    default:   return CHAR_SPACE;
    }
}

void VitaUiArt_AsciiToGba(const char *const ascii, uint8_t *const outGba, const size_t cap)
{
    if (outGba == NULL || cap == 0U)
    {
        return;
    }
    if (ascii == NULL)
    {
        outGba[0] = EOS;
        return;
    }

    size_t s = 0U;
    size_t d = 0U;
    while (ascii[s] != '\0' && d + 1U < cap)
    {
        const uint8_t c0 = (uint8_t)ascii[s++];
        if (c0 == '\n')
        {
            outGba[d++] = CHAR_NEWLINE;
            continue;
        }
        // UTF-8 Latin-1 two-byte sequences (0xC3 xx and 0xC2 xx)
        if (c0 == 0xC3U && ascii[s] != '\0')
        {
            const uint8_t c1 = (uint8_t)ascii[s++];
            switch (c1)
            {
            case 0xA1U: outGba[d++] = CHAR_a_ACUTE; break; // á
            case 0xA9U: outGba[d++] = CHAR_e_ACUTE; break; // é
            case 0xADU: outGba[d++] = CHAR_i_ACUTE; break; // í
            case 0xB3U: outGba[d++] = CHAR_o_ACUTE; break; // ó
            case 0xBAU: outGba[d++] = CHAR_u_ACUTE; break; // ú
            case 0xB1U: outGba[d++] = 0x69U; break;        // ñ
            case 0x81U: outGba[d++] = CHAR_A_ACUTE; break; // Á
            case 0x89U: outGba[d++] = CHAR_E_ACUTE; break; // É
            case 0x8DU: outGba[d++] = CHAR_I_ACUTE; break; // Í
            case 0x93U: outGba[d++] = CHAR_O_ACUTE; break; // Ó
            case 0x9AU: outGba[d++] = CHAR_U_ACUTE; break; // Ú
            case 0x91U: outGba[d++] = 0x68U; break;        // Ñ
            case 0x97U: outGba[d++] = CHAR_MULT_SIGN; break; // ×
            default:    outGba[d++] = CHAR_SPACE; break;
            }
            continue;
        }
        if (c0 == 0xC2U && ascii[s] != '\0')
        {
            const uint8_t c1 = (uint8_t)ascii[s++];
            if (c1 == 0xA1U) outGba[d++] = CHAR_INV_EXCL_MARK;
            else if (c1 == 0xBFU) outGba[d++] = CHAR_INV_QUESTION_MARK;
            else if (c1 == 0xB7U) outGba[d++] = CHAR_PERIOD;
            else outGba[d++] = CHAR_SPACE;
            continue;
        }
        outGba[d++] = MapAsciiCharToGba(c0);
    }
    outGba[d] = EOS;
}

void VitaUiArt_GbaToAscii(const uint8_t *const gba, char *const outAscii, const size_t cap)
{
    if (outAscii == NULL || cap == 0U)
    {
        return;
    }
    if (gba == NULL)
    {
        outAscii[0] = '\0';
        return;
    }

    size_t s = 0U;
    size_t d = 0U;
    while (gba[s] != EOS && d + 1U < cap)
    {
        const uint8_t c = gba[s++];
        if (c == CHAR_NEWLINE || c == CHAR_PROMPT_SCROLL || c == CHAR_PROMPT_CLEAR)
        {
            outAscii[d++] = '\n';
            continue;
        }
        if (c == EXT_CTRL_CODE_BEGIN)
        {
            if (gba[s] != EOS) s += (size_t)GetExtCtrlCodeLength(gba[s]);
            continue;
        }
        if (c == PLACEHOLDER_BEGIN || c == CHAR_DYNAMIC || c == CHAR_KEYPAD_ICON || c == CHAR_EXTRA_SYMBOL)
        {
            if (gba[s] != EOS) s++;
            continue;
        }
        if (c >= CHAR_A && c <= CHAR_Z) { outAscii[d++] = (char)('A' + (c - CHAR_A)); continue; }
        if (c >= CHAR_a && c <= CHAR_z) { outAscii[d++] = (char)('a' + (c - CHAR_a)); continue; }
        if (c >= CHAR_0 && c <= CHAR_9) { outAscii[d++] = (char)('0' + (c - CHAR_0)); continue; }
        switch (c)
        {
        case CHAR_SPACE: outAscii[d++] = ' '; break;
        case CHAR_e_ACUTE: case CHAR_e_GRAVE: case CHAR_e_CIRCUMFLEX: case CHAR_e_DIAERESIS:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0xA9; } else outAscii[d++] = 'e';
            break;
        case CHAR_E_ACUTE: case CHAR_E_GRAVE: case CHAR_E_CIRCUMFLEX: case CHAR_E_DIAERESIS:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0x89; } else outAscii[d++] = 'E';
            break;
        case CHAR_a_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0xA1; } else outAscii[d++] = 'a';
            break;
        case CHAR_A_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0x81; } else outAscii[d++] = 'A';
            break;
        case CHAR_i_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0xAD; } else outAscii[d++] = 'i';
            break;
        case CHAR_I_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0x8D; } else outAscii[d++] = 'I';
            break;
        case CHAR_o_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0xB3; } else outAscii[d++] = 'o';
            break;
        case CHAR_O_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0x93; } else outAscii[d++] = 'O';
            break;
        case CHAR_u_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0xBA; } else outAscii[d++] = 'u';
            break;
        case CHAR_U_ACUTE:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0x9A; } else outAscii[d++] = 'U';
            break;
        case 0x68U:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0x91; } else outAscii[d++] = 'N';
            break;
        case 0x69U:
            if (d + 2U < cap) { outAscii[d++] = (char)0xC3; outAscii[d++] = (char)0xB1; } else outAscii[d++] = 'n';
            break;
        case CHAR_PK:
            outAscii[d++] = 'P';
            if (d + 1U < cap) outAscii[d++] = 'K';
            break;
        case CHAR_MN:
            outAscii[d++] = 'M';
            if (d + 1U < cap) outAscii[d++] = 'N';
            break;
        case CHAR_EXCL_MARK:     outAscii[d++] = '!'; break;
        case CHAR_QUESTION_MARK: outAscii[d++] = '?'; break;
        case CHAR_PERIOD: case CHAR_ELLIPSIS: outAscii[d++] = '.'; break;
        case CHAR_HYPHEN:        outAscii[d++] = '-'; break;
        case CHAR_SLASH:         outAscii[d++] = '/'; break;
        case CHAR_COMMA:         outAscii[d++] = ','; break;
        case CHAR_COLON:         outAscii[d++] = ':'; break;
        case CHAR_SGL_QUOTE_LEFT: case CHAR_SGL_QUOTE_RIGHT: outAscii[d++] = '\''; break;
        case CHAR_DBL_QUOTE_LEFT: case CHAR_DBL_QUOTE_RIGHT: outAscii[d++] = '"'; break;
        case CHAR_MALE:          outAscii[d++] = 'M'; break;
        case CHAR_FEMALE:        outAscii[d++] = 'F'; break;
        case CHAR_CURRENCY:      outAscii[d++] = '$'; break;
        case CHAR_MULT_SIGN:     outAscii[d++] = 'x'; break;
        case CHAR_PERCENT:       outAscii[d++] = '%'; break;
        case CHAR_PLUS:          outAscii[d++] = '+'; break;
        case CHAR_LEFT_PAREN:    outAscii[d++] = '('; break;
        case CHAR_RIGHT_PAREN:   outAscii[d++] = ')'; break;
        default:                 outAscii[d++] = ' '; break;
        }
    }
    outAscii[d] = '\0';
}

static uint16_t NextGbaGlyph(const uint8_t **const str)
{
    for (;;)
    {
        const uint8_t c = *(*str)++;
        switch (c)
        {
        case EOS:
            --(*str);
            return 0xFFFFU;
        case CHAR_NEWLINE:
        case CHAR_PROMPT_SCROLL:
        case CHAR_PROMPT_CLEAR:
            return 0xFFFEU;
        case EXT_CTRL_CODE_BEGIN:
            if (**str == EOS) return 0xFFFFU;
            *str += GetExtCtrlCodeLength(**str);
            break;
        case PLACEHOLDER_BEGIN:
        case CHAR_DYNAMIC:
        case CHAR_KEYPAD_ICON:
            if (**str != EOS) ++(*str);
            break;
        case CHAR_EXTRA_SYMBOL:
            if (**str == EOS) return 0xFFFFU;
            return (uint16_t)(0x100U | *(*str)++);
        default:
            return c;
        }
    }
}

static inline uint16_t GlyphBits(const FontDesc *const font, const uint16_t glyph, const int32_t row, const int32_t half)
{
    return font->glyphs[(size_t)glyph * 0x20U + (size_t)(row >= 8 ? 0x10 : 0) + (size_t)half * 8U + (size_t)(row & 7)];
}

static inline int32_t GlyphWidth(const FontDesc *const font, const uint16_t glyph)
{
    return (glyph < 512U) ? (int32_t)font->widths[glyph] : 6;
}

int32_t VitaUiArt_StrWidthGba(const enum VitaUiFontId fontId, const uint8_t *str)
{
    if (!sArtInit) VitaUiArt_Init();
    if (str == NULL || fontId >= VITA_FONT_COUNT)
    {
        return 0;
    }

    const FontDesc *const font = &sFonts[fontId];
    int32_t width = 0;
    int32_t best = 0;
    for (uint16_t g; (g = NextGbaGlyph(&str)) != 0xFFFFU;)
    {
        if (g == 0xFFFEU)
        {
            if (width > best) best = width;
            width = 0;
            continue;
        }
        width += GlyphWidth(font, g);
    }
    return (width > best) ? width : best;
}

int32_t VitaUiArt_StrWidthAscii(const enum VitaUiFontId fontId, const char *const asciiStr)
{
    uint8_t buf[160];
    VitaUiArt_AsciiToGba(asciiStr, buf, sizeof(buf));
    return VitaUiArt_StrWidthGba(fontId, buf);
}

static int32_t DrawSingleGlyph(
    uint16_t *const dst,
    const size_t pitch,
    const int32_t cx0,
    const int32_t cy0,
    const int32_t cx1,
    const int32_t cy1,
    const FontDesc *const font,
    const uint16_t glyph,
    const int32_t x,
    const int32_t y,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    const int32_t adv = GlyphWidth(font, glyph);
    if (x >= cx1 || y >= cy1 || x + adv <= cx0 || y + font->height <= cy0)
    {
        return adv;
    }

    const int32_t drawW = adv > 16 ? 16 : adv;
    const bool hasShadow = (shadow565OrNone != VITA_UI_NO_SHADOW);
    const uint16_t sh565 = (uint16_t)(shadow565OrNone & 0xFFFFU);

    for (int32_t row = 0; row < font->height; ++row)
    {
        const int32_t py = y + row;
        if (py < cy0 || py >= cy1)
        {
            continue;
        }
        uint16_t *const dstRow = &dst[(size_t)py * pitch];
        for (int32_t half = 0; half < 2 && half * 8 < drawW; ++half)
        {
            const uint16_t bits = GlyphBits(font, glyph, row, half);
            if (bits == 0U)
            {
                continue;
            }
            for (int32_t k = 0; k < 8 && half * 8 + k < drawW; ++k)
            {
                const uint8_t byte = (uint8_t)(k < 4 ? (bits >> 8) : (bits & 0xFFU));
                const uint8_t v = (uint8_t)((byte >> (6 - 2 * (k & 3))) & 3U);
                if (v != 1U && !(v == 2U && hasShadow))
                {
                    continue;
                }
                const int32_t px = x + half * 8 + k;
                if (px >= cx0 && px < cx1)
                {
                    dstRow[px] = (v == 1U) ? fg565 : sh565;
                }
            }
        }
    }
    return adv;
}

int32_t VitaUiArt_DrawStrGba(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const uint8_t *str,
    int32_t x,
    int32_t y,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    if (!sArtInit) VitaUiArt_Init();
    if (dst == NULL || str == NULL || fontId >= VITA_FONT_COUNT)
    {
        return x;
    }

    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;
    const FontDesc *const font = &sFonts[fontId];
    const int32_t left = x;

    for (uint16_t g; (g = NextGbaGlyph(&str)) != 0xFFFFU;)
    {
        if (g == 0xFFFEU)
        {
            x = left;
            y += font->lineHeight;
            continue;
        }
        x += DrawSingleGlyph(dst, pitch, cx0, cy0, cx1, cy1, font, g, x, y, fg565, shadow565OrNone);
    }
    return x;
}

int32_t VitaUiArt_DrawStrAscii(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const char *const asciiStr,
    const int32_t x,
    const int32_t y,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    uint8_t buf[160];
    VitaUiArt_AsciiToGba(asciiStr, buf, sizeof(buf));
    return VitaUiArt_DrawStrGba(dst, pitch, clip, fontId, buf, x, y, fg565, shadow565OrNone);
}

int32_t VitaUiArt_TextY(const int32_t py, const int32_t ph, const enum VitaUiFontId fontId)
{
    const bool isSmall = (fontId == VITA_FONT_SMALL || fontId == VITA_FONT_SMALL_NARROW);
    const int32_t k = isSmall ? ((ph - 8) / 2 - 4) : ((ph - 10) / 2 - 3);
    return py + k;
}

int32_t VitaUiArt_DrawStrGbaInPlateEx(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const uint8_t *const gbaStr,
    const int32_t px,
    const int32_t py,
    const int32_t pw,
    const int32_t ph,
    const char align,
    const int32_t pad,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    if (dst == NULL || gbaStr == NULL)
    {
        return px;
    }
    const int32_t advW = VitaUiArt_StrWidthGba(fontId, gbaStr);
    const int32_t inkW = advW > 1 ? (advW - 1) : advW;
    int32_t tx;
    if (align == 'c')
    {
        tx = px + (pw - inkW) / 2;
    }
    else if (align == 'r')
    {
        tx = px + pw - pad - inkW;
    }
    else
    {
        tx = px + pad;
    }
    const int32_t ty = VitaUiArt_TextY(py, ph, fontId);
    return VitaUiArt_DrawStrGba(dst, pitch, clip, fontId, gbaStr, tx, ty, fg565, shadow565OrNone);
}

int32_t VitaUiArt_DrawStrGbaInPlate(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const uint8_t *const gbaStr,
    const int32_t px,
    const int32_t py,
    const int32_t pw,
    const int32_t ph,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    return VitaUiArt_DrawStrGbaInPlateEx(dst, pitch, clip, fontId, gbaStr, px, py, pw, ph, 'c', 0, fg565, shadow565OrNone);
}

int32_t VitaUiArt_DrawStrAsciiInPlateEx(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const char *const asciiStr,
    const int32_t px,
    const int32_t py,
    const int32_t pw,
    const int32_t ph,
    const char align,
    const int32_t pad,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    uint8_t buf[160];
    VitaUiArt_AsciiToGba(asciiStr, buf, sizeof(buf));
    return VitaUiArt_DrawStrGbaInPlateEx(dst, pitch, clip, fontId, buf, px, py, pw, ph, align, pad, fg565, shadow565OrNone);
}

int32_t VitaUiArt_DrawStrAsciiInPlate(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const char *const asciiStr,
    const int32_t px,
    const int32_t py,
    const int32_t pw,
    const int32_t ph,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    return VitaUiArt_DrawStrAsciiInPlateEx(dst, pitch, clip, fontId, asciiStr, px, py, pw, ph, 'c', 0, fg565, shadow565OrNone);
}

void VitaUiArt_DrawStrAsciiRight(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const char *const asciiStr,
    const int32_t rightX,
    const int32_t y,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    uint8_t buf[160];
    VitaUiArt_AsciiToGba(asciiStr, buf, sizeof(buf));
    const int32_t w = VitaUiArt_StrWidthGba(fontId, buf);
    VitaUiArt_DrawStrGba(dst, pitch, clip, fontId, buf, rightX - w, y, fg565, shadow565OrNone);
}

void VitaUiArt_DrawStrAsciiCenter(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const char *const asciiStr,
    const int32_t centerX,
    const int32_t y,
    const uint16_t fg565,
    const uint32_t shadow565OrNone)
{
    uint8_t buf[160];
    VitaUiArt_AsciiToGba(asciiStr, buf, sizeof(buf));
    int32_t w = VitaUiArt_StrWidthGba(fontId, buf);
    if (w > 1)
    {
        w -= 1;
    }
    VitaUiArt_DrawStrGba(dst, pitch, clip, fontId, buf, centerX - w / 2, y, fg565, shadow565OrNone);
}

int32_t VitaUiArt_SmoothWidthGba(const enum VitaUiFontId fontId, const uint8_t *const gbaStr, const int32_t scaleQuarter)
{
    const int32_t k = scaleQuarter < 4 ? 4 : scaleQuarter;
    return (VitaUiArt_StrWidthGba(fontId, gbaStr) * k + 2) >> 2;
}

static bool BuildSmoothSlot(
    SmoothSlot *const slot,
    const enum VitaUiFontId fontId,
    const uint8_t *const gbaStr,
    const size_t keyLen,
    const int32_t k,
    const VitaUiRgb fg,
    const VitaUiRgb outline,
    const bool dropShadow)
{
    const FontDesc *const font = &sFonts[fontId];
    const int32_t baseW = VitaUiArt_StrWidthGba(fontId, gbaStr);
    const int32_t w4 = (baseW + SMOOTH_PAD * 2) * k;
    const int32_t h4 = (font->height + SMOOTH_PAD * 2) * k;
    const int32_t outW = (w4 + 3) >> 2;
    const int32_t outH = (h4 + 3) >> 2;
    if (outW <= 0 || outH <= 0)
    {
        return false;
    }

    const size_t hiArea = (size_t)w4 * (size_t)h4;
    uint8_t *const glyphHi = (uint8_t *)calloc(hiArea, 2U);
    if (glyphHi == NULL)
    {
        return false;
    }
    uint8_t *const ringHi = glyphHi + hiArea;

    const uint8_t *scan = gbaStr;
    int32_t gx = SMOOTH_PAD;
    for (uint16_t g; (g = NextGbaGlyph(&scan)) != 0xFFFFU && g != 0xFFFEU;)
    {
        const int32_t gw = GlyphWidth(font, g) > 16 ? 16 : GlyphWidth(font, g);
        for (int32_t row = 0; row < font->height; ++row)
        {
            for (int32_t px = 0; px < gw; ++px)
            {
                const uint16_t bits = GlyphBits(font, g, row, px / 8);
                const uint8_t byte = (uint8_t)((px & 7) < 4 ? (bits >> 8) : (bits & 0xFFU));
                if (((byte >> (6 - 2 * (px & 3))) & 3U) != 1U)
                {
                    continue;
                }
                for (int32_t dy = 0; dy < k; ++dy)
                {
                    memset(&glyphHi[((SMOOTH_PAD + row) * k + dy) * w4 + (gx + px) * k], 255, (size_t)k);
                }
            }
        }
        gx += GlyphWidth(font, g);
    }

    const int32_t rad = (k >= 6) ? 4 : 3;
    for (int32_t y = 0; y < h4; ++y)
    {
        for (int32_t x = 0; x < w4; ++x)
        {
            if (glyphHi[y * w4 + x] == 0U)
            {
                continue;
            }
            const int32_t y0 = (y - rad > 0) ? (y - rad) : 0;
            const int32_t y1 = (y + rad < h4 - 1) ? (y + rad) : (h4 - 1);
            const int32_t x0 = (x - rad > 0) ? (x - rad) : 0;
            const int32_t x1 = (x + rad < w4 - 1) ? (x + rad) : (w4 - 1);
            for (int32_t ny = y0; ny <= y1; ++ny)
            {
                const int32_t dy = ny - y;
                for (int32_t nx = x0; nx <= x1; ++nx)
                {
                    const int32_t dx = nx - x;
                    if (dx * dx + dy * dy <= rad * rad)
                    {
                        ringHi[ny * w4 + nx] = 255U;
                    }
                }
            }
        }
    }

    const size_t outArea = (size_t)outW * (size_t)outH;
    free(slot->p);
    slot->p = (uint16_t *)malloc(outArea * sizeof(uint16_t) + outArea);
    if (slot->p == NULL)
    {
        free(glyphHi);
        return false;
    }
    slot->a = (uint8_t *)(slot->p + outArea);
    slot->w = (uint16_t)outW;
    slot->h = (uint16_t)outH;
    slot->fontId = (uint8_t)fontId;
    slot->scaleQ = (uint8_t)k;
    slot->dropShadow = dropShadow;
    slot->fg = fg;
    slot->outline = outline;
    memcpy(slot->key, gbaStr, keyLen);
    slot->key[keyLen] = EOS;

    for (int32_t oy = 0; oy < outH; ++oy)
    {
        for (int32_t ox = 0; ox < outW; ++ox)
        {
            int32_t sumFg = 0;
            int32_t sumRing = 0;
            int32_t sumSh = 0;
            for (int32_t sy = 0; sy < 4; ++sy)
            {
                const int32_t hy = oy * 4 + sy;
                if (hy >= h4) continue;
                for (int32_t sx = 0; sx < 4; ++sx)
                {
                    const int32_t hx = ox * 4 + sx;
                    if (hx >= w4) continue;
                    sumFg += glyphHi[hy * w4 + hx];
                    sumRing += ringHi[hy * w4 + hx];
                    if (dropShadow && hy >= 4 && hx >= 2)
                    {
                        sumSh += ringHi[(hy - 4) * w4 + (hx - 2)];
                    }
                }
            }
            const float aFg = (float)sumFg / (16.0f * 255.0f);
            const float aRing = (float)sumRing / (16.0f * 255.0f);
            const float aSh = dropShadow ? ((float)sumSh / (16.0f * 255.0f)) * 0.42f : 0.0f;

            float r = (float)outline.r * 0.35f * aSh;
            float g = (float)outline.g * 0.35f * aSh;
            float b = (float)outline.b * 0.35f * aSh;
            float a = aSh;

            if (aRing > 0.002f)
            {
                const float inv = 1.0f - aRing;
                r = r * inv + (float)outline.r * aRing;
                g = g * inv + (float)outline.g * aRing;
                b = b * inv + (float)outline.b * aRing;
                a = a * inv + aRing;
            }
            if (aFg > 0.002f)
            {
                const float inv = 1.0f - aFg;
                r = r * inv + (float)fg.r * aFg;
                g = g * inv + (float)fg.g * aFg;
                b = b * inv + (float)fg.b * aFg;
                a = a * inv + aFg;
            }

            const int32_t al = (int32_t)(a * 255.0f + 0.5f);
            const size_t idx = (size_t)oy * (size_t)outW + (size_t)ox;
            slot->a[idx] = (uint8_t)(al > 255 ? 255 : al);
            slot->p[idx] = (al > 0) ? VitaUi_PackRgb565((int32_t)(r + 0.5f), (int32_t)(g + 0.5f), (int32_t)(b + 0.5f)) : 0U;
        }
    }

    free(glyphHi);
    return true;
}

void VitaUiArt_DrawSmoothGba(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const uint8_t *const gbaStr,
    const int32_t x,
    const int32_t y,
    const int32_t scaleQuarter,
    const VitaUiRgb fg,
    const VitaUiRgb outline,
    const bool dropShadow)
{
    if (!sArtInit) VitaUiArt_Init();
    if (dst == NULL || gbaStr == NULL || fontId >= VITA_FONT_COUNT)
    {
        return;
    }

    const int32_t k = scaleQuarter < 4 ? 4 : (scaleQuarter > 12 ? 12 : scaleQuarter);
    size_t keyLen = 0U;
    while (gbaStr[keyLen] != EOS && keyLen + 1U < SMOOTH_KEY_CAP)
    {
        keyLen++;
    }

    SmoothSlot *found = NULL;
    SmoothSlot *victim = &sSmoothCache[0];
    for (int32_t i = 0; i < SMOOTH_TEXT_SLOTS; ++i)
    {
        SmoothSlot *const s = &sSmoothCache[i];
        if (s->p != NULL && s->fontId == (uint8_t)fontId && s->scaleQ == (uint8_t)k && s->dropShadow == dropShadow
            && s->fg.r == fg.r && s->fg.g == fg.g && s->fg.b == fg.b
            && s->outline.r == outline.r && s->outline.g == outline.g && s->outline.b == outline.b
            && memcmp(s->key, gbaStr, keyLen) == 0 && s->key[keyLen] == EOS)
        {
            found = s;
            break;
        }
        if (s->age < victim->age)
        {
            victim = s;
        }
    }

    if (found == NULL)
    {
        if (!BuildSmoothSlot(victim, fontId, gbaStr, keyLen, k, fg, outline, dropShadow))
        {
            return;
        }
        found = victim;
    }
    found->age = ++sArtClock;

    const int32_t padPx = (SMOOTH_PAD * k) >> 2;
    const int32_t bx = x - padPx;
    const int32_t by = y - padPx;
    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;

    const int32_t i0 = (cx0 - bx > 0) ? (cx0 - bx) : 0;
    const int32_t j0 = (cy0 - by > 0) ? (cy0 - by) : 0;
    const int32_t i1 = (cx1 - bx < (int32_t)found->w) ? (cx1 - bx) : (int32_t)found->w;
    const int32_t j1 = (cy1 - by < (int32_t)found->h) ? (cy1 - by) : (int32_t)found->h;

    for (int32_t j = j0; j < j1; ++j)
    {
        uint16_t *const dstRow = &dst[(size_t)(by + j) * pitch + (size_t)bx];
        const uint16_t *const pRow = &found->p[(size_t)j * found->w];
        const uint8_t *const aRow = &found->a[(size_t)j * found->w];
        for (int32_t i = i0; i < i1; ++i)
        {
            const int32_t al = aRow[i];
            if (al == 0) continue;
            if (al == 255)
            {
                dstRow[i] = pRow[i];
                continue;
            }
            const VitaUiRgb under = VitaUi_UnpackRgb565(dstRow[i]);
            const VitaUiRgb add = VitaUi_UnpackRgb565(pRow[i]);
            dstRow[i] = VitaUi_PackRgb565(
                Div255((int32_t)under.r * (255 - al)) + (int32_t)add.r,
                Div255((int32_t)under.g * (255 - al)) + (int32_t)add.g,
                Div255((int32_t)under.b * (255 - al)) + (int32_t)add.b);
        }
    }
}

void VitaUiArt_DrawSmoothAscii(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const enum VitaUiFontId fontId,
    const char *const asciiStr,
    const int32_t x,
    const int32_t y,
    const int32_t scaleQuarter,
    const VitaUiRgb fg,
    const VitaUiRgb outline,
    const bool dropShadow)
{
    uint8_t buf[SMOOTH_KEY_CAP];
    VitaUiArt_AsciiToGba(asciiStr, buf, sizeof(buf));
    VitaUiArt_DrawSmoothGba(dst, pitch, clip, fontId, buf, x, y, scaleQuarter, fg, outline, dropShadow);
}

void VitaUiArt_FillRect(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const int32_t x,
    const int32_t y,
    const int32_t w,
    const int32_t h,
    const uint16_t col565)
{
    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;

    const int32_t x0 = x > cx0 ? x : cx0;
    const int32_t y0 = y > cy0 ? y : cy0;
    const int32_t x1 = (x + w) < cx1 ? (x + w) : cx1;
    const int32_t y1 = (y + h) < cy1 ? (y + h) : cy1;
    if (x0 >= x1 || y0 >= y1)
    {
        return;
    }

    for (int32_t py = y0; py < y1; ++py)
    {
        uint16_t *const row = &dst[(size_t)py * pitch];
        for (int32_t px = x0; px < x1; ++px)
        {
            row[px] = col565;
        }
    }
}

void VitaUiArt_DrawTiles4bpp(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t *const tiles,
    const int32_t tilesW,
    const int32_t tilesH,
    const int32_t x,
    const int32_t y,
    const uint16_t *const pal565)
{
    if (dst == NULL || tiles == NULL || pal565 == NULL)
    {
        return;
    }
    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;
    if (x >= cx1 || y >= cy1 || x + tilesW * 8 <= cx0 || y + tilesH * 8 <= cy0)
    {
        return;
    }

    for (int32_t ty = 0; ty < tilesH; ++ty)
    {
        for (int32_t tx = 0; tx < tilesW; ++tx)
        {
            const uint8_t *const tile = &tiles[(ty * tilesW + tx) * 32];
            for (int32_t py = 0; py < 8; ++py)
            {
                const int32_t sy = y + ty * 8 + py;
                if (sy < cy0 || sy >= cy1) continue;
                uint16_t *const dstRow = &dst[(size_t)sy * pitch];
                for (int32_t px = 0; px < 8; ++px)
                {
                    const int32_t sx = x + tx * 8 + px;
                    if (sx < cx0 || sx >= cx1) continue;
                    const uint8_t byte = tile[py * 4 + (px >> 1)];
                    const uint8_t idx = (uint8_t)(((px & 1) != 0) ? (byte >> 4) : (byte & 0x0FU));
                    if (idx != 0U)
                    {
                        dstRow[sx] = pal565[idx];
                    }
                }
            }
        }
    }
}

void VitaUiArt_DrawSpriteCentred(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t *const tiles,
    const int32_t tilesW,
    const int32_t tilesH,
    const int32_t cx,
    const int32_t cy,
    const uint16_t *const pal565)
{
    if (dst == NULL || tiles == NULL || pal565 == NULL || tilesW <= 0 || tilesH <= 0)
        return;

    int32_t x0 = tilesW * 8;
    int32_t y0 = tilesH * 8;
    int32_t x1 = 0;
    int32_t y1 = 0;

    for (int32_t y = 0; y < tilesH * 8; ++y)
    {
        for (int32_t tx = 0; tx < tilesW; ++tx)
        {
            const uint8_t *const row = tiles + ((y / 8) * tilesW + tx) * 32 + (y & 7) * 4;
            const uint32_t bits = (uint32_t)row[0] | ((uint32_t)row[1] << 8) | ((uint32_t)row[2] << 16) | ((uint32_t)row[3] << 24);
            if (!bits)
                continue;
            const int32_t first = tx * 8 + __builtin_ctz(bits) / 4;
            const int32_t last = tx * 8 + (31 - __builtin_clz(bits)) / 4;
            if (first < x0) x0 = first;
            if (y < y0) y0 = y;
            if (last + 1 > x1) x1 = last + 1;
            if (y + 1 > y1) y1 = y + 1;
        }
    }

    if (x1 > x0 && y1 > y0)
    {
        const int32_t drawX = cx - (x0 + x1) / 2;
        const int32_t drawY = cy - (y0 + y1) / 2;
        VitaUiArt_DrawTiles4bpp(dst, pitch, clip, tiles, tilesW, tilesH, drawX, drawY, pal565);
    }
}

void VitaUiArt_DrawTiles4bppBgrPal(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t *const tiles,
    const int32_t tilesW,
    const int32_t tilesH,
    const int32_t x,
    const int32_t y,
    const uint16_t *const palBgr555)
{
    if (palBgr555 == NULL) return;
    uint16_t pal565[16];
    ConvertPal16(palBgr555, pal565);
    VitaUiArt_DrawTiles4bpp(dst, pitch, clip, tiles, tilesW, tilesH, x, y, pal565);
}

static const FrontPicSlot *GetFrontPicSlot(const uint16_t species, const bool shiny)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
    {
        return NULL;
    }

    FrontPicSlot *victim = &sFrontPics[0];
    for (int32_t i = 0; i < FRONT_PIC_SLOTS; ++i)
    {
        FrontPicSlot *const s = &sFrontPics[i];
        if (s->loaded && s->species == species && s->shiny == shiny)
        {
            s->age = ++sArtClock;
            return s;
        }
        if (s->age < victim->age)
        {
            victim = s;
        }
    }

    static uint8_t sMonDecompBuffer[8192];
    static uint16_t sPalDecompBuffer[256];

    LZ77UnCompWram(gMonStillFrontPicTable[species].data, sMonDecompBuffer);
    memcpy(victim->tiles, sMonDecompBuffer, sizeof(victim->tiles));

    LZ77UnCompWram((shiny ? gMonShinyPaletteTable : gMonPaletteTable)[species].data, sPalDecompBuffer);
    ConvertPal16(sPalDecompBuffer, victim->pal565);
    victim->species = species;
    victim->shiny = shiny;
    victim->loaded = true;
    victim->age = ++sArtClock;
    return victim;
}

void VitaUiArt_DrawMonFrontPic(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint16_t species,
    const bool shiny,
    const int32_t x,
    const int32_t y,
    const int32_t targetSize,
    const bool silhouette)
{
    if (!sArtInit) VitaUiArt_Init();
    const FrontPicSlot *const slot = GetFrontPicSlot(species, shiny);
    if (slot == NULL || targetSize <= 0)
    {
        return;
    }

    uint16_t silPal[16];
    const uint16_t *pal = slot->pal565;
    if (silhouette)
    {
        const uint16_t silCol = VitaUi_PackRgb565(24, 52, 44);
        for (int32_t i = 0; i < 16; ++i) silPal[i] = silCol;
        pal = silPal;
    }

    if (targetSize == 64)
    {
        VitaUiArt_DrawTiles4bpp(dst, pitch, clip, slot->tiles, 8, 8, x, y, pal);
        return;
    }

    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;

    for (int32_t dy = 0; dy < targetSize; ++dy)
    {
        const int32_t py = y + dy;
        if (py < cy0 || py >= cy1) continue;
        const int32_t sy = (dy * 64) / targetSize;
        const int32_t ty = sy >> 3;
        const int32_t inY = sy & 7;
        uint16_t *const dstRow = &dst[(size_t)py * pitch];

        for (int32_t dx = 0; dx < targetSize; ++dx)
        {
            const int32_t px = x + dx;
            if (px < cx0 || px >= cx1) continue;
            const int32_t sx = (dx * 64) / targetSize;
            const uint8_t *const tile = &slot->tiles[((ty << 3) + (sx >> 3)) * 32];
            const uint8_t byte = tile[inY * 4 + ((sx & 7) >> 1)];
            const uint8_t idx = (uint8_t)(((sx & 1) != 0) ? (byte >> 4) : (byte & 0x0FU));
            if (idx != 0U)
            {
                dstRow[px] = pal[idx];
            }
        }
    }
}

void VitaUiArt_DrawMonIcon(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint16_t species,
    const uint8_t frame,
    const int32_t x,
    const int32_t y,
    const int32_t scale)
{
    if (!sArtInit) VitaUiArt_Init();
    if (species == SPECIES_NONE || species >= NUM_SPECIES || scale <= 0)
    {
        return;
    }

    const uint8_t *const baseTiles = gMonIconTable[species];
    const uint16_t *const bgrPal = GetValidMonIconPalettePtr(species);
    if (baseTiles == NULL || bgrPal == NULL)
    {
        return;
    }

    const uint8_t *const tiles = baseTiles + ((size_t)(frame & 1U) * 512U);
    uint16_t pal565[16];
    ConvertPal16(bgrPal, pal565);

    if (scale == 1)
    {
        VitaUiArt_DrawTiles4bpp(dst, pitch, clip, tiles, 4, 4, x, y, pal565);
        return;
    }

    const int32_t outSize = 32 * scale;
    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;

    for (int32_t dy = 0; dy < outSize; ++dy)
    {
        const int32_t py = y + dy;
        if (py < cy0 || py >= cy1) continue;
        const int32_t sy = dy / scale;
        uint16_t *const dstRow = &dst[(size_t)py * pitch];
        for (int32_t dx = 0; dx < outSize; ++dx)
        {
            const int32_t px = x + dx;
            if (px < cx0 || px >= cx1) continue;
            const int32_t sx = dx / scale;
            const uint8_t *const tile = &tiles[((sy >> 3) * 4 + (sx >> 3)) * 32];
            const uint8_t byte = tile[(sy & 7) * 4 + ((sx & 7) >> 1)];
            const uint8_t idx = (uint8_t)(((sx & 1) != 0) ? (byte >> 4) : (byte & 0x0FU));
            if (idx != 0U)
            {
                dstRow[px] = pal565[idx];
            }
        }
    }
}

void VitaUiArt_DrawMonIconCentred(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint16_t species,
    const uint8_t frame,
    const int32_t cx,
    const int32_t cy)
{
    if (!sArtInit) VitaUiArt_Init();
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
    {
        return;
    }
    const uint8_t *const baseTiles = gMonIconTable[species];
    const uint16_t *const bgrPal = GetValidMonIconPalettePtr(species);
    if (baseTiles == NULL || bgrPal == NULL)
    {
        return;
    }
    const uint8_t *const tiles = baseTiles + ((size_t)(frame & 1U) * 512U);
    uint16_t pal565[16];
    ConvertPal16(bgrPal, pal565);
    VitaUiArt_DrawSpriteCentred(dst, pitch, clip, tiles, 4, 4, cx, cy, pal565);
}

static void MakeGreyPal16(const uint16_t *const src, uint16_t *const dst)
{
    dst[0] = 0;
    for (int32_t i = 1; i < 16; ++i)
    {
        const VitaUiRgb c = VitaUi_UnpackRgb565(src[i]);
        const uint8_t lum = (uint8_t)(((uint32_t)c.r * 30U + (uint32_t)c.g * 59U + (uint32_t)c.b * 11U) / 100U);
        dst[i] = VitaUi_PackRgb565(lum, lum, lum);
    }
}

static const ItemIconSlot *GetItemIconSlot(const uint16_t itemId)
{
    ItemIconSlot *victim = &sItemIcons[0];
    for (int32_t i = 0; i < ITEM_ICON_SLOTS; ++i)
    {
        ItemIconSlot *const s = &sItemIcons[i];
        if (s->loaded && s->itemId == itemId)
        {
            s->age = ++sArtClock;
            return s;
        }
        if (s->age < victim->age)
        {
            victim = s;
        }
    }

    const void *const picGfx = GetItemIconPicOrPalette(itemId, 0);
    const void *const palGfx = GetItemIconPicOrPalette(itemId, 1);
    if (picGfx == NULL || palGfx == NULL)
    {
        return NULL;
    }

    static uint8_t rawTiles[512];
    static uint16_t rawPal[16];
    memset(rawTiles, 0, sizeof(rawTiles));
    memset(rawPal, 0, sizeof(rawPal));
    LZ77UnCompWram(picGfx, rawTiles);
    LZ77UnCompWram(palGfx, rawPal);

    // Raw GBA item icons are decompressed directly as 3x3 contiguous 4bpp tiles (288 bytes)
    memcpy(victim->tiles, rawTiles, 9U * 32U);
    ConvertPal16(rawPal, victim->pal565);
    victim->itemId = itemId;
    victim->loaded = true;
    victim->age = ++sArtClock;
    return victim;
}

void VitaUiArt_DrawItemIcon(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint16_t itemId,
    const int32_t x,
    const int32_t y,
    const bool shadowOnly)
{
    if (!sArtInit) VitaUiArt_Init();
    const ItemIconSlot *const slot = GetItemIconSlot(itemId);
    if (slot == NULL)
    {
        return;
    }
    if (!shadowOnly)
    {
        VitaUiArt_DrawTiles4bpp(dst, pitch, clip, slot->tiles, 3, 3, x, y, slot->pal565);
        return;
    }

    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;

    for (int32_t ty = 0; ty < 3; ++ty)
    {
        for (int32_t tx = 0; tx < 3; ++tx)
        {
            const uint8_t *const tile = &slot->tiles[(ty * 3 + tx) * 32];
            for (int32_t py = 0; py < 8; ++py)
            {
                const int32_t sy = y + ty * 8 + py;
                if (sy < cy0 || sy >= cy1) continue;
                uint16_t *const dstRow = &dst[(size_t)sy * pitch];
                for (int32_t px = 0; px < 8; ++px)
                {
                    const int32_t sx = x + tx * 8 + px;
                    if (sx < cx0 || sx >= cx1) continue;
                    const uint8_t byte = tile[py * 4 + (px >> 1)];
                    const uint8_t idx = (uint8_t)(((px & 1) != 0) ? (byte >> 4) : (byte & 0x0FU));
                    if (idx != 0U)
                    {
                        const VitaUiRgb u = VitaUi_UnpackRgb565(dstRow[sx]);
                        dstRow[sx] = VitaUi_PackRgb565(u.r * 62 / 100, u.g * 62 / 100, u.b * 62 / 100);
                    }
                }
            }
        }
    }
}

void VitaUiArt_DrawItemIconCentredEx(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint16_t itemId,
    const int32_t cx,
    const int32_t cy,
    const bool grey)
{
    if (!sArtInit) VitaUiArt_Init();
    const ItemIconSlot *const slot = GetItemIconSlot(itemId);
    if (slot == NULL)
    {
        return;
    }
    if (grey)
    {
        uint16_t gpal[16];
        MakeGreyPal16(slot->pal565, gpal);
        VitaUiArt_DrawSpriteCentred(dst, pitch, clip, slot->tiles, 3, 3, cx, cy, gpal);
    }
    else
    {
        VitaUiArt_DrawSpriteCentred(dst, pitch, clip, slot->tiles, 3, 3, cx, cy, slot->pal565);
    }
}

void VitaUiArt_DrawItemIconCentred(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint16_t itemId,
    const int32_t cx,
    const int32_t cy)
{
    VitaUiArt_DrawItemIconCentredEx(dst, pitch, clip, itemId, cx, cy, false);
}

void VitaUiArt_DrawDockIcon(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const int32_t section,
    const bool enabled,
    const int32_t cx,
    const int32_t cy)
{
    switch ((enum VitaUiSection)section)
    {
    case VITA_UI_SEC_MAP:
        VitaUiArt_DrawItemIconCentredEx(dst, pitch, clip, ITEM_TOWN_MAP, cx, cy, !enabled);
        break;
    case VITA_UI_SEC_POKEDEX:
    {
        uint16_t pal[16];
        ConvertPal16(sArtPalette, pal);
        if (!enabled)
        {
            uint16_t gpal[16];
            MakeGreyPal16(pal, gpal);
            VitaUiArt_DrawSpriteCentred(dst, pitch, clip, sArtPokedex, 3, 3, cx, cy, gpal);
        }
        else
        {
            VitaUiArt_DrawSpriteCentred(dst, pitch, clip, sArtPokedex, 3, 3, cx, cy, pal);
        }
        break;
    }
    case VITA_UI_SEC_POKEMON:
        VitaUiArt_DrawItemIconCentredEx(dst, pitch, clip, ITEM_POKE_BALL, cx, cy, !enabled);
        break;
    case VITA_UI_SEC_BAG:
        VitaUiArt_DrawItemIconCentredEx(dst, pitch, clip, ITEM_BERRY_POUCH, cx, cy, !enabled);
        break;
    case VITA_UI_SEC_POKENAV:
    {
        uint16_t pal[16];
        ConvertPal16(sPokenavPal, pal);
        if (!enabled)
        {
            uint16_t gpal[16];
            MakeGreyPal16(pal, gpal);
            VitaUiArt_DrawSpriteCentred(dst, pitch, clip, sPokenavGfx, 4, 4, cx, cy, gpal);
        }
        else
        {
            VitaUiArt_DrawSpriteCentred(dst, pitch, clip, sPokenavGfx, 4, 4, cx, cy, pal);
        }
        break;
    }
    case VITA_UI_SEC_CARD:
        VitaUiArt_DrawItemIconCentredEx(dst, pitch, clip, ITEM_CONTEST_PASS, cx, cy, !enabled);
        break;
    case VITA_UI_SEC_SAVE:
        VitaUiArt_DrawItemIconCentredEx(dst, pitch, clip, ITEM_LETTER, cx, cy, !enabled);
        break;
    case VITA_UI_SEC_OPTIONS:
        VitaUiArt_DrawItemIconCentredEx(dst, pitch, clip, ITEM_TEACHY_TV, cx, cy, !enabled);
        break;
    case VITA_UI_SEC_NUZLOCKE:
    {
        uint16_t pal[16];
        ConvertPal16(sArtPalette2, pal);
        if (!enabled)
        {
            uint16_t gpal[16];
            MakeGreyPal16(pal, gpal);
            VitaUiArt_DrawSpriteCentred(dst, pitch, clip, sArtNuzlocke, 3, 3, cx, cy, gpal);
        }
        else
        {
            VitaUiArt_DrawSpriteCentred(dst, pitch, clip, sArtNuzlocke, 3, 3, cx, cy, pal);
        }
        break;
    }
    default:
        break;
    }
}

void VitaUiArt_DrawBagPic(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const bool female,
    const uint8_t pocketFrame,
    const int32_t x,
    const int32_t y)
{
    if (!sArtInit) VitaUiArt_Init();
    const uint8_t clampedFrame = pocketFrame > 5U ? 0U : pocketFrame;

    if (!sBagCache.loaded || sBagCache.female != female || sBagCache.frame != clampedFrame)
    {
        static uint8_t sDecompBagSheet[8 * 2048];
        uint16_t rawPal[16];
        LZ77UnCompWram(female ? gBagFemaleTiles : gBagMaleTiles, sDecompBagSheet);
        LZ77UnCompWram(gBagPalette, rawPal);
        memcpy(sBagCache.tiles, &sDecompBagSheet[(size_t)clampedFrame * sizeof(sBagCache.tiles)], sizeof(sBagCache.tiles));
        ConvertPal16(rawPal, sBagCache.pal565);
        sBagCache.female = female;
        sBagCache.frame = clampedFrame;
        sBagCache.loaded = true;
    }

    VitaUiArt_DrawTiles4bpp(dst, pitch, clip, sBagCache.tiles, 8, 8, x, y, sBagCache.pal565);
}

void VitaUiArt_DrawTrainerFrontPic(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t gender,
    const int32_t x,
    const int32_t y)
{
    if (!sArtInit) VitaUiArt_Init();
    const uint8_t g = (gender == FEMALE) ? FEMALE : MALE;
    if (sTrainerPicValid[g])
    {
        VitaUiArt_DrawTiles4bpp(dst, pitch, clip, sTrainerPicTiles[g], 8, 8, x, y, sTrainerPicPal565[g]);
    }
}

void VitaUiArt_DrawTypeBadge(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint8_t moveType,
    const int32_t x,
    const int32_t y)
{
    if (!sArtInit) VitaUiArt_Init();
    if (sMoveTypeTiles == NULL || moveType >= NUMBER_OF_MON_TYPES)
    {
        return;
    }
    VitaUiArt_DrawTiles4bpp(
        dst, pitch, clip,
        &sMoveTypeTiles[(size_t)moveType * 8U * 32U],
        4, 2, x, y,
        sMoveTypePals565[sTypeToPalIdx[moveType]]);
}

void VitaUiArt_DrawCategoryBadge(
    uint16_t *const dst,
    const size_t pitch,
    const VitaUiClip *const clip,
    const uint16_t moveId,
    const int32_t x,
    const int32_t y)
{
    if (!sArtInit) VitaUiArt_Init();
    if (dst == NULL || moveId == MOVE_NONE || moveId >= MOVES_COUNT)
    {
        return;
    }

    uint8_t cat = 2U;
    if (gBattleMoves[moveId].power != 0U)
    {
        if (VitaExtras_GetFast(FAST_EXTRA_MOVE_SPLIT) != 0)
        {
            cat = gMoveCategories[moveId];
        }
        else
        {
            cat = IS_TYPE_PHYSICAL(gBattleMoves[moveId].type) ? 0U : 1U;
        }
    }
    if (cat > 2U) cat = 2U;

    const int32_t cx0 = (clip != NULL && clip->x0 > 0) ? clip->x0 : 0;
    const int32_t cy0 = (clip != NULL && clip->y0 > 0) ? clip->y0 : 0;
    const int32_t cx1 = (clip != NULL && clip->x1 < VITA_UI_WIDTH) ? clip->x1 : VITA_UI_WIDTH;
    const int32_t cy1 = (clip != NULL && clip->y1 < VITA_UI_HEIGHT) ? clip->y1 : VITA_UI_HEIGHT;

    for (int32_t dy = 0; dy < CAT_BADGE_H; ++dy)
    {
        const int32_t py = y + dy;
        if (py < cy0 || py >= cy1) continue;
        uint16_t *const dstRow = &dst[(size_t)py * pitch];
        for (int32_t dx = 0; dx < CAT_BADGE_W; ++dx)
        {
            const int32_t px = x + dx;
            if (px < cx0 || px >= cx1) continue;
            const size_t idx = (size_t)dy * CAT_BADGE_W + (size_t)dx;
            if (sCategoryBadgeMask[cat][idx] != 0U)
            {
                dstRow[px] = sCategoryBadgePx[cat][idx];
            }
        }
    }
}

void VitaUiArt_Prewarm(void)
{
    if (!sArtInit)
        VitaUiArt_Init();

    // Pre-cache persistent dock item icons so they are never decompressed in runtime
    GetItemIconSlot(ITEM_TOWN_MAP);
    GetItemIconSlot(ITEM_POKE_BALL);
    GetItemIconSlot(ITEM_BERRY_POUCH);
    GetItemIconSlot(ITEM_CONTEST_PASS);
    GetItemIconSlot(ITEM_LETTER);
    GetItemIconSlot(ITEM_TEACHY_TV);
}

void VitaUiArt_PrewarmPartyPics(void)
{
    if (!sArtInit)
        VitaUiArt_Init();

    const int32_t count = (int32_t)CalculatePlayerPartyCount();
    for (int32_t i = 0; i < count; ++i)
    {
        struct Pokemon *const mon = &gPlayerParty[i];
        if (GetMonData(mon, MON_DATA_IS_EGG) == 0)
        {
            const uint16_t species = (uint16_t)GetMonData(mon, MON_DATA_SPECIES);
            if (species != SPECIES_NONE && species < NUM_SPECIES)
            {
                GetFrontPicSlot(species, (bool)IsMonShiny(mon));
            }
        }
    }
}
