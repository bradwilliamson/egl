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
// rm_tess.c - Modern renderer tessellator implementation
//

#include "../r_local.h"
#include "../rb_gl.h"
#include "rm_caps.h"
#include "rm_dsa.h"
#include "rm_tess.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Fallback defines for enums missing from minimal GL headers */
#ifndef GL_STREAM_DRAW
#define GL_STREAM_DRAW 0x88E0
#endif

#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT 0x0002
#endif

#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT 0x0040
#endif

#ifndef GL_MAP_COHERENT_BIT
#define GL_MAP_COHERENT_BIT 0x0080
#endif

#ifndef GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT 0x00004000
#endif

#ifndef GL_SYNC_GPU_COMMANDS_COMPLETE
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#endif

#ifndef GL_SYNC_FLUSH_COMMANDS_BIT
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#endif

#ifndef GL_ALREADY_SIGNALED
#define GL_ALREADY_SIGNALED 0x911A
#endif

#ifndef GL_CONDITION_SATISFIED
#define GL_CONDITION_SATISFIED 0x911C
#endif

#ifndef GL_TIMEOUT_EXPIRED
#define GL_TIMEOUT_EXPIRED 0x911B
#endif

#ifndef GL_WAIT_FAILED
#define GL_WAIT_FAILED 0x911D
#endif

typedef uint64_t GLuint64;

typedef void *(APIENTRY *PFNGLMAPBUFFERRANGEPROC)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
typedef GLboolean (APIENTRY *PFNGLUNMAPBUFFERPROC)(GLenum target);

typedef GLsync (APIENTRY *PFNGLFENCESYNCPROC)(GLenum condition, GLbitfield flags);
typedef GLenum (APIENTRY *PFNGLCLIENTWAITSYNCPROC)(GLsync sync, GLbitfield flags, GLuint64 timeout);
typedef void (APIENTRY *PFNGLDELETESYNCPROC)(GLsync sync);
typedef void (APIENTRY *PFNGLMEMORYBARRIERPROC)(GLbitfield barriers);

static PFNGLMAPBUFFERRANGEPROC rm_glMapBufferRange = NULL;
static PFNGLUNMAPBUFFERPROC rm_glUnmapBuffer = NULL;
static PFNGLFENCESYNCPROC rm_glFenceSync = NULL;
static PFNGLCLIENTWAITSYNCPROC rm_glClientWaitSync = NULL;
static PFNGLDELETESYNCPROC rm_glDeleteSync = NULL;
static PFNGLMEMORYBARRIERPROC rm_glMemoryBarrier = NULL;

rmTess_t rm_tess;

extern cVar_t *r_persistent_map;

static qBool rm_tessInitialized = qFalse;

static const size_t rm_tessVertexStride = RM_TESS_VERTEX_STRIDE_BYTES;
static const size_t rm_tessIndexStride = RM_TESS_INDEX_STRIDE_BYTES;

static void RM_Tess_LoadPersistentProcs (void)
{
	if (!rm_glMapBufferRange)
		rm_glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)GL_GetProcAddress ("glMapBufferRange");
	if (!rm_glUnmapBuffer)
		rm_glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)GL_GetProcAddress ("glUnmapBuffer");

	if (!rm_glFenceSync)
		rm_glFenceSync = (PFNGLFENCESYNCPROC)GL_GetProcAddress ("glFenceSync");
	if (!rm_glClientWaitSync)
		rm_glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)GL_GetProcAddress ("glClientWaitSync");
	if (!rm_glDeleteSync)
		rm_glDeleteSync = (PFNGLDELETESYNCPROC)GL_GetProcAddress ("glDeleteSync");
	if (!rm_glMemoryBarrier)
		rm_glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)GL_GetProcAddress ("glMemoryBarrier");
}

