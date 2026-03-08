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
// rm_dsa.c - Modern renderer DSA wrappers
//

#include "../r_local.h"
#include "../rb_gl.h"
#include "rm_caps.h"
#include "rm_dsa.h"

#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif

#ifndef GL_ARRAY_BUFFER_BINDING
#define GL_ARRAY_BUFFER_BINDING 0x8894
#endif

#ifndef GL_ELEMENT_ARRAY_BUFFER_BINDING
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#endif

#ifndef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_BINDING_2D 0x8069
#endif

#ifndef GL_R8
#define GL_R8 0x8229
#endif

#ifndef GL_RG8
#define GL_RG8 0x822B
#endif

#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16 0x81A5
#endif

typedef void (APIENTRY *PFNGLCREATEBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *PFNGLNAMEDBUFFERDATAPROC)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY *PFNGLNAMEDBUFFERSUBDATAPROC)(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
typedef void (APIENTRY *PFNGLNAMEDBUFFERSTORAGEPROC)(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
typedef void (APIENTRY *PFNGLCREATETEXTURESPROC)(GLenum target, GLsizei n, GLuint *textures);
typedef void (APIENTRY *PFNGLTEXTURESTORAGE2DPROC)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *PFNGLTEXTURESUBIMAGE2DPROC)(GLuint texture, GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRY *PFNGLTEXTUREPARAMETERIPROC)(GLuint texture, GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLCREATEFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)(GLuint framebuffer, GLenum target);
typedef void (APIENTRY *PFNGLBUFFERSTORAGEPROC)(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);

static PFNGLCREATEBUFFERSPROC rm_glCreateBuffers = NULL;
static PFNGLNAMEDBUFFERDATAPROC rm_glNamedBufferData = NULL;
static PFNGLNAMEDBUFFERSUBDATAPROC rm_glNamedBufferSubData = NULL;
static PFNGLNAMEDBUFFERSTORAGEPROC rm_glNamedBufferStorage = NULL;
static PFNGLCREATETEXTURESPROC rm_glCreateTextures = NULL;
static PFNGLTEXTURESTORAGE2DPROC rm_glTextureStorage2D = NULL;
static PFNGLTEXTURESUBIMAGE2DPROC rm_glTextureSubImage2D = NULL;
static PFNGLTEXTUREPARAMETERIPROC rm_glTextureParameteri = NULL;
static PFNGLCREATEFRAMEBUFFERSPROC rm_glCreateFramebuffers = NULL;
static PFNGLNAMEDFRAMEBUFFERTEXTUREPROC rm_glNamedFramebufferTexture = NULL;
static PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC rm_glCheckNamedFramebufferStatus = NULL;
static PFNGLBUFFERSTORAGEPROC rm_glBufferStorage = NULL;

static qBool rm_dsaInitialized = qFalse;
static qBool rm_dsaAvailable = qFalse;

static GLenum RM_BufferBindingEnum (GLenum target)
{
	switch (target) {
	case GL_ARRAY_BUFFER:
		return GL_ARRAY_BUFFER_BINDING;
	case GL_ELEMENT_ARRAY_BUFFER:
		return GL_ELEMENT_ARRAY_BUFFER_BINDING;
#if defined(GL_UNIFORM_BUFFER) && defined(GL_UNIFORM_BUFFER_BINDING)
	case GL_UNIFORM_BUFFER:
		return GL_UNIFORM_BUFFER_BINDING;
#endif
#if defined(GL_COPY_READ_BUFFER) && defined(GL_COPY_READ_BUFFER_BINDING)
	case GL_COPY_READ_BUFFER:
		return GL_COPY_READ_BUFFER_BINDING;
#endif
#if defined(GL_COPY_WRITE_BUFFER) && defined(GL_COPY_WRITE_BUFFER_BINDING)
	case GL_COPY_WRITE_BUFFER:
		return GL_COPY_WRITE_BUFFER_BINDING;
#endif
#if defined(GL_PIXEL_PACK_BUFFER) && defined(GL_PIXEL_PACK_BUFFER_BINDING)
	case GL_PIXEL_PACK_BUFFER:
		return GL_PIXEL_PACK_BUFFER_BINDING;
#endif
#if defined(GL_PIXEL_UNPACK_BUFFER) && defined(GL_PIXEL_UNPACK_BUFFER_BINDING)
	case GL_PIXEL_UNPACK_BUFFER:
		return GL_PIXEL_UNPACK_BUFFER_BINDING;
#endif
#if defined(GL_TEXTURE_BUFFER) && defined(GL_TEXTURE_BUFFER_BINDING)
	case GL_TEXTURE_BUFFER:
		return GL_TEXTURE_BUFFER_BINDING;
#endif
#if defined(GL_TRANSFORM_FEEDBACK_BUFFER) && defined(GL_TRANSFORM_FEEDBACK_BUFFER_BINDING)
	case GL_TRANSFORM_FEEDBACK_BUFFER:
		return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
#endif
#if defined(GL_DRAW_INDIRECT_BUFFER) && defined(GL_DRAW_INDIRECT_BUFFER_BINDING)
	case GL_DRAW_INDIRECT_BUFFER:
		return GL_DRAW_INDIRECT_BUFFER_BINDING;
#endif
#if defined(GL_ATOMIC_COUNTER_BUFFER) && defined(GL_ATOMIC_COUNTER_BUFFER_BINDING)
	case GL_ATOMIC_COUNTER_BUFFER:
		return GL_ATOMIC_COUNTER_BUFFER_BINDING;
#endif
#if defined(GL_DISPATCH_INDIRECT_BUFFER) && defined(GL_DISPATCH_INDIRECT_BUFFER_BINDING)
	case GL_DISPATCH_INDIRECT_BUFFER:
		return GL_DISPATCH_INDIRECT_BUFFER_BINDING;
#endif
#if defined(GL_SHADER_STORAGE_BUFFER) && defined(GL_SHADER_STORAGE_BUFFER_BINDING)
	case GL_SHADER_STORAGE_BUFFER:
		return GL_SHADER_STORAGE_BUFFER_BINDING;
#endif
	default:
		break;
	}

	return 0;
}

static qBool RM_IsSizedTextureFormat (GLenum internalFmt)
{
	switch (internalFmt) {
	case GL_R8:
	case GL_RG8:
	case GL_RGB8:
	case GL_RGBA8:
#ifdef GL_SRGB8_ALPHA8
	case GL_SRGB8_ALPHA8:
#endif
#ifdef GL_R16F
	case GL_R16F:
#endif
#ifdef GL_RG16F
	case GL_RG16F:
#endif
#ifdef GL_RGB16F
	case GL_RGB16F:
#endif
#ifdef GL_RGBA16F
	case GL_RGBA16F:
#endif
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
#ifdef GL_DEPTH_COMPONENT32
	case GL_DEPTH_COMPONENT32:
#endif
#ifdef GL_DEPTH_COMPONENT32F
	case GL_DEPTH_COMPONENT32F:
#endif
#ifdef GL_DEPTH24_STENCIL8
	case GL_DEPTH24_STENCIL8:
#endif
#ifdef GL_DEPTH32F_STENCIL8
	case GL_DEPTH32F_STENCIL8:
#endif
		return qTrue;
	default:
		break;
	}

	return qFalse;
}

static void RM_DSA_EnsureInit (void)
{
	if (!rm_dsaInitialized)
		RM_DSA_Init ();
}

void RM_DSA_Init (void)
{
	if (rm_dsaInitialized)
		return;

	rm_dsaInitialized = qTrue;
	rm_dsaAvailable = qFalse;

	rm_glBufferStorage = (PFNGLBUFFERSTORAGEPROC)GL_GetProcAddress ("glBufferStorage");

	if (!RM_HAS_DSA())
		return;

	rm_glCreateBuffers = (PFNGLCREATEBUFFERSPROC)GL_GetProcAddress ("glCreateBuffers");
	rm_glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)GL_GetProcAddress ("glNamedBufferData");
	rm_glNamedBufferSubData = (PFNGLNAMEDBUFFERSUBDATAPROC)GL_GetProcAddress ("glNamedBufferSubData");
	rm_glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)GL_GetProcAddress ("glNamedBufferStorage");
	rm_glCreateTextures = (PFNGLCREATETEXTURESPROC)GL_GetProcAddress ("glCreateTextures");
	rm_glTextureStorage2D = (PFNGLTEXTURESTORAGE2DPROC)GL_GetProcAddress ("glTextureStorage2D");
	rm_glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)GL_GetProcAddress ("glTextureSubImage2D");
	rm_glTextureParameteri = (PFNGLTEXTUREPARAMETERIPROC)GL_GetProcAddress ("glTextureParameteri");
	rm_glCreateFramebuffers = (PFNGLCREATEFRAMEBUFFERSPROC)GL_GetProcAddress ("glCreateFramebuffers");
	rm_glNamedFramebufferTexture = (PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)GL_GetProcAddress ("glNamedFramebufferTexture");
	rm_glCheckNamedFramebufferStatus = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)GL_GetProcAddress ("glCheckNamedFramebufferStatus");

	if (!rm_glCreateBuffers || !rm_glNamedBufferData || !rm_glNamedBufferSubData ||
		!rm_glNamedBufferStorage || !rm_glCreateTextures || !rm_glTextureStorage2D ||
		!rm_glTextureSubImage2D || !rm_glTextureParameteri || !rm_glCreateFramebuffers ||
		!rm_glNamedFramebufferTexture || !rm_glCheckNamedFramebufferStatus) {
		Com_Printf (PRNT_WARNING, "RM_DSA_Init: DSA advertised but required entry points are missing; using bind-to-edit fallbacks\n");
		return;
	}

	rm_dsaAvailable = qTrue;
	Com_Printf (0, "RM_DSA_Init: direct state access enabled\n");
}

