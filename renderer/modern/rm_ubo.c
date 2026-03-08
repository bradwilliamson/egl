#define RM_NO_LEGACY_GL  // <--- Must be BEFORE r_local.h
#include "../glad/glad.h"
#include "r_local.h"
#include "rm_ubo.h"
#include <stdio.h>

#ifndef GL_UNIFORM_BUFFER
  #error "GLAD failed to load. Isolation guard is working, but GLAD is missing."
#endif

static GLuint rm_ubo_handle = 0;

/*
==================
RM_UBO_Init
==================
*/
void RM_UBO_Init (void)
{
	FILE *f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Init entry\n"); fclose(f); }
	if (rm_ubo_handle) {
		Com_Printf (PRNT_WARNING, "RM_UBO_Init: UBO already initialized\n");
		return;
	}

	glGenBuffers (1, &rm_ubo_handle);
	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Init: glGenBuffers handle=%u\n", rm_ubo_handle); fclose(f); }
	glBindBuffer (GL_UNIFORM_BUFFER, rm_ubo_handle);
	glBufferData (GL_UNIFORM_BUFFER, sizeof(ubo_per_frame_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase (GL_UNIFORM_BUFFER, 0, rm_ubo_handle);
	glBindBuffer (GL_UNIFORM_BUFFER, 0);

	Com_Printf (0, "RM_UBO_Init: Interface block created (binding point 0)\n");
	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Init completed\n"); fclose(f); }
}

/*
==================
RM_UBO_Shutdown
==================
*/
void RM_UBO_Shutdown (void)
{
	if (rm_ubo_handle) {
		glDeleteBuffers (1, &rm_ubo_handle);
		rm_ubo_handle = 0;
		Com_Printf (0, "RM_UBO_Shutdown: Interface block destroyed\n");
	}
}

/*
==================
RM_UBO_Update
==================
*/
void RM_UBO_Update (refDef_t *fd)
{
	ubo_per_frame_t data;
	FILE *f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update entry, handle=%u fd=%p\n", rm_ubo_handle, fd); fclose(f); }

	if (!rm_ubo_handle || !fd) {
		f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update early exit\n"); fclose(f); }
		return;
	}

	memset (&data, 0, sizeof(data));
	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update after memset\n"); fclose(f); }

	/* Setup View Matrix */
	R_SetupModelviewMatrix (fd, data.view);
	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update after view matrix\n"); fclose(f); }

	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update fd: width=%d height=%d\n", fd->width, fd->height); fclose(f); }

	/* Setup Projection Matrix */
	if (fd->width <= 0 || fd->height <= 0) {
		f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update skipping projection (zero dims)\n"); fclose(f); }
		/* Set identity projection as fallback */
		memset(data.proj, 0, sizeof(data.proj));
		data.proj[0] = data.proj[5] = data.proj[10] = data.proj[15] = 1.0f;
	} else {
		R_SetupProjectionMatrix (fd, data.proj);
	}
	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update after proj matrix\n"); fclose(f); }

	/* Setup Time */
	data.time[0] = fd->time;

	/* Upload to GPU */
	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update binding buffer\n"); fclose(f); }
	glBindBuffer (GL_UNIFORM_BUFFER, rm_ubo_handle);
	glBufferSubData (GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
	glBindBuffer (GL_UNIFORM_BUFFER, 0);
	f = fopen("modern.log", "a"); if (f) { fprintf(f, "RM_UBO_Update completed\n"); fclose(f); }
}
