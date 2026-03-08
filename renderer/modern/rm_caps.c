#define RM_NO_LEGACY_GL
#include "../glad/glad.h"

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
// rm_caps.c - Modern renderer OpenGL capability detection implementation
//

#include "../r_local.h"
#include "../rb_gl.h"
#include "rm_caps.h"

#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

/* Fallback defines for enums missing from minimal GL headers */
#ifndef GL_MAJOR_VERSION
#define GL_MAJOR_VERSION 0x821B
#endif

#ifndef GL_MINOR_VERSION
#define GL_MINOR_VERSION 0x821C
#endif

#ifndef GL_SHADING_LANGUAGE_VERSION
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

#ifndef GL_NUM_EXTENSIONS
#define GL_NUM_EXTENSIONS 0x821D
#endif

#ifndef GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#endif

#ifndef GL_MAX_UNIFORM_BLOCK_SIZE
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#endif

#ifndef GL_MAX_SHADER_STORAGE_BLOCK_SIZE
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#endif

#ifndef GL_MAX_COMPUTE_WORK_GROUP_SIZE
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE 0x91BF
#endif

#ifndef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_MAX_TEXTURE_UNITS
#define GL_MAX_TEXTURE_UNITS 0x84E2
#endif

#ifndef GL_MAX_TEXTURE_IMAGE_UNITS
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#endif

typedef int64_t GLint64;

typedef const GLubyte *(APIENTRY *PFNGLGETSTRINGIPROC)(GLenum name, GLuint index);
typedef void (APIENTRY *PFNGLGETINTEGERI_VPROC)(GLenum target, GLuint index, GLint *data);
typedef void (APIENTRY *PFNGLGETINTEGER64VPROC)(GLenum pname, GLint64 *data);

static PFNGLGETSTRINGIPROC rm_glGetStringi = NULL;
static PFNGLGETINTEGERI_VPROC rm_glGetIntegeri_v = NULL;
static PFNGLGETINTEGER64VPROC rm_glGetInteger64v = NULL;

rmCaps_t rm_caps;

static qBool rm_capsInitialized = qFalse;

static void RM_Caps_LoadProcFallbacks (void)
{
	if (!rm_glGetStringi)
		rm_glGetStringi = (PFNGLGETSTRINGIPROC)GL_GetProcAddress ("glGetStringi");
	if (!rm_glGetIntegeri_v)
		rm_glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)GL_GetProcAddress ("glGetIntegeri_v");
	if (!rm_glGetInteger64v)
		rm_glGetInteger64v = (PFNGLGETINTEGER64VPROC)GL_GetProcAddress ("glGetInteger64v");
}

static qBool RM_GLVersionAtLeast (int major, int minor)
{
	if (rm_caps.glVersionMajor > major)
		return qTrue;
	if (rm_caps.glVersionMajor < major)
		return qFalse;
	return (rm_caps.glVersionMinor >= minor) ? qTrue : qFalse;
}

static int RM_ParseVersionNumber (const char *versionString)
{
	int major = 0;
	int minor = 0;

	if (!versionString)
		return 0;
	if (sscanf (versionString, "%d.%d", &major, &minor) < 2)
		return 0;

	/* GLSL convention: "4.60" -> 460, "3.30" -> 330 */
	return major * 100 + minor;
}

static qBool RM_StrIContains (const char *haystack, const char *needle)
{
	size_t needleLen;
	size_t i;

	if (!haystack || !needle || !*needle)
		return qFalse;

	needleLen = strlen (needle);
	for (i = 0; haystack[i] != '\0'; i++) {
		size_t j;
		for (j = 0; j < needleLen; j++) {
			unsigned char hc = (unsigned char)haystack[i + j];
			unsigned char nc = (unsigned char)needle[j];
			if (hc == '\0')
				return qFalse;
			if (tolower (hc) != tolower (nc))
				break;
		}
		if (j == needleLen)
			return qTrue;
	}

	return qFalse;
}

static qBool RM_CheckExtensionString (const char *extString, const char *extension)
{
	const char *start;
	const char *where, *terminator;

	if (!extString || !extension)
		return qFalse;

	/* Extension names should not have spaces */
	where = strchr (extension, ' ');
	if (where || *extension == '\0')
		return qFalse;

	start = extString;
	for (;;) {
		where = strstr (start, extension);
		if (!where)
			break;
		terminator = where + strlen (extension);
		if (where == start || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0')
				return qTrue;
		}
		start = terminator;
	}

	return qFalse;
}

