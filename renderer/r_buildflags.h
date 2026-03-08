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
// r_buildflags.h - Renderer build configuration flags
//

#ifndef __R_BUILDFLAGS_H__
#define __R_BUILDFLAGS_H__

/*
=============================================================================

	RENDERER BUILD CONFIGURATION

	These flags control which renderer implementations are compiled in.
	They are set by the build system via -D flags, but have safe defaults.

	Build Configurations:
	1. Legacy-only (default):  EGL_LEGACY_RENDERER=1, EGL_MODERN_RENDERER=0
	2. Dual (both compiled):   EGL_LEGACY_RENDERER=1, EGL_MODERN_RENDERER=1
	3. Modern-only (future):   EGL_LEGACY_RENDERER=0, EGL_MODERN_RENDERER=1

=============================================================================
*/

/* Default to legacy-only if not specified by build system */
#ifndef EGL_LEGACY_RENDERER
# define EGL_LEGACY_RENDERER 1
#endif

#ifndef EGL_MODERN_RENDERER
# define EGL_MODERN_RENDERER 0
#endif

/* Sanity check: At least one renderer must be enabled */
#if !EGL_LEGACY_RENDERER && !EGL_MODERN_RENDERER
# error "At least one renderer (EGL_LEGACY_RENDERER or EGL_MODERN_RENDERER) must be enabled"
#endif

/* Configuration validation */
#if EGL_LEGACY_RENDERER && EGL_MODERN_RENDERER
# define EGL_RENDERER_CONFIG "Dual (Legacy + Modern)"
#elif EGL_LEGACY_RENDERER
# define EGL_RENDERER_CONFIG "Legacy-only"
#elif EGL_MODERN_RENDERER
# define EGL_RENDERER_CONFIG "Modern-only"
#endif

#endif /* __R_BUILDFLAGS_H__ */
