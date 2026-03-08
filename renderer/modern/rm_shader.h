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
// rm_shader.h - Modern renderer GLSL shader loader/compiler (MODERN-ONLY)
//

#ifndef __RM_SHADER_H__
#define __RM_SHADER_H__

/* This file is ONLY compiled when EGL_MODERN_RENDERER=1 */
#if !EGL_MODERN_RENDERER
#error "rm_shader.h should only be included in modern renderer builds"
#endif

#include "shared/shared.h"

/*
=============================================================================

	GLSL SHADER LOADER/COMPILER SUBSYSTEM (MODERN-ONLY)

	This module provides shader loading, compilation, and management.
	It is MODERN-ONLY and does not affect legacy rendering.
	
	Features:
	- File search: moddir/shaders/ then baseq2/shaders/
	- #include preprocessing with cycle detection
	- #version injection if missing (defaults to #version 330 core)
	- Full compile/link error reporting with line numbers
	- Shader caching and hot-reload support

=============================================================================
*/

#define RM_SHADER_MAX_INCLUDES 32
#define RM_SHADER_DEFAULT_VERSION "#version 330 core\n"

/* Shader program handle */
typedef struct rmShader_s {
	char			name[64];
	GLuint			program;
	GLuint			vertShader;
	GLuint			fragShader;
	qBool			valid;
	struct rmShader_s *next;
} rmShader_t;

/*
==================
RM_InitShaderSystem

Initializes the modern shader subsystem.
Must be called after OpenGL context creation and GLAD initialization.
==================
*/
void RM_InitShaderSystem (void);

/*
==================
RM_ShutdownShaderSystem

Shuts down the shader subsystem and frees all resources.
==================
*/
void RM_ShutdownShaderSystem (void);

/*
==================
RM_LoadShader

Loads, compiles, and links a shader program.
Searches for <name>.vert and <name>.frag in:
  1. moddir/shaders/
  2. baseq2/shaders/

Returns: Shader program handle, or NULL on failure
==================
*/
rmShader_t *RM_LoadShader (const char *name);

/*
==================
RM_ReloadShader

Reloads a shader from disk and recompiles.
Returns: qTrue on success, qFalse on failure
==================
*/
qBool RM_ReloadShader (const char *name);

/*
==================
RM_ReloadAllShaders

Reloads all loaded shaders from disk.
==================
*/
void RM_ReloadAllShaders (void);

/*
==================
RM_FindShader

Finds a loaded shader by name.
Returns: Shader handle, or NULL if not found
==================
*/
rmShader_t *RM_FindShader (const char *name);

/*
==================
RM_ListShaders

Lists all loaded shaders to console.
==================
*/
void RM_ListShaders (void);

/*
==================
Console Commands (Modern-only)
==================
*/
void RM_ShaderTest_f (void);
void RM_ShaderList_f (void);
void RM_ShaderReload_f (void);

#endif /* __RM_SHADER_H__ */