static void RM_Tess_SetVaoPointers (size_t vboOffsetBytes)
{
	const GLsizei stride = (GLsizei)rm_tessVertexStride;
	const GLintptr base = (GLintptr)vboOffsetBytes;

	glBindVertexArray (rm_tess.vao);
	glBindBuffer (GL_ARRAY_BUFFER, rm_tess.vbo);
	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, rm_tess.ebo);

	/* Vertex format (q2pro-style): 8 floats = 32 bytes
	   layout(location=0): vec3 position      @ float[0]
	   layout(location=1): vec2 texcoord      @ float[4]
	   layout(location=2): vec4 color (ub4n)  @ float[3]
	*/
	glEnableVertexAttribArray (0);
	glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, stride, (void *)(base + 0));

	glEnableVertexAttribArray (1);
	glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(base + 4 * sizeof(float)));

	glEnableVertexAttribArray (2);
	glVertexAttribPointer (2, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void *)(base + 3 * sizeof(float)));

	glBindVertexArray (0);
	glBindBuffer (GL_ARRAY_BUFFER, 0);
	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void RM_Tess_SetWritePointers (void)
{
	if (!rm_tess.isPersistent) {
		/* CPU staging buffers are always at offset 0. */
		return;
	}

	{
		const size_t vboFrameSize = RM_TESS_MAX_VERTICES * rm_tessVertexStride;
		const size_t eboFrameSize = RM_TESS_MAX_INDICES * rm_tessIndexStride;
		const size_t vboOffset = (size_t)rm_tess.currentFrame * vboFrameSize + (size_t)rm_tess.frameVertOffset * rm_tessVertexStride;
		const size_t eboOffset = (size_t)rm_tess.currentFrame * eboFrameSize + (size_t)rm_tess.frameIndexOffset * rm_tessIndexStride;

		rm_tess.vertices = (float *)((uint8_t *)rm_tess.mappedVerts + vboOffset);
		rm_tess.indices = (GLuint *)((uint8_t *)rm_tess.mappedIndices + eboOffset);
	}
}

static void RM_Tess_WaitFence (int frame)
{
	GLsync fence;

	RM_Tess_LoadPersistentProcs ();

	if (!rm_glClientWaitSync || !rm_glDeleteSync)
		return;

	fence = rm_tess.fences[frame];
	if (!fence)
		return;

	for (;;) {
		const GLuint64 timeout = 1000000000ull; /* 1s */
		GLenum res = rm_glClientWaitSync (fence, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
		if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED)
			break;
		if (res == GL_TIMEOUT_EXPIRED)
			continue;
		if (res == GL_WAIT_FAILED) {
			Com_Printf (PRNT_WARNING, "RM_Tess_WaitFence: GL_WAIT_FAILED\n");
			break;
		}
		break;
	}

	rm_glDeleteSync (fence);
	rm_tess.fences[frame] = NULL;
}

static void RM_Tess_AdvanceFrame (void)
{
	const int oldFrame = rm_tess.currentFrame;
	const int nextFrame = (oldFrame + 1) % RM_TESS_FRAMES;

	RM_Tess_LoadPersistentProcs ();
	if (rm_glFenceSync) {
		rm_tess.fences[oldFrame] = rm_glFenceSync (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	}

	rm_tess.currentFrame = nextFrame;
	RM_Tess_WaitFence (nextFrame);

	rm_tess.frameVertOffset = 0;
	rm_tess.frameIndexOffset = 0;
	RM_Tess_SetWritePointers ();
}

static void RM_Tess_InitStreaming (void)
{
	rm_tess.isPersistent = qFalse;
	rm_tess.mappedVerts = NULL;
	rm_tess.mappedIndices = NULL;

	rm_tess.vertices = (float *)malloc (RM_TESS_MAX_VERTICES * rm_tessVertexStride);
	rm_tess.indices = (GLuint *)malloc (RM_TESS_MAX_INDICES * rm_tessIndexStride);

	RM_BufferData (rm_tess.vbo, GL_ARRAY_BUFFER, RM_TESS_MAX_VERTICES * rm_tessVertexStride, NULL, GL_STREAM_DRAW);
	RM_BufferData (rm_tess.ebo, GL_ELEMENT_ARRAY_BUFFER, RM_TESS_MAX_INDICES * rm_tessIndexStride, NULL, GL_STREAM_DRAW);

	RM_Tess_SetVaoPointers (0);

	Com_Printf (0, "Tessellator: Traditional streaming\n");
}

static qBool RM_Tess_InitPersistent (void)
{
	const size_t vboFrameSize = RM_TESS_MAX_VERTICES * rm_tessVertexStride;
	const size_t eboFrameSize = RM_TESS_MAX_INDICES * rm_tessIndexStride;
	const size_t vboSize = vboFrameSize * RM_TESS_FRAMES;
	const size_t eboSize = eboFrameSize * RM_TESS_FRAMES;
	const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

	RM_Tess_LoadPersistentProcs ();

	if (!rm_glMapBufferRange || !rm_glFenceSync || !rm_glClientWaitSync || !rm_glDeleteSync || !rm_glMemoryBarrier) {
		return qFalse;
	}

	if (!RM_BufferStorage (rm_tess.vbo, GL_ARRAY_BUFFER, (GLsizeiptr)vboSize, NULL, flags))
		return qFalse;
	if (!RM_BufferStorage (rm_tess.ebo, GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)eboSize, NULL, flags))
		return qFalse;

	glBindBuffer (GL_ARRAY_BUFFER, rm_tess.vbo);
	rm_tess.mappedVerts = rm_glMapBufferRange (GL_ARRAY_BUFFER, 0, (GLsizeiptr)vboSize, flags);

	glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, rm_tess.ebo);
	rm_tess.mappedIndices = rm_glMapBufferRange (GL_ELEMENT_ARRAY_BUFFER, 0, (GLsizeiptr)eboSize, flags);

	if (!rm_tess.mappedVerts || !rm_tess.mappedIndices) {
		Com_Printf (PRNT_WARNING, "Tessellator: Persistent mapping failed; falling back\n");

		if (rm_glUnmapBuffer) {
			if (rm_tess.mappedVerts) {
				glBindBuffer (GL_ARRAY_BUFFER, rm_tess.vbo);
				rm_glUnmapBuffer (GL_ARRAY_BUFFER);
			}
			if (rm_tess.mappedIndices) {
				glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, rm_tess.ebo);
				rm_glUnmapBuffer (GL_ELEMENT_ARRAY_BUFFER);
			}
		}

		rm_tess.mappedVerts = NULL;
		rm_tess.mappedIndices = NULL;
		return qFalse;
	}

	rm_tess.isPersistent = qTrue;
	rm_tess.currentFrame = 0;
	rm_tess.frameVertOffset = 0;
	rm_tess.frameIndexOffset = 0;
	memset (rm_tess.fences, 0, sizeof (rm_tess.fences));

	RM_Tess_SetWritePointers ();
	RM_Tess_SetVaoPointers (0);

	Com_Printf (0, "Tessellator: Persistent mapping enabled\n");
	return qTrue;
}

