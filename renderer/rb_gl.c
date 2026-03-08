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
// rb_gl.c - GLAD-based OpenGL loader implementation
//

#include "r_local.h"
#include "rb_gl.h"
#include "glad/glad.h"

#ifdef HAVE_SDL2
# include <SDL2/SDL.h>
#endif

/* Static variables to track loader state */
static qBool gl_loader_initialized = qFalse;

/*
==================
GL_GetProcAddress

Backend-neutral OpenGL function pointer loader.
This provides a unified interface for loading OpenGL function pointers
across different platforms and backends.
==================
*/
void *GL_GetProcAddress (const char *name)
{
#ifdef HAVE_SDL2
	/* SDL2 backend: Use SDL's cross-platform loader */
	return SDL_GL_GetProcAddress (name);
#elif defined(_WIN32)
	/* Win32 backend: Use the existing QGL_GetProcAddress from rb_qgl.c */
	extern void *QGL_GetProcAddress (const char *name);
	return QGL_GetProcAddress (name);
#elif defined(__unix__)
	/* Unix backend: Use the existing QGL_GetProcAddress from rb_qgl.c */
	extern void *QGL_GetProcAddress (const char *name);
	return QGL_GetProcAddress (name);
#else
	/* Fallback: Should not reach here */
	return NULL;
#endif
}

/*
==================
GL_ProcLoader

Internal wrapper to get OpenGL function pointers.
This is passed as a callback to GLAD's gladLoadGLLoader.
==================
*/
static void *GL_ProcLoader (const char *name)
{
	return GL_GetProcAddress (name);
}

/*
==================
GL_InitLoader

Initializes the GLAD OpenGL loader using SDL_GL_GetProcAddress.
Must be called after OpenGL context creation.

Returns: qTrue on success, qFalse on failure
==================
*/
qBool GL_InitLoader (void)
{
	int version;

	if (gl_loader_initialized) {
		Com_Printf (0, "GL_InitLoader: Already initialized\n");
		return qTrue;
	}

	Com_Printf (0, "GL_InitLoader: Initializing GLAD loader...\n");

	/* Load OpenGL function pointers via GLAD */
	version = gladLoadGLLoader (GL_ProcLoader);
	
	if (!version) {
		Com_Printf (PRNT_ERROR, "GLAD: init FAILED (continuing legacy)\n");
		return qFalse;
	}

	gl_loader_initialized = qTrue;
	
	/* Print success with backend method */
#ifdef HAVE_SDL2
	Com_Printf (0, "GLAD: initialized (SDL_GL_GetProcAddress)\n");
#elif defined(_WIN32)
	Com_Printf (0, "GLAD: initialized (wglGetProcAddress)\n");
#elif defined(__unix__)
	Com_Printf (0, "GLAD: initialized (dlsym)\n");
#else
	Com_Printf (0, "GLAD: initialized (unknown backend)\n");
#endif

	return qTrue;
}

/*
==================
GL_DetectVersion

Queries and prints OpenGL version, vendor, and renderer information.
Should be called after GL_InitLoader() succeeds.
==================
*/
void GL_DetectVersion (void)
{
	const GLubyte *vendor;
	const GLubyte *renderer;
	const GLubyte *version;

	if (!gl_loader_initialized) {
		Com_Printf (PRNT_WARNING, "GL_DetectVersion: Loader not initialized\n");
		return;
	}

	/* Query OpenGL strings */
	vendor = glGetString (GL_VENDOR);
	renderer = glGetString (GL_RENDERER);
	version = glGetString (GL_VERSION);

	/* Print detected information */
	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "OpenGL Information:\n");
	Com_Printf (0, "  Vendor:   %s\n", vendor ? (const char*)vendor : "Unknown");
	Com_Printf (0, "  Renderer: %s\n", renderer ? (const char*)renderer : "Unknown");
	Com_Printf (0, "  Version:  %s\n", version ? (const char*)version : "Unknown");
	Com_Printf (0, "----------------------------------------\n");
}

/*
==================
GL_Loader_f

Console command handler for "gl_loader".
Prints detected OpenGL information.
==================
*/
void GL_Loader_f (void)
{
	if (!gl_loader_initialized) {
		Com_Printf (0, "gl_loader: GLAD loader not initialized\n");
		Com_Printf (0, "  Status: NOT INITIALIZED (using legacy qgl* loader)\n");
		return;
	}

	Com_Printf (0, "gl_loader: GLAD loader status\n");
#ifdef HAVE_SDL2
	Com_Printf (0, "  Status: INITIALIZED (SDL_GL_GetProcAddress)\n");
#elif defined(_WIN32)
	Com_Printf (0, "  Status: INITIALIZED (wglGetProcAddress)\n");
#elif defined(__unix__)
	Com_Printf (0, "  Status: INITIALIZED (dlsym)\n");
#else
	Com_Printf (0, "  Status: INITIALIZED (unknown backend)\n");
#endif
	GL_DetectVersion ();
}
