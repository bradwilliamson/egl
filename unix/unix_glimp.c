/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// unix_glimp.c
// This file contains GL context related code and some glue
//

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

#include "../renderer/r_local.h"
#include "../client/cl_local.h"
#include "unix_glimp.h"
#include "unix_local.h"

static qBool    vid_queueRestart;
static qBool    vid_isActive;

glxState_t glxState = {.OpenGLLib = NULL};

/* Swap control function pointers */
static void (*glXSwapIntervalEXT_ptr) (Display*, GLXDrawable, int) = NULL;
static int  (*glXSwapIntervalSGI_ptr) (int) = NULL;
static int  (*glXSwapIntervalMESA_ptr) (int) = NULL;
static int  gl_swap_method = 0; /* 0: none, 1: EXT, 2: SGI, 3: MESA */

/* Cvars */
static cVar_t *gl_swap_control = NULL;
static cVar_t *gl_swap_interval = NULL;

/* Set swap interval using the detected method */
static void GLimp_SetSwapInterval (int interval)
{
	if (!gl_swap_method || !gl_swap_control || !gl_swap_interval)
		return;

	if (!gl_swap_control->intVal)
		interval = 0;

	switch (gl_swap_method) {
	case 1: /* EXT */
		if (glXSwapIntervalEXT_ptr) {
			glXSwapIntervalEXT_ptr (x11display.dpy, x11display.gl_win, interval);
			Com_Printf (0, "gl_swap_control: set EXT interval %d\n", interval);
		}
		break;
	case 2: /* SGI */
		if (glXSwapIntervalSGI_ptr) {
			glXSwapIntervalSGI_ptr (interval);
			Com_Printf (0, "gl_swap_control: set SGI interval %d\n", interval);
		}
		break;
	case 3: /* MESA */
		if (glXSwapIntervalMESA_ptr) {
			glXSwapIntervalMESA_ptr (interval);
			Com_Printf (0, "gl_swap_control: set MESA interval %d\n", interval);
		}
		break;
	default:
		break;
	}
}

/* Detect available swap control extensions and functions */
static void GLimp_InitSwapControl (void)
{
	void *sym = NULL;

	/* Prefer EXT, fall back to SGI / MESA */
	if (!glxState.OpenGLLib) {
		Com_Printf (PRNT_WARNING, "GLimp_InitSwapControl: OpenGL library not loaded yet\n");
		return;
	}

	/* Try glXSwapIntervalEXT */
	sym = dlsym (glxState.OpenGLLib, "glXSwapIntervalEXT");
	if (sym) {
		glXSwapIntervalEXT_ptr = (void (*)(Display*, GLXDrawable, int)) sym;
		gl_swap_method = 1;
		Com_Printf (0, "GLimp: Found glXSwapIntervalEXT\n");
		GLimp_SetSwapInterval (gl_swap_interval->intVal);
		return;
	}

	/* Try SGI */
	sym = dlsym (glxState.OpenGLLib, "glXSwapIntervalSGI");
	if (sym) {
		glXSwapIntervalSGI_ptr = (int (*)(int)) sym;
		gl_swap_method = 2;
		Com_Printf (0, "GLimp: Found glXSwapIntervalSGI\n");
		GLimp_SetSwapInterval (gl_swap_interval->intVal);
		return;
	}

	/* Try MESA */
	sym = dlsym (glxState.OpenGLLib, "glXSwapIntervalMESA");
	if (sym) {
		glXSwapIntervalMESA_ptr = (int (*)(int)) sym;
		gl_swap_method = 3;
		Com_Printf (0, "GLimp: Found glXSwapIntervalMESA\n");
		GLimp_SetSwapInterval (gl_swap_interval->intVal);
		return;
	}

	/* If none found, try indirect lookup via glXGetProcAddressARB (some drivers only export via that) */
	sym = dlsym (glxState.OpenGLLib, "glXGetProcAddressARB");
	if (sym) {
		void *(*glXGetProcAddressARB) (const GLubyte *) = (void *(*)(const GLubyte *)) sym;
		void *p = glXGetProcAddressARB ((const GLubyte *)"glXSwapIntervalEXT");
		if (p) {
			glXSwapIntervalEXT_ptr = (void (*)(Display*, GLXDrawable, int)) p;
			gl_swap_method = 1;
			Com_Printf (0, "GLimp: Found glXSwapIntervalEXT via glXGetProcAddressARB\n");
			GLimp_SetSwapInterval (gl_swap_interval->intVal);
			return;
		}

		p = glXGetProcAddressARB ((const GLubyte *)"glXSwapIntervalSGI");
		if (p) {
			glXSwapIntervalSGI_ptr = (int (*)(int)) p;
			gl_swap_method = 2;
			Com_Printf (0, "GLimp: Found glXSwapIntervalSGI via glXGetProcAddressARB\n");
			GLimp_SetSwapInterval (gl_swap_interval->intVal);
			return;
		}

		p = glXGetProcAddressARB ((const GLubyte *)"glXSwapIntervalMESA");
		if (p) {
			glXSwapIntervalMESA_ptr = (int (*)(int)) p;
			gl_swap_method = 3;
			Com_Printf (0, "GLimp: Found glXSwapIntervalMESA via glXGetProcAddressARB\n");
			GLimp_SetSwapInterval (gl_swap_interval->intVal);
			return;
		}
	}

	Com_Printf (0, "GLimp: No GLX swap control found (vsync not supported)\n");
	gl_swap_method = 0;
}