static qBool RM_HasExtension (const char *extension)
{
	GLint i, numExt;
	const char *extString;

	if (!extension || !*extension)
		return qFalse;

	RM_Caps_LoadProcFallbacks ();

	/* Preferred method for core profiles / GL >= 3.0 */
	if (rm_glGetStringi) {
		numExt = 0;
		glGetIntegerv (GL_NUM_EXTENSIONS, &numExt);
		for (i = 0; i < numExt; i++) {
			const char *ext = (const char *)rm_glGetStringi (GL_EXTENSIONS, (GLuint)i);
			if (ext && !strcmp (ext, extension))
				return qTrue;
		}
		return qFalse;
	}

	/* Fallback for compatibility contexts */
	extString = (const char *)glGetString (GL_EXTENSIONS);
	return RM_CheckExtensionString (extString, extension);
}

static void RM_Caps_QueryComputeLimits (void)
{
	int i;

	if (!rm_caps.hasComputeShaders)
		return;

	/* Work group invocations (non-indexed) */
	rm_caps.maxComputeWorkGroupInvocations = 0;
	glGetIntegerv (GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &rm_caps.maxComputeWorkGroupInvocations);

	/* Work group size is indexed; requires glGetIntegeri_v */
	for (i = 0; i < 3; i++)
		rm_caps.maxComputeWorkGroupSize[i] = 0;

	RM_Caps_LoadProcFallbacks ();
	if (!rm_glGetIntegeri_v)
		return;

	rm_glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &rm_caps.maxComputeWorkGroupSize[0]);
	rm_glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &rm_caps.maxComputeWorkGroupSize[1]);
	rm_glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &rm_caps.maxComputeWorkGroupSize[2]);
}

static void RM_Caps_QuerySSBOLimit (void)
{
	GLint v32;
	GLint64 v64;

	rm_caps.maxSSBOSize = 0;

	if (!rm_caps.hasSSBO)
		return;

	RM_Caps_LoadProcFallbacks ();
	if (rm_glGetInteger64v) {
		v64 = 0;
		rm_glGetInteger64v (GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &v64);
		if (v64 > (GLint64)INT_MAX)
			rm_caps.maxSSBOSize = INT_MAX;
		else if (v64 > 0)
			rm_caps.maxSSBOSize = (int)v64;
		return;
	}

	/* Best-effort fallback */
	v32 = 0;
	glGetIntegerv (GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &v32);
	if (v32 > 0)
		rm_caps.maxSSBOSize = v32;
}

static void RM_Caps_QueryTextureUnitLimit (void)
{
	rm_caps.maxTextureUnits = 0;
	glGetIntegerv (GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &rm_caps.maxTextureUnits);
	if (rm_caps.maxTextureUnits > 0)
		return;

	/* Best-effort fallbacks for older contexts */
	glGetIntegerv (GL_MAX_TEXTURE_IMAGE_UNITS, &rm_caps.maxTextureUnits);
	if (rm_caps.maxTextureUnits > 0)
		return;

	glGetIntegerv (GL_MAX_TEXTURE_UNITS, &rm_caps.maxTextureUnits);
}

static const char *RM_BoolStr (qBool value)
{
	return value ? "true" : "false";
}

