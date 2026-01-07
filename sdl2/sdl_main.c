/* sdl2/sdl_main.c - minimal SDL2 glue for Windows build (scaffold)
 * This file intentionally keeps a minimal footprint so it can be included in the
 * Windows build when USE_SDL2=1. It provides an init hook that can be called
 * by the platform code later if needed.
 *
 * Note: On Unix, unix_main.c provides Sys_UMilliseconds, Sys_Milliseconds, etc.
 */

#include "../client/cl_local.h"
#include <SDL2/SDL.h>

void SDL2_InitHook (void)
{
    if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
        Com_Printf (PRNT_WARNING, "SDL2_InitHook: SDL_Init failed: %s\n", SDL_GetError ());
    } else {
        Com_Printf (0, "SDL2_InitHook: SDL initialized\n");
    }
}

void SDL2_ShutdownHook (void)
{
    SDL_Quit ();
}
