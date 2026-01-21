/* sdl2/sdl_input.c - SDL2 input glue (scaffold)

This file demonstrates how SDL2 events can be forwarded into the engine's input
state. It's minimal and intended as a scaffold; full mapping and focus handling
should be implemented later.
*/

#include "../client/cl_local.h"
#if defined(__has_include)
#  if __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#  else
#    include <SDL.h>
#  endif
#else
#  include <SDL2/SDL.h>
#endif

/* From sdl_glimp.c */
extern cVar_t *vid_fullscreen;

void SDL2_SetMouseGrab (qBool grab);
void SDL2_OnWindowResized (int width, int height);
void GLimp_AppActivate (qBool active);

static qBool sdl_appActive = qTrue;
static qBool sdl_mouseGrabbed = qFalse;

static void *cmd_in_restart = NULL;

static void SDL2_SetGrabState (qBool grab)
{
    if (sdl_mouseGrabbed == grab)
        return;
    sdl_mouseGrabbed = grab;
    SDL2_SetMouseGrab (grab);
}

static keyNum_t SDL2_TranslateKey (SDL_Keycode key)
{
    // For normal printable keys, SDL_Keycode matches ASCII.
    if (key >= 32 && key < 127)
        return (keyNum_t)key;

    switch (key) {
    case SDLK_ESCAPE:        return K_ESCAPE;
    case SDLK_RETURN:        return K_ENTER;
    case SDLK_TAB:           return K_TAB;
    case SDLK_BACKSPACE:     return K_BACKSPACE;
    case SDLK_SPACE:         return K_SPACE;

    case SDLK_UP:            return K_UPARROW;
    case SDLK_DOWN:          return K_DOWNARROW;
    case SDLK_LEFT:          return K_LEFTARROW;
    case SDLK_RIGHT:         return K_RIGHTARROW;

    case SDLK_LALT:
    case SDLK_RALT:          return K_ALT;
    case SDLK_LCTRL:
    case SDLK_RCTRL:         return K_CTRL;
    case SDLK_LSHIFT:        return K_LSHIFT;
    case SDLK_RSHIFT:        return K_RSHIFT;
    case SDLK_CAPSLOCK:      return K_CAPSLOCK;

    case SDLK_F1:            return K_F1;
    case SDLK_F2:            return K_F2;
    case SDLK_F3:            return K_F3;
    case SDLK_F4:            return K_F4;
    case SDLK_F5:            return K_F5;
    case SDLK_F6:            return K_F6;
    case SDLK_F7:            return K_F7;
    case SDLK_F8:            return K_F8;
    case SDLK_F9:            return K_F9;
    case SDLK_F10:           return K_F10;
    case SDLK_F11:           return K_F11;
    case SDLK_F12:           return K_F12;

    case SDLK_INSERT:        return K_INS;
    case SDLK_DELETE:        return K_DEL;
    case SDLK_PAGEDOWN:      return K_PGDN;
    case SDLK_PAGEUP:        return K_PGUP;
    case SDLK_HOME:          return K_HOME;
    case SDLK_END:           return K_END;
    case SDLK_PAUSE:         return K_PAUSE;

    case SDLK_KP_ENTER:      return K_KP_ENTER;
    case SDLK_KP_7:          return K_KP_HOME;
    case SDLK_KP_8:          return K_KP_UPARROW;
    case SDLK_KP_9:          return K_KP_PGUP;
    case SDLK_KP_4:          return K_KP_LEFTARROW;
    case SDLK_KP_5:          return K_KP_FIVE;
    case SDLK_KP_6:          return K_KP_RIGHTARROW;
    case SDLK_KP_1:          return K_KP_END;
    case SDLK_KP_2:          return K_KP_DOWNARROW;
    case SDLK_KP_3:          return K_KP_PGDN;
    case SDLK_KP_0:          return K_KP_INS;
    case SDLK_KP_PERIOD:     return K_KP_DEL;
    case SDLK_KP_DIVIDE:     return K_KP_SLASH;
    case SDLK_KP_MINUS:      return K_KP_MINUS;
    case SDLK_KP_PLUS:       return K_KP_PLUS;
    default:
        break;
    }

    return K_BADKEY;
}

static keyNum_t SDL2_TranslateMouseButton (Uint8 button)
{
    switch (button) {
    case SDL_BUTTON_LEFT:    return K_MOUSE1;
    case SDL_BUTTON_RIGHT:   return K_MOUSE2;
    case SDL_BUTTON_MIDDLE:  return K_MOUSE3;
    case SDL_BUTTON_X1:      return K_MOUSE4;
    case SDL_BUTTON_X2:      return K_MOUSE5;
    default:
        return K_BADKEY;
    }
}