void RM_Tess_Init (void)
{
	qBool wantPersistent;

	if (rm_tessInitialized)
		return;
	rm_tessInitialized = qTrue;

	memset (&rm_tess, 0, sizeof (rm_tess));

	glGenVertexArrays (1, &rm_tess.vao);
	rm_tess.vbo = RM_CreateBuffer ();
	rm_tess.ebo = RM_CreateBuffer ();

	wantPersistent = (RM_HAS_PERSISTENT () && r_persistent_map && r_persistent_map->intVal != 0) ? qTrue : qFalse;
	if (wantPersistent) {
		if (!RM_Tess_InitPersistent ()) {
			/* Persistent failed; recreate mutable buffers for streaming */
			glDeleteBuffers (1, &rm_tess.vbo);
			glDeleteBuffers (1, &rm_tess.ebo);
			rm_tess.vbo = RM_CreateBuffer ();
			rm_tess.ebo = RM_CreateBuffer ();
			RM_Tess_InitStreaming ();
		}
	} else {
		RM_Tess_InitStreaming ();
	}

	rm_tess.numVerts = 0;
	rm_tess.numIndices = 0;
	rm_tess.currentTexture = 0;
	rm_tess.stateFlags = 0;
}

void RM_Tess_Begin (GLuint texture, uint64_t stateFlags)
{
	if (!rm_tessInitialized)
		return;

	if ((rm_tess.numVerts || rm_tess.numIndices) &&
		(rm_tess.currentTexture != texture || rm_tess.stateFlags != stateFlags)) {
		RM_Tess_Flush ();
	}

	rm_tess.currentTexture = texture;
	rm_tess.stateFlags = stateFlags;
}

qBool RM_Tess_Reserve (int verts, int indices)
{
	if (!rm_tessInitialized)
		return qFalse;
	if (verts < 0 || indices < 0)
		return qFalse;
	if (verts > RM_TESS_MAX_VERTICES || indices > RM_TESS_MAX_INDICES) {
		Com_Printf (PRNT_ERROR, "RM_Tess_Reserve: request too large (verts=%d indices=%d)\n", verts, indices);
		return qFalse;
	}

	if (rm_tess.numVerts + verts > RM_TESS_MAX_VERTICES ||
		rm_tess.numIndices + indices > RM_TESS_MAX_INDICES) {
		RM_Tess_Flush ();
	}

	if (rm_tess.isPersistent) {
		if (rm_tess.frameVertOffset + rm_tess.numVerts + verts > RM_TESS_MAX_VERTICES ||
			rm_tess.frameIndexOffset + rm_tess.numIndices + indices > RM_TESS_MAX_INDICES) {
			/* Finish current batch first, then rotate to a fresh ring segment. */
			if (rm_tess.numVerts || rm_tess.numIndices) {
				RM_Tess_Flush ();
			}
			RM_Tess_AdvanceFrame ();
		}
	}

	return qTrue;
}

