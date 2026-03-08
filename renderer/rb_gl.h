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
// rb_gl.h - GLAD-based OpenGL loader wrapper for EGL Quake 2
//

#ifndef __RB_GL_H__
#define __RB_GL_H__

/*
=============================================================================

	GLAD LOADER INTERFACE

	This module provides a clean interface to GLAD (OpenGL loader)
	while preserving the existing qgl* API used throughout the codebase.

=============================================================================
*/

/*
==================
GL_GetProcAddress

Backend-neutral OpenGL function pointer loader.
Returns function pointer for the given OpenGL function name.

Implementation:
- SDL2: Uses SDL_GL_GetProcAddress
- Win32: Uses wglGetProcAddress or GetProcAddress
- Unix: Uses dlsym

Returns: Function pointer or NULL if not found
==================
*/
void *GL_GetProcAddress (const char *name);

/*
==================
GL_InitLoader

Initializes the GLAD OpenGL loader using SDL_GL_GetProcAddress.
Must be called after OpenGL context creation.

Returns: qTrue on success, qFalse on failure
==================
*/
qBool GL_InitLoader (void);

/*
==================
GL_DetectVersion

Queries and prints OpenGL version, vendor, and renderer information.
Should be called after GL_InitLoader() succeeds.
==================
*/
void GL_DetectVersion (void);

/*
==================
GL_Loader_f

Console command handler for "gl_loader".
Prints GLAD loader status and OpenGL information.
==================
*/
void GL_Loader_f (void);

#endif /* __RB_GL_H__ */
