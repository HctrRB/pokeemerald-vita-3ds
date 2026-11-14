#ifndef GUARD_DECORATION_H
#define GUARD_DECORATION_H

enum DecorationPermission
{
    /*
     * The nomenclature here describes collision and placement permissions, in that order.
     */
    DECORPERM_SOLID_FLOOR,
    DECORPERM_PASS_FLOOR,
    DECORPERM_BEHIND_FLOOR,
    DECORPERM_NA_WALL,
    DECORPERM_SPRITE,
};

enum DecorationShape
{
    DECORSHAPE_1x1,
    DECORSHAPE_2x1,
    DECORSHAPE_3x1, // unused
    DECORSHAPE_4x2,
    DECORSHAPE_2x2,
    DECORSHAPE_1x2,
    DECORSHAPE_1x3, // unused
    DECORSHAPE_2x4,
    DECORSHAPE_3x3,
    DECORSHAPE_3x2,
};

enum DecorationCategory
{
    DECORCAT_DESK,
    DECORCAT_CHAIR,
    DECORCAT_PLANT,
    DECORCAT_ORNAMENT,
    DECORCAT_MAT,
    DECORCAT_POSTER,
    DECORCAT_DOLL,
    DECORCAT_CUSHION,
    DECORCAT_COUNT,
};

struct Decoration
{
    u8 id;
    u8 name[16];
    u8 permission;
    u8 shape;
    u8 category;
    u16 price;
    const u8 *description;
    const u16 *tiles;
};

extern const struct Decoration gDecorations[];
extern u8 *gCurDecorationItems;
extern u8 gCurDecorationIndex;

void InitDecorationContextItems(void);
void DoSecretBaseDecorationMenu(u8 taskId);
void ShowDecorationOnMap(u16 mapX, u16 mapY, u16 decoration);
void DoPlayerRoomDecorationMenu(u8 taskId);
void ShowDecorationCategoriesWindow(u8 taskId);
void CopyDecorationCategoryName(u8 *dest, u8 category);
bool8 IsSelectedDecorInThePC(void);
u8 AddDecorationIconObject(u8 decor, s16 x, s16 y, u8 priority, u16 tilesTag, u16 paletteTag);

// New UI: the PC's DECORATION tab (ui_scr_playerpc.c), with the game's rules and lines
void Decoration_InitForUi(bool8 isPlayerRoom);
bool8 Decoration_IsFieldModeActive(void);   // placing / putting away: the field is drawn 2D
bool8 Decoration_InUseForUi(u8 category, u8 index);
bool8 Decoration_HasAnyForUi(void);
bool8 Decoration_CategoryPlaceableForUi(u8 category);
bool8 Decoration_CanPlaceForUi(u8 category, u8 index);
bool8 Decoration_CanTossForUi(u8 category, u8 index);
void Decoration_TossForUi(u8 category, u8 index);
bool8 Decoration_CanPutAwayForUi(void);
void Decoration_StartPlaceFromUi(u8 taskId, u8 category, u8 index);
void Decoration_StartPutAwayFromUi(u8 taskId);
void Decoration_EndUiSession(u8 taskId);

#endif //GUARD_DECORATION_H
