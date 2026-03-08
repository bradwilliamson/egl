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
// r_caps.c - OpenGL capabilities detection implementation
//

#include "r_local.h"
#include "r_caps.h"
#include "glad/glad.h"
#if EGL_MODERN_RENDERER
# include "modern/rm_caps.h"
#endif
#include <string.h>
#include <stdlib.h>

/* Fallback defines for constants not in GL 1.1 headers */
#ifndef GL_MAX_DRAW_BUFFERS
#define GL_MAX_DRAW_BUFFERS 0x8824
#endif

#ifndef GL_MAX_SAMPLES
#define GL_MAX_SAMPLES 0x8D57
#endif

#ifndef GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#endif

#ifndef GL_MAX_VERTEX_ATTRIBS
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#endif

#ifndef GL_MAX_VERTEX_UNIFORM_COMPONENTS
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS 0x8B4A
#endif

#ifndef GL_MAX_FRAGMENT_UNIFORM_COMPONENTS
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#endif

#ifndef GL_MAX_VARYING_FLOATS
#define GL_MAX_VARYING_FLOATS 0x8B4B
#endif

#ifndef GL_MAX_UNIFORM_BLOCK_SIZE
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#endif

#ifndef GL_MAX_UNIFORM_BUFFER_BINDINGS
#define GL_MAX_UNIFORM_BUFFER_BINDINGS 0x8A2F
#endif

#ifndef GL_MAX_CUBE_MAP_TEXTURE_SIZE
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#endif

#ifndef GL_MAX_3D_TEXTURE_SIZE
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

/* Global capabilities structure */
glCaps_t glCaps;

/*
==================
GL_ParseVersion

Parse OpenGL version string into major.minor.
Example: "4.6.0 NVIDIA 531.61" -> major=4, minor=6
==================
*/
static void GL_ParseVersion (const char *versionString, int *major, int *minor)
{
	*major = 0;
	*minor = 0;
	
	if (!versionString) {
		return;
	}
	
	/* Parse major.minor from start of string */
	if (sscanf (versionString, "%d.%d", major, minor) < 2) {
		/* Fallback: try to parse just major */
		sscanf (versionString, "%d", major);
	}
}

/*
==================
GL_CheckExtension

Check if an extension is supported via extension string.
==================
*/
static qBool GL_CheckExtension (const char *extString, const char *extension)
{
	const char *start;
	const char *where, *terminator;
	
	if (!extString || !extension) {
		return qFalse;
	}
	
	/* Extension names should not have spaces */
	where = strchr (extension, ' ');
	if (where || *extension == '\0') {
		return qFalse;
	}
	
	start = extString;
	for (;;) {
		where = strstr (start, extension);
		if (!where) {
			break;
		}
		terminator = where + strlen (extension);
		if (where == start || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0') {
				return qTrue;
			}
		}
		start = terminator;
	}
	
	return qFalse;
}

