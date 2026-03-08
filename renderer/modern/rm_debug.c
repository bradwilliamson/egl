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
// rm_debug.c - Modern renderer GL debug output helpers
//

#include "../r_local.h"
#include "../rb_gl.h"
#include "rm_caps.h"
#include "rm_debug.h"

#include <string.h>

/* Fallback defines for tokens missing from minimal GL headers */
#ifndef GL_DONT_CARE
#define GL_DONT_CARE 0x1100
#endif

#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT 0x92E0
#endif

#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#endif

#ifndef GL_DEBUG_SOURCE_APPLICATION
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#endif

#ifndef GL_DEBUG_TYPE_ERROR
#define GL_DEBUG_TYPE_ERROR 0x824C
#endif

#ifndef GL_DEBUG_SEVERITY_HIGH
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#endif

#ifndef GL_DEBUG_SEVERITY_MEDIUM
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#endif

#ifndef GL_DEBUG_SEVERITY_NOTIFICATION
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#endif

typedef void (APIENTRY *GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar *message, const void *userParam);

typedef void (APIENTRY *PFNGLDEBUGMESSAGECALLBACKPROC)(GLDEBUGPROC callback, const void *userParam);
typedef void (APIENTRY *PFNGLDEBUGMESSAGECONTROLPROC)(GLenum source, GLenum type, GLenum severity,
	GLsizei count, const GLuint *ids, GLboolean enabled);
typedef void (APIENTRY *PFNGLPUSHDEBUGGROUPPROC)(GLenum source, GLuint id, GLsizei length, const GLchar *message);
typedef void (APIENTRY *PFNGLPOPDEBUGGROUPPROC)(void);

/* ARB_debug_output fallbacks */
typedef void (APIENTRY *PFNGLDEBUGMESSAGECALLBACKARBPROC)(GLDEBUGPROC callback, const void *userParam);
typedef void (APIENTRY *PFNGLDEBUGMESSAGECONTROLARBPROC)(GLenum source, GLenum type, GLenum severity,
	GLsizei count, const GLuint *ids, GLboolean enabled);

static PFNGLDEBUGMESSAGECALLBACKPROC rm_glDebugMessageCallback = NULL;
static PFNGLDEBUGMESSAGECONTROLPROC rm_glDebugMessageControl = NULL;
static PFNGLPUSHDEBUGGROUPPROC rm_glPushDebugGroup = NULL;
static PFNGLPOPDEBUGGROUPPROC rm_glPopDebugGroup = NULL;

static PFNGLDEBUGMESSAGECALLBACKARBPROC rm_glDebugMessageCallbackARB = NULL;
static PFNGLDEBUGMESSAGECONTROLARBPROC rm_glDebugMessageControlARB = NULL;

static qBool rm_debugInitialized = qFalse;
static qBool rm_debugActive = qFalse;

static void RM_Debug_LoadProcs (void)
{
	if (!rm_glDebugMessageCallback)
		rm_glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)GL_GetProcAddress ("glDebugMessageCallback");
	if (!rm_glDebugMessageControl)
		rm_glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)GL_GetProcAddress ("glDebugMessageControl");
	if (!rm_glPushDebugGroup)
		rm_glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)GL_GetProcAddress ("glPushDebugGroup");
	if (!rm_glPopDebugGroup)
		rm_glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)GL_GetProcAddress ("glPopDebugGroup");

	if (!rm_glDebugMessageCallbackARB)
		rm_glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)GL_GetProcAddress ("glDebugMessageCallbackARB");
	if (!rm_glDebugMessageControlARB)
		rm_glDebugMessageControlARB = (PFNGLDEBUGMESSAGECONTROLARBPROC)GL_GetProcAddress ("glDebugMessageControlARB");
}

static void APIENTRY RM_DebugCallback (
	GLenum source, GLenum type, GLuint id, GLenum severity,
	GLsizei length, const GLchar *message, const void *userParam)
{
	const char *sevStr;
	(void)source;
	(void)id;
	(void)length;
	(void)userParam;

	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;

	sevStr = "INFO";
	if (severity == GL_DEBUG_SEVERITY_HIGH)
		sevStr = "ERROR";
	else if (severity == GL_DEBUG_SEVERITY_MEDIUM)
		sevStr = "WARN";

	Com_Printf (type == GL_DEBUG_TYPE_ERROR ? PRNT_ERROR : 0,
		"GL [%s] %s\n", sevStr, message ? (const char *)message : "<null>");
}

void RM_Debug_Init (void)
{
	if (rm_debugInitialized)
		return;
	rm_debugInitialized = qTrue;

	/* Ensure caps are populated (safe to call multiple times) */
	RM_Caps_Init ();

	if (!rm_caps.hasDebugOutput)
		return;

	RM_Debug_LoadProcs ();

	/* Prefer KHR_debug / core entry points */
	if (rm_glDebugMessageCallback && rm_glDebugMessageControl) {
		glEnable (GL_DEBUG_OUTPUT);
		glEnable (GL_DEBUG_OUTPUT_SYNCHRONOUS);
		rm_glDebugMessageCallback (RM_DebugCallback, NULL);
		rm_glDebugMessageControl (GL_DONT_CARE, GL_DONT_CARE,
			GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
		rm_debugActive = qTrue;
		return;
	}

	/* Fallback: ARB_debug_output (no debug groups, no notification severity filtering) */
	if (rm_glDebugMessageCallbackARB) {
		glEnable (GL_DEBUG_OUTPUT_SYNCHRONOUS);
		rm_glDebugMessageCallbackARB (RM_DebugCallback, NULL);
		if (rm_glDebugMessageControlARB) {
			/* Best-effort: enable everything (filtering handled in callback). */
			rm_glDebugMessageControlARB (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		}
		rm_debugActive = qTrue;
		return;
	}

	Com_Printf (PRNT_WARNING, "RM_Debug_Init: Debug output advertised but entry points are missing\n");
}

void RM_Debug_PushGroup (const char *name)
{
	if (!rm_debugActive || !rm_caps.hasDebugOutput)
		return;
	if (!rm_glPushDebugGroup)
		return;

	rm_glPushDebugGroup (GL_DEBUG_SOURCE_APPLICATION, 0, -1, name ? name : "<unnamed>");
}

void RM_Debug_PopGroup (void)
{
	if (!rm_debugActive || !rm_caps.hasDebugOutput)
		return;
	if (!rm_glPopDebugGroup)
		return;

	rm_glPopDebugGroup ();
}

