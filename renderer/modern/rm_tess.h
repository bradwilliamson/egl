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
// rm_tess.h - Modern renderer tessellator (MODERN-ONLY)
//

#ifndef __RM_TESS_H__
#define __RM_TESS_H__

/* This file is ONLY compiled when EGL_MODERN_RENDERER=1 */
#if !EGL_MODERN_RENDERER
#error "rm_tess.h should only be included in modern renderer builds"
#endif

#include "shared/shared.h"

#define RM_TESS_MAX_VERTICES  16384
#define RM_TESS_MAX_INDICES   49152
#define RM_TESS_FRAMES        3

#define RM_TESS_VERTEX_STRIDE_BYTES 32
#define RM_TESS_INDEX_STRIDE_BYTES  4

/*
Vertex format (q2pro-style, 32 bytes / 8 floats):

  float[0..2]  position.xyz
  byte [12..15] color.rgba (normalized ubyte4)
  float[4..5]  texcoord.st
  float[6..7]  reserved (can be used for lightmap st later)

This matches the common modern shaders that expect:
  layout(location=0) vec3 in_Position
  layout(location=1) vec2 in_TexCoord
  layout(location=2) vec4 in_Color
*/
#define RM_TESS_VERTEX_STRIDE_FLOATS (RM_TESS_VERTEX_STRIDE_BYTES / (int)sizeof(float))
#define RM_TESS_VTX_POS_OFFSET_FLOATS   0
#define RM_TESS_VTX_COLOR_OFFSET_FLOATS 3
#define RM_TESS_VTX_TC_OFFSET_FLOATS    4
#define RM_TESS_VTX_LMTC_OFFSET_FLOATS  6

#define RM_TESS_VERTEX_PTR(v)	(&rm_tess.vertices[(v) * RM_TESS_VERTEX_STRIDE_FLOATS])
#define RM_TESS_POS_PTR(v)		(RM_TESS_VERTEX_PTR(v) + RM_TESS_VTX_POS_OFFSET_FLOATS)
#define RM_TESS_TC_PTR(v)		(RM_TESS_VERTEX_PTR(v) + RM_TESS_VTX_TC_OFFSET_FLOATS)
#define RM_TESS_LMTC_PTR(v)		(RM_TESS_VERTEX_PTR(v) + RM_TESS_VTX_LMTC_OFFSET_FLOATS)
#define RM_TESS_COLOR_PTR(v)	((byte *)(RM_TESS_VERTEX_PTR(v) + RM_TESS_VTX_COLOR_OFFSET_FLOATS))
#define RM_TESS_SET_COLOR_RGBA(v, r, g, b, a) do { \
	byte *rm__c = RM_TESS_COLOR_PTR(v); \
	rm__c[0] = (byte)(r); rm__c[1] = (byte)(g); rm__c[2] = (byte)(b); rm__c[3] = (byte)(a); \
} while (0)

/* GLsync is not provided by our OpenGL 3.3 core loader headers */
typedef void *GLsync;

typedef struct {
	float *vertices;
	GLuint *indices;
	int numVerts, numIndices;
	GLuint currentTexture;
	uint64_t stateFlags;

	GLuint vao, vbo, ebo;

	/* Persistent mapping (GL 4.4) */
	qBool isPersistent;
	void *mappedVerts, *mappedIndices;
	GLsync fences[RM_TESS_FRAMES];
	int currentFrame;
	int frameVertOffset;
	int frameIndexOffset;

	/* Stats */
	int totalBatches, totalVerts, totalIndices;
} rmTess_t;

extern rmTess_t rm_tess;

void RM_Tess_Init (void);
void RM_Tess_Shutdown (void);
void RM_Tess_Begin (GLuint texture, uint64_t stateFlags);
qBool RM_Tess_Reserve (int verts, int indices);
void RM_Tess_Flush (void);

#endif /* __RM_TESS_H__ */
