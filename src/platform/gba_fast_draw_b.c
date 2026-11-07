// Second copy of the fast renderer for the first parallel draw worker (see the comment at the
// top of gba_fast_draw.c). Each copy has its own register/palette redirect pointers and sprite
// lists, so several cores can draw different lines of the same frame at once.
#if defined(PLATFORM_VITA) && defined(VITA_PARALLEL_DRAW)
#define FAST_DRAW_INSTANCE_B
#include "gba_fast_draw.c"
#endif
