/*
==============================================================================

	MODERN RENDERER - ALIAS MODEL (MD2) AND SPRITE (SP2) RENDERING
	
	Phase 12: Render entities (items, monsters) using MD2 models.
	Phase 13: Render SP2 sprites (explosions, BFG, pickups).
	
	Uses existing engine structures:
	- mAliasModel_t, mAliasMesh_t, mAliasFrame_t from rf_model.h
	- mSpriteModel_t, mSpriteFrame_t from rf_model.h
	- dMd2Header_t, dMd2Frame_t etc from files.h
	- refEntity_t from cg_shared.h
	
	For this phase, we render frame 0 only (no animation).

==============================================================================
*/

#define RM_NO_LEGACY_GL
#include "../glad/glad.h"

#include "../r_local.h"
#include "rm_backend.h"
#include "rm_alias.h"
#include "rm_shader.h"
#include <math.h>

/*
==============================================================================
	ALIAS MODEL RENDERING RESOURCES
==============================================================================
*/

/* Shader for alias models - uses same format as solid BSP (pos + normal + uv) */
static rmShader_t *rm_aliasShader = NULL;
static GLuint rm_aliasVAO = 0;
static GLuint rm_aliasVBO = 0;
static GLint rm_aliasLoc_Projection = -1;
static GLint rm_aliasLoc_View = -1;
static GLint rm_aliasLoc_Model = -1;
static GLint rm_aliasLoc_LightDir = -1;
static GLint rm_aliasLoc_Texture = -1;
static GLint rm_aliasLoc_NormalMatrix = -1;
static GLint rm_aliasLoc_UseNormalMatrix = -1;

/* Dynamic light uniforms for alias models */
#define RM_ALIAS_MAX_DLIGHTS 16
static GLint rm_aliasLoc_DLightPos = -1;
static GLint rm_aliasLoc_DLightColor = -1;
static GLint rm_aliasLoc_NumDLights = -1;
static GLint rm_aliasLoc_UseLightmap = -1;

/* Vertex format matching BSP solid shader */
typedef struct {
	float pos[3];
	float normal[3];
	float uv[2];
} aliasVertex_t;

/* Dynamic vertex buffer for alias model triangles */
#define RM_ALIAS_MAX_VERTS 65536
static aliasVertex_t rm_aliasVerts[RM_ALIAS_MAX_VERTS];
static int rm_aliasVertCount = 0;

/* Cached matrices from RM_DrawWorld */
static float rm_projMatrix[16];
static float rm_viewMatrix[16];
static qBool rm_matricesCached = qFalse;

/*
==================
RM_BuildModelMatrix

Build a model matrix from entity origin, axis, and scale.
For RF_FRAMELERP entities (monsters), interpolate between origin and oldOrigin
using backLerp to match the legacy renderer behavior.
==================
*/
static void RM_BuildModelMatrix(refEntity_t *ent, float *m)
{
	float scale;
	vec3_t interpOrigin;
	
	memset(m, 0, 16 * sizeof(float));
	
	scale = ent->scale > 0.0f ? ent->scale : 1.0f;
	
	/* Interpolate origin for RF_FRAMELERP entities (monsters) 
	 * backLerp: 0.0 = use new origin, 1.0 = use old origin */
	if (ent->flags & RF_FRAMELERP) {
		float frontLerp = 1.0f - ent->backLerp;
		interpOrigin[0] = ent->oldOrigin[0] * ent->backLerp + ent->origin[0] * frontLerp;
		interpOrigin[1] = ent->oldOrigin[1] * ent->backLerp + ent->origin[1] * frontLerp;
		interpOrigin[2] = ent->oldOrigin[2] * ent->backLerp + ent->origin[2] * frontLerp;
	} else {
		Vec3Copy(ent->origin, interpOrigin);
	}
	
	/* Use Matrix3_Compare to check for identity axis */
	if (Matrix3_Compare(ent->axis, axisIdentity)) {
		/* Use identity rotation */
		m[0] = scale;
		m[5] = scale;
		m[10] = scale;
	} else {
		/* Column-major order for OpenGL */
		/* Columns 0-2: rotated and scaled axis */
		m[0]  = ent->axis[0][0] * scale;
		m[1]  = ent->axis[0][1] * scale;
		m[2]  = ent->axis[0][2] * scale;
		
		m[4]  = ent->axis[1][0] * scale;
		m[5]  = ent->axis[1][1] * scale;
		m[6]  = ent->axis[1][2] * scale;
		
		m[8]  = ent->axis[2][0] * scale;
		m[9]  = ent->axis[2][1] * scale;
		m[10] = ent->axis[2][2] * scale;
	}
	
	/* Column 3: translation - use interpolated origin */
	m[12] = interpOrigin[0];
	m[13] = interpOrigin[1];
	m[14] = interpOrigin[2];
	m[15] = 1.0f;
}