void RM_DSA_Shutdown (void)
{
	rm_glCreateBuffers = NULL;
	rm_glNamedBufferData = NULL;
	rm_glNamedBufferSubData = NULL;
	rm_glNamedBufferStorage = NULL;
	rm_glCreateTextures = NULL;
	rm_glTextureStorage2D = NULL;
	rm_glTextureSubImage2D = NULL;
	rm_glTextureParameteri = NULL;
	rm_glCreateFramebuffers = NULL;
	rm_glNamedFramebufferTexture = NULL;
	rm_glCheckNamedFramebufferStatus = NULL;
	rm_glBufferStorage = NULL;
	rm_dsaAvailable = qFalse;
	rm_dsaInitialized = qFalse;
}

qBool RM_DSA_Available (void)
{
	RM_DSA_EnsureInit ();
	return rm_dsaAvailable;
}

GLuint RM_CreateBuffer (void)
{
	GLuint buffer = 0;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glCreateBuffers)
		rm_glCreateBuffers (1, &buffer);
	else
		glGenBuffers (1, &buffer);

	return buffer;
}

void RM_BufferData (GLuint buf, GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
	GLint previous = 0;
	GLenum bindingEnum;

	if (!buf)
		return;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glNamedBufferData) {
		rm_glNamedBufferData (buf, size, data, usage);
		return;
	}

	bindingEnum = RM_BufferBindingEnum (target);
	if (bindingEnum)
		glGetIntegerv (bindingEnum, &previous);

	glBindBuffer (target, buf);
	glBufferData (target, size, data, usage);

	if (bindingEnum)
		glBindBuffer (target, (GLuint)previous);
	else
		glBindBuffer (target, 0);
}