void RM_Tess_Shutdown (void)
{
	int i;

	if (!rm_tessInitialized)
		return;

	if (rm_tess.isPersistent) {
		RM_Tess_LoadPersistentProcs ();

		for (i = 0; i < RM_TESS_FRAMES; i++) {
			if (rm_tess.fences[i] && rm_glDeleteSync) {
				rm_glDeleteSync (rm_tess.fences[i]);
				rm_tess.fences[i] = NULL;
			}
		}

		if (rm_glUnmapBuffer) {
			if (rm_tess.mappedVerts) {
				glBindBuffer (GL_ARRAY_BUFFER, rm_tess.vbo);
				rm_glUnmapBuffer (GL_ARRAY_BUFFER);
			}
			if (rm_tess.mappedIndices) {
				glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, rm_tess.ebo);
				rm_glUnmapBuffer (GL_ELEMENT_ARRAY_BUFFER);
			}
		}
		rm_tess.mappedVerts = NULL;
		rm_tess.mappedIndices = NULL;
	} else {
		free (rm_tess.vertices);
		free (rm_tess.indices);
	}

	rm_tess.vertices = NULL;
	rm_tess.indices = NULL;
	rm_tess.numVerts = 0;
	rm_tess.numIndices = 0;

	if (rm_tess.ebo) {
		glDeleteBuffers (1, &rm_tess.ebo);
		rm_tess.ebo = 0;
	}
	if (rm_tess.vbo) {
		glDeleteBuffers (1, &rm_tess.vbo);
		rm_tess.vbo = 0;
	}
	if (rm_tess.vao) {
		glDeleteVertexArrays (1, &rm_tess.vao);
		rm_tess.vao = 0;
	}

	memset (&rm_tess, 0, sizeof (rm_tess));
	rm_tessInitialized = qFalse;
}

void RM_Tess_Flush (void)
{
	size_t vboOffset = 0;
	size_t eboOffset = 0;
	const size_t vboBytes = (size_t)rm_tess.numVerts * rm_tessVertexStride;
	const size_t eboBytes = (size_t)rm_tess.numIndices * rm_tessIndexStride;

	if (!rm_tessInitialized)
		return;
	if (rm_tess.numVerts <= 0 || rm_tess.numIndices <= 0)
		return;

	if (rm_tess.isPersistent) {
		const size_t vboFrameSize = RM_TESS_MAX_VERTICES * rm_tessVertexStride;
		const size_t eboFrameSize = RM_TESS_MAX_INDICES * rm_tessIndexStride;
		vboOffset = (size_t)rm_tess.currentFrame * vboFrameSize + (size_t)rm_tess.frameVertOffset * rm_tessVertexStride;
		eboOffset = (size_t)rm_tess.currentFrame * eboFrameSize + (size_t)rm_tess.frameIndexOffset * rm_tessIndexStride;

		RM_Tess_SetVaoPointers (vboOffset);

		/* Ensure client writes to persistent-mapped buffers are visible to the GPU. */
		RM_Tess_LoadPersistentProcs ();
		if (rm_glMemoryBarrier)
			rm_glMemoryBarrier (GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	} else {
		/* q2pro-style streaming: allocate+upload in one call (driver may orphan internally). */
		RM_BufferData (rm_tess.vbo, GL_ARRAY_BUFFER, (GLsizeiptr)vboBytes, rm_tess.vertices, GL_STREAM_DRAW);
		RM_BufferData (rm_tess.ebo, GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)eboBytes, rm_tess.indices, GL_STREAM_DRAW);
	}

	glBindVertexArray (rm_tess.vao);
	glDrawElements (GL_TRIANGLES, rm_tess.numIndices, GL_UNSIGNED_INT, (void *)eboOffset);
	glBindVertexArray (0);

	rm_tess.totalBatches++;
	rm_tess.totalVerts += rm_tess.numVerts;
	rm_tess.totalIndices += rm_tess.numIndices;

	if (rm_tess.isPersistent) {
		rm_tess.frameVertOffset += rm_tess.numVerts;
		rm_tess.frameIndexOffset += rm_tess.numIndices;

		/* If we filled this ring segment, advance now. */
		if (rm_tess.frameVertOffset >= RM_TESS_MAX_VERTICES ||
			rm_tess.frameIndexOffset >= RM_TESS_MAX_INDICES) {
			rm_tess.frameVertOffset = RM_TESS_MAX_VERTICES;
			rm_tess.frameIndexOffset = RM_TESS_MAX_INDICES;
			rm_tess.numVerts = 0;
			rm_tess.numIndices = 0;
			RM_Tess_AdvanceFrame ();
			return;
		}
	}

	rm_tess.numVerts = 0;
	rm_tess.numIndices = 0;

	if (rm_tess.isPersistent) {
		RM_Tess_SetWritePointers ();
	}
}