void RM_Caps_Print (void)
{
	if (!rm_capsInitialized) {
		Com_Printf (PRNT_WARNING, "RM_Caps_Print: capabilities not initialized\n");
		return;
	}

	Com_Printf (0, "----------------------------------------\n");
	Com_Printf (0, "Modern Renderer OpenGL Capabilities\n");
	Com_Printf (0, "----------------------------------------\n");

	Com_Printf (0, "Version: %d.%d  GLSL: %d\n", rm_caps.glVersionMajor, rm_caps.glVersionMinor, rm_caps.glslVersion);
	Com_Printf (0, "Vendor:  nvidia=%s amd=%s intel=%s mesa=%s\n",
		RM_BoolStr (rm_caps.isNvidia),
		RM_BoolStr (rm_caps.isAMD),
		RM_BoolStr (rm_caps.isIntel),
		RM_BoolStr (rm_caps.isMesa));

	Com_Printf (0, "\n--- GL 3.3 Baseline ---\n");
	Com_Printf (0, "hasVAO:        %s\n", RM_BoolStr (rm_caps.hasVAO));
	Com_Printf (0, "hasUBO:        %s\n", RM_BoolStr (rm_caps.hasUBO));
	Com_Printf (0, "hasFBO:        %s\n", RM_BoolStr (rm_caps.hasFBO));
	Com_Printf (0, "hasInstancing: %s\n", RM_BoolStr (rm_caps.hasInstancing));

	Com_Printf (0, "\n--- GL 4.x Features ---\n");
	Com_Printf (0, "hasTessellation:       %s\n", RM_BoolStr (rm_caps.hasTessellation));
	Com_Printf (0, "hasBaseInstance:       %s\n", RM_BoolStr (rm_caps.hasBaseInstance));
	Com_Printf (0, "hasImageLoadStore:     %s\n", RM_BoolStr (rm_caps.hasImageLoadStore));
	Com_Printf (0, "hasComputeShaders:     %s\n", RM_BoolStr (rm_caps.hasComputeShaders));
	Com_Printf (0, "hasSSBO:               %s\n", RM_BoolStr (rm_caps.hasSSBO));
	Com_Printf (0, "hasDebugOutput:        %s\n", RM_BoolStr (rm_caps.hasDebugOutput));
	Com_Printf (0, "hasMultiDrawIndirect:  %s\n", RM_BoolStr (rm_caps.hasMultiDrawIndirect));
	Com_Printf (0, "hasExplicitUniformLoc: %s\n", RM_BoolStr (rm_caps.hasExplicitUniformLoc));
	Com_Printf (0, "hasPersistentMap:      %s\n", RM_BoolStr (rm_caps.hasPersistentMap));
	Com_Printf (0, "hasMultiBind:          %s\n", RM_BoolStr (rm_caps.hasMultiBind));
	Com_Printf (0, "hasDirectStateAccess:  %s\n", RM_BoolStr (rm_caps.hasDirectStateAccess));
	Com_Printf (0, "hasClipControl:        %s\n", RM_BoolStr (rm_caps.hasClipControl));

	Com_Printf (0, "\n--- Extensions ---\n");
	Com_Printf (0, "hasBindlessTexture: %s\n", RM_BoolStr (rm_caps.hasBindlessTexture));
	Com_Printf (0, "hasAnisotropic:     %s\n", RM_BoolStr (rm_caps.hasAnisotropic));
	Com_Printf (0, "hasSparseTexture:   %s\n", RM_BoolStr (rm_caps.hasSparseTexture));

	Com_Printf (0, "\n--- Limits ---\n");
	Com_Printf (0, "maxTextureSize:        %d\n", rm_caps.maxTextureSize);
	Com_Printf (0, "maxTextureUnits:       %d\n", rm_caps.maxTextureUnits);
	Com_Printf (0, "maxUniformBlockSize:   %d bytes\n", rm_caps.maxUniformBlockSize);
	if (rm_caps.hasSSBO)
		Com_Printf (0, "maxSSBOSize:           %d bytes\n", rm_caps.maxSSBOSize);
	if (rm_caps.hasComputeShaders) {
		Com_Printf (0, "maxComputeWorkGroupSize: %d %d %d\n",
			rm_caps.maxComputeWorkGroupSize[0],
			rm_caps.maxComputeWorkGroupSize[1],
			rm_caps.maxComputeWorkGroupSize[2]);
		Com_Printf (0, "maxComputeInvocations:   %d\n", rm_caps.maxComputeWorkGroupInvocations);
	}
	if (rm_caps.hasAnisotropic)
		Com_Printf (0, "maxAnisotropy:         %.1f\n", rm_caps.maxAnisotropy);

	Com_Printf (0, "----------------------------------------\n");
}