void RM_BufferSubData (GLuint buf, GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
{
	GLint previous = 0;
	GLenum bindingEnum;

	if (!buf)
		return;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glNamedBufferSubData) {
		rm_glNamedBufferSubData (buf, offset, size, data);
		return;
	}

	bindingEnum = RM_BufferBindingEnum (target);
	if (bindingEnum)
		glGetIntegerv (bindingEnum, &previous);

	glBindBuffer (target, buf);
	glBufferSubData (target, offset, size, data);

	if (bindingEnum)
		glBindBuffer (target, (GLuint)previous);
	else
		glBindBuffer (target, 0);
}

qBool RM_BufferStorage (GLuint buf, GLenum target, GLsizeiptr size, const void *data, GLbitfield flags)
{
	GLint previous = 0;
	GLenum bindingEnum;

	if (!buf)
		return qFalse;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glNamedBufferStorage) {
		rm_glNamedBufferStorage (buf, size, data, flags);
		return qTrue;
	}

	if (!rm_glBufferStorage)
		return qFalse;

	bindingEnum = RM_BufferBindingEnum (target);
	if (bindingEnum)
		glGetIntegerv (bindingEnum, &previous);

	glBindBuffer (target, buf);
	rm_glBufferStorage (target, size, data, flags);

	if (bindingEnum)
		glBindBuffer (target, (GLuint)previous);
	else
		glBindBuffer (target, 0);

	return qTrue;
}