/*
==================
RM_DecompressVertex

Decompress a packed MD2 vertex to world coordinates.
RealX = (PackedX * ScaleX) + TranslateX
==================
*/
static void RM_DecompressVertex(mAliasVertex_t *v, mAliasFrame_t *frame, float *out)
{
	out[0] = (float)v->point[0] * frame->scale[0] + frame->translate[0];
	out[1] = (float)v->point[1] * frame->scale[1] + frame->translate[1];
	out[2] = (float)v->point[2] * frame->scale[2] + frame->translate[2];
}

/*
==================
RM_LatLongToNormal

Convert latitude/longitude encoded normal to unit vector.
==================
*/
static void RM_LatLongToNormal(byte *latLong, float *out)
{
	float lat = (float)latLong[0] * (2.0f * 3.14159265f / 255.0f);
	float lng = (float)latLong[1] * (2.0f * 3.14159265f / 255.0f);
	
	out[0] = cosf(lat) * sinf(lng);
	out[1] = sinf(lat) * sinf(lng);
	out[2] = cosf(lng);
}

/*
==================
RM_Alias_Init

Initialize alias model rendering.
==================
*/
void RM_Alias_Init(void)
{
	/* Load alias shader - use same shader as BSP solid for now */
	rm_aliasShader = RM_LoadShader("rm_solid");
	if (!rm_aliasShader) {
		Com_Printf(PRNT_ERROR, "RM_Alias_Init: FAILED to load rm_solid shader\n");
		return;
	}
	
	/* Get uniform locations */
	rm_aliasLoc_Projection = glGetUniformLocation(rm_aliasShader->program, "u_Projection");
	rm_aliasLoc_View = glGetUniformLocation(rm_aliasShader->program, "u_View");
	rm_aliasLoc_Model = glGetUniformLocation(rm_aliasShader->program, "u_Model");
	rm_aliasLoc_LightDir = glGetUniformLocation(rm_aliasShader->program, "u_LightDir");
	rm_aliasLoc_Texture = glGetUniformLocation(rm_aliasShader->program, "u_Texture");
	rm_aliasLoc_NormalMatrix = glGetUniformLocation(rm_aliasShader->program, "u_NormalMatrix");
	rm_aliasLoc_UseNormalMatrix = glGetUniformLocation(rm_aliasShader->program, "u_UseNormalMatrix");
	
	/* Get dynamic light uniform locations */
	rm_aliasLoc_DLightPos = glGetUniformLocation(rm_aliasShader->program, "u_DLightPos");
	rm_aliasLoc_DLightColor = glGetUniformLocation(rm_aliasShader->program, "u_DLightColor");
	rm_aliasLoc_NumDLights = glGetUniformLocation(rm_aliasShader->program, "u_NumDLights");
	rm_aliasLoc_UseLightmap = glGetUniformLocation(rm_aliasShader->program, "u_UseLightmap");
	
	/* Create VAO/VBO for alias model rendering */
	glGenVertexArrays(1, &rm_aliasVAO);
	glBindVertexArray(rm_aliasVAO);
	
	glGenBuffers(1, &rm_aliasVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rm_aliasVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rm_aliasVerts), NULL, GL_DYNAMIC_DRAW);
	
	/* Attribute 0: position (3 floats) */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(aliasVertex_t), (void*)0);
	
	/* Attribute 1: normal (3 floats) */
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(aliasVertex_t), (void*)(3 * sizeof(float)));
	
	/* Attribute 2: UV (2 floats) */
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(aliasVertex_t), (void*)(6 * sizeof(float)));
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	Com_Printf(0, "RM_Alias_Init: VAO=%u VBO=%u Shader=%u\n",
		rm_aliasVAO, rm_aliasVBO, rm_aliasShader->program);
}

