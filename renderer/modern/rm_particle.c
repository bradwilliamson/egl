/*
==============================================================================

	MODERN PARTICLE RENDERER

	Renders particles (quads) from refPoly_t submissions.
	
	Particles in EGL are submitted as quads (4 vertices) with texture coordinates
	and per-vertex colors. They use alpha blending and are rendered after world
	geometry and entities (translucent pass).

==============================================================================
*/

#define RM_NO_LEGACY_GL
#include "../glad/glad.h"

#include "../r_local.h"
#include "rm_dsa.h"
#include "rm_particle.h"
#include "rm_shader.h"
#include "rm_tess.h"

/*
==============================================================================

	PARTICLE SHADER AND STATE

==============================================================================
*/

/* Particle shader and uniform locations */
static rmShader_t *rm_particleShader = NULL;
static GLint rm_partLoc_Projection = -1;
static GLint rm_partLoc_View = -1;
static GLint rm_partLoc_Texture = -1;

/* Fallback white texture (same as world uses) */
static GLuint rm_particleWhiteTex = 0;

/* Track initialization */
static qBool rm_particleInitialized = qFalse;

/*
==================
RM_Particle_Init

Initialize particle rendering resources.
==================
*/
void RM_Particle_Init(void)
{
	unsigned char whitePixel[4] = { 255, 255, 255, 255 };
	
	if (rm_particleInitialized)
		return;
	
	/* Load particle shader */
	rm_particleShader = RM_LoadShader("rm_particle");
	if (!rm_particleShader) {
		Com_Printf(PRNT_ERROR, "RM_Particle_Init: Failed to load particle shader\n");
		return;
	}
	
	/* Get uniform locations */
	rm_partLoc_Projection = glGetUniformLocation(rm_particleShader->program, "u_Projection");
	rm_partLoc_View = glGetUniformLocation(rm_particleShader->program, "u_View");
	rm_partLoc_Texture = glGetUniformLocation(rm_particleShader->program, "u_Texture");
	
	/* Create fallback white texture */
	rm_particleWhiteTex = RM_CreateTexture2D();
	if (!rm_particleWhiteTex) {
		Com_Printf(PRNT_ERROR, "RM_Particle_Init: Failed to create fallback texture\n");
		return;
	}
	RM_TexImage2D(rm_particleWhiteTex, 1, 1, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
	RM_TexParameteri(rm_particleWhiteTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	RM_TexParameteri(rm_particleWhiteTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	rm_particleInitialized = qTrue;
	
	Com_Printf(0, "RM_Particle_Init: Particle system initialized\n");
}

/*
==================
RM_Particle_Shutdown

Free particle rendering resources.
==================
*/
void RM_Particle_Shutdown(void)
{
	if (!rm_particleInitialized)
		return;
	
	if (rm_particleWhiteTex) {
		glDeleteTextures(1, &rm_particleWhiteTex);
		rm_particleWhiteTex = 0;
	}
	
	rm_particleShader = NULL;  /* Shader system owns this */
	rm_particleInitialized = qFalse;
	
	Com_Printf(0, "RM_Particle_Shutdown: Particle system shutdown\n");
}

/*
==================
RM_GetParticleTexNum

Get the GL texture number from a material's first pass.
Returns 0 if no valid texture found.
==================
*/
static GLuint RM_GetParticleTexNum(material_t *mat)
{
	matPass_t *pass;
	image_t *img;
	
	if (!mat)
		return 0;
	
	if (mat->numPasses <= 0 || !mat->passes)
		return 0;
	
	pass = &mat->passes[0];
	
	if (pass->animNumImages <= 0 || !pass->animImages[0])
		return 0;
	
	img = pass->animImages[0];
	return img->texNum;
}

/*
==================
RM_AddParticleQuad

Add a quad (2 triangles, 4 vertices + 6 indices) to the shared tessellator.
==================
*/
static void RM_AddParticleQuad(refPoly_t *poly)
{
	int i;
	int baseVert, baseIndex;

	/* Need 4 vertices for a quad */
	if (!poly || poly->numVerts != 4)
		return;

	if (!RM_Tess_Reserve (4, 6))
		return;

	baseVert = rm_tess.numVerts;
	baseIndex = rm_tess.numIndices;
	rm_tess.numVerts += 4;
	rm_tess.numIndices += 6;

	/* Copy 4 unique vertices */
	for (i = 0; i < 4; i++) {
		float *pos = RM_TESS_POS_PTR (baseVert + i);
		float *tc = RM_TESS_TC_PTR (baseVert + i);
		float *lmtc = RM_TESS_LMTC_PTR (baseVert + i);

		pos[0] = poly->vertices[i][0];
		pos[1] = poly->vertices[i][1];
		pos[2] = poly->vertices[i][2];

		tc[0] = poly->texCoords[i][0];
		tc[1] = poly->texCoords[i][1];

		lmtc[0] = 0.0f;
		lmtc[1] = 0.0f;

		RM_TESS_SET_COLOR_RGBA (baseVert + i,
			poly->colors[i][0], poly->colors[i][1], poly->colors[i][2], poly->colors[i][3]);
	}

	/* Indices: 0-1-2, 0-2-3 */
	rm_tess.indices[baseIndex + 0] = (GLuint)(baseVert + 0);
	rm_tess.indices[baseIndex + 1] = (GLuint)(baseVert + 1);
	rm_tess.indices[baseIndex + 2] = (GLuint)(baseVert + 2);
	rm_tess.indices[baseIndex + 3] = (GLuint)(baseVert + 0);
	rm_tess.indices[baseIndex + 4] = (GLuint)(baseVert + 2);
	rm_tess.indices[baseIndex + 5] = (GLuint)(baseVert + 3);
}

/*
==================
RM_GetParticleBlendMode

Get the blend source and dest from a material's first pass.
Returns default alpha blend if not found.
==================
*/
static void RM_GetParticleBlendMode(material_t *mat, GLenum *blendSrc, GLenum *blendDst)
{
	matPass_t *pass;
	
	/* Default: standard alpha blending */
	*blendSrc = GL_SRC_ALPHA;
	*blendDst = GL_ONE_MINUS_SRC_ALPHA;
	
	if (!mat || mat->numPasses <= 0 || !mat->passes)
		return;
	
	pass = &mat->passes[0];
	
	/* If the material specifies blend modes, use them */
	if (pass->blendSource != 0 || pass->blendDest != 0) {
		*blendSrc = pass->blendSource ? pass->blendSource : GL_SRC_ALPHA;
		*blendDst = pass->blendDest ? pass->blendDest : GL_ONE_MINUS_SRC_ALPHA;
	}
}

/*
==================
RM_Particle_Draw

Draw all particles from ri.scn.polyList.
==================
*/
void RM_Particle_Draw(float *projMatrix, float *viewMatrix)
{
	uint32 i;
	refPoly_t *poly;
	GLuint currentTexNum;
	GLuint polyTexNum;
	GLenum currentBlendSrc, currentBlendDst;
	GLenum polyBlendSrc, polyBlendDst;
	uint64_t polyStateFlags;
	
	if (!rm_particleInitialized)
		return;
	
	/* Match legacy behavior: allow disabling poly effects entirely. */
	if (r_drawPolys && !r_drawPolys->intVal)
		return;

	if (!rm_particleShader || !rm_tess.vao)
		return;
	
	/* No particles to draw */
	if (ri.scn.numPolys == 0)
		return;
	
	/* Set up initial blending state - will be updated per-particle */
	glEnable(GL_BLEND);
	currentBlendSrc = GL_SRC_ALPHA;
	currentBlendDst = GL_ONE_MINUS_SRC_ALPHA;
	glBlendFunc(currentBlendSrc, currentBlendDst);
	
	/* Be defensive about state leakage from other passes (weapons/decals) */
	glDepthRange(0.0, 1.0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	
	/* Depth test but no depth write (particles are translucent) */
	glEnable(GL_DEPTH_TEST);
	/* Legacy material system defaults to GL_LEQUAL for passes (including particles). */
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	
	/* No face culling for particles (they're billboards) */
	glDisable(GL_CULL_FACE);
	
	/* Use particle shader */
	glUseProgram(rm_particleShader->program);
	glUniformMatrix4fv(rm_partLoc_Projection, 1, GL_FALSE, projMatrix);
	glUniformMatrix4fv(rm_partLoc_View, 1, GL_FALSE, viewMatrix);
	glUniform1i(rm_partLoc_Texture, 0);
	
	glActiveTexture(GL_TEXTURE0);
	
	/* Batch particles by texture and blend mode for efficiency */
	currentTexNum = 0;
	
	for (i = 0; i < ri.scn.numPolys; i++) {
		poly = ri.scn.polyList[i];
		
		if (!poly || poly->numVerts != 4)
			continue;
		
		/* Get texture for this poly */
		polyTexNum = RM_GetParticleTexNum(poly->mat);
		if (polyTexNum == 0)
			polyTexNum = rm_particleWhiteTex;
		
		/* Get blend mode for this poly */
		RM_GetParticleBlendMode(poly->mat, &polyBlendSrc, &polyBlendDst);

		polyStateFlags = (uint64_t)polyBlendSrc | ((uint64_t)polyBlendDst << 32);
		RM_Tess_Begin (polyTexNum, polyStateFlags);
		
		/* Update blend mode if changed */
		if (polyBlendSrc != currentBlendSrc || polyBlendDst != currentBlendDst) {
			currentBlendSrc = polyBlendSrc;
			currentBlendDst = polyBlendDst;
			glBlendFunc(currentBlendSrc, currentBlendDst);
		}
		
		/* Bind texture if changed */
		if (polyTexNum != currentTexNum) {
			currentTexNum = polyTexNum;
			glBindTexture(GL_TEXTURE_2D, currentTexNum ? currentTexNum : rm_particleWhiteTex);
		}
		
		/* Add this quad to the buffer */
		RM_AddParticleQuad(poly);
	}
	
	/* Flush remaining */
	RM_Tess_Flush ();
	
	/* Restore state */
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);
	
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glDisable(GL_BLEND);
}
