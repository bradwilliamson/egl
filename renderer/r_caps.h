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
// r_caps.h - OpenGL capabilities detection (shared/neutral)
//

#ifndef __R_CAPS_H__
#define __R_CAPS_H__

#include "shared/shared.h"

/*
=============================================================================

	OPENGL CAPABILITIES DETECTION

	This module provides comprehensive OpenGL capabilities detection.
	It is SHARED/NEUTRAL - works with both legacy and modern renderers.
	
	NO RENDERING LOGIC - detection only.
	NO BEHAVIOR CHANGES - information gathering only.

=============================================================================
*/

typedef struct glCaps_s {
	/* Version Information */
	int				glVersionMajor;
	int				glVersionMinor;
	const char		*glVersionString;
	const char		*glslVersionString;
	
	/* Vendor/Renderer Information */
	const char		*vendorString;
	const char		*rendererString;
	
	/* Core Features (OpenGL 1.x - 3.x) */
	qBool			hasVBO;					/* GL_ARB_vertex_buffer_object / GL 1.5 */
	qBool			hasVAO;					/* GL_ARB_vertex_array_object / GL 3.0 */
	qBool			hasFBO;					/* GL_ARB_framebuffer_object / GL 3.0 */
	qBool			hasShaders;				/* GL_ARB_shader_objects / GL 2.0 */
	qBool			hasUBO;					/* GL_ARB_uniform_buffer_object / GL 3.1 */
	qBool			hasMapBuffer;			/* GL_ARB_map_buffer_range / GL 3.0 */
	qBool			hasSync;				/* GL_ARB_sync / GL 3.2 */
	qBool			hasDebugOutput;			/* GL_ARB_debug_output / GL 4.3 */
	
	/* Texture Features */
	qBool			hasTextureCompression;	/* GL_ARB_texture_compression */
	qBool			hasAnisotropic;			/* GL_EXT_texture_filter_anisotropic */
	qBool			hasNPOT;				/* GL_ARB_texture_non_power_of_two */
	qBool			hasSRGB;				/* GL_EXT_texture_sRGB */
	qBool			hasMultisample;			/* GL_ARB_multisample */
	
	/* Limits - Textures */
	int				maxTextureSize;
	int				maxTextureUnits;		/* Legacy: GL_MAX_TEXTURE_UNITS */
	int				maxCombinedTextureUnits;/* Modern: GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS */
	int				maxAnisotropy;
	int				maxCubeMapSize;
	int				max3DTextureSize;
	
	/* Limits - Vertex/Fragment */
	int				maxVertexAttribs;
	int				maxVertexUniforms;
	int				maxFragmentUniforms;
	int				maxVaryingFloats;
	
	/* Limits - Buffers */
	int				maxUBOSize;				/* GL_MAX_UNIFORM_BLOCK_SIZE */
	int				maxUBOBindings;			/* GL_MAX_UNIFORM_BUFFER_BINDINGS */
	int				maxDrawBuffers;
	int				maxSamples;				/* For MSAA */
	
	/* Limits - Viewport */
	int				maxViewportWidth;
	int				maxViewportHeight;
	
} glCaps_t;

/* Global capabilities structure (read-only after initialization) */
extern glCaps_t glCaps;

/*
==================
GL_InitCaps

Detects and initializes OpenGL capabilities.
Must be called after OpenGL context creation and GLAD initialization.

This function only DETECTS capabilities - it does not change any behavior.
==================
*/
void GL_InitCaps (void);

/*
==================
GL_Caps_f

Console command handler for "gl_caps".
Prints comprehensive OpenGL capabilities report.
==================
*/
void GL_Caps_f (void);

#endif /* __R_CAPS_H__ */
