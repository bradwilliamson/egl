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
// rm_backend.c - Modern renderer backend implementation
//

#include "../r_local.h"
#include "rm_backend.h"
#include "rm_dsa.h"
#include "rm_debug.h"
#include "rm_fbo.h"
#include "rm_geom.h"
#include "rm_tess.h"
#include "rm_ubo.h"
extern cVar_t *rm_use_fbo;
extern cVar_t *rm_test_triangle;

/*
=============================================================================

	MODERN BACKEND IMPLEMENTATION

=============================================================================
*/

/* Current active backend (NULL = legacy) */
static const rm_backend_t *rm_activeBackend = NULL;

static rm_fbo_t *rm_sceneFBO = NULL;
static int rm_frameWidth = 0;
static int rm_frameHeight = 0;

static void RM_Modern_Init (void)
{
	int w, h;
	FILE *logf = fopen("modern.log", "a"); if (logf) { fprintf(logf, "RM_Modern_Init entry\n"); fclose(logf); }

	Com_Printf (0, "RM_Modern_Init: Modern backend initialized\n");
	if (!glad_glClearColor) {
		Com_Printf (PRNT_ERROR, "GLAD not initialized! Cannot proceed.\n");
		logf = fopen("modern.log", "a"); if (logf) { fprintf(logf, "RM_Modern_Init: GLAD not initialized\n"); fclose(logf); }
		return;
	}

	/* Enable GL debug output if available */
	RM_Debug_Init ();

	/* Optional DSA wrappers reduce bind-to-edit churn when supported. */
	RM_DSA_Init ();

	/* Initialize tessellator buffers (persistent-map or streaming) */
	RM_Tess_Init ();

	const GLubyte *version = glGetString(GL_VERSION);
	Com_Printf (0, "OpenGL version: %s\n", version ? (const char*)version : "NULL");
	Com_Printf (0, "GLAD glClearColor: %p\n", glad_glClearColor);
	FILE *f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_Modern_Init: OpenGL version %s, glad_glClearColor %p\n", version ? (const char*)version : "NULL", glad_glClearColor); fclose(f); }

	/* Initialize modern geometry module */
	FILE *f2 = fopen("modern.log", "a"); if (f2) { fprintf(f2, "Before RM_Geom_Init\n"); fclose(f2); }
	RM_Geom_Init ();
	FILE *f3 = fopen("modern.log", "a"); if (f3) { fprintf(f3, "After RM_Geom_Init\n"); fclose(f3); }

	/* Initialize UBOs */
	RM_UBO_Init ();

	/* Fullscreen quad for drawing FBO to backbuffer (replaces blit) */
	RM_FBO_InitQuad ();

	/* Scene FBO for off-screen rendering */
	w = ri.def.width;
	h = ri.def.height;
	if (w <= 0 || h <= 0) {
		w = 1920;
		h = 1080;
	}
	rm_sceneFBO = RM_FBO_Create (w, h);
}

/* Forward declare RM_World_Shutdown - defined in rm_world.c */
extern void RM_World_Shutdown(void);

static void RM_Modern_Shutdown (void)
{
	Com_Printf (0, "RM_Modern_Shutdown: Modern backend shut down\n");
	FILE *f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_Modern_Shutdown\n"); fclose(f); }

	/* Shutdown world rendering (VAOs, VBOs, shaders) - MUST be first */
	RM_World_Shutdown();

	RM_Tess_Shutdown ();

	RM_FBO_Destroy (rm_sceneFBO);
	rm_sceneFBO = NULL;

	RM_FBO_ShutdownQuad ();

	/* Shutdown UBOs */
	RM_UBO_Shutdown ();

	/* Shutdown modern geometry module */
	RM_Geom_Shutdown ();

	RM_DSA_Shutdown ();
}

