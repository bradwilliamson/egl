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

#include <math.h>

/* From sdl_glimp.c */
extern cVar_t *vid_fullscreen;

void SDL2_SetMouseGrab (qBool grab);
void SDL2_OnWindowResized (int width, int height);
void GLimp_AppActivate (qBool active);

static qBool sdl_appActive = qTrue;
static qBool sdl_mouseGrabbed = qFalse;

static cVar_t *in_joystick = NULL;
static cVar_t *in_joystick_auto = NULL;
static cVar_t *joy_deadzone = NULL;
static cVar_t *joy_move_scale = NULL;
static cVar_t *joy_look_scale = NULL;
static cVar_t *joy_invert_y = NULL;
static cVar_t *joy_trigger_threshold = NULL;

static SDL_GameController *sdl_controller = NULL;
static SDL_JoystickID sdl_controllerInstance = -1;

static int sdl_joy_left_x = 0;
static int sdl_joy_left_y = 0;
static int sdl_joy_right_x = 0;
static int sdl_joy_right_y = 0;
static int sdl_joy_trigger_l = 0;
static int sdl_joy_trigger_r = 0;

static qBool sdl_menu_left = qFalse;
static qBool sdl_menu_right = qFalse;
static qBool sdl_menu_up = qFalse;
static qBool sdl_menu_down = qFalse;
static qBool sdl_trigger_l_down = qFalse;
static qBool sdl_trigger_r_down = qFalse;

static void *cmd_in_restart = NULL;
static void *cmd_joy_rumble = NULL;

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

static keyNum_t SDL2_TranslateControllerButton (Uint8 button)
{
    int b = (int)button;
    if (b < 0)
        return K_BADKEY;

    if (b < 4)
        return (keyNum_t)(K_JOY1 + b);

    b -= 4;
    if (b >= 0 && b < 28)
        return (keyNum_t)(K_AUX1 + b);

    return K_BADKEY;
}

static void SDL2_VirtualKeyEvent (keyNum_t key, qBool *state, qBool down)
{
    if (*state == down)
        return;
    *state = down;
    Key_Event ((int)key, down, Sys_Milliseconds ());
}

static float SDL2_ApplyDeadzone (float x, float dz)
{
    float ax = fabsf (x);
    if (ax <= dz)
        return 0.0f;
    if (dz >= 0.999f)
        return 0.0f;
    return (x > 0.0f ? 1.0f : -1.0f) * ((ax - dz) / (1.0f - dz));
}

static qBool SDL2_OpenFirstController (void)
{
    int num = SDL_NumJoysticks ();
    int i;

    if (sdl_controller)
        return qTrue;

    for (i = 0; i < num; i++) {
        if (!SDL_IsGameController (i))
            continue;

        sdl_controller = SDL_GameControllerOpen (i);
        if (!sdl_controller)
            continue;

        SDL_Joystick *joy = SDL_GameControllerGetJoystick (sdl_controller);
        sdl_controllerInstance = joy ? SDL_JoystickInstanceID (joy) : -1;

        Com_Printf (0, "Gamepad: %s\n", SDL_GameControllerName (sdl_controller));
        return qTrue;
    }

    return qFalse;
}

static void SDL2_CloseController (void)
{
    if (sdl_controller) {
        SDL_GameControllerClose (sdl_controller);
        sdl_controller = NULL;
    }
    sdl_controllerInstance = -1;

    sdl_joy_left_x = sdl_joy_left_y = 0;
    sdl_joy_right_x = sdl_joy_right_y = 0;
    sdl_joy_trigger_l = sdl_joy_trigger_r = 0;

    sdl_menu_left = sdl_menu_right = qFalse;
    sdl_menu_up = sdl_menu_down = qFalse;
    sdl_trigger_l_down = sdl_trigger_r_down = qFalse;
}

static void SDL2_UpdateControllerEnabled (void)
{
    if (!in_joystick)
        return;

    if (in_joystick->modified) {
        in_joystick->modified = qFalse;

        if (in_joystick->intVal) {
            SDL2_OpenFirstController ();
        }
        else {
            SDL2_CloseController ();
        }
    }
}

static qBool SDL2_WantsController (void)
{
    if (in_joystick && in_joystick->intVal)
        return qTrue;
    if (in_joystick_auto && in_joystick_auto->intVal)
        return qTrue;
    return qFalse;
}