/*
==================
RM_Alias_Shutdown

Cleanup alias model resources.
==================
*/
void RM_Alias_Shutdown(void)
{
	if (rm_aliasVAO) {
		glDeleteVertexArrays(1, &rm_aliasVAO);
		rm_aliasVAO = 0;
	}
	if (rm_aliasVBO) {
		glDeleteBuffers(1, &rm_aliasVBO);
		rm_aliasVBO = 0;
	}
	rm_aliasShader = NULL;
}

/*
==================
RM_SetMatricesForEntities

Cache projection/view matrices from world rendering for entity use.
==================
*/
void RM_SetMatricesForEntities(float *proj, float *view)
{
	memcpy(rm_projMatrix, proj, 16 * sizeof(float));
	memcpy(rm_viewMatrix, view, 16 * sizeof(float));
	rm_matricesCached = qTrue;
}

/*
==================
RM_DrawAliasModel

Render a single alias model (MD2/MD3).
For Phase 12: renders frame 0 only.
==================
*/
void RM_DrawAliasModel(refEntity_t *ent)
{
	mAliasModel_t *aliasModel;
	mAliasMesh_t *mesh;
	mAliasFrame_t *frame;
	mAliasVertex_t *verts;
	mAliasSkin_t *skin;
	material_t *mat;
	image_t *texImage;
	float modelMatrix[16];
	int i, j;
	int frameNum;
	GLuint texNum = 0;
	
	if (!ent || !ent->model || !ent->model->aliasModel)
		return;
	
	if (!rm_aliasShader || !rm_aliasVAO || !rm_matricesCached)
		return;
	
	aliasModel = ent->model->aliasModel;
	
	/* Get current frame and lerp factor for interpolation */
	frameNum = 0;
	if (ent->frame >= 0 && ent->frame < aliasModel->numFrames) {
		frameNum = ent->frame;
	}
	
	/* backLerp: 0.0 = new frame, 1.0 = old frame */
	float frameLerp = ent->backLerp;
	int oldFrameNum = frameNum;  /* Will be blended with current */
	if (ent->oldFrame >= 0 && ent->oldFrame < aliasModel->numFrames) {
		oldFrameNum = ent->oldFrame;
	}
	
	/* Debug: Log first few entities */
	{
		static int entLogCount = 0;
		if (entLogCount < 5) {
			FILE *f = fopen("modern.log", "a");
			if (f) {
				fprintf(f, "RM_DrawAliasModel[%d]: '%s' frame=%d meshes=%d origin=(%.1f,%.1f,%.1f)\n",
					entLogCount, ent->model->name, frameNum, aliasModel->numMeshes,
					ent->origin[0], ent->origin[1], ent->origin[2]);
				fclose(f);
			}
			entLogCount++;
		}
	}
	
	/* Build model transformation matrix */
	RM_BuildModelMatrix(ent, modelMatrix);
	
	/* Process each mesh in the model */
	for (i = 0; i < aliasModel->numMeshes; i++) {
		mesh = &aliasModel->meshes[i];
		frame = &aliasModel->frames[frameNum];
		mAliasFrame_t *oldFrame = &aliasModel->frames[oldFrameNum];
		verts = mesh->vertexes + (frameNum * mesh->numVerts);
		mAliasVertex_t *oldVerts = mesh->vertexes + (oldFrameNum * mesh->numVerts);
		
		/* Get texture from skin */
		texNum = 0;
		if (ent->material) {
			/* Custom material */
			mat = ent->material;
			if (mat && mat->numPasses > 0 && mat->passes[0].animNumImages > 0) {
				texImage = mat->passes[0].animImages[0];
				if (texImage) texNum = texImage->texNum;
			}
		} else if (mesh->numSkins > 0) {
			/* Use model's skin */
			int skinIdx = (ent->skinNum >= 0 && ent->skinNum < mesh->numSkins) ? ent->skinNum : 0;
			skin = &mesh->skins[skinIdx];
			if (skin && skin->material) {
				mat = skin->material;
				if (mat && mat->numPasses > 0 && mat->passes[0].animNumImages > 0) {
					texImage = mat->passes[0].animImages[0];
					if (texImage) texNum = texImage->texNum;
				}
			}
		}
		
		/* Build vertex buffer from mesh */
		rm_aliasVertCount = 0;
		
		for (j = 0; j < mesh->numTris * 3 && rm_aliasVertCount < RM_ALIAS_MAX_VERTS; j++) {
			index_t idx = mesh->indexes[j];
			mAliasVertex_t *v, *vOld;
			aliasVertex_t *out;
			vec3_t posNew, posOld;
			vec3_t normNew, normOld, normInterp;
			
			if (idx >= mesh->numVerts)
				continue;
			
			v = &verts[idx];
			vOld = &oldVerts[idx];
			out = &rm_aliasVerts[rm_aliasVertCount];
			
			/* Decompress both vertices */
			RM_DecompressVertex(v, frame, posNew);
			RM_DecompressVertex(vOld, oldFrame, posOld);
			
			/* Interpolate position: (1 - lerp) * new + lerp * old */
			float interp = 1.0f - frameLerp;
			out->pos[0] = posNew[0] * interp + posOld[0] * frameLerp;
			out->pos[1] = posNew[1] * interp + posOld[1] * frameLerp;
			out->pos[2] = posNew[2] * interp + posOld[2] * frameLerp;
			
			/* Decode normals */
			RM_LatLongToNormal(v->latLong, normNew);
			RM_LatLongToNormal(vOld->latLong, normOld);
			
			/* Interpolate normal */
			normInterp[0] = normNew[0] * interp + normOld[0] * frameLerp;
			normInterp[1] = normNew[1] * interp + normOld[1] * frameLerp;
			normInterp[2] = normNew[2] * interp + normOld[2] * frameLerp;
			VectorNormalizef(normInterp, out->normal);
			
			/* Get UV coordinates */
			if (mesh->coords) {
				out->uv[0] = mesh->coords[idx][0];
				out->uv[1] = mesh->coords[idx][1];
			} else {
				out->uv[0] = 0.0f;
				out->uv[1] = 0.0f;
			}
			
			rm_aliasVertCount++;
		}
		
		if (rm_aliasVertCount < 3)
			continue;
		
		/* Render this mesh */
		glUseProgram(rm_aliasShader->program);
		
		/* Set uniforms */
		glUniformMatrix4fv(rm_aliasLoc_Projection, 1, GL_FALSE, rm_projMatrix);
		glUniformMatrix4fv(rm_aliasLoc_View, 1, GL_FALSE, rm_viewMatrix);
		if (rm_aliasLoc_Model >= 0) {
			glUniformMatrix4fv(rm_aliasLoc_Model, 1, GL_FALSE, modelMatrix);
		}
		if (rm_aliasLoc_NormalMatrix >= 0)
			glUniformMatrix4fv(rm_aliasLoc_NormalMatrix, 1, GL_FALSE, modelMatrix);
		if (rm_aliasLoc_UseNormalMatrix >= 0)
			glUniform1f(rm_aliasLoc_UseNormalMatrix, 1.0f);
		glUniform3f(rm_aliasLoc_LightDir, 0.424f, 0.848f, 0.254f);
		glUniform1i(rm_aliasLoc_Texture, 0);
		
		/* Set UseLightmap to 0 (use directional + dynamic lights) */
		if (rm_aliasLoc_UseLightmap >= 0)
			glUniform1f(rm_aliasLoc_UseLightmap, 0.0f);
		
		/* Upload dynamic lights (muzzle flashes, explosions, etc.) */
		{
			int numDLights = ri.scn.numDLights;
			if (numDLights > RM_ALIAS_MAX_DLIGHTS)
				numDLights = RM_ALIAS_MAX_DLIGHTS;
			
			if (numDLights > 0 && rm_aliasLoc_NumDLights >= 0) {
				float dLightPos[RM_ALIAS_MAX_DLIGHTS * 4];
				float dLightColor[RM_ALIAS_MAX_DLIGHTS * 4];
				int dl;
				
				for (dl = 0; dl < numDLights; dl++) {
					refDLight_t *light = &ri.scn.dLightList[dl];
					
					/* Pack position (xyz) and intensity (w) */
					dLightPos[dl * 4 + 0] = light->origin[0];
					dLightPos[dl * 4 + 1] = light->origin[1];
					dLightPos[dl * 4 + 2] = light->origin[2];
					dLightPos[dl * 4 + 3] = light->intensity;
					
					/* Pack color (rgb) */
					dLightColor[dl * 4 + 0] = light->color[0];
					dLightColor[dl * 4 + 1] = light->color[1];
					dLightColor[dl * 4 + 2] = light->color[2];
					dLightColor[dl * 4 + 3] = 1.0f;
				}
				
				glUniform4fv(rm_aliasLoc_DLightPos, numDLights, dLightPos);
				glUniform4fv(rm_aliasLoc_DLightColor, numDLights, dLightColor);
				glUniform1i(rm_aliasLoc_NumDLights, numDLights);
			} else {
				/* No dynamic lights */
				if (rm_aliasLoc_NumDLights >= 0)
					glUniform1i(rm_aliasLoc_NumDLights, 0);
			}
		}
		
		/* Bind texture */
		glActiveTexture(GL_TEXTURE0);
		if (texNum > 0) {
			glBindTexture(GL_TEXTURE_2D, texNum);
		} else {
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		/* Upload and draw */
		glBindVertexArray(rm_aliasVAO);
		glBindBuffer(GL_ARRAY_BUFFER, rm_aliasVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, rm_aliasVertCount * sizeof(aliasVertex_t), rm_aliasVerts);
		
		glDrawArrays(GL_TRIANGLES, 0, rm_aliasVertCount);
		
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	glUseProgram(0);
}

/*
==============================================================================

	SP2 SPRITE RENDERING

	Sprites are billboarded quads that always face the camera.
	Used for explosions, BFG ball, some pickups.

==============================================================================
*/

/* Sprite VAO/VBO - reuses particle shader */
static GLuint rm_spriteVAO = 0;
static GLuint rm_spriteVBO = 0;
static qBool rm_spriteInitialized = qFalse;

/* Sprite vertex format (same as particle) */
typedef struct {
	float pos[3];
	float uv[2];
	unsigned char color[4];
} spriteVertex_t;

/* Sprite buffer */
#define RM_SPRITE_MAX_VERTS 1024
static spriteVertex_t rm_spriteVerts[RM_SPRITE_MAX_VERTS];
static int rm_spriteVertCount = 0;

/*
==================
RM_Sprite_Init

Initialize sprite rendering resources.
==================
*/
void RM_Sprite_Init(void)
{
	if (rm_spriteInitialized)
		return;
	
	/* Create VAO */
	glGenVertexArrays(1, &rm_spriteVAO);
	glBindVertexArray(rm_spriteVAO);
	
	/* Create VBO */
	glGenBuffers(1, &rm_spriteVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rm_spriteVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rm_spriteVerts), NULL, GL_DYNAMIC_DRAW);
	
	/* Vertex attributes - matches particle shader */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(spriteVertex_t), (void*)offsetof(spriteVertex_t, pos));
	
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(spriteVertex_t), (void*)offsetof(spriteVertex_t, uv));
	
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(spriteVertex_t), (void*)offsetof(spriteVertex_t, color));
	
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	rm_spriteInitialized = qTrue;
	Com_Printf(0, "RM_Sprite_Init: Sprite system initialized\n");
}

