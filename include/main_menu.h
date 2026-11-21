#ifndef GUARD_MAIN_MENU_H
#define GUARD_MAIN_MENU_H

void CB2_InitMainMenu(void);
void CreateYesNoMenuParameterized(u8 x, u8 y, u16 baseTileNum, u16 baseBlock, u8 yesNoPalNum, u8 winPalNum);
bool8 MainMenu_IsShown(void);   // Vita port: the CONTINUE / NEW GAME list is on screen (Core 1)

#endif // GUARD_MAIN_MENU_H