/* Poll SDL2 events and forward into engine */
void SDL2_PollInputEvents (void)
{
    SDL_Event ev;
    while (SDL_PollEvent (&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            Cbuf_AddText ("quit\n");
            break;

        case SDL_KEYDOWN: {
            /* Ignore SDL key repeat - the engine handles its own repeat logic */
            if (ev.key.repeat)
                break;
            
            /* Alt+Enter toggles fullscreen */
            if (ev.key.keysym.sym == SDLK_RETURN && (ev.key.keysym.mod & KMOD_ALT)) {
                if (vid_fullscreen) {
                    Cvar_VariableSetValue (vid_fullscreen, !vid_fullscreen->intVal, qTrue);
                    Cbuf_AddText ("vid_restart\n");
                }
                break;
            }
            
            keyNum_t k = SDL2_TranslateKey (ev.key.keysym.sym);
            if (k != K_BADKEY)
                Key_Event ((int)k, qTrue, Sys_Milliseconds ());
            break; }
        case SDL_KEYUP: {
            keyNum_t k = SDL2_TranslateKey (ev.key.keysym.sym);
            if (k != K_BADKEY)
                Key_Event ((int)k, qFalse, Sys_Milliseconds ());
            break; }

        case SDL_MOUSEMOTION:
            /* SDL provides relative motion when in relative mouse mode */
            if (sdl_mouseGrabbed)
                CL_MoveMouse (ev.motion.xrel, ev.motion.yrel);
            break;

        case SDL_MOUSEBUTTONDOWN:
        {
            keyNum_t mb = SDL2_TranslateMouseButton (ev.button.button);
            if (mb != K_BADKEY)
                Key_Event ((int)mb, qTrue, Sys_Milliseconds ());
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
            keyNum_t mb = SDL2_TranslateMouseButton (ev.button.button);
            if (mb != K_BADKEY)
                Key_Event ((int)mb, qFalse, Sys_Milliseconds ());
            break;
        }

        case SDL_MOUSEWHEEL:
            if (ev.wheel.y > 0) { Key_Event (K_MWHEELUP, qTrue, Sys_Milliseconds ()); Key_Event (K_MWHEELUP, qFalse, Sys_Milliseconds ()); }
            else if (ev.wheel.y < 0) { Key_Event (K_MWHEELDOWN, qTrue, Sys_Milliseconds ()); Key_Event (K_MWHEELDOWN, qFalse, Sys_Milliseconds ()); }
            break;

        case SDL_WINDOWEVENT:
            switch (ev.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                SDL2_OnWindowResized (ev.window.data1, ev.window.data2);
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                sdl_appActive = qTrue;
                GLimp_AppActivate (qTrue);
                IN_Activate (qTrue);
                SndImp_Activate (qTrue);
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                sdl_appActive = qFalse;
                GLimp_AppActivate (qFalse);
                IN_Activate (qFalse);
                SndImp_Activate (qFalse);
                Key_ClearStates ();
                break;
            default:
                break;
            }
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
    if (!cls.refreshPrepped) {
        SDL2_SetGrabState (qFalse);
        SDL2_PollInputEvents ();
        return;
    }

    if (!sdl_appActive) {
        SDL2_SetGrabState (qFalse);
        SDL2_PollInputEvents ();
        return;
    }

    // Match the Win32 behavior: don't grab when windowed and in console.
    if ((Key_GetDest () == KD_CONSOLE || Key_GetDest () == KD_MESSAGE)
    && Key_GetDest () != KD_MENU
    && !cls.refConfig.vidFullScreen) {
        SDL2_SetGrabState (qFalse);
        SDL2_PollInputEvents ();
        return;
    }

    SDL2_SetGrabState (qTrue);
    SDL2_PollInputEvents ();
}

void IN_Init (void)
{
    sdl_appActive = qTrue;
    SDL2_SetGrabState (qFalse);

    if (!cmd_in_restart)
        cmd_in_restart = Cmd_AddCommand ("in_restart", IN_Restart_f, "Restarts input subsystem");
}

void IN_Shutdown (void)
{
    SDL2_SetGrabState (qFalse);

    if (cmd_in_restart) {
        Cmd_RemoveCommand ("in_restart", cmd_in_restart);
        cmd_in_restart = NULL;
    }
}

void IN_Activate (qBool active)
{
    // Force a new grab check in IN_Frame.
    if (!active)
        SDL2_SetGrabState (qFalse);
}

/*
=================
IN_Restart_f

Needed after vid_restart: SDL window recreation can reset relative mouse mode.
=================
*/
void IN_Restart_f (void)
{
    // Force a grab state re-apply.
    sdl_mouseGrabbed = qFalse;
    SDL2_SetGrabState (qFalse);
    IN_Frame ();
}

/*
=================
In_GetKeyState

Used by the key system for a small number of OS-level toggles.
SDL2 provides this via modifier state.
=================
*/
qBool In_GetKeyState (keyNum_t keyNum)
{
    switch (keyNum) {
    case K_CAPSLOCK:
        return (SDL_GetModState () & KMOD_CAPS) ? qTrue : qFalse;
    default:
        break;
    }

    Com_Printf (PRNT_ERROR, "In_GetKeyState: Invalid key");
    return qFalse;
}