/*
=============================================================================

	FRAME SETUP

=============================================================================
*/

/*
=================
GLimp_BeginFrame
=================
*/
void GLimp_BeginFrame (void)
{
}


/*
=================
GLimp_EndFrame

Responsible for doing a swapbuffers and possibly for other stuff as yet to be determined.
Probably better not to make this a GLimp function and instead do a call to GLimp_SwapBuffers.

Only error check if active, and don't swap if not active an you're in fullscreen
=================
*/
void GLimp_EndFrame (void)
{
	/* Apply swap interval changes immediately when the cvars are modified */
	if (gl_swap_control && gl_swap_control->modified) {
		gl_swap_control->modified = qFalse;
		GLimp_SetSwapInterval (gl_swap_control->intVal ? gl_swap_interval->intVal : 0);
	}
	if (gl_swap_interval && gl_swap_interval->modified) {
		gl_swap_interval->modified = qFalse;
		GLimp_SetSwapInterval (gl_swap_control && gl_swap_control->intVal ? gl_swap_interval->intVal : 0);
	}

	X11_SwapBuffers ();
}

/*
=============================================================================

	DLL GLUE

=============================================================================
*/

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f (void)
{
	vid_queueRestart = qTrue;
}


/*
============
ListRemaps_f

Console command to list all keys mapped to AUX%d
============
*/
static void ListRemaps_f (void)
{
	int	i, a;
	char	*k;

	Com_Printf (0, "Remapped keys:\n");
	for (i=0 ; ; i++) {
		k = X11_GetAuxKeyRemapName (i, &a);
		if (!k)
			break;

		Com_Printf (0, "AUX%-2d = %s\n", a-K_AUX1+1, k);
	}
}


/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (refConfig_t *outConfig)
{
	int errNum;

	while (vid_queueRestart) {
		qBool cgWasActive = cls.mapLoaded;

		CL_MediaShutdown ();

		// Refresh has changed
		vid_queueRestart = qFalse;
		cls.refreshPrepped = qFalse;
		cls.disableScreen = qTrue;

		// Kill if already active
		if (vid_isActive) {
			R_Shutdown (qFalse);
			vid_isActive = qFalse;
		}

		// Initialize renderer
		errNum = R_Init ();

		// Refresh init failed!
		if (errNum != R_INIT_SUCCESS) {
			R_Shutdown (qTrue);
			vid_isActive = qFalse;

			switch (errNum) {
			case R_INIT_QGL_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "QGL library failure!");
				break;

			case R_INIT_OS_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "Incorrect operating system!");
				break;

			case R_INIT_MODE_FAIL:
				Com_Error (ERR_FATAL, "Couldn't initialize OpenGL!\n" "Couldn't set video mode!");
				break;
			}
		}

		R_GetRefConfig (outConfig);

	/* Initialize swap-control / vsync support if available */
	GLimp_InitSwapControl ();


		cls.disableScreen = qFalse;

		CL_ConsoleClose ();

		// This is to stop cgame from initializing on first load
		// and so it will load after a vid_restart while connected somewhere
		if (cgWasActive) {
			CL_CGModule_LoadMap ();
			Key_SetDest (KD_GAME);
		}
		else {
			CL_CGModule_MainMenu ();
		}

		vid_isActive = qTrue;
	}
}


