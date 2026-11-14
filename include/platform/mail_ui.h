#ifndef GUARD_PLATFORM_MAIL_UI_H
#define GUARD_PLATFORM_MAIL_UI_H

// A mail as the game's ReadMail lays it out (mail.c, Mail_LayoutForUi), for the new UI.

#include <stdint.h>
struct MailUiLine
{
    uint8_t text[64];   // game charset
    uint8_t x, y;       // in window 0 (screen 16, 24), GBA pixels
};

struct MailUiLayout
{
    uint8_t mailType;
    const uint32_t *tiles;      // LZ77, 4bpp
    const uint32_t *tilemap;    // LZ77, 32 x 20 entries
    uint16_t palette[16];       // with the player's two frame colours
    uint16_t textColor;
    uint8_t numLines;
    struct MailUiLine lines[8];
    uint8_t signature[32];      // "From " + the sender (game charset)
    uint8_t signatureWidth;     // centred in this width, from x 104 (window 0)
    uint8_t signatureY;         // window 0
    uint16_t iconSpecies;       // SPECIES_NONE: no icon
    uint8_t iconX, iconY;       // icon centre, screen GBA pixels
};

struct Mail;
void Mail_LayoutForUi(const struct Mail *mail, struct MailUiLayout *out);
void Mail_StationeryForUi(uint8_t mailType, const uint32_t **tiles, const uint32_t **tilemap, uint16_t *palette);

#endif // GUARD_PLATFORM_MAIL_UI_H