GLuint RM_CreateTexture2D (void)
{
	GLuint texture = 0;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glCreateTextures)
		rm_glCreateTextures (GL_TEXTURE_2D, 1, &texture);
	else
		glGenTextures (1, &texture);

	return texture;
}

void RM_TexImage2D (GLuint tex, int width, int height, GLenum internalFmt, GLenum fmt, GLenum type, const void *pixels)
{
	GLint previous = 0;

	if (!tex)
		return;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glTextureStorage2D && rm_glTextureSubImage2D &&
		RM_IsSizedTextureFormat (internalFmt)) {
		rm_glTextureStorage2D (tex, 1, internalFmt, width, height);
		if (pixels)
			rm_glTextureSubImage2D (tex, 0, 0, 0, width, height, fmt, type, pixels);
		return;
	}

	glGetIntegerv (GL_TEXTURE_BINDING_2D, &previous);
	glBindTexture (GL_TEXTURE_2D, tex);
	glTexImage2D (GL_TEXTURE_2D, 0, internalFmt, width, height, 0, fmt, type, pixels);
	glBindTexture (GL_TEXTURE_2D, (GLuint)previous);
}

void RM_TexParameteri (GLuint tex, GLenum pname, GLint value)
{
	GLint previous = 0;

	if (!tex)
		return;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glTextureParameteri) {
		rm_glTextureParameteri (tex, pname, value);
		return;
	}

	glGetIntegerv (GL_TEXTURE_BINDING_2D, &previous);
	glBindTexture (GL_TEXTURE_2D, tex);
	glTexParameteri (GL_TEXTURE_2D, pname, value);
	glBindTexture (GL_TEXTURE_2D, (GLuint)previous);
}

GLuint RM_CreateFramebuffer (void)
{
	GLuint framebuffer = 0;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glCreateFramebuffers)
		rm_glCreateFramebuffers (1, &framebuffer);
	else
		glGenFramebuffers (1, &framebuffer);

	return framebuffer;
}

void RM_FramebufferTexture2D (GLuint fbo, GLenum attachment, GLuint tex)
{
	GLint previous = 0;

	if (!fbo)
		return;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glNamedFramebufferTexture) {
		rm_glNamedFramebufferTexture (fbo, attachment, tex, 0);
		return;
	}

	glGetIntegerv (GL_FRAMEBUFFER_BINDING, &previous);
	glBindFramebuffer (GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D (GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, tex, 0);
	glBindFramebuffer (GL_FRAMEBUFFER, (GLuint)previous);
}

GLenum RM_CheckFramebufferStatus (GLuint fbo)
{
	GLenum status;
	GLint previous = 0;

	if (!fbo)
		return 0;

	RM_DSA_EnsureInit ();

	if (rm_dsaAvailable && rm_glCheckNamedFramebufferStatus)
		return rm_glCheckNamedFramebufferStatus (fbo, GL_FRAMEBUFFER);

	glGetIntegerv (GL_FRAMEBUFFER_BINDING, &previous);
	glBindFramebuffer (GL_FRAMEBUFFER, fbo);
	status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	glBindFramebuffer (GL_FRAMEBUFFER, (GLuint)previous);

	return status;
}