/*
==================
GL_InitCaps

Detects and initializes OpenGL capabilities.
==================
*/
void GL_InitCaps (void)
{
	const GLubyte *versionString;
	const GLubyte *glslString;
	const GLubyte *vendorString;
	const GLubyte *rendererString;
	const GLubyte *extensionString;
	GLint intVal;
	
	Com_Printf (0, "Detecting OpenGL capabilities...\n");
	
	/* Zero out structure */
	memset (&glCaps, 0, sizeof(glCaps));
	
	/* Get version strings */
	versionString = glGetString (GL_VERSION);
	vendorString = glGetString (GL_VENDOR);
	rendererString = glGetString (GL_RENDERER);
	extensionString = glGetString (GL_EXTENSIONS);
	
	glCaps.glVersionString = versionString ? (const char*)versionString : "Unknown";
	glCaps.vendorString = vendorString ? (const char*)vendorString : "Unknown";
	glCaps.rendererString = rendererString ? (const char*)rendererString : "Unknown";
	
	/* Parse GL version */
	GL_ParseVersion (glCaps.glVersionString, &glCaps.glVersionMajor, &glCaps.glVersionMinor);
	
	/* Get GLSL version (GL 2.0+) */
	if (glCaps.glVersionMajor >= 2) {
		glslString = glGetString (GL_SHADING_LANGUAGE_VERSION);
		glCaps.glslVersionString = glslString ? (const char*)glslString : "Not supported";
	} else {
		glCaps.glslVersionString = "Not supported";
	}
	
	/* Core Features Detection */
	
	/* VBO: GL 1.5 or GL_ARB_vertex_buffer_object */
	glCaps.hasVBO = (glCaps.glVersionMajor > 1 || (glCaps.glVersionMajor == 1 && glCaps.glVersionMinor >= 5)) ||
	                 GL_CheckExtension ((const char*)extensionString, "GL_ARB_vertex_buffer_object");
	
	/* VAO: GL 3.0 or GL_ARB_vertex_array_object */
	glCaps.hasVAO = (glCaps.glVersionMajor >= 3) ||
	                 GL_CheckExtension ((const char*)extensionString, "GL_ARB_vertex_array_object");
	
	/* FBO: GL 3.0 or GL_ARB_framebuffer_object */
	glCaps.hasFBO = (glCaps.glVersionMajor >= 3) ||
	                 GL_CheckExtension ((const char*)extensionString, "GL_ARB_framebuffer_object");
	
	/* Shaders: GL 2.0 or GL_ARB_shader_objects */
	glCaps.hasShaders = (glCaps.glVersionMajor >= 2) ||
	                     GL_CheckExtension ((const char*)extensionString, "GL_ARB_shader_objects");
	
	/* UBO: GL 3.1 or GL_ARB_uniform_buffer_object */
	glCaps.hasUBO = (glCaps.glVersionMajor > 3 || (glCaps.glVersionMajor == 3 && glCaps.glVersionMinor >= 1)) ||
	                 GL_CheckExtension ((const char*)extensionString, "GL_ARB_uniform_buffer_object");
	
	/* Map Buffer: GL 3.0 or GL_ARB_map_buffer_range */
	glCaps.hasMapBuffer = (glCaps.glVersionMajor >= 3) ||
	                       GL_CheckExtension ((const char*)extensionString, "GL_ARB_map_buffer_range");
	
	/* Sync: GL 3.2 or GL_ARB_sync */
	glCaps.hasSync = (glCaps.glVersionMajor > 3 || (glCaps.glVersionMajor == 3 && glCaps.glVersionMinor >= 2)) ||
	                  GL_CheckExtension ((const char*)extensionString, "GL_ARB_sync");
	
	/* Debug Output: GL 4.3 or GL_ARB_debug_output */
	glCaps.hasDebugOutput = (glCaps.glVersionMajor > 4 || (glCaps.glVersionMajor == 4 && glCaps.glVersionMinor >= 3)) ||
	                         GL_CheckExtension ((const char*)extensionString, "GL_ARB_debug_output");
	
	/* Texture Features */
	glCaps.hasTextureCompression = GL_CheckExtension ((const char*)extensionString, "GL_ARB_texture_compression");
	glCaps.hasAnisotropic = GL_CheckExtension ((const char*)extensionString, "GL_EXT_texture_filter_anisotropic");
	glCaps.hasNPOT = GL_CheckExtension ((const char*)extensionString, "GL_ARB_texture_non_power_of_two");
	glCaps.hasSRGB = GL_CheckExtension ((const char*)extensionString, "GL_EXT_texture_sRGB");
	glCaps.hasMultisample = GL_CheckExtension ((const char*)extensionString, "GL_ARB_multisample");
	
	/* Query Limits */
	
	/* Texture limits */
	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &glCaps.maxTextureSize);
	
	/* Legacy texture units (GL 1.x) */
	glGetIntegerv (GL_MAX_TEXTURE_UNITS, &intVal);
	glCaps.maxTextureUnits = intVal;
	
	/* Modern combined texture units (GL 2.0+) */
	if (glCaps.glVersionMajor >= 2) {
		glGetIntegerv (GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &intVal);
		glCaps.maxCombinedTextureUnits = intVal;
	}
	
	/* Anisotropic filtering */
	if (glCaps.hasAnisotropic) {
		GLfloat floatVal;
		glGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &floatVal);
		glCaps.maxAnisotropy = (int)floatVal;
	}
	
	/* Cube map size */
	if (GL_CheckExtension ((const char*)extensionString, "GL_ARB_texture_cube_map")) {
		glGetIntegerv (GL_MAX_CUBE_MAP_TEXTURE_SIZE, &intVal);
		glCaps.maxCubeMapSize = intVal;
	}
	
	/* 3D texture size */
	if (GL_CheckExtension ((const char*)extensionString, "GL_EXT_texture3D")) {
		glGetIntegerv (GL_MAX_3D_TEXTURE_SIZE, &intVal);
		glCaps.max3DTextureSize = intVal;
	}
	
	/* Vertex/Fragment limits (GL 2.0+) */
	if (glCaps.hasShaders) {
		glGetIntegerv (GL_MAX_VERTEX_ATTRIBS, &intVal);
		glCaps.maxVertexAttribs = intVal;
		
		glGetIntegerv (GL_MAX_VERTEX_UNIFORM_COMPONENTS, &intVal);
		glCaps.maxVertexUniforms = intVal;
		
		glGetIntegerv (GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &intVal);
		glCaps.maxFragmentUniforms = intVal;
		
		if (glCaps.glVersionMajor >= 3) {
			glGetIntegerv (GL_MAX_VARYING_FLOATS, &intVal);
			glCaps.maxVaryingFloats = intVal;
		}
	}
	
	/* UBO limits (GL 3.1+) */
	if (glCaps.hasUBO) {
		glGetIntegerv (GL_MAX_UNIFORM_BLOCK_SIZE, &intVal);
		glCaps.maxUBOSize = intVal;
		
		glGetIntegerv (GL_MAX_UNIFORM_BUFFER_BINDINGS, &intVal);
		glCaps.maxUBOBindings = intVal;
	}
	
	/* Draw buffers (GL 2.0+) */
	if (glCaps.glVersionMajor >= 2) {
		glGetIntegerv (GL_MAX_DRAW_BUFFERS, &intVal);
		glCaps.maxDrawBuffers = intVal;
	}
	
	/* Multisample (MSAA) */
	if (glCaps.hasMultisample && glCaps.glVersionMajor >= 3) {
		glGetIntegerv (GL_MAX_SAMPLES, &intVal);
		glCaps.maxSamples = intVal;
	}
	
	/* Viewport limits */
	{
		GLint viewport[2];
		glGetIntegerv (GL_MAX_VIEWPORT_DIMS, viewport);
		glCaps.maxViewportWidth = viewport[0];
		glCaps.maxViewportHeight = viewport[1];
	}
	
	Com_Printf (0, "OpenGL capabilities detection complete.\n");
}

