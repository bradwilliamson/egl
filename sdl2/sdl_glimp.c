/* sdl2/sdl_glimp.c - SDL2 GL context backend (scaffold)

This file provides a minimal SDL2-backed GL context and swap control glue.
It's intentionally small and only implements the basic APIs required by the engine.
Build: make -f Makefile.unix USE_SDL2=1

TODO: flesh out full mode enumeration/setting, input hookup, and gamma control.
*/

#include "../unix/unix_glimp.h"
#include "../client/cl_local.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <limits.h>

static SDL_Window *sdl_window = NULL;
static SDL_GLContext sdl_glctx = NULL;

/* Platform cvars */
static cVar_t *vid_xpos = NULL;
static cVar_t *vid_ypos = NULL;
static cVar_t *vid_fullscreen = NULL;
static cVar_t *gl_swap_control = NULL;
static cVar_t *gl_swap_interval = NULL;

static qBool vid_queueRestart = qFalse;
static qBool vid_isActive = qFalse;

/* Helper: set SDL swap interval based on cvars */
static void SDL_GLimp_SetSwapInterval (int interval)
{
    if (!gl_swap_control || !gl_swap_interval)
        return;

    if (!gl_swap_control->intVal)
        interval = 0;

    if (SDL_GL_SetSwapInterval (interval) == 0)
        Com_Printf (0, "SDL2: Set swap interval %d\n", interval);
    else
        Com_Printf (PRNT_WARNING, "SDL2: Failed to set swap interval %d\n", interval);
}

qBool GLimp_Init (void)
{
    if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        Com_Printf (PRNT_ERROR, "SDL_Init failed: %s\n", SDL_GetError ());
        return qFalse;
    }

    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);

    sdl_window = SDL_CreateWindow (APPLICATION, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!sdl_window) {
        Com_Printf (PRNT_ERROR, "SDL_CreateWindow failed: %s\n", SDL_GetError ());
        SDL_Quit ();
        return qFalse;
    }

    sdl_glctx = SDL_GL_CreateContext (sdl_window);
    if (!sdl_glctx) {
        Com_Printf (PRNT_ERROR, "SDL_GL_CreateContext failed: %s\n", SDL_GetError ());
        SDL_DestroyWindow (sdl_window);
        sdl_window = NULL;
        SDL_Quit ();
        return qFalse;
    }

    /* Register swap control cvars */
    gl_swap_control = Cvar_Register ("gl_swap_control", "1", CVAR_ARCHIVE);
    gl_swap_interval = Cvar_Register ("gl_swap_interval", "1", CVAR_ARCHIVE);

    /* Try to enable vsync via SDL immediately */
    SDL_GLimp_SetSwapInterval (gl_swap_interval->intVal);

    return qTrue;
}

void GLimp_Shutdown (qBool full)
{
    if (sdl_glctx) {
        SDL_GL_DeleteContext (sdl_glctx);
        sdl_glctx = NULL;
    }
    if (sdl_window) {
        SDL_DestroyWindow (sdl_window);
        sdl_window = NULL;
    }
    SDL_Quit ();
}

/* Attempts to set the requested mode using SDL display modes */
qBool GLimp_AttemptMode (qBool fullScreen, int width, int height)
{
    if (!sdl_window)
        return qFalse;

    if (fullScreen) {
        /* find best matching mode on the current display */
        int displayIndex = SDL_GetWindowDisplayIndex (sdl_window);
        int modeCount = SDL_GetNumDisplayModes (displayIndex);
        SDL_DisplayMode bestMode;
        int bestIndex = -1;
        int bestDist = INT_MAX;

        for (int i = 0; i < modeCount; ++i) {
            SDL_DisplayMode dm;
            if (SDL_GetDisplayMode (displayIndex, i, &dm) != 0)
                continue;
            int dx = abs (dm.w - width);
            int dy = abs (dm.h - height);
            int dist = (dx > dy) ? dx : dy;
            if (dist < bestDist) {
                bestDist = dist;
                bestIndex = i;
                bestMode = dm;
            }
        }

        if (bestIndex >= 0) {
            SDL_SetWindowDisplayMode (sdl_window, &bestMode);
            if (SDL_SetWindowFullscreen (sdl_window, SDL_WINDOW_FULLSCREEN) == 0) {
                SDL_SetWindowSize (sdl_window, width, height);
                return qTrue;
            }
        }
        return qFalse;
    }

    /* Windowed mode */
    SDL_SetWindowFullscreen (sdl_window, 0);
    SDL_SetWindowSize (sdl_window, width, height);
    return qTrue;
}

