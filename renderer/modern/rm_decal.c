/*
==============================================================================

	MODERN DECAL RENDERER

	Renders decals (bullet holes, scorch marks, blood splats) projected onto
	world surfaces.
	
	Decals in EGL are clipped polygons that conform to world geometry surfaces.
	Each refDecal_t contains:
	- vertices (vec3_t array) - world-space positions
	- normals (vec3_t array) - per-vertex normals
	- texCoords (vec2_t array) - texture coordinates
	- colors (bvec4_t array) - per-vertex RGBA colors
	- indexes (int array) - triangle indices
	- material - the decal texture (bullet hole, scorch, etc.)
	
	Decals are rendered with alpha blending after world geometry.

==============================================================================
*/

#define RM_NO_LEGACY_GL
#include "../glad/glad.h"

#include "../r_local.h"
#include "rm_decal.h"
#include "rm_shader.h"
#include "rm_tess.h"

/*
==============================================================================

	DECAL SHADER AND STATE

==============================================================================
*/

/* Decal shader and uniform locations */
static rmShader_t *rm_decalShader = NULL;
static GLint rm_decalLoc_Projection = -1;
static GLint rm_decalLoc_View = -1;
static GLint rm_decalLoc_Texture = -1;

/* Fallback white texture */
static GLuint rm_decalWhiteTex = 0;

/* Track initialization */
static qBool rm_decalInitialized = qFalse;

