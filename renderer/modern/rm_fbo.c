/*
=============================================================================
	MODERN RENDERER - FRAMEBUFFER OBJECTS (FBO)
	Off-screen rendering; RM_NO_LEGACY_GL required.
	CRITICAL: RM_FBO_Bind(NULL, w, h) restores window viewport for legacy UI.
	FSQ: fullscreen quad draws FBO color texture to backbuffer (replaces blit).
=============================================================================
*/

#define RM_NO_LEGACY_GL
#include "../glad/glad.h"
#include "r_local.h"
#include "rm_fbo.h"

#include <stdlib.h>
#include <string.h>

/*
==================
FSQ Shaders (embedded, #version 330 core)
==================
*/
static const char *rm_fsqVertSource =
	"#version 330 core\n"
	"layout(location = 0) in vec2 a_pos;\n"
	"layout(location = 1) in vec2 a_uv;\n"
	"out vec2 v_uv;\n"
	"void main() {\n"
	"    v_uv = a_uv;\n"
	"    gl_Position = vec4(a_pos, 0.0, 1.0);\n"
	"}\n";

static const char *rm_fsqFragSource =
	"#version 330 core\n"
	"in vec2 v_uv;\n"
	"out vec4 FragColor;\n"
	"uniform sampler2D u_tex;\n"
	"void main() {\n"
	"    // DEBUG: Output UVs\n"
	"    // FragColor = vec4(v_uv.x, v_uv.y, 0.0, 1.0);\n"
	"    // NORMAL: Sample Texture\n"
	"    FragColor = texture(u_tex, v_uv);\n"
	"}\n";

/* FSQ state (internal) */
static GLuint rm_fsqVao = 0;
static GLuint rm_fsqVbo = 0;
static GLuint rm_fsqProgram = 0;
static GLint rm_fsqUniformTex = -1;
static qBool rm_fsqInitialized = qFalse;

/*
==================
RM_FBO_Create
==================
*/
rm_fbo_t *RM_FBO_Create (int width, int height)
{
	rm_fbo_t *fbo;
	GLenum status;

	if (width <= 0 || height <= 0) {
		Com_Printf (PRNT_ERROR, "RM_FBO_Create: invalid size %d x %d\n", width, height);
		return NULL;
	}

	fbo = (rm_fbo_t *)malloc (sizeof (*fbo));
	if (!fbo) {
		Com_Printf (PRNT_ERROR, "RM_FBO_Create: out of memory\n");
		return NULL;
	}
	memset (fbo, 0, sizeof (*fbo));
	fbo->width = width;
	fbo->height = height;

	glGenFramebuffers (1, (GLuint *)&fbo->id);
	glBindFramebuffer (GL_FRAMEBUFFER, (GLuint)fbo->id);

	/* Color attachment */
	glGenTextures (1, (GLuint *)&fbo->color_tex);
	glBindTexture (GL_TEXTURE_2D, (GLuint)fbo->color_tex);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	/* CRITICAL: Disable mipmaps, set filter/wrap so texture is complete (no "incomplete" error) */
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)fbo->color_tex, 0);

	/* Depth attachment (pure depth, no stencil – safe mode for compatibility) */
	glGenTextures (1, (GLuint *)&fbo->depth_tex);
	glBindTexture (GL_TEXTURE_2D, (GLuint)fbo->depth_tex);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, (GLuint)fbo->depth_tex, 0);

	status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
	glBindTexture (GL_TEXTURE_2D, 0);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Com_Printf (PRNT_ERROR, "RM_FBO_Create: framebuffer incomplete (0x%X)\n", (unsigned int)status);
		RM_FBO_Destroy (fbo);
		return NULL;
	}

	Com_Printf (0, "RM_FBO_Create: %d x %d OK\n", width, height);
	return fbo;
}

/*
==================
RM_FBO_Destroy
==================
*/
void RM_FBO_Destroy (rm_fbo_t *fbo)
{
	if (!fbo) {
		return;
	}
	if (fbo->id) {
		glDeleteFramebuffers (1, (const GLuint *)&fbo->id);
		fbo->id = 0;
	}
	if (fbo->color_tex) {
		glDeleteTextures (1, (const GLuint *)&fbo->color_tex);
		fbo->color_tex = 0;
	}
	if (fbo->depth_tex) {
		glDeleteTextures (1, (const GLuint *)&fbo->depth_tex);
		fbo->depth_tex = 0;
	}
	free (fbo);
}

/*
==================
RM_FBO_Bind
==================
*/
void RM_FBO_Bind (rm_fbo_t *fbo, int winWidth, int winHeight)
{
	if (fbo) {
		glBindFramebuffer (GL_FRAMEBUFFER, (GLuint)fbo->id);
		glViewport (0, 0, fbo->width, fbo->height);
		glScissor (0, 0, fbo->width, fbo->height);
	} else {
		int w = winWidth  > 0 ? winWidth  : 1;
		int h = winHeight > 0 ? winHeight : 1;
		glBindFramebuffer (GL_FRAMEBUFFER, 0);  /* Backbuffer */
		glViewport (0, 0, w, h);  /* CRITICAL: Restore Window Viewport */
		/* Legacy leaves scissor set (e.g. 3D view rect); clear/blit must see full window */
		glScissor (0, 0, w, h);
	}
}

