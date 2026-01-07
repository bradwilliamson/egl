/* sdl2/sdl_input.c - SDL2 input glue (scaffold)

This file demonstrates how SDL2 events can be forwarded into the engine's input
state. It's minimal and intended as a scaffold; full mapping and focus handling
should be implemented later.
*/

#include "../client/cl_local.h"
#include <SDL2/SDL.h>

/* Poll SDL2 events and forward into engine */
void SDL2_PollInputEvents (void)
{
    SDL_Event ev;
    while (SDL_PollEvent (&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            Cbuf_AddText ("quit\n");
            break;

        case SDL_KEYDOWN:
            Key_Event (ev.key.keysym.scancode, qTrue, Sys_Milliseconds ());
            break;
        case SDL_KEYUP:
            Key_Event (ev.key.keysym.scancode, qFalse, Sys_Milliseconds ());
            break;

        case SDL_MOUSEMOTION:
            /* SDL provides relative motion when in relative mouse mode */
            CL_MoveMouse (ev.motion.xrel, ev.motion.yrel);
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (ev.button.button == SDL_BUTTON_LEFT) Key_Event (K_MOUSE1, qTrue, Sys_Milliseconds ());
            if (ev.button.button == SDL_BUTTON_MIDDLE) Key_Event (K_MOUSE3, qTrue, Sys_Milliseconds ());
            if (ev.button.button == SDL_BUTTON_RIGHT) Key_Event (K_MOUSE2, qTrue, Sys_Milliseconds ());
            break;
        case SDL_MOUSEBUTTONUP:
            if (ev.button.button == SDL_BUTTON_LEFT) Key_Event (K_MOUSE1, qFalse, Sys_Milliseconds ());
            if (ev.button.button == SDL_BUTTON_MIDDLE) Key_Event (K_MOUSE3, qFalse, Sys_Milliseconds ());
            if (ev.button.button == SDL_BUTTON_RIGHT) Key_Event (K_MOUSE2, qFalse, Sys_Milliseconds ());
            break;

        case SDL_MOUSEWHEEL:
            if (ev.wheel.y > 0) { Key_Event (K_MWHEELUP, qTrue, Sys_Milliseconds ()); Key_Event (K_MWHEELUP, qFalse, Sys_Milliseconds ()); }
            else if (ev.wheel.y < 0) { Key_Event (K_MWHEELDOWN, qTrue, Sys_Milliseconds ()); Key_Event (K_MWHEELDOWN, qFalse, Sys_Milliseconds ()); }
            break;
        }
    }
}

/* Engine input hooks for SDL backend */
void IN_Commands (void)
{
    SDL2_PollInputEvents ();
}

void IN_Move (userCmd_t *cmd)
{
    /* Nothing special to do here; SDL sends relative motion via events */
}

void IN_Frame (void)
{
    /* Keep the OS from idling while playing */
#ifdef SDL_HINT_IDLE_TIMER_DISABLED
    /* SDL2-specific: platform hints may be available per platform */
#endif
    SDL2_PollInputEvents ();
}