void RM_Caps_Init (void)
{
	const char *vendor;
	const char *renderer;
	const char *versionString;
	const char *glslString;

	if (rm_capsInitialized)
		return;

	if (!glad_glGetString || !glad_glGetIntegerv) {
		Com_Printf (PRNT_WARNING, "RM_Caps_Init: GLAD not initialized; skipping capability detection\n");
		return;
	}

	memset (&rm_caps, 0, sizeof (rm_caps));

	/* Version info */
	rm_caps.glVersionMajor = 0;
	rm_caps.glVersionMinor = 0;
	glGetIntegerv (GL_MAJOR_VERSION, &rm_caps.glVersionMajor);
	glGetIntegerv (GL_MINOR_VERSION, &rm_caps.glVersionMinor);

	/* Fallback: parse GL_VERSION string if integer query fails */
	versionString = (const char *)glGetString (GL_VERSION);
	if ((rm_caps.glVersionMajor <= 0 || rm_caps.glVersionMinor < 0) && versionString) {
		rm_caps.glVersionMajor = 0;
		rm_caps.glVersionMinor = 0;
		sscanf (versionString, "%d.%d", &rm_caps.glVersionMajor, &rm_caps.glVersionMinor);
	}

	glslString = (const char *)glGetString (GL_SHADING_LANGUAGE_VERSION);
	rm_caps.glslVersion = RM_ParseVersionNumber (glslString);

	/* Vendor strings */
	vendor = (const char *)glGetString (GL_VENDOR);
	renderer = (const char *)glGetString (GL_RENDERER);

	rm_caps.isNvidia = RM_StrIContains (vendor, "NVIDIA");
	rm_caps.isAMD = (RM_StrIContains (vendor, "AMD") || RM_StrIContains (vendor, "ATI"));
	rm_caps.isIntel = RM_StrIContains (vendor, "Intel");
	rm_caps.isMesa = RM_StrIContains (renderer, "Mesa");

	/* GL 3.3 baseline */
	rm_caps.hasVAO = RM_GLVersionAtLeast (3, 0) || RM_HasExtension ("GL_ARB_vertex_array_object");
	rm_caps.hasUBO = RM_GLVersionAtLeast (3, 1) || RM_HasExtension ("GL_ARB_uniform_buffer_object");
	rm_caps.hasFBO = RM_GLVersionAtLeast (3, 0) || RM_HasExtension ("GL_ARB_framebuffer_object") || RM_HasExtension ("GL_EXT_framebuffer_object");
	rm_caps.hasInstancing = RM_GLVersionAtLeast (3, 3) || RM_HasExtension ("GL_ARB_instanced_arrays") || RM_HasExtension ("GL_ARB_draw_instanced") ||
		RM_HasExtension ("GL_EXT_instanced_arrays") || RM_HasExtension ("GL_EXT_draw_instanced");

	/* GL 4.0 */
	rm_caps.hasTessellation = RM_GLVersionAtLeast (4, 0) || RM_HasExtension ("GL_ARB_tessellation_shader");

	/* GL 4.2 */
	rm_caps.hasBaseInstance = RM_GLVersionAtLeast (4, 2) || RM_HasExtension ("GL_ARB_base_instance");
	rm_caps.hasImageLoadStore = RM_GLVersionAtLeast (4, 2) || RM_HasExtension ("GL_ARB_shader_image_load_store");

	/* GL 4.3 */
	rm_caps.hasComputeShaders = RM_GLVersionAtLeast (4, 3) || RM_HasExtension ("GL_ARB_compute_shader");
	rm_caps.hasSSBO = RM_GLVersionAtLeast (4, 3) || RM_HasExtension ("GL_ARB_shader_storage_buffer_object");
	rm_caps.hasDebugOutput = RM_GLVersionAtLeast (4, 3) || RM_HasExtension ("GL_KHR_debug") || RM_HasExtension ("GL_ARB_debug_output");
	rm_caps.hasMultiDrawIndirect = RM_GLVersionAtLeast (4, 3) || RM_HasExtension ("GL_ARB_multi_draw_indirect");
	rm_caps.hasExplicitUniformLoc = RM_GLVersionAtLeast (4, 3) || RM_HasExtension ("GL_ARB_explicit_uniform_location");

	/* GL 4.4 */
	rm_caps.hasPersistentMap = RM_GLVersionAtLeast (4, 4) || RM_HasExtension ("GL_ARB_buffer_storage");
	rm_caps.hasMultiBind = RM_GLVersionAtLeast (4, 4) || RM_HasExtension ("GL_ARB_multi_bind");

	/* GL 4.5 */
	rm_caps.hasDirectStateAccess = RM_GLVersionAtLeast (4, 5) || RM_HasExtension ("GL_ARB_direct_state_access");
	rm_caps.hasClipControl = RM_GLVersionAtLeast (4, 5) || RM_HasExtension ("GL_ARB_clip_control");

	/* Extensions */
	rm_caps.hasBindlessTexture = RM_HasExtension ("GL_ARB_bindless_texture");
	rm_caps.hasAnisotropic = RM_HasExtension ("GL_EXT_texture_filter_anisotropic");
	rm_caps.hasSparseTexture = RM_HasExtension ("GL_ARB_sparse_texture");

	/* Limits */
	rm_caps.maxTextureSize = 0;
	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &rm_caps.maxTextureSize);

	RM_Caps_QueryTextureUnitLimit ();

	rm_caps.maxUniformBlockSize = 0;
	if (rm_caps.hasUBO)
		glGetIntegerv (GL_MAX_UNIFORM_BLOCK_SIZE, &rm_caps.maxUniformBlockSize);

	RM_Caps_QuerySSBOLimit ();
	RM_Caps_QueryComputeLimits ();

	rm_caps.maxAnisotropy = 1.0f;
	if (rm_caps.hasAnisotropic)
		glGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &rm_caps.maxAnisotropy);

	rm_capsInitialized = qTrue;

	RM_Caps_Print ();
}