/*
==================
RM_FBO_CompileFSQProgram (internal)
==================
*/
static GLuint RM_FBO_CompileFSQProgram (void)
{
	GLuint vertShader, fragShader, program;
	GLint compiled, linked;

	vertShader = glCreateShader (GL_VERTEX_SHADER);
	glShaderSource (vertShader, 1, &rm_fsqVertSource, NULL);
	glCompileShader (vertShader);
	glGetShaderiv (vertShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		glDeleteShader (vertShader);
		return 0;
	}

	fragShader = glCreateShader (GL_FRAGMENT_SHADER);
	glShaderSource (fragShader, 1, &rm_fsqFragSource, NULL);
	glCompileShader (fragShader);
	glGetShaderiv (fragShader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		glDeleteShader (vertShader);
		glDeleteShader (fragShader);
		return 0;
	}

	program = glCreateProgram ();
	glAttachShader (program, vertShader);
	glAttachShader (program, fragShader);
	glDeleteShader (vertShader);
	glDeleteShader (fragShader);
	glLinkProgram (program);
	glGetProgramiv (program, GL_LINK_STATUS, &linked);
	if (!linked) {
		glDeleteProgram (program);
		return 0;
	}
	return program;
}

/*
==================
RM_FBO_InitQuad
==================
*/
void RM_FBO_InitQuad (void)
{
	/* Quad: (-1,-1) to (1,1), two triangles, pos.xy + uv.xy per vertex */
	static const float fsqVerts[] = {
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	if (rm_fsqInitialized) {
		return;
	}

	rm_fsqProgram = RM_FBO_CompileFSQProgram ();
	if (!rm_fsqProgram) {
		Com_Printf (PRNT_ERROR, "RM_FBO_InitQuad: failed to compile FSQ shaders\n");
		return;
	}
	rm_fsqUniformTex = glGetUniformLocation (rm_fsqProgram, "u_tex");

	glGenVertexArrays (1, &rm_fsqVao);
	glBindVertexArray (rm_fsqVao);

	glGenBuffers (1, &rm_fsqVbo);
	glBindBuffer (GL_ARRAY_BUFFER, rm_fsqVbo);
	glBufferData (GL_ARRAY_BUFFER, sizeof (fsqVerts), fsqVerts, GL_STATIC_DRAW);

	glEnableVertexAttribArray (0);
	glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void *)0);
	glEnableVertexAttribArray (1);
	glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void *)(2 * sizeof (float)));

	glBindBuffer (GL_ARRAY_BUFFER, 0);
	glBindVertexArray (0);

	rm_fsqInitialized = qTrue;
	Com_Printf (0, "RM_FBO_InitQuad: fullscreen quad OK\n");
}

/*
==================
RM_FBO_ShutdownQuad
==================
*/
void RM_FBO_ShutdownQuad (void)
{
	if (!rm_fsqInitialized) {
		return;
	}
	if (rm_fsqProgram) {
		glDeleteProgram (rm_fsqProgram);
		rm_fsqProgram = 0;
	}
	rm_fsqUniformTex = -1;
	if (rm_fsqVbo) {
		glDeleteBuffers (1, &rm_fsqVbo);
		rm_fsqVbo = 0;
	}
	if (rm_fsqVao) {
		glDeleteVertexArrays (1, &rm_fsqVao);
		rm_fsqVao = 0;
	}
	rm_fsqInitialized = qFalse;
}

/*
==================
RM_FBO_DrawToBackbuffer
==================
*/
void RM_FBO_DrawToBackbuffer (rm_fbo_t *fbo)
{
	int winWidth, winHeight;

	if (!fbo || !fbo->id || !fbo->color_tex) {
		return;
	}
	if (!rm_fsqInitialized || !rm_fsqProgram || !rm_fsqVao) {
		return;
	}

	winWidth = fbo->width > 0 ? fbo->width : 1;
	winHeight = fbo->height > 0 ? fbo->height : 1;

	/* 1. Bind backbuffer and restore viewport */
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
	glViewport (0, 0, winWidth, winHeight);
	glScissor (0, 0, winWidth, winHeight);

	/* 2. Disable depth/culling for 2D overlay (quad must draw on top) */
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);

	/* 3. Setup shader */
	glUseProgram (rm_fsqProgram);

	/* 4. Bind texture (even if debug shader ignores it, keep logic valid) */
	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, (GLuint)fbo->color_tex);
	if (rm_fsqUniformTex >= 0) {
		glUniform1i (rm_fsqUniformTex, 0);
	}

	/* 5. Draw */
	glBindVertexArray (rm_fsqVao);
	glDrawArrays (GL_TRIANGLES, 0, 6);
	glBindVertexArray (0);

	/* 6. Restore state */
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);

	glBindTexture (GL_TEXTURE_2D, 0);
	glUseProgram (0);
}

/*
==================
RM_FBO_BlitToBackbuffer
==================
*/
void RM_FBO_BlitToBackbuffer (rm_fbo_t *fbo, int screenWidth, int screenHeight)
{
	if (!fbo || !fbo->id) {
		return;
	}
	glBindFramebuffer (GL_READ_FRAMEBUFFER, (GLuint)fbo->id);
	glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer (0, 0, fbo->width, fbo->height, 0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
}
