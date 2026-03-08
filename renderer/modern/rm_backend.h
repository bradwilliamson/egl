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
// rm_backend.h - Modern renderer backend interface (MODERN-ONLY)
//

#ifndef __RM_BACKEND_H__
#define __RM_BACKEND_H__

/* This file is ONLY compiled when EGL_MODERN_RENDERER=1 */
#if !EGL_MODERN_RENDERER
#error "rm_backend.h should only be included in modern renderer builds"
#endif

#include "shared/shared.h"

struct refDef_s;

/*
=============================================================================

	MODERN RENDERER BACKEND INTERFACE

	This module provides a minimal backend interface for modern renderer.
	INFRASTRUCTURE ONLY - No rendering, no geometry, no shaders.
	
	Purpose: Allow runtime selection between legacy and modern backends.
	Constraint: Rendering behavior must remain identical to legacy.

=============================================================================
*/

/*
==================
Backend Interface Structure

Defines lifecycle functions for a renderer backend.
All functions are NO-OP stubs in this phase - infrastructure only.
==================
*/
typedef struct rm_backend_s {
	const char		*name;			/* Backend identifier */
	
	void			(*init)(void);			/* Initialize backend */
	void			(*shutdown)(void);		/* Shutdown backend */
	void			(*begin_frame)(struct refDef_s *fd);	/* Called at frame start */
	void			(*end_frame)(void);		/* Called at frame end */
} rm_backend_t;

/*
==================
RM_GetActiveBackend

Returns the currently active backend.
Returns NULL if no modern backend is active.
==================
*/
const rm_backend_t *RM_GetActiveBackend (void);

/*
==================
RM_SetBackend

Sets the active backend by name.
Valid names: "legacy", "modern"

Returns: qTrue on success, qFalse on failure
==================
*/
qBool RM_SetBackend (const char *name);

/*
==================
RM_IsModernBackendActive

Returns qTrue if a modern backend is currently active.
==================
*/
qBool RM_IsModernBackendActive (void);

/*
==================
Backend Lifecycle Functions

These are called by the bridge layer.
They dispatch to the active backend if one is set.
==================
*/
void RM_Backend_Init (void);
void RM_Backend_Shutdown (void);
void RM_Backend_BeginFrame (struct refDef_s *fd);
void RM_Backend_EndFrame (void);

#endif /* __RM_BACKEND_H__ */