static void RM_Modern_BeginFrame (refDef_t *fd)
{
	/* DON'T clear here - RM_DrawWorld will clear once before drawing */
	/* This prevents double-clearing which could erase the wireframe */
	
	/* Update UBOs */
	RM_UBO_Update (fd);

	rm_frameWidth = fd ? fd->width : 0;
	rm_frameHeight = fd ? fd->height : 0;
	/* Use ref window size when refDef dims not set (e.g. map load) */
	if (rm_frameWidth <= 0 && ri.config.vidWidth > 0) rm_frameWidth = ri.config.vidWidth;
	if (rm_frameHeight <= 0 && ri.config.vidHeight > 0) rm_frameHeight = ri.config.vidHeight;
	if (rm_frameWidth <= 0) rm_frameWidth = 1920;
	if (rm_frameHeight <= 0) rm_frameHeight = 1080;

	/* Resize check: recreate FBO if size changed */
	if (rm_sceneFBO && (rm_sceneFBO->width != rm_frameWidth || rm_sceneFBO->height != rm_frameHeight)) {
		RM_FBO_Destroy (rm_sceneFBO);
		rm_sceneFBO = NULL;
	}
	if (!rm_sceneFBO) {
		rm_sceneFBO = RM_FBO_Create (rm_frameWidth, rm_frameHeight);
	}

	/* When rm_use_fbo 0: do not bind FBO so legacy draws to backbuffer (workaround black screen) */
	if (rm_use_fbo && rm_use_fbo->intVal != 0) {
		/* 1. Bind FBO */
		RM_FBO_Bind (rm_sceneFBO, rm_frameWidth, rm_frameHeight);
		if (rm_sceneFBO) {
			/* 2. Clear FBO (Magenta allows us to see if Legacy draws *anything*) */
			glClearColor (1.0f, 0.0f, 1.0f, 1.0f);
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 3. FORCE VIEWPORT & SCISSOR (Crucial for Legacy Projection)
            // This ensures the Legacy Renderer sees the correct "Window Size"
            glViewport (0, 0, rm_frameWidth, rm_frameHeight);
            glScissor (0, 0, rm_frameWidth, rm_frameHeight);

			/* 3. CRITICAL STATE RESET – Legacy assumes these defaults (fixes invisible geometry) */
			glActiveTexture (GL_TEXTURE0);
			glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthMask (GL_TRUE);
			glEnable (GL_DEPTH_TEST);
			glDepthFunc (GL_LEQUAL);
			glDisable (GL_CULL_FACE);  /* Let legacy handle culling */
			glDisable (GL_BLEND);
			glDisable (GL_SCISSOR_TEST);  /* Ensure full FBO is writable */
			/* Matrix mode: legacy sets PROJECTION/MODELVIEW in RB_SetupGL3D (no core GL matrix API here) */
		}
	}
}

static void RM_Modern_EndFrame (void)
{
	int w = rm_frameWidth  > 0 ? rm_frameWidth  : (rm_sceneFBO ? rm_sceneFBO->width  : 1920);
	int h = rm_frameHeight > 0 ? rm_frameHeight : (rm_sceneFBO ? rm_sceneFBO->height : 1080);

	if (rm_use_fbo && rm_use_fbo->intVal != 0) {
		/* FBO path: draw to FBO, then fullscreen quad to backbuffer */
		RM_FBO_Bind (rm_sceneFBO, rm_frameWidth, rm_frameHeight);
		if (rm_test_triangle && rm_test_triangle->intVal != 0)
			RM_Geom_DrawTest ();
		/* RESTORE: Re-enable for safety before switching to backbuffer */
		glEnable (GL_CULL_FACE);
		glEnable (GL_DEPTH_TEST);
		RM_FBO_Bind (NULL, w, h);
		if (rm_sceneFBO)
			RM_FBO_DrawToBackbuffer (rm_sceneFBO);
	} else {
		/* Bypass: legacy drew to backbuffer; only restore viewport */
		RM_FBO_Bind (NULL, w, h);
	}
}