/*
============
VID_Init
============
*/
void VID_Init (refConfig_t *outConfig)
{
	vid_xpos = Cvar_Register ("vid_xpos", "3", CVAR_ARCHIVE);
	vid_ypos = Cvar_Register ("vid_ypos", "22", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Register ("vid_fullscreen", "0", CVAR_ARCHIVE);

	/* VSync / swap control */
	gl_swap_control = Cvar_Register ("gl_swap_control", "1", CVAR_ARCHIVE);
	gl_swap_interval = Cvar_Register ("gl_swap_interval", "1", CVAR_ARCHIVE);

	// Add some console commands that we want to handle
	Cmd_AddCommand (qFalse, "vid_restart", VID_Restart_f, "Restarts refresh and media");
	Cmd_AddCommand (qFalse, "listremaps", ListRemaps_f, "Lists what keys are remapped to AUX* bindings");

	// Start the graphics mode and load refresh DLL
	vid_isActive = qFalse;
	vid_fullscreen->modified = qTrue;
	vid_queueRestart = qTrue;

	VID_CheckChanges (outConfig);
}


/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if(vid_isActive) {
		R_Shutdown (qTrue);
		vid_isActive = qFalse;
	}

	Cmd_RemoveCommand ("vid_restart", NULL);
	Cmd_RemoveCommand ("listremaps", NULL);
}

/*
=============================================================================

	INIT / SHUTDOWN

=============================================================================
*/

/*
=================
GLimp_Shutdown
=================
*/
void GLimp_Shutdown (qBool full)
{
	X11_DestroyGLContext ();
}


/*
=================
GLimp_Init
=================
*/
qBool GLimp_Init (void)
{
	GLimp_Shutdown (qFalse);

	if (X11_CreateGLContext ()) {
		if (X11_GetGLAttribute (GLX_RGBA)) {
			ri.cColorBits = X11_GetGLAttribute (GLX_RED_SIZE) + X11_GetGLAttribute (GLX_GREEN_SIZE) + X11_GetGLAttribute (GLX_BLUE_SIZE);
			ri.cAlphaBits = X11_GetGLAttribute (GLX_ALPHA_SIZE);
		}
		else {
			// Indexed colors
			ri.cColorBits = ri.cAlphaBits = 0;
		}

		ri.cDepthBits = X11_GetGLAttribute (GLX_DEPTH_SIZE);
		ri.cStencilBits = X11_GetGLAttribute (GLX_STENCIL_SIZE);

		// Grab mouse input
		X11_SetKMGrab (qFalse, qTrue);
		return qTrue;
	}

	return qFalse;
}


/*
=================
GLimp_AttemptMode

Returns qTrue when the a mode change was successful
=================
*/
qBool GLimp_AttemptMode (qBool fullScreen, int width, int height)
{
	ri.config.vidFullScreen = fullScreen;
	ri.config.vidWidth = width;
	ri.config.vidHeight = height;
	// FIXME: These are windows specific really, they can be moved to glwState or something...
	ri.config.vidBitDepth = 0;
	ri.config.vidFrequency = 0;

	Com_Printf (0, "Mode: %d x %d %s\n", width, height, fullScreen ? "(fullscreen)" : "(windowed)");
	
	// Attempt fullscreen if desired
	if (fullScreen) {
		if (X11_SetVideoMode (width, height, qTrue))
			return qTrue;

		Com_Printf (PRNT_ERROR, "...fullscreen mode failed\n");
		return qFalse;
	}

	// Otherwise, attempt windowed mode
	Com_Printf (0, "...attempting windowed mode\n");
	if (X11_SetVideoMode (width, height, qFalse))
		return qTrue;

	Com_Printf (PRNT_ERROR, "...windowed mode failed\n");
	return qFalse;
}



/*
=================
GLimp_GetGammaRamp
=================
*/
qBool GLimp_GetGammaRamp (uint16 *ramp)
{
	return SCR_GetGammaRamp (ramp);
}


/*
=================
GLimp_SetGammaRamp
=================
*/
void GLimp_SetGammaRamp (uint16 *ramp)
{
	SCR_SetGammaRamp (ramp);
}