/*
==================
RM_Decal_Init

Initialize decal rendering resources.
==================
*/
void RM_Decal_Init(void)
{
	unsigned char whitePixel[4] = { 255, 255, 255, 255 };
	
	if (rm_decalInitialized)
		return;
	
	/* Load decal shader (reuse particle shader - same functionality) */
	rm_decalShader = RM_LoadShader("rm_particle");
	if (!rm_decalShader) {
		Com_Printf(PRNT_ERROR, "RM_Decal_Init: Failed to load decal shader\n");
		return;
	}
	
	/* Get uniform locations */
	rm_decalLoc_Projection = glGetUniformLocation(rm_decalShader->program, "u_Projection");
	rm_decalLoc_View = glGetUniformLocation(rm_decalShader->program, "u_View");
	rm_decalLoc_Texture = glGetUniformLocation(rm_decalShader->program, "u_Texture");
	
	/* Create fallback white texture */
	glGenTextures(1, &rm_decalWhiteTex);
	glBindTexture(GL_TEXTURE_2D, rm_decalWhiteTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	rm_decalInitialized = qTrue;
	Com_Printf(PRNT_CHATHUD, "Modern decal renderer initialized\n");
}

/*
==================
RM_Decal_Shutdown

Cleanup decal rendering resources.
==================
*/
void RM_Decal_Shutdown(void)
{
	if (!rm_decalInitialized)
		return;
	
	if (rm_decalWhiteTex) {
		glDeleteTextures(1, &rm_decalWhiteTex);
		rm_decalWhiteTex = 0;
	}
	
	rm_decalShader = NULL;
	rm_decalInitialized = qFalse;
}

/*
==================
RM_AddDecalToBatch

Add a single decal's indexed mesh to the shared tessellator.
==================
*/
static void RM_AddDecalToBatch(refDecal_t *decal)
{
	int i, numVerts, numIndexes;
	int baseVert, baseIndex;
	
	if (!decal || !decal->poly.vertices || !decal->poly.texCoords || !decal->poly.colors || !decal->indexes)
		return;

	numVerts = decal->poly.numVerts;
	if (numVerts <= 0)
		return;
	numIndexes = (int)decal->numIndexes;
	if (numIndexes <= 0)
		return;

	if (!RM_Tess_Reserve (numVerts, numIndexes))
		return;

	baseVert = rm_tess.numVerts;
	baseIndex = rm_tess.numIndices;
	rm_tess.numVerts += numVerts;
	rm_tess.numIndices += numIndexes;

	/* Copy vertices */
	for (i = 0; i < numVerts; i++) {
		float *pos = RM_TESS_POS_PTR (baseVert + i);
		float *tc = RM_TESS_TC_PTR (baseVert + i);
		float *lmtc = RM_TESS_LMTC_PTR (baseVert + i);

		pos[0] = decal->poly.vertices[i][0];
		pos[1] = decal->poly.vertices[i][1];
		pos[2] = decal->poly.vertices[i][2];

		tc[0] = decal->poly.texCoords[i][0];
		tc[1] = decal->poly.texCoords[i][1];

		lmtc[0] = 0.0f;
		lmtc[1] = 0.0f;

		RM_TESS_SET_COLOR_RGBA (baseVert + i,
			decal->poly.colors[i][0], decal->poly.colors[i][1], decal->poly.colors[i][2], decal->poly.colors[i][3]);
	}

	/* Copy indices (with baseVert offset) */
	for (i = 0; i < numIndexes; i++) {
		const int idx = decal->indexes[i];
		if (idx < 0 || idx >= numVerts) {
			rm_tess.indices[baseIndex + i] = (GLuint)baseVert;
			continue;
		}
		rm_tess.indices[baseIndex + i] = (GLuint)(baseVert + idx);
	}
}

/*
==================
RM_DrawDecals

Render all decals in the current scene.
==================
*/
void RM_DrawDecals(const float *projMatrix, const float *viewMatrix)
{
	static int rm_decalFrameNum = 0;
	int i;
	refDecal_t *decal;
	GLuint lastTexNum = 0;
	GLuint currentTexNum;
	int submittedVerts = 0;
	int submittedIndices = 0;

	if (!projMatrix || !viewMatrix)
		return;

	/* Match legacy behavior: allow disabling decals. */
	if (r_drawDecals && !r_drawDecals->intVal)
		return;
		
	if (!rm_decalInitialized || !rm_decalShader)
		return;
	
	if (ri.scn.numDecals == 0)
		return;

	rm_decalFrameNum++;
	if (rm_decalFrameNum % 60 == 0) {
		FILE *f = fopen("modern.log", "a");
		if (f) {
			fprintf(f, "RM_DrawDecals frame %d: numDecals=%d\n", rm_decalFrameNum, (int)ri.scn.numDecals);
			fclose(f);
		}
	}
	
	/* Setup GL state for decals - blend with depth test but no depth write */
	glEnable(GL_BLEND);
	/* Blood decals use subtractive blending: GL_ZERO, GL_ONE_MINUS_SRC_COLOR
	 * This makes the texture color "remove" that color from the destination,
	 * creating a darkening/staining effect. Cyan texture + this blend = red stain. */
	glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);  /* Don't write to depth buffer */
	glDepthFunc(GL_LEQUAL);  /* Render at same depth as surface */
	glDisable(GL_CULL_FACE);  /* Decals can face any direction */
	
	/* Small polygon offset to prevent z-fighting with underlying surface */
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1.0f, -1.0f);
	
	/* Use decal shader */
	glUseProgram(rm_decalShader->program);
	glUniformMatrix4fv(rm_decalLoc_Projection, 1, GL_FALSE, projMatrix);
	glUniformMatrix4fv(rm_decalLoc_View, 1, GL_FALSE, viewMatrix);
	glUniform1i(rm_decalLoc_Texture, 0);
	
	glActiveTexture(GL_TEXTURE0);

	for (i = 0; i < (int)ri.scn.numDecals; i++) {
		decal = ri.scn.decalList[i];
		if (!decal)
			continue;

		/* Get texture ID from material (same pattern as particles) */
		currentTexNum = rm_decalWhiteTex;
		if (decal->poly.mat && decal->poly.mat->numPasses > 0 && decal->poly.mat->passes) {
			matPass_t *pass = &decal->poly.mat->passes[0];
			if (pass->animNumImages > 0 && pass->animImages[0]) {
				currentTexNum = pass->animImages[0]->texNum;
			}
		}
		
		RM_Tess_Begin (currentTexNum, 0);

		if (currentTexNum != lastTexNum) {
			lastTexNum = currentTexNum;
			glBindTexture(GL_TEXTURE_2D, lastTexNum ? lastTexNum : rm_decalWhiteTex);
		}

		if (decal->poly.numVerts > 0) {
			submittedVerts += decal->poly.numVerts;
			submittedIndices += (int)decal->numIndexes;
		}
		RM_AddDecalToBatch(decal);
	}

	RM_Tess_Flush ();

	if (rm_decalFrameNum % 60 == 0) {
		FILE *f = fopen("modern.log", "a");
		if (f) {
			fprintf(f, "RM_DrawDecals frame %d: verts=%d indices=%d\n", rm_decalFrameNum, submittedVerts, submittedIndices);
			fclose(f);
		}
	}
	
	/* Restore GL state */
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);  /* Re-enable culling */
	glCullFace(GL_BACK);     /* Ensure standard culling mode */
	glFrontFace(GL_CCW);     /* Ensure standard winding */
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0); /* Unbind to prevent state leakage */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}
