// Third copy of the fast renderer for the second parallel draw worker (see the comment at
// the top of gba_fast_draw.c).
#if defined(PLATFORM_VITA) && defined(VITA_PARALLEL_DRAW)
#define FAST_DRAW_INSTANCE_C
#include "gba_fast_draw.c"
#endif