/*
==================
GL_Caps_f

Console command handler for "gl_caps".
Prints comprehensive capabilities report.
==================
*/
void GL_Caps_f (void)
{
	Com_Printf (0, "========================================\n");
	Com_Printf (0, "OpenGL Capabilities Report\n");
	Com_Printf (0, "========================================\n");
	
	/* Build Configuration Header */
	Com_Printf (0, "\n--- Build Configuration ---\n");
#if EGL_LEGACY_RENDERER && EGL_MODERN_RENDERER
	Com_Printf (0, "Build: LEGACY=1 MODERN=1");
#elif EGL_LEGACY_RENDERER
	Com_Printf (0, "Build: LEGACY=1 MODERN=0");
#elif EGL_MODERN_RENDERER
	Com_Printf (0, "Build: LEGACY=0 MODERN=1");
#else
	Com_Printf (0, "Build: LEGACY=0 MODERN=0");
#endif
#ifdef HAVE_SDL2
	Com_Printf (0, " SDL2=1\n");
#else
	Com_Printf (0, " SDL2=0\n");
#endif
	Com_Printf (0, "Config: %s\n", EGL_RENDERER_CONFIG);
	
	/* Version Section */
	Com_Printf (0, "\n--- Version ---\n");
	Com_Printf (0, "GL Version:   %d.%d (%s)\n", glCaps.glVersionMajor, glCaps.glVersionMinor, glCaps.glVersionString);
	Com_Printf (0, "GLSL Version: %s\n", glCaps.glslVersionString);
	Com_Printf (0, "Vendor:       %s\n", glCaps.vendorString);
	Com_Printf (0, "Renderer:     %s\n", glCaps.rendererString);
	
	/* Core Features Section */
	Com_Printf (0, "\n--- Core Features ---\n");
	Com_Printf (0, "hasVBO:         %s\n", glCaps.hasVBO ? "true" : "false");
	Com_Printf (0, "hasVAO:         %s\n", glCaps.hasVAO ? "true" : "false");
	Com_Printf (0, "hasFBO:         %s\n", glCaps.hasFBO ? "true" : "false");
	Com_Printf (0, "hasShaders:     %s\n", glCaps.hasShaders ? "true" : "false");
	Com_Printf (0, "hasUBO:         %s\n", glCaps.hasUBO ? "true" : "false");
	Com_Printf (0, "hasMapBuffer:   %s\n", glCaps.hasMapBuffer ? "true" : "false");
	Com_Printf (0, "hasSync:        %s\n", glCaps.hasSync ? "true" : "false");
	Com_Printf (0, "hasDebugOutput: %s\n", glCaps.hasDebugOutput ? "true" : "false");
	
	/* Texture Features Section */
	Com_Printf (0, "\n--- Texture Features ---\n");
	Com_Printf (0, "hasTextureCompression: %s\n", glCaps.hasTextureCompression ? "true" : "false");
	Com_Printf (0, "hasAnisotropic:        %s\n", glCaps.hasAnisotropic ? "true" : "false");
	Com_Printf (0, "hasNPOT:               %s\n", glCaps.hasNPOT ? "true" : "false");
	Com_Printf (0, "hasSRGB:               %s\n", glCaps.hasSRGB ? "true" : "false");
	Com_Printf (0, "hasMultisample:        %s\n", glCaps.hasMultisample ? "true" : "false");
	
	/* Texture Limits Section */
	Com_Printf (0, "\n--- Texture Limits ---\n");
	Com_Printf (0, "maxTextureSize:           %d\n", glCaps.maxTextureSize);
	Com_Printf (0, "maxTextureUnits:          %d\n", glCaps.maxTextureUnits);
	if (glCaps.maxCombinedTextureUnits > 0) {
		Com_Printf (0, "maxCombinedTextureUnits:  %d\n", glCaps.maxCombinedTextureUnits);
	}
	if (glCaps.hasAnisotropic) {
		Com_Printf (0, "maxAnisotropy:            %d\n", glCaps.maxAnisotropy);
	}
	if (glCaps.maxCubeMapSize > 0) {
		Com_Printf (0, "maxCubeMapSize:           %d\n", glCaps.maxCubeMapSize);
	}
	if (glCaps.max3DTextureSize > 0) {
		Com_Printf (0, "max3DTextureSize:         %d\n", glCaps.max3DTextureSize);
	}
	
	/* Shader Limits Section */
	if (glCaps.hasShaders) {
		Com_Printf (0, "\n--- Shader Limits ---\n");
		Com_Printf (0, "maxVertexAttribs:     %d\n", glCaps.maxVertexAttribs);
		Com_Printf (0, "maxVertexUniforms:    %d\n", glCaps.maxVertexUniforms);
		Com_Printf (0, "maxFragmentUniforms:  %d\n", glCaps.maxFragmentUniforms);
		if (glCaps.maxVaryingFloats > 0) {
			Com_Printf (0, "maxVaryingFloats:     %d\n", glCaps.maxVaryingFloats);
		}
	}
	
	/* Buffer Limits Section */
	Com_Printf (0, "\n--- Buffer Limits ---\n");
	if (glCaps.hasUBO) {
		Com_Printf (0, "maxUBOSize:      %d bytes\n", glCaps.maxUBOSize);
		Com_Printf (0, "maxUBOBindings:  %d\n", glCaps.maxUBOBindings);
	}
	if (glCaps.maxDrawBuffers > 0) {
		Com_Printf (0, "maxDrawBuffers:  %d\n", glCaps.maxDrawBuffers);
	}
	if (glCaps.maxSamples > 0) {
		Com_Printf (0, "maxSamples:      %d (MSAA)\n", glCaps.maxSamples);
	}
	
	/* Viewport Limits Section */
	Com_Printf (0, "\n--- Viewport Limits ---\n");
	Com_Printf (0, "maxViewportDims: %d x %d\n", glCaps.maxViewportWidth, glCaps.maxViewportHeight);
	
	Com_Printf (0, "\n========================================\n");

#if EGL_MODERN_RENDERER
	RM_Caps_Print ();
#endif
}
