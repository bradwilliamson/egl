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
// rm_dsa.h - Modern renderer DSA wrappers (MODERN-ONLY)
//

#ifndef __RM_DSA_H__
#define __RM_DSA_H__

/* This file is ONLY compiled when EGL_MODERN_RENDERER=1 */
#if !EGL_MODERN_RENDERER
#error "rm_dsa.h should only be included in modern renderer builds"
#endif

#include "shared/shared.h"

void  RM_DSA_Init (void);
void  RM_DSA_Shutdown (void);
qBool RM_DSA_Available (void);

GLuint RM_CreateBuffer (void);
void   RM_BufferData (GLuint buf, GLenum target, GLsizeiptr size, const void *data, GLenum usage);
void   RM_BufferSubData (GLuint buf, GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
qBool  RM_BufferStorage (GLuint buf, GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);

GLuint RM_CreateTexture2D (void);
void   RM_TexImage2D (GLuint tex, int width, int height, GLenum internalFmt, GLenum fmt, GLenum type, const void *pixels);
void   RM_TexParameteri (GLuint tex, GLenum pname, GLint value);

GLuint RM_CreateFramebuffer (void);
void   RM_FramebufferTexture2D (GLuint fbo, GLenum attachment, GLuint tex);
GLenum RM_CheckFramebufferStatus (GLuint fbo);

#endif /* __RM_DSA_H__ */