static void IN_Rumble_f (void)
{
    int low = 0, high = 0, ms = 200;

    if (!sdl_controller) {
        Com_Printf (0, "No gamepad open.\n");
        return;
    }

    if (Cmd_Argc () >= 2)
        low = atoi (Cmd_Argv (1));
    if (Cmd_Argc () >= 3)
        high = atoi (Cmd_Argv (2));
    if (Cmd_Argc () >= 4)
        ms = atoi (Cmd_Argv (3));

    if (low < 0) low = 0;
    if (high < 0) high = 0;
    if (low > 65535) low = 65535;
    if (high > 65535) high = 65535;
    if (ms < 1) ms = 1;
    if (ms > 5000) ms = 5000;

    if (SDL_GameControllerRumble (sdl_controller, (Uint16)low, (Uint16)high, (Uint32)ms) != 0) {
        Com_Printf (PRNT_WARNING, "Gamepad rumble failed: %s\n", SDL_GetError ());
    }
}

/* Poll SDL2 events and forward into engine */
void SDL2_PollInputEvents (void)
{
    SDL_Event ev;
    SDL2_UpdateControllerEnabled ();

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

        case SDL_CONTROLLERDEVICEADDED:
            if (SDL2_WantsController ()) {
                if (!sdl_controller) {
                    if (SDL2_OpenFirstController () && in_joystick_auto && in_joystick_auto->intVal && in_joystick && !in_joystick->intVal)
                        Cvar_VariableSetValue (in_joystick, 1, qFalse);
                }
            }
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            if (sdl_controller && ev.cdevice.which == sdl_controllerInstance) {
                Com_Printf (0, "Gamepad disconnected.\n");
                SDL2_CloseController ();
            }
            break;

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        {
            qBool down = (ev.type == SDL_CONTROLLERBUTTONDOWN) ? qTrue : qFalse;
            Uint8 btn = ev.cbutton.button;

            if (!(in_joystick && in_joystick->intVal))
                break;

            // Special-case Start as Escape by default, but still allow binds via AUX.
            if (btn == SDL_CONTROLLER_BUTTON_START) {
                Key_Event (K_ESCAPE, down, Sys_Milliseconds ());
            }

            keyNum_t k = SDL2_TranslateControllerButton (btn);
            if (k != K_BADKEY)
                Key_Event ((int)k, down, Sys_Milliseconds ());
            break;
        }

        case SDL_CONTROLLERAXISMOTION:
        {
            int axis_value = (int)ev.caxis.value;
            float trig_thresh = 16384.0f;

            if (!(in_joystick && in_joystick->intVal))
                break;

            if (joy_trigger_threshold)
                trig_thresh = 32766.0f * clamp (joy_trigger_threshold->floatVal, 0.001f, 1.0f);

            switch (ev.caxis.axis) {
            case SDL_CONTROLLER_AXIS_LEFTX: sdl_joy_left_x = axis_value; break;
            case SDL_CONTROLLER_AXIS_LEFTY: sdl_joy_left_y = axis_value; break;
            case SDL_CONTROLLER_AXIS_RIGHTX: sdl_joy_right_x = axis_value; break;
            case SDL_CONTROLLER_AXIS_RIGHTY: sdl_joy_right_y = axis_value; break;
            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                sdl_joy_trigger_l = axis_value;
                SDL2_VirtualKeyEvent (K_AUX27, &sdl_trigger_l_down, (axis_value > trig_thresh) ? qTrue : qFalse);
                break;
            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                sdl_joy_trigger_r = axis_value;
                SDL2_VirtualKeyEvent (K_AUX28, &sdl_trigger_r_down, (axis_value > trig_thresh) ? qTrue : qFalse);
                break;
            default:
                break;
            }

            // Virtual keys to navigate menus with left stick.
            if (Key_GetDest () == KD_MENU) {
                const int thresh = 16896;
                if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    SDL2_VirtualKeyEvent (K_LEFTARROW, &sdl_menu_left, (axis_value < -thresh) ? qTrue : qFalse);
                    SDL2_VirtualKeyEvent (K_RIGHTARROW, &sdl_menu_right, (axis_value > thresh) ? qTrue : qFalse);
                }
                else if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    SDL2_VirtualKeyEvent (K_UPARROW, &sdl_menu_up, (axis_value < -thresh) ? qTrue : qFalse);
                    SDL2_VirtualKeyEvent (K_DOWNARROW, &sdl_menu_down, (axis_value > thresh) ? qTrue : qFalse);
                }
            }
            break;
        }

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
    float dz, moveScale, lookScale, dt;
    float lx, ly, rx, ry;
    int dx, dy;

    if (!cmd)
        return;

    if (!(in_joystick && in_joystick->intVal))
        return;

    if (!sdl_controller)
        return;

    if (!cls.refreshPrepped || !sdl_appActive)
        return;

    if (Key_GetDest () != KD_GAME)
        return;

    dz = joy_deadzone ? clamp (joy_deadzone->floatVal, 0.0f, 0.95f) : 0.15f;
    moveScale = joy_move_scale ? joy_move_scale->floatVal : 1.0f;
    lookScale = joy_look_scale ? joy_look_scale->floatVal : 700.0f; // "mouse counts" per second at full deflection

    dt = cls.netFrameTime;
    if (dt <= 0.0f || dt > 0.25f)
        dt = 0.016f;

    lx = SDL2_ApplyDeadzone ((float)sdl_joy_left_x / 32767.0f, dz);
    ly = SDL2_ApplyDeadzone ((float)sdl_joy_left_y / 32767.0f, dz);
    rx = SDL2_ApplyDeadzone ((float)sdl_joy_right_x / 32767.0f, dz);
    ry = SDL2_ApplyDeadzone ((float)sdl_joy_right_y / 32767.0f, dz);

    // Left stick: movement
    cmd->sideMove += (int16)(cl_sidespeed->floatVal * moveScale * lx);
    cmd->forwardMove += (int16)(cl_forwardspeed->floatVal * moveScale * -ly);

    // Right stick: look (feed into mouse path so existing sensitivity/freelook/strafe logic applies)
    if (joy_invert_y && joy_invert_y->intVal)
        ry = -ry;

    dx = (int)(rx * lookScale * dt);
    dy = (int)(ry * lookScale * dt);
    if (dx || dy)
        CL_MoveMouse (dx, dy);
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

    // Ensure controller subsystem is up for SDL2 gamepad support.
    SDL_InitSubSystem (SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    SDL_GameControllerEventState (SDL_ENABLE);

    // Keep legacy CVAR name so existing menus/scripts work.
    in_joystick = Cvar_Register ("in_joystick", "0", CVAR_ARCHIVE);
    in_joystick_auto = Cvar_Register ("in_joystick_auto", "0", CVAR_ARCHIVE);
    joy_deadzone = Cvar_Register ("joy_deadzone", "0.15", CVAR_ARCHIVE);
    joy_move_scale = Cvar_Register ("joy_move_scale", "1.0", CVAR_ARCHIVE);
    joy_look_scale = Cvar_Register ("joy_look_scale", "700", CVAR_ARCHIVE);
    joy_invert_y = Cvar_Register ("joy_invert_y", "0", CVAR_ARCHIVE);
    joy_trigger_threshold = Cvar_Register ("joy_trigger_threshold", "0.55", CVAR_ARCHIVE);

    if (!cmd_in_restart)
        cmd_in_restart = Cmd_AddCommand ("in_restart", IN_Restart_f, "Restarts input subsystem");

    if (!cmd_joy_rumble)
        cmd_joy_rumble = Cmd_AddCommand ("joy_rumble", IN_Rumble_f, "Test gamepad rumble: joy_rumble <low 0-65535> <high 0-65535> <ms>");

    if (SDL2_WantsController ()) {
        if (SDL2_OpenFirstController () && in_joystick_auto && in_joystick_auto->intVal && in_joystick && !in_joystick->intVal)
            Cvar_VariableSetValue (in_joystick, 1, qFalse);
    }
}

void IN_Shutdown (void)
{
    SDL2_SetGrabState (qFalse);

    SDL2_CloseController ();

    if (cmd_joy_rumble) {
        Cmd_RemoveCommand ("joy_rumble", cmd_joy_rumble);
        cmd_joy_rumble = NULL;
    }

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
