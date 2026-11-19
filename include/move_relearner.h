#ifndef GUARD_MOVE_RELEARNER_H
#define GUARD_MOVE_RELEARNER_H

// Room for the moves a POKéMON can remember (shared with the new UI's MOVE RELEARNER)
#define MAX_RELEARNER_MOVES max(MAX_LEVEL_UP_MOVES, 25)

void TeachMoveRelearnerMove(void);

// Port: the MOVE RELEARNER's steps for the new UI (move_relearner.c)
u8 MoveRelearner_MovesForUi(u16 *moves);
bool8 MoveRelearner_TeachForUi(u16 move);
void MoveRelearner_ReplaceForUi(u8 slot, u16 move);
void MoveRelearner_GiveUpForUi(void);
void MoveRelearner_ExitForUi(void);
void MoveRelearnerShowHideHearts(s32 move);

#endif //GUARD_MOVE_RELEARNER_H
