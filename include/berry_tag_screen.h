#ifndef GUARD_BERRY_TAG_SCREEN_H
#define GUARD_BERRY_TAG_SCREEN_H

void DoBerryTagScreen(void);
// Port: from the new UI's BAG, returning to `exit` (berry_tag_screen.c)
void DoBerryTagScreenForUi(u16 itemId, u16 pocketPosition, void (*exit)(void));

#endif // GUARD_BERRY_TAG_SCREEN_H
