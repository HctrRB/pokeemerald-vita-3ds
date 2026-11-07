#ifndef GUARD_VITA_RESET_H
#define GUARD_VITA_RESET_H

#include <stdbool.h>

// Game resets (the game's own soft reset, and the port menu's reset and save file switch). All
// game state is linked into two sections (gamedata, gamebss; see CMakeLists.txt). A copy of them
// taken at power on, before the game thread starts, puts the game back to its boot state, and
// the game then initialises everything again.
void VitaReset_TakeBootSnapshot(void);
bool VitaReset_RestoreBootSnapshot(void);

#endif // GUARD_VITA_RESET_H
