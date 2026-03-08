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
// rm_debug.h - Modern renderer GL debug output helpers (MODERN-ONLY)
//

#ifndef __RM_DEBUG_H__
#define __RM_DEBUG_H__

/* This file is ONLY compiled when EGL_MODERN_RENDERER=1 */
#if !EGL_MODERN_RENDERER
#error "rm_debug.h should only be included in modern renderer builds"
#endif

#include "shared/shared.h"

void RM_Debug_Init (void);
void RM_Debug_PushGroup (const char *name);
void RM_Debug_PopGroup (void);

#endif /* __RM_DEBUG_H__ */

