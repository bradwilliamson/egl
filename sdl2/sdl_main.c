/* sdl2/sdl_main.c - minimal SDL2 glue for Windows build (scaffold)
 * This file intentionally keeps a minimal footprint so it can be included in the
 * Windows build when USE_SDL2=1. It provides an init hook that can be called
 * by the platform code later if needed.
 */

#include "../client/cl_local.h"
#include <SDL2/SDL.h>
#include <sys/time.h>

/*
================
Sys_UMilliseconds
================
*/
uint32 Sys_UMilliseconds (void)
{
    struct timeval tp;
    static int secbase;

    gettimeofday (&tp, NULL);

    if (!secbase) {
        secbase = tp.tv_sec;
        return tp.tv_usec / 1000;
    }
    
    return (tp.tv_sec - secbase) * 1000 + tp.tv_usec / 1000;
}

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds (void)
{
    return (int)Sys_UMilliseconds ();
}

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
