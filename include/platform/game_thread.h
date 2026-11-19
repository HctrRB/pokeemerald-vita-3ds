#ifndef GUARD_PLATFORM_GAME_THREAD_H
#define GUARD_PLATFORM_GAME_THREAD_H

/*
 * The game runs on one thread: AgbMain, on Core 1 (docs/THREADS.md). The other cores see it
 * only through the game frame (include/platform/game_frame.h), and tools/core_boundary proves it
 * on every build.
 *
 * This is the run-time tripwire behind that check, for what a static walk cannot see
 * (a callback set at run time). The game's primitives that change state in place - the
 * Pokemon data cipher, flags, vars - call GAME_THREAD_ONLY(): on any other thread it logs
 * the caller once per call site to ux0:data/POKEVITA/thread_violations.log. Before the game
 * thread starts (boot) nothing is checked: Core 0 owns the game's memory then.
 *
 * The test is whether the stack pointer lies in the game thread's own stack: a subtraction and
 * a compare, no system call (GetMonData runs it hundreds of times a frame in the PC's boxes).
 * Each thread's stack is its own memory block, so no other thread's stack pointer is in range.
 */

#include <stdint.h>

// The game thread's stack (SDL_CreateThreadWithStackSize in sdl2.c)
#define GAME_THREAD_STACK_SIZE (4 * 1024 * 1024)
// Checked below the top recorded at registration, with a margin for the frames above it
#define GAME_THREAD_STACK_CHECKED (GAME_THREAD_STACK_SIZE - 64 * 1024)

extern volatile uintptr_t gGameThreadStackTop;   // 0 until the game thread registers

void GameThread_Register(void);
void GameThread_Violation(const void *caller, const char *what);

static inline uintptr_t GameThread_StackPointer(void)
{
    uintptr_t sp;
    __asm__ volatile("mov %0, sp" : "=r"(sp));
    return sp;
}

#define GAME_THREAD_ONLY()                                                                  \
    do                                                                                      \
    {                                                                                       \
        const uintptr_t top_ = gGameThreadStackTop;                                         \
        if (__builtin_expect(top_ != 0                                                      \
                             && top_ - GameThread_StackPointer() >= GAME_THREAD_STACK_CHECKED, 0)) \
            GameThread_Violation(__builtin_return_address(0),                               \
                                 "game state changed off the game thread");                 \
    } while (0)

#endif // GUARD_PLATFORM_GAME_THREAD_H