/*
==================
Modern Backend Definition
==================
*/
static const rm_backend_t rm_modernBackend = {
	"modern",
	RM_Modern_Init,
	RM_Modern_Shutdown,
	RM_Modern_BeginFrame,
	RM_Modern_EndFrame
};

/*
==================
RM_GetActiveBackend

Returns the currently active modern backend, or NULL if using legacy.
==================
*/
const rm_backend_t *RM_GetActiveBackend (void)
{
	return rm_activeBackend;
}

/*
==================
RM_IsModernBackendActive

Returns qTrue if a modern backend is active.
==================
*/
qBool RM_IsModernBackendActive (void)
{
	return (rm_activeBackend != NULL);
}

/*
==================
RM_SetBackend

Sets the active backend by name.
==================
*/
qBool RM_SetBackend (const char *name)
{
	FILE *f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_SetBackend: name=%s\n", name ? name : "NULL"); fclose(f); }
	if (!name) {
		Com_Printf (PRNT_ERROR, "RM_SetBackend: NULL name\n");
		return qFalse;
	}
	
	/* Switch to legacy */
	if (!Q_stricmp (name, "legacy")) {
		if (rm_activeBackend) {
			Com_Printf (0, "Switching to legacy backend...\n");
			rm_activeBackend->shutdown ();
			rm_activeBackend = NULL;
			Com_Printf (0, "Legacy backend active\n");
		} else {
			Com_Printf (0, "Legacy backend already active\n");
		}
		return qTrue;
	}
	
	/* Switch to modern */
	if (!Q_stricmp (name, "modern")) {
		if (rm_activeBackend) {
			Com_Printf (PRNT_WARNING, "Modern backend already active\n");
			return qTrue;
		}
		
		Com_Printf (0, "Switching to modern backend...\n");
		rm_activeBackend = &rm_modernBackend;
		FILE *f2 = fopen("modern.log", "a"); if (f2) { fprintf(f2, "RM_SetBackend: calling init\n"); fclose(f2); }
		rm_activeBackend->init ();
		FILE *f3 = fopen("modern.log", "a"); if (f3) { fprintf(f3, "RM_SetBackend: init completed\n"); fclose(f3); }
		Com_Printf (0, "Modern backend active\n");
		return qTrue;
	}
	
	/* Unknown backend */
	Com_Printf (PRNT_ERROR, "RM_SetBackend: Unknown backend '%s' (valid: legacy, modern)\n", name);
	return qFalse;
}

/*
==================
RM_Backend_Init

Initializes the active backend (if any).
Called during renderer initialization.
==================
*/
void RM_Backend_Init (void)
{
	Com_Printf(0, "RM_Backend_Init: r_backend=%s\n", r_backend ? r_backend->string : "NULL");
	FILE *f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_Backend_Init: r_backend=%s\n", r_backend ? r_backend->string : "NULL"); fclose(f); }
	/* Activate backend from r_backend default so modern path runs without user setting cvar */
	if (r_backend && r_backend->string)
		RM_SetBackend (r_backend->string);
}

/*
==================
RM_Backend_Shutdown

Shuts down the active backend (if any).
Called during renderer shutdown.
==================
*/
void RM_Backend_Shutdown (void)
{
	if (rm_activeBackend) {
		Com_Printf (0, "Shutting down modern backend...\n");
		rm_activeBackend->shutdown ();
		rm_activeBackend = NULL;
	}
}

/*
==================
RM_Backend_BeginFrame

Called at the beginning of each frame.
Dispatches to active backend if one is set.
==================
*/
void RM_Backend_BeginFrame (refDef_t *fd)
{
	if (rm_activeBackend && rm_activeBackend->begin_frame) {
		rm_activeBackend->begin_frame (fd);
	}
}

/*
==================
RM_Backend_EndFrame

Called at the end of each frame.
Dispatches to active backend if one is set.
==================
*/
void RM_Backend_EndFrame (void)
{
	if (rm_activeBackend && rm_activeBackend->end_frame) {
		rm_activeBackend->end_frame ();
	}
}