void GLimp_EndFrame (void)
{
    /* Apply swap interval changes immediately when the cvars are modified */
    if (gl_swap_control && gl_swap_control->modified) {
        gl_swap_control->modified = qFalse;
        SDL_GLimp_SetSwapInterval (gl_swap_interval->intVal ? gl_swap_interval->intVal : 0);
    }
    if (gl_swap_interval && gl_swap_interval->modified) {
        gl_swap_interval->modified = qFalse;
        SDL_GLimp_SetSwapInterval (gl_swap_control && gl_swap_control->intVal ? gl_swap_interval->intVal : 0);
    }

    if (sdl_window)
        SDL_GL_SwapWindow (sdl_window);
}

/* ---------- Video management / VID_* ---------- */
static void VID_Restart_f (void)
{
    vid_queueRestart = qTrue;
}

static void VID_Front_f (void)
{
    if (sdl_window) SDL_RaiseWindow (sdl_window);
}

static void VID_UpdateWindowPosAndSize (void)
{
    if (!vid_xpos && !vid_ypos)
        return;

    if (!vid_fullscreen || !vid_fullscreen->intVal) {
        int x = vid_xpos->intVal;
        int y = vid_ypos->intVal;
        SDL_SetWindowPosition (sdl_window, x, y);
    }
    vid_xpos->modified = qFalse;
    vid_ypos->modified = qFalse;
}

void VID_CheckChanges (refConfig_t *outConfig)
{
    if (!sdl_window)
        return;

    if (vid_xpos && vid_xpos->modified || vid_ypos && vid_ypos->modified) {
        VID_UpdateWindowPosAndSize ();
    }

    while (vid_queueRestart) {
        qBool cgWasActive = cls.mapLoaded;

        CL_MediaShutdown ();

        vid_queueRestart = qFalse;
        cls.refreshPrepped = qFalse;
        cls.disableScreen = qTrue;

        if (vid_isActive) {
            R_Shutdown (qFalse);
            vid_isActive = qFalse;
        }

        if (R_Init () != R_INIT_SUCCESS) {
            R_Shutdown (qTrue);
            vid_isActive = qFalse;
            Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n");
        }

        R_GetRefConfig (outConfig);

        /* Initialize swap-control via SDL */
        if (gl_swap_control && gl_swap_interval)
            SDL_GLimp_SetSwapInterval (gl_swap_interval->intVal);

        Snd_Init ();
        CL_MediaInit ();

        cls.disableScreen = qFalse;

        CL_ConsoleClose ();

        if (cgWasActive) {
            CL_CGModule_LoadMap ();
            Key_SetDest (KD_GAME);
        }
        else if (Com_ClientState() < CA_CONNECTED) {
            CL_CGModule_MainMenu ();
        }

        vid_isActive = qTrue;
    }
}

void VID_Init (refConfig_t *outConfig)
{
    vid_xpos = Cvar_Register ("vid_xpos", "3", CVAR_ARCHIVE);
    vid_ypos = Cvar_Register ("vid_ypos", "22", CVAR_ARCHIVE);
    vid_fullscreen = Cvar_Register ("vid_fullscreen", "0", CVAR_ARCHIVE);

    Cmd_AddCommand ("vid_restart", VID_Restart_f, "Restarts refresh and media");
    Cmd_AddCommand ("vid_front", VID_Front_f, "Brings window to front");

    vid_queueRestart = qTrue;
    vid_isActive = qFalse;
    VID_CheckChanges (outConfig);
}

void VID_Shutdown (void)
{
    if (vid_isActive) {
        R_Shutdown (qTrue);
        vid_isActive = qFalse;
    }

    Cmd_RemoveCommand ("vid_restart", NULL);
    Cmd_RemoveCommand ("vid_front", NULL);
}

/* Basic gamma ramp support using SDL */
qBool GLimp_GetGammaRamp (uint16 *ramp)
{
    if (!sdl_window)
        return qFalse;

    Uint16 r[256], g[256], b[256];
    if (SDL_GetWindowGammaRamp (sdl_window, r, g, b) != 0)
        return qFalse;

    for (int i = 0; i < 256; ++i) {
        ramp[i] = r[i];
        ramp[i + 256] = g[i];
        ramp[i + 512] = b[i];
    }
    return qTrue;
}

void GLimp_SetGammaRamp (uint16 *ramp)
{
    if (!sdl_window)
        return;

    Uint16 r[256], g[256], b[256];
    for (int i = 0; i < 256; ++i) {
        r[i] = ramp[i];
        g[i] = ramp[i + 256];
        b[i] = ramp[i + 512];
    }
    SDL_SetWindowGammaRamp (sdl_window, r, g, b);
}