/*
==================
RM_Sprite_Shutdown

Free sprite resources.
==================
*/
void RM_Sprite_Shutdown(void)
{
	if (!rm_spriteInitialized)
		return;
	
	if (rm_spriteVAO) {
		glDeleteVertexArrays(1, &rm_spriteVAO);
		rm_spriteVAO = 0;
	}
	
	if (rm_spriteVBO) {
		glDeleteBuffers(1, &rm_spriteVBO);
		rm_spriteVBO = 0;
	}
	
	rm_spriteInitialized = qFalse;
}

/*
==================
RM_GetSpriteTexNum

Get the GL texture number from a sprite's material.
==================
*/
static GLuint RM_GetSpriteTexNum(material_t *mat)
{
	matPass_t *pass;
	image_t *img;
	
	if (!mat || mat->numPasses <= 0 || !mat->passes)
		return 0;
	
	pass = &mat->passes[0];
	if (pass->animNumImages <= 0 || !pass->animImages[0])
		return 0;
	
	img = pass->animImages[0];
	return img->texNum;
}

/*
==================
RM_DrawSprite

Draw a single SP2 sprite entity as a billboarded quad.
==================
*/
static void RM_DrawSprite(refEntity_t *ent)
{
	mSpriteModel_t *spriteModel;
	mSpriteFrame_t *spriteFrame;
	spriteVertex_t *v;
	float scale, scaledWidth, scaledHeight, scaledOriginX, scaledOriginY;
	vec3_t up, right;
	
	if (!ent || !ent->model || !ent->model->spriteModel)
		return;
	
	spriteModel = ent->model->spriteModel;
	spriteFrame = &spriteModel->frames[ent->frame % spriteModel->numFrames];
	
	if (!spriteFrame->material)
		return;
	
	/* Cull sprites behind the camera */
	{
		float dot;
		dot = (ent->origin[0] - ri.def.viewOrigin[0]) * ri.def.viewAxis[0][0]
			+ (ent->origin[1] - ri.def.viewOrigin[1]) * ri.def.viewAxis[0][1]
			+ (ent->origin[2] - ri.def.viewOrigin[2]) * ri.def.viewAxis[0][2];
		if (dot < 0)
			return;
	}
	
	/* Effect scale */
	scale = r_effectscale->floatVal;
	if (scale == 0.0f)
		scale = sqrt((vid_width->intVal * vid_height->intVal) / (1024.0f * 768.0f));
	
	scaledWidth = spriteFrame->width * scale;
	scaledHeight = spriteFrame->height * scale;
	scaledOriginX = spriteFrame->originX * scale;
	scaledOriginY = spriteFrame->originY * scale;
	
	/* Get billboard vectors - face the camera */
	Vec3Copy(ri.def.viewAxis[2], up);
	Vec3Copy(ri.def.rightVec, right);
	
	/* Check buffer space */
	if (rm_spriteVertCount + 6 > RM_SPRITE_MAX_VERTS)
		return;
	
	/* Build quad vertices (2 triangles = 6 vertices)
	 * Vertex order: top-left, bottom-left, bottom-right (tri 1)
	 *               top-left, bottom-right, top-right (tri 2) */
	
	/* Triangle 1 */
	/* Vertex 0: top-left */
	v = &rm_spriteVerts[rm_spriteVertCount++];
	v->pos[0] = ent->origin[0] + (up[0] * -scaledOriginY) + (right[0] * -scaledOriginX);
	v->pos[1] = ent->origin[1] + (up[1] * -scaledOriginY) + (right[1] * -scaledOriginX);
	v->pos[2] = ent->origin[2] + (up[2] * -scaledOriginY) + (right[2] * -scaledOriginX);
	v->uv[0] = 0.0f; v->uv[1] = 1.0f;
	v->color[0] = ent->color[0]; v->color[1] = ent->color[1];
	v->color[2] = ent->color[2]; v->color[3] = ent->color[3];
	
	/* Vertex 1: bottom-left (actually top according to tex coords) */
	v = &rm_spriteVerts[rm_spriteVertCount++];
	v->pos[0] = ent->origin[0] + (up[0] * (scaledHeight - scaledOriginY)) + (right[0] * -scaledOriginX);
	v->pos[1] = ent->origin[1] + (up[1] * (scaledHeight - scaledOriginY)) + (right[1] * -scaledOriginX);
	v->pos[2] = ent->origin[2] + (up[2] * (scaledHeight - scaledOriginY)) + (right[2] * -scaledOriginX);
	v->uv[0] = 0.0f; v->uv[1] = 0.0f;
	v->color[0] = ent->color[0]; v->color[1] = ent->color[1];
	v->color[2] = ent->color[2]; v->color[3] = ent->color[3];
	
	/* Vertex 2: bottom-right (actually top-right) */
	v = &rm_spriteVerts[rm_spriteVertCount++];
	v->pos[0] = ent->origin[0] + (up[0] * (scaledHeight - scaledOriginY)) + (right[0] * (scaledWidth - scaledOriginX));
	v->pos[1] = ent->origin[1] + (up[1] * (scaledHeight - scaledOriginY)) + (right[1] * (scaledWidth - scaledOriginX));
	v->pos[2] = ent->origin[2] + (up[2] * (scaledHeight - scaledOriginY)) + (right[2] * (scaledWidth - scaledOriginX));
	v->uv[0] = 1.0f; v->uv[1] = 0.0f;
	v->color[0] = ent->color[0]; v->color[1] = ent->color[1];
	v->color[2] = ent->color[2]; v->color[3] = ent->color[3];
	
	/* Triangle 2 */
	/* Vertex 3: top-left (same as vertex 0) */
	v = &rm_spriteVerts[rm_spriteVertCount++];
	v->pos[0] = ent->origin[0] + (up[0] * -scaledOriginY) + (right[0] * -scaledOriginX);
	v->pos[1] = ent->origin[1] + (up[1] * -scaledOriginY) + (right[1] * -scaledOriginX);
	v->pos[2] = ent->origin[2] + (up[2] * -scaledOriginY) + (right[2] * -scaledOriginX);
	v->uv[0] = 0.0f; v->uv[1] = 1.0f;
	v->color[0] = ent->color[0]; v->color[1] = ent->color[1];
	v->color[2] = ent->color[2]; v->color[3] = ent->color[3];
	
	/* Vertex 4: bottom-right (same as vertex 2) */
	v = &rm_spriteVerts[rm_spriteVertCount++];
	v->pos[0] = ent->origin[0] + (up[0] * (scaledHeight - scaledOriginY)) + (right[0] * (scaledWidth - scaledOriginX));
	v->pos[1] = ent->origin[1] + (up[1] * (scaledHeight - scaledOriginY)) + (right[1] * (scaledWidth - scaledOriginX));
	v->pos[2] = ent->origin[2] + (up[2] * (scaledHeight - scaledOriginY)) + (right[2] * (scaledWidth - scaledOriginX));
	v->uv[0] = 1.0f; v->uv[1] = 0.0f;
	v->color[0] = ent->color[0]; v->color[1] = ent->color[1];
	v->color[2] = ent->color[2]; v->color[3] = ent->color[3];
	
	/* Vertex 5: top-right */
	v = &rm_spriteVerts[rm_spriteVertCount++];
	v->pos[0] = ent->origin[0] + (up[0] * -scaledOriginY) + (right[0] * (scaledWidth - scaledOriginX));
	v->pos[1] = ent->origin[1] + (up[1] * -scaledOriginY) + (right[1] * (scaledWidth - scaledOriginX));
	v->pos[2] = ent->origin[2] + (up[2] * -scaledOriginY) + (right[2] * (scaledWidth - scaledOriginX));
	v->uv[0] = 1.0f; v->uv[1] = 1.0f;
	v->color[0] = ent->color[0]; v->color[1] = ent->color[1];
	v->color[2] = ent->color[2]; v->color[3] = ent->color[3];
}

