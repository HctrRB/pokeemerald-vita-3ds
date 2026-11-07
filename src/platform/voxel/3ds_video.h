#ifndef _3DS_VIDEO_H_VOXEL_STUB_
#define _3DS_VIDEO_H_VOXEL_STUB_

#include <stdint.h>
#include "global.h"
#include "voxel_unswizzle.h"
#include "sprite.h"
#include "field_weather.h"
#include "constants/weather.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline u8 GetSpriteExtPalette(const struct Sprite *sprite)
{
    (void)sprite;
    return 0xFF;
}

static inline const uint16_t *CtrVideo_ExtPalette(uint32_t index)
{
    (void)index;
    return NULL;
}

static inline bool8 CtrSprite_IsVoxelWeather(const struct Sprite *sprite)
{
    if (sprite == NULL)
        return FALSE;

    if (sprite->template != NULL)
    {
        if (sprite->template->paletteTag == PALTAG_WEATHER || sprite->template->paletteTag == PALTAG_WEATHER_2)
            return TRUE;
        if (sprite->template->tileTag >= GFXTAG_CLOUD && sprite->template->tileTag <= GFXTAG_RAIN)
            return TRUE;
    }

    if (gWeatherPtr == NULL)
        return FALSE;

    uint32_t i;
    switch (gWeatherPtr->currWeather)
    {
    case WEATHER_SUNNY_CLOUDS:
        for (i = 0; i < NUM_CLOUD_SPRITES; ++i)
            if (gWeatherPtr->sprites.s1.cloudSprites[i] == sprite) return TRUE;
        break;
    case WEATHER_RAIN:
    case WEATHER_RAIN_THUNDERSTORM:
    case WEATHER_DOWNPOUR:
        for (i = 0; i < gWeatherPtr->rainSpriteCount; ++i)
            if (gWeatherPtr->sprites.s1.rainSprites[i] == sprite) return TRUE;
        break;
    case WEATHER_SNOW:
        for (i = 0; i < gWeatherPtr->snowflakeSpriteCount; ++i)
            if (gWeatherPtr->sprites.s1.snowflakeSprites[i] == sprite) return TRUE;
        break;
    case WEATHER_FOG_HORIZONTAL:
    case WEATHER_UNDERWATER:
        for (i = 0; i < NUM_FOG_HORIZONTAL_SPRITES; ++i)
            if (gWeatherPtr->sprites.s2.fogHSprites[i] == sprite) return TRUE;
        break;
    case WEATHER_VOLCANIC_ASH:
        for (i = 0; i < NUM_ASH_SPRITES; ++i)
            if (gWeatherPtr->sprites.s2.ashSprites[i] == sprite) return TRUE;
        break;
    case WEATHER_SANDSTORM:
        for (i = 0; i < NUM_SANDSTORM_SPRITES; ++i)
            if (gWeatherPtr->sprites.s2.sandstormSprites1[i] == sprite) return TRUE;
        for (i = 0; i < NUM_SWIRL_SANDSTORM_SPRITES; ++i)
            if (gWeatherPtr->sprites.s2.sandstormSprites2[i] == sprite) return TRUE;
        break;
    case WEATHER_FOG_DIAGONAL:
        for (i = 0; i < NUM_FOG_DIAGONAL_SPRITES; ++i)
            if (gWeatherPtr->sprites.s2.fogDSprites[i] == sprite) return TRUE;
        break;
    case WEATHER_UNDERWATER_BUBBLES:
        if (sprite->usingSheet && GetSpriteTileTagByTileStart(sprite->sheetTileStart) == GFXTAG_BUBBLE)
            return TRUE;
        break;
    }
    return FALSE;
}

static inline bool8 CtrSprite_IsVoxelCloud(const struct Sprite *sprite)
{
    uint32_t i;
    if (gWeatherPtr == NULL || gWeatherPtr->currWeather != WEATHER_SUNNY_CLOUDS)
        return FALSE;
    for (i = 0; i < NUM_CLOUD_SPRITES; ++i)
        if (gWeatherPtr->sprites.s1.cloudSprites[i] == sprite)
            return TRUE;
    return FALSE;
}

static inline const uint8_t *CtrVideo_GetBgVram(void)
{
    extern uint8_t VRAM_[];
    return (const uint8_t *)VRAM_;
}

#ifdef __cplusplus
}
#endif

#endif // _3DS_VIDEO_H_VOXEL_STUB_
