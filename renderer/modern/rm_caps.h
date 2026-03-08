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
// rm_caps.h - Modern renderer OpenGL capability detection (MODERN-ONLY)
//

#ifndef __RM_CAPS_H__
#define __RM_CAPS_H__

/* This file is ONLY compiled when EGL_MODERN_RENDERER=1 */
#if !EGL_MODERN_RENDERER
#error "rm_caps.h should only be included in modern renderer builds"
#endif

#include "shared/shared.h"

typedef struct {
	/* Version info */
	int glVersionMajor;
	int glVersionMinor;
	int glslVersion;

	/* GL 3.3 baseline features (always available) */
	qBool hasVAO;
	qBool hasUBO;
	qBool hasFBO;
	qBool hasInstancing;

	/* GL 4.0 */
	qBool hasTessellation;

	/* GL 4.2 */
	qBool hasBaseInstance;
	qBool hasImageLoadStore;

	/* GL 4.3 */
	qBool hasComputeShaders;
	qBool hasSSBO;
	qBool hasDebugOutput;
	qBool hasMultiDrawIndirect;
	qBool hasExplicitUniformLoc;

	/* GL 4.4 */
	qBool hasPersistentMap;
	qBool hasMultiBind;

	/* GL 4.5 */
	qBool hasDirectStateAccess;
	qBool hasClipControl;

	/* Extensions */
	qBool hasBindlessTexture;
	qBool hasAnisotropic;
	qBool hasSparseTexture;

	/* Limits */
	int maxTextureSize;
	int maxTextureUnits;
	int maxUniformBlockSize;
	int maxSSBOSize;
	int maxComputeWorkGroupSize[3];
	int maxComputeWorkGroupInvocations;
	float maxAnisotropy;

	/* Vendor */
	qBool isNvidia;
	qBool isAMD;
	qBool isIntel;
	qBool isMesa;
} rmCaps_t;

extern rmCaps_t rm_caps;

void RM_Caps_Init (void);
void RM_Caps_Print (void);

/* Convenience macros */
#define RM_HAS_COMPUTE()	(rm_caps.hasComputeShaders)
#define RM_HAS_SSBO()		(rm_caps.hasSSBO)
#define RM_HAS_PERSISTENT()	(rm_caps.hasPersistentMap)
#define RM_HAS_DSA()		(rm_caps.hasDirectStateAccess)
#define RM_HAS_BINDLESS()	(rm_caps.hasBindlessTexture)
#define RM_HAS_MDI()		(rm_caps.hasMultiDrawIndirect)

#endif /* __RM_CAPS_H__ */