/*
==================
RM_DrawSprites

Render all queued sprites. Should be called after RM_DrawEntities
with appropriate blending state for translucent sprites.
==================
*/
void RM_DrawSprites(void)
{
	extern rmShader_t *RM_LoadShader(const char *name);
	static rmShader_t *rm_spriteShader = NULL;
	static GLint rm_spriteLoc_Projection = -1;
	static GLint rm_spriteLoc_View = -1;
	static GLint rm_spriteLoc_Texture = -1;
	
	refEntity_t *ent;
	int i;
	GLuint currentTex = 0;
	GLuint lastTex = 0;
	mSpriteModel_t *spriteModel;
	mSpriteFrame_t *spriteFrame;
	
	if (!rm_matricesCached)
		return;
	
	/* Initialize sprite system on first use */
	if (!rm_spriteInitialized)
		RM_Sprite_Init();
	
	if (!rm_spriteVAO)
		return;
	
	/* Load shader on first use */
	if (!rm_spriteShader) {
		rm_spriteShader = RM_LoadShader("rm_particle"); /* Reuse particle shader */
		if (rm_spriteShader) {
			rm_spriteLoc_Projection = glGetUniformLocation(rm_spriteShader->program, "u_Projection");
			rm_spriteLoc_View = glGetUniformLocation(rm_spriteShader->program, "u_View");
			rm_spriteLoc_Texture = glGetUniformLocation(rm_spriteShader->program, "u_Texture");
		}
	}
	
	if (!rm_spriteShader || !rm_spriteShader->program)
		return;
	
	/* Set up blending for translucent sprites */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	
	glUseProgram(rm_spriteShader->program);
	glUniformMatrix4fv(rm_spriteLoc_Projection, 1, GL_FALSE, rm_projMatrix);
	glUniformMatrix4fv(rm_spriteLoc_View, 1, GL_FALSE, rm_viewMatrix);
	glUniform1i(rm_spriteLoc_Texture, 0);
	
	glBindVertexArray(rm_spriteVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rm_spriteVBO);
	glActiveTexture(GL_TEXTURE0);
	
	/* Collect and render sprites, batching by texture where possible */
	for (i = 2; i < ri.scn.numEntities; i++) {
		ent = &ri.scn.entityList[i];
		
		if (!ent->model)
			continue;
		
		if (ent->model->type != MODEL_SP2)
			continue;
		
		if (ent->flags & RF_VIEWERMODEL)
			continue;
		
		spriteModel = ent->model->spriteModel;
		if (!spriteModel)
			continue;
		
		spriteFrame = &spriteModel->frames[ent->frame % spriteModel->numFrames];
		if (!spriteFrame->material)
			continue;
		
		currentTex = RM_GetSpriteTexNum(spriteFrame->material);
		
		/* If texture changed and we have pending verts, flush */
		if (currentTex != lastTex && rm_spriteVertCount > 0) {
			glBindTexture(GL_TEXTURE_2D, lastTex ? lastTex : currentTex);
			glBufferSubData(GL_ARRAY_BUFFER, 0, rm_spriteVertCount * sizeof(spriteVertex_t), rm_spriteVerts);
			glDrawArrays(GL_TRIANGLES, 0, rm_spriteVertCount);
			rm_spriteVertCount = 0;
		}
		
		lastTex = currentTex;
		RM_DrawSprite(ent);
	}
	
	/* Flush remaining sprites */
	if (rm_spriteVertCount > 0) {
		glBindTexture(GL_TEXTURE_2D, lastTex);
		glBufferSubData(GL_ARRAY_BUFFER, 0, rm_spriteVertCount * sizeof(spriteVertex_t), rm_spriteVerts);
		glDrawArrays(GL_TRIANGLES, 0, rm_spriteVertCount);
		rm_spriteVertCount = 0;
	}
	
	/* Restore state */
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

/*
==================
RM_DrawEntities

Loop through all entities and render alias models.
First pass: regular entities (no depth hack)
Second pass: weapon models (with depth hack for proper Z handling)
==================
*/
void RM_DrawEntities(void)
{
	refEntity_t *ent;
	int i;
	
	if (!rm_matricesCached)
		return;
	
	/* First pass: regular entities (no weapon/viewermodel) */
	for (i = 2; i < ri.scn.numEntities; i++) {
		ent = &ri.scn.entityList[i];
		
		if (!ent->model)
			continue;
		
		/* Skip viewer model (player's body in first-person) */
		if (ent->flags & RF_VIEWERMODEL)
			continue;
		
		/* Skip weapon models - rendered in second pass with depth hack */
		if (ent->flags & (RF_WEAPONMODEL | RF_DEPTHHACK))
			continue;
		
		/* Only render MD2/MD3 models for now */
		if (ent->model->type == MODEL_MD2 || ent->model->type == MODEL_MD3) {
			RM_DrawAliasModel(ent);
		}
	}
	
	/* Second pass: weapon models with depth hack
	 * RF_DEPTHHACK uses glDepthRange(0, 0.3) to prevent weapons from
	 * clipping into walls while appearing close to the camera */
	for (i = 2; i < ri.scn.numEntities; i++) {
		ent = &ri.scn.entityList[i];
		
		if (!ent->model)
			continue;
		
		/* Only process weapon/depthhack entities */
		if (!(ent->flags & (RF_WEAPONMODEL | RF_DEPTHHACK)))
			continue;
		
		/* Skip viewermodel even if it has depthhack */
		if (ent->flags & RF_VIEWERMODEL)
			continue;
		
		/* Only render MD2/MD3 models */
		if (ent->model->type == MODEL_MD2 || ent->model->type == MODEL_MD3) {
			/* Apply depth hack - compress Z range to prevent wall clipping */
			glDepthRange(0.0, 0.3);
			
			/* Left-handed weapon support (RF_CULLHACK) */
			if (ent->flags & RF_CULLHACK)
				glFrontFace(GL_CW);
			
			RM_DrawAliasModel(ent);
			
			/* Restore front face */
			if (ent->flags & RF_CULLHACK)
				glFrontFace(GL_CCW);
			
			/* Restore normal depth range */
			glDepthRange(0.0, 1.0);
		}
	}
}
