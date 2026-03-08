#define RM_NO_LEGACY_GL
#include "../glad/glad.h"

#include "../r_local.h"
#include "rm_backend.h"
#include "rm_world.h"
#include "rm_geom.h"
#include "rm_shader.h"
#include "rm_alias.h"
#include "rm_particle.h"
#include "rm_decal.h"
#include <math.h>

/* Forward declarations for sky module */
extern void RM_Sky_Init(void);
extern void RM_Sky_Shutdown(void);
extern void RM_Sky_Draw(float *projMatrix, float *viewMatrix);
extern void RM_Sky_SetTextures(const char *baseName);
extern qBool RM_Sky_IsLoaded(void);

/*
==============================================================================

	MODERN WORLD RENDERER
	
	Phase 9: Solid BSP rendering with triangulation and depth testing.
	Optimized: Uses STATIC VBOs for world geometry (calculated once).

==============================================================================
*/

/* Wireframe rendering resources (kept for debug toggle) */
static rmShader_t *rm_wireShader = NULL;
static GLuint rm_wireVAO = 0;
static GLuint rm_wireVBO = 0;
static GLint rm_wireLoc_Projection = -1;
static GLint rm_wireLoc_View = -1;
static GLint rm_wireLoc_Color = -1;

/* Dynamic vertex buffer for wireframe lines */
#define RM_WIRE_MAX_VERTS 262144
static vec3_t rm_wireVerts[RM_WIRE_MAX_VERTS];
static int rm_wireVertCount = 0;

/* Solid rendering resources */
static rmShader_t *rm_solidShader = NULL;

/* 
 * STATIC WORLD GEOMETRY 
 * These buffers hold the pre-calculated world geometry (walls, floors, etc.)
 */
static GLuint rm_staticWorldVAO = 0;
static GLuint rm_staticWorldVBO = 0;
static int rm_staticWorldVertCount = 0;

/*
 * DYNAMIC GEOMETRY (Brush models, etc.)
 * These buffers are reused frame-by-frame for moving/dynamic BSP models
 */
static GLuint rm_dynamicVAO = 0;
static GLuint rm_dynamicVBO = 0;

/* PVS CULLING RESOURCES */
static GLuint rm_dynamicIBO = 0; /* Dynamic Index Buffer for visible surfaces */
static int rm_visFrameCount = 0; /* Frame counter for visibility marking */

/* 
 * DRAW SURFACE REGISTRY
 * Maps a BSP surface to its location in the Static VBO.
 * Sorted by Texture to allow batching.
 */
typedef struct {
	mBspSurface_t *surf;
	GLuint texNum;
	int startVert; /* Offset in Static VBO */
	int numVerts;
	int numIndices;
} rm_drawSurf_t;

#define RM_MAX_DRAW_SURFS 65536
static rm_drawSurf_t rm_drawSurfs[RM_MAX_DRAW_SURFS];
static int rm_numDrawSurfs = 0;

/* Temp index buffer for building batches (large enough for all verts) */
#define RM_MAX_INDICES 1048576 
static GLuint rm_tempIndices[RM_MAX_INDICES];

static GLint rm_solidLoc_Projection = -1;
static GLint rm_solidLoc_View = -1;
static GLint rm_solidLoc_Model = -1;
static GLint rm_solidLoc_LightDir = -1;
static GLint rm_solidLoc_Texture = -1;
static GLint rm_solidLoc_Lightmap = -1;    /* Lightmap sampler (unit 1, stubbed) */
static GLint rm_solidLoc_UseLightmap = -1; /* Toggle: 0=fake directional, 1=real lightmap */
static GLint rm_solidLoc_NormalMatrix = -1;
static GLint rm_solidLoc_UseNormalMatrix = -1;

/* Dynamic light uniforms */
#define RM_MAX_DLIGHTS 16
static GLint rm_solidLoc_DLightPos = -1;    /* vec4 array: xyz=pos, w=intensity */
static GLint rm_solidLoc_DLightColor = -1;  /* vec4 array: rgb=color */
static GLint rm_solidLoc_NumDLights = -1;   /* int: active light count */

/* Fallback texture (1x1 white) used when a surface has no valid texNum */
static GLuint rm_whiteTex = 0;

/* Track whether world rendering resources have been initialized */
static qBool rm_worldInitialized = qFalse;

/* Identity matrix for BSP (no model transform) */
static float rm_identityMatrix[16] = {
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};


/* Texture batching for efficient rendering */
typedef struct {
	GLuint texNum;        /* OpenGL texture handle */
	int startVert;        /* First vertex in array */
	int numVerts;         /* Number of vertices for this texture batch */
} texBatch_t;

#define RM_MAX_TEX_BATCHES 16384 /* Increased for static world + dynamic brushes */

/* Static world batches - NO LONGER USED, we use PVS sorting now */
/* static texBatch_t rm_staticBatches[RM_MAX_TEX_BATCHES]; */
/* static int rm_numStaticBatches = 0; */
/* static int rm_numStaticBatches = 0; */

/* Dynamic batches (reused per frame/entity) */
static texBatch_t rm_dynamicBatches[RM_MAX_TEX_BATCHES];
static int rm_numDynamicBatches = 0;

/* Current texture accumulation variables (used during building) */
static GLuint rm_currentTexNum = 0;
static int rm_currentBatchStart = 0;

/* Vertex with position, normal, and UV for textured rendering */
typedef struct {
	float pos[3];
	float normal[3];
	float uv[2];
} solidVertex_t;

/* 
 * Dynamic buffer for solid triangles - store triangle verts with normals 
 * Used for building static geometry ONCE, and dynamic geometry EVERY FRAME 
 */
#define RM_SOLID_MAX_VERTS 2097152 /* Increased to 2M to hold large maps safely */
static solidVertex_t rm_tempVerts[RM_SOLID_MAX_VERTS]; /* Temporary scratchpad */
static int rm_tempVertCount = 0;

/* Render mode: 0 = solid, 1 = wireframe */
static int rm_renderMode = 0;  /* 0=solid with depth test, 1=wireframe */

/*
==================
RM_CalcSurfaceUV

Calculate texture coordinates for a vertex position using BSP texture vectors.
Formula: u = dot(pos, vecs[0].xyz) + vecs[0].w
         v = dot(pos, vecs[1].xyz) + vecs[1].w
==================
*/
static void RM_CalcSurfaceUV(vec3_t pos, mQ2BspTexInfo_t *texInfo, image_t *texImage, float *u, float *v)
{
	if (!texInfo) {
		*u = 0.0f;
		*v = 0.0f;
		return;
	}
	
	/* Calculate raw texture coordinates using dot product with texture vectors */
	float rawU = pos[0] * texInfo->vecs[0][0] + 
	             pos[1] * texInfo->vecs[0][1] + 
	             pos[2] * texInfo->vecs[0][2] + 
	             texInfo->vecs[0][3];
	
	float rawV = pos[0] * texInfo->vecs[1][0] + 
	             pos[1] * texInfo->vecs[1][1] + 
	             pos[2] * texInfo->vecs[1][2] + 
	             texInfo->vecs[1][3];
	
	/* Normalize by actual texture dimensions from loaded image */
	int width, height;
	if (texImage && texImage->width > 0 && texImage->height > 0) {
		width = texImage->width;
		height = texImage->height;
	} else if (texInfo->width > 0 && texInfo->height > 0) {
		width = texInfo->width;
		height = texInfo->height;
	} else {
		width = 64;
		height = 64;
	}
	
	*u = rawU / (float)width;
	*v = rawV / (float)height;
}

/*
==================
RM_AccumulateSurface

Extract triangles from a BSP surface and add to the temporary vertex buffer.
Part of the batching system: simply adds verts. 
Batch tracking is done by the Caller.
==================
*/
static void RM_AccumulateSurface(mBspSurface_t *surf, GLuint texNum, image_t *texImage)
{
	mesh_t *mesh;
	int i;
	vec3_t normal;
	mQ2BspTexInfo_t *texInfo;
	float u, v;

	if (!surf || !surf->mesh)
		return;

	mesh = surf->mesh;
	texInfo = surf->q2_texInfo;

	if (!mesh->vertexArray || mesh->numVerts < 3)
		return;

	/* Get surface normal from plane */
	if (surf->q2_plane) {
		Vec3Copy(surf->q2_plane->normal, normal);
		/* Negate normal if surface is on back side of plane */
		if (surf->q2_flags & SURF_PLANEBACK) {
			normal[0] = -normal[0];
			normal[1] = -normal[1];
			normal[2] = -normal[2];
		}
	} else {
		/* Fallback: up vector */
		normal[0] = 0.0f;
		normal[1] = 0.0f;
		normal[2] = 1.0f;
	}

	/* Add triangles using vec3_t array access */
	if (mesh->indexArray && mesh->numIndexes >= 3) {
		/* Use existing index array to emit triangles */
		for (i = 0; i < mesh->numIndexes; i += 3) {
			if (rm_tempVertCount + 3 > RM_SOLID_MAX_VERTS)
				return;

			int i0 = mesh->indexArray[i];
			int i1 = mesh->indexArray[i + 1];
			int i2 = mesh->indexArray[i + 2];

			if (i0 < mesh->numVerts && i1 < mesh->numVerts && i2 < mesh->numVerts) {
				Vec3Copy(mesh->vertexArray[i0], rm_tempVerts[rm_tempVertCount].pos);
				Vec3Copy(normal, rm_tempVerts[rm_tempVertCount].normal);
				RM_CalcSurfaceUV(mesh->vertexArray[i0], texInfo, texImage, &u, &v);
				rm_tempVerts[rm_tempVertCount].uv[0] = u;
				rm_tempVerts[rm_tempVertCount].uv[1] = v;
				rm_tempVertCount++;

				Vec3Copy(mesh->vertexArray[i1], rm_tempVerts[rm_tempVertCount].pos);
				Vec3Copy(normal, rm_tempVerts[rm_tempVertCount].normal);
				RM_CalcSurfaceUV(mesh->vertexArray[i1], texInfo, texImage, &u, &v);
				rm_tempVerts[rm_tempVertCount].uv[0] = u;
				rm_tempVerts[rm_tempVertCount].uv[1] = v;
				rm_tempVertCount++;

				Vec3Copy(mesh->vertexArray[i2], rm_tempVerts[rm_tempVertCount].pos);
				Vec3Copy(normal, rm_tempVerts[rm_tempVertCount].normal);
				RM_CalcSurfaceUV(mesh->vertexArray[i2], texInfo, texImage, &u, &v);
				rm_tempVerts[rm_tempVertCount].uv[0] = u;
				rm_tempVerts[rm_tempVertCount].uv[1] = v;
				rm_tempVertCount++;
			}
		}
	} else {
		/* Generate triangle fan for convex polygon */
		for (i = 1; i < mesh->numVerts - 1; i++) {
			if (rm_tempVertCount + 3 > RM_SOLID_MAX_VERTS)
				return;

			Vec3Copy(mesh->vertexArray[0], rm_tempVerts[rm_tempVertCount].pos);
			Vec3Copy(normal, rm_tempVerts[rm_tempVertCount].normal);
			RM_CalcSurfaceUV(mesh->vertexArray[0], texInfo, texImage, &u, &v);
			rm_tempVerts[rm_tempVertCount].uv[0] = u;
			rm_tempVerts[rm_tempVertCount].uv[1] = v;
			rm_tempVertCount++;

			Vec3Copy(mesh->vertexArray[i], rm_tempVerts[rm_tempVertCount].pos);
			Vec3Copy(normal, rm_tempVerts[rm_tempVertCount].normal);
			RM_CalcSurfaceUV(mesh->vertexArray[i], texInfo, texImage, &u, &v);
			rm_tempVerts[rm_tempVertCount].uv[0] = u;
			rm_tempVerts[rm_tempVertCount].uv[1] = v;
			rm_tempVertCount++;

			Vec3Copy(mesh->vertexArray[i+1], rm_tempVerts[rm_tempVertCount].pos);
			Vec3Copy(normal, rm_tempVerts[rm_tempVertCount].normal);
			RM_CalcSurfaceUV(mesh->vertexArray[i+1], texInfo, texImage, &u, &v);
			rm_tempVerts[rm_tempVertCount].uv[0] = u;
			rm_tempVerts[rm_tempVertCount].uv[1] = v;
			rm_tempVertCount++;
		}
	}
}

/*
==================
RM_CopySurfaceVerts

Copy the surface vertex array once (no triangulation). Indices are generated at
draw time (triangle list from mesh->indexArray, or triangle-fan fallback).

This matches the q2pro-style world path: static world vertex buffer + per-frame
dynamic index batching by state/texture.
==================
*/
static void RM_CopySurfaceVerts(mBspSurface_t *surf, image_t *texImage)
{
	mesh_t *mesh;
	int i;
	vec3_t normal;
	mQ2BspTexInfo_t *texInfo;
	float u, v;

	if (!surf || !surf->mesh)
		return;

	mesh = surf->mesh;
	texInfo = surf->q2_texInfo;
	if (!texInfo)
		return;
	if (!texInfo)
		return;

	if (!mesh->vertexArray || mesh->numVerts < 3)
		return;

	if (rm_tempVertCount + mesh->numVerts > RM_SOLID_MAX_VERTS)
		return;

	/* Get surface normal from plane */
	if (surf->q2_plane) {
		Vec3Copy(surf->q2_plane->normal, normal);
		/* Negate normal if surface is on back side of plane */
		if (surf->q2_flags & SURF_PLANEBACK) {
			normal[0] = -normal[0];
			normal[1] = -normal[1];
			normal[2] = -normal[2];
		}
	} else {
		/* Fallback: up vector */
		normal[0] = 0.0f;
		normal[1] = 0.0f;
		normal[2] = 1.0f;
	}

	for (i = 0; i < mesh->numVerts; i++) {
		Vec3Copy(mesh->vertexArray[i], rm_tempVerts[rm_tempVertCount].pos);
		Vec3Copy(normal, rm_tempVerts[rm_tempVertCount].normal);
		RM_CalcSurfaceUV(mesh->vertexArray[i], texInfo, texImage, &u, &v);
		rm_tempVerts[rm_tempVertCount].uv[0] = u;
		rm_tempVerts[rm_tempVertCount].uv[1] = v;
		rm_tempVertCount++;
	}
}

/*
==================
CompareDrawSurfs
==================
*/
static int CompareDrawSurfs(const void *a, const void *b) {
	rm_drawSurf_t *sa = (rm_drawSurf_t *)a;
	rm_drawSurf_t *sb = (rm_drawSurf_t *)b;
	/* Sort by texture */
	if (sa->texNum > sb->texNum) return 1;
	if (sa->texNum < sb->texNum) return -1;
	return 0;
}

/*
==================
RM_BuildWorldBuffers

Iterate all world surfaces, sort by texture, and build
the STATIC world VBO and the Registry.
==================
*/
static void RM_BuildWorldBuffers(void)
{
	refModel_t *world = ri.scn.worldModel;
	int i;
	GLuint texNum;
	image_t *texImage;
	mQ2BspTexInfo_t *texInfo;

	/* Reset scratchpad */
	rm_numDrawSurfs = 0;
	rm_tempVertCount = 0;
	/* rm_numStaticBatches = 0; */
	rm_currentTexNum = 0;
	rm_currentBatchStart = 0;

	if (!world || !world->touchFrame || world->type != MODEL_Q2BSP || !world->bspModel.surfaces)
		return;

	/* Iterate only World surfaces (inline model 0) */
	/* Surfaces 0 to numSurfaces - actually we should filter surfaces that
	 * belong to brush models if they are separate. In Q2, surfaces are 
	 * all in one big pool. Brush models reference subset ranges.
	 * World references all? No.
	 * 
	 * We iterate ALL surfaces. But we should check if they are SKY.
	 */

	int numSurfaces = world->bspModel.numModelSurfaces;
	mBspSurface_t *surfaces = world->bspModel.firstModelSurface;

	/* 1. Collection Phase: Gather valid surfaces */
	for (i = 0; i < numSurfaces; i++) {
		mBspSurface_t *surf = &surfaces[i];
		texInfo = surf->q2_texInfo;
		if (texInfo && (texInfo->flags & SURF_TEXINFO_SKY)) continue;

		texNum = 0; 
		if (texInfo && texInfo->texName[0]) {
			texImage = R_RegisterImage(texInfo->texName, 0);
			if (texImage) texNum = texImage->texNum;
		}

		if (rm_numDrawSurfs < RM_MAX_DRAW_SURFS) {
			rm_drawSurfs[rm_numDrawSurfs].surf = surf;
			rm_drawSurfs[rm_numDrawSurfs].texNum = texNum;
			rm_drawSurfs[rm_numDrawSurfs].startVert = 0; /* filled later */
			rm_drawSurfs[rm_numDrawSurfs].numVerts = 0;
			rm_drawSurfs[rm_numDrawSurfs].numIndices = 0;
			rm_numDrawSurfs++;
		}
	}

	/* 2. Sorting Phase: Minimize state changes by sorting by texture */
	qsort(rm_drawSurfs, rm_numDrawSurfs, sizeof(rm_drawSurf_t), CompareDrawSurfs);

	/* 3. Build Verts: Accumulate in temp buffer and record offsets */
	for (i = 0; i < rm_numDrawSurfs; i++) {
		rm_drawSurf_t *ds = &rm_drawSurfs[i];
		mBspSurface_t *surf = ds->surf;
		image_t *img = NULL;
		const mesh_t *mesh = NULL;

		if (surf && surf->q2_texInfo && surf->q2_texInfo->texName[0])
			img = R_RegisterImage(surf->q2_texInfo->texName, 0);

		ds->startVert = rm_tempVertCount;
		RM_CopySurfaceVerts(surf, img);
		ds->numVerts = rm_tempVertCount - ds->startVert;

		ds->numIndices = 0;
		mesh = (surf) ? surf->mesh : NULL;
		if (mesh && ds->numVerts > 0) {
			if (mesh->indexArray && mesh->numIndexes >= 3)
				ds->numIndices = mesh->numIndexes;
			else if (mesh->numVerts >= 3)
				ds->numIndices = (mesh->numVerts - 2) * 3;
		}
	}

	/* 
	 * UPLOAD TO STATIC VBO 
	 */
	rm_staticWorldVertCount = rm_tempVertCount;

	if (rm_staticWorldVAO == 0) glGenVertexArrays(1, &rm_staticWorldVAO);
	if (rm_staticWorldVBO == 0) glGenBuffers(1, &rm_staticWorldVBO);
	if (rm_dynamicIBO == 0) glGenBuffers(1, &rm_dynamicIBO);

	glBindVertexArray(rm_staticWorldVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rm_staticWorldVBO);
	
	/* Upload data only once */
	glBufferData(GL_ARRAY_BUFFER, rm_staticWorldVertCount * sizeof(solidVertex_t), rm_tempVerts, GL_STATIC_DRAW);

	/* Set Attributes */
	/* 0: Pos */
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(solidVertex_t), (void*)0);
	/* 1: Normal */
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(solidVertex_t), (void*)(3 * sizeof(float)));
	/* 2: UV */
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(solidVertex_t), (void*)(6 * sizeof(float)));

	/* Bind Dynamic IBO to VAO so we can just update it later */
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm_dynamicIBO);
	/* Initialize with modest size */
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, RM_MAX_INDICES * sizeof(GLuint), NULL, GL_STREAM_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	Com_Printf(0, "RM_BuildWorldBuffers: Sorted & Batched. %d verts, %d drawSurfs.\n", 
		rm_staticWorldVertCount, rm_numDrawSurfs);
}

/*
==================
RM_World_Init

Initialize rendering resources for both wireframe and solid modes.
==================
*/
void RM_World_Init(void)
{
	FILE *f = fopen("modern.log", "a");
	if (f) {
		fprintf(f, "RM_World_Init: Starting Init\n");
		fclose(f);
	}

	/* Load wireframe shader */
	rm_wireShader = RM_LoadShader("rm_wireframe");
	if (rm_wireShader) {
		rm_wireLoc_Projection = glGetUniformLocation(rm_wireShader->program, "u_Projection");
		rm_wireLoc_View = glGetUniformLocation(rm_wireShader->program, "u_View");
		rm_wireLoc_Color = glGetUniformLocation(rm_wireShader->program, "u_Color");

		glGenVertexArrays(1, &rm_wireVAO);
		glBindVertexArray(rm_wireVAO);
		glGenBuffers(1, &rm_wireVBO);
		glBindBuffer(GL_ARRAY_BUFFER, rm_wireVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(rm_wireVerts), NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), (void*)0);
		glBindVertexArray(0);
	}

	/* Load solid shader */
	rm_solidShader = RM_LoadShader("rm_solid");
	if (rm_solidShader) {
		rm_solidLoc_Projection = glGetUniformLocation(rm_solidShader->program, "u_Projection");
		rm_solidLoc_View = glGetUniformLocation(rm_solidShader->program, "u_View");
		rm_solidLoc_Model = glGetUniformLocation(rm_solidShader->program, "u_Model");
		rm_solidLoc_LightDir = glGetUniformLocation(rm_solidShader->program, "u_LightDir");
		rm_solidLoc_Texture = glGetUniformLocation(rm_solidShader->program, "u_Texture");
		rm_solidLoc_Lightmap = glGetUniformLocation(rm_solidShader->program, "u_Lightmap");
		rm_solidLoc_UseLightmap = glGetUniformLocation(rm_solidShader->program, "u_UseLightmap");
		rm_solidLoc_NormalMatrix = glGetUniformLocation(rm_solidShader->program, "u_NormalMatrix");
		rm_solidLoc_UseNormalMatrix = glGetUniformLocation(rm_solidShader->program, "u_UseNormalMatrix");
		
		rm_solidLoc_DLightPos = glGetUniformLocation(rm_solidShader->program, "u_DLightPos");
		rm_solidLoc_DLightColor = glGetUniformLocation(rm_solidShader->program, "u_DLightColor");
		rm_solidLoc_NumDLights = glGetUniformLocation(rm_solidShader->program, "u_NumDLights");

		/* Init Dynamic VBO for brush models */
		glGenVertexArrays(1, &rm_dynamicVAO);
		glBindVertexArray(rm_dynamicVAO);
		glGenBuffers(1, &rm_dynamicVBO);
		glBindBuffer(GL_ARRAY_BUFFER, rm_dynamicVBO);
		glBufferData(GL_ARRAY_BUFFER, RM_SOLID_MAX_VERTS * sizeof(solidVertex_t), NULL, GL_DYNAMIC_DRAW); // Pre-allocate size
		
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(solidVertex_t), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(solidVertex_t), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(solidVertex_t), (void*)(6 * sizeof(float)));

		glBindVertexArray(0);
	}

	/* Create fallback white texture */
	if (!rm_whiteTex) {
		const unsigned char white[4] = { 255, 255, 255, 255 };
		glGenTextures(1, &rm_whiteTex);
		glBindTexture(GL_TEXTURE_2D, rm_whiteTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	/* Build the static world buffers immediately if map is loaded */
	if (ri.scn.worldModel) {
		RM_BuildWorldBuffers();
	}
	
	/* Initialize other modules */
	RM_Alias_Init();
	RM_Sky_Init();
	RM_Particle_Init();
	RM_Decal_Init();
}

/*
==================
RM_World_Shutdown
==================
*/
void RM_World_Shutdown(void)
{
	/* Destroy Wireframe */
	if (rm_wireVAO) { glDeleteVertexArrays(1, &rm_wireVAO); rm_wireVAO = 0; }
	if (rm_wireVBO) { glDeleteBuffers(1, &rm_wireVBO); rm_wireVBO = 0; }
	rm_wireShader = NULL;

	/* Destroy Static World */
	if (rm_staticWorldVAO) { glDeleteVertexArrays(1, &rm_staticWorldVAO); rm_staticWorldVAO = 0; }
	if (rm_staticWorldVBO) { glDeleteBuffers(1, &rm_staticWorldVBO); rm_staticWorldVBO = 0; }
	if (rm_dynamicIBO) { glDeleteBuffers(1, &rm_dynamicIBO); rm_dynamicIBO = 0; }
	
	/* Destroy Dynamic */
	if (rm_dynamicVAO) { glDeleteVertexArrays(1, &rm_dynamicVAO); rm_dynamicVAO = 0; }
	if (rm_dynamicVBO) { glDeleteBuffers(1, &rm_dynamicVBO); rm_dynamicVBO = 0; }
	
	rm_solidShader = NULL;

	if (rm_whiteTex) { glDeleteTextures(1, &rm_whiteTex); rm_whiteTex = 0; }
	
	RM_Alias_Shutdown();
	RM_Sprite_Shutdown();
	RM_Sky_Shutdown();
	RM_Particle_Shutdown();
	RM_Decal_Shutdown();
	
	rm_worldInitialized = qFalse;
}

/*
==================
RM_BuildProjectionMatrix
==================
*/
static void RM_BuildProjectionMatrix(float fovY, float aspect, float zNear, float zFar, float *m)
{
	float f = 1.0f / tanf(fovY * 0.5f * (3.14159265f / 180.0f));
	memset(m, 0, 16 * sizeof(float));
	m[0] = f / aspect;
	m[5] = f;
	m[10] = (zFar + zNear) / (zNear - zFar);
	m[11] = -1.0f;
	m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
}

/*
==================
RM_BuildViewMatrix
==================
*/
static void RM_BuildViewMatrix(vec3_t origin, vec3_t angles, float *m)
{
	float pitch = angles[0] * (3.14159265f / 180.0f);
	float yaw   = angles[1] * (3.14159265f / 180.0f);
	float roll  = angles[2] * (3.14159265f / 180.0f);
	
	float cp = cosf(pitch), sp = sinf(pitch);
	float cy = cosf(yaw),   sy = sinf(yaw);
	float cr = cosf(roll),  sr = sinf(roll);

	vec3_t forward, right, up;
	forward[0] = cp * cy;
	forward[1] = cp * sy;
	forward[2] = -sp;
	right[0] = -sr * sp * cy + cr * sy;
	right[1] = -sr * sp * sy - cr * cy;
	right[2] = -sr * cp;
	up[0] = cr * sp * cy + sr * sy;
	up[1] = cr * sp * sy - sr * cy;
	up[2] = cr * cp;
	
	memset(m, 0, 16 * sizeof(float));
	m[0]  = right[0];   m[4]  = right[1];   m[8]  = right[2];
	m[1]  = up[0];      m[5]  = up[1];      m[9]  = up[2];
	m[2]  = -forward[0]; m[6] = -forward[1]; m[10] = -forward[2];
	m[15] = 1.0f;
	m[12] = -(right[0] * origin[0] + right[1] * origin[1] + right[2] * origin[2]);
	m[13] = -(up[0] * origin[0] + up[1] * origin[1] + up[2] * origin[2]);
	m[14] = -(-forward[0] * origin[0] - forward[1] * origin[1] - forward[2] * origin[2]);
}

/*
==================
RM_AddWireframeLine
==================
*/
static void RM_AddWireframeLine(vec3_t v1, vec3_t v2)
{
	if (rm_wireVertCount + 2 > RM_WIRE_MAX_VERTS) return;
	Vec3Copy(v1, rm_wireVerts[rm_wireVertCount]); rm_wireVertCount++;
	Vec3Copy(v2, rm_wireVerts[rm_wireVertCount]); rm_wireVertCount++;
}

/*
==================
RM_DrawWireframeSurface
==================
*/
static void RM_DrawWireframeSurface(mBspSurface_t *surf)
{
	mesh_t *mesh;
	int i;
	if (!surf || !surf->mesh) return;
	mesh = surf->mesh;
	if (!mesh->vertexArray || mesh->numVerts < 3) return;
	
	if (mesh->indexArray && mesh->numIndexes >= 3) {
		for (i = 0; i < mesh->numIndexes; i += 3) {
			int i0 = mesh->indexArray[i];
			int i1 = mesh->indexArray[i + 1];
			int i2 = mesh->indexArray[i + 2];
			if (i0 < mesh->numVerts && i1 < mesh->numVerts && i2 < mesh->numVerts) {
				RM_AddWireframeLine(mesh->vertexArray[i0], mesh->vertexArray[i1]);
				RM_AddWireframeLine(mesh->vertexArray[i1], mesh->vertexArray[i2]);
				RM_AddWireframeLine(mesh->vertexArray[i2], mesh->vertexArray[i0]);
			}
		}
	} else {
		for (i = 1; i < mesh->numVerts - 1; i++) {
			RM_AddWireframeLine(mesh->vertexArray[0], mesh->vertexArray[i]);
			RM_AddWireframeLine(mesh->vertexArray[i], mesh->vertexArray[i + 1]);
			RM_AddWireframeLine(mesh->vertexArray[i + 1], mesh->vertexArray[0]);
		}
	}
}

/*
==================
RM_MarkVisibleSurfaces

Standard PVS Traversal to mark visible surfaces
==================
*/
static void RM_MarkVisibleSurfaces(refDef_t *fd)
{
	refModel_t *world = ri.scn.worldModel;
	if (!world || !world->touchFrame || !world->bspModel.nodes) return;

	rm_visFrameCount++;

	/* View Cluster */
	mBspLeaf_t *viewLeaf = R_PointInQ2BSPLeaf(fd->viewOrigin, world);
	int viewCluster = viewLeaf ? viewLeaf->cluster : -1;
	
	/* PVS bytes */
	byte *visBytes = R_Q2BSPClusterPVS(viewCluster, world);
	
	/* Iterate all leaves in world */
	int i;
	for (i = 0; i < world->bspModel.numLeafs; i++) {
		mBspLeaf_t *leaf = &world->bspModel.leafs[i];
		int cluster = leaf->cluster;

		if (cluster == -1) continue;
		
		if (visBytes && !(visBytes[cluster>>3] & (1<<(cluster&7))))
			continue; /* CULLED */

		if (leaf->q2_firstMarkSurface) {
			int s;
			for (s = 0; s < leaf->q2_numMarkSurfaces; s++) {
				mBspSurface_t *surf = leaf->q2_firstMarkSurface[s];
				if (surf) surf->visFrame = rm_visFrameCount;
			}
		}
	}
}

/*
==================
RM_DrawWorld
==================
*/
void RM_DrawWorld(refDef_t *fd)
{
	/* refModel_t *world; */
	float projMatrix[16];
	float viewMatrix[16];
	float aspect;
	int i;
	
	refDef_t *def = fd;
	
	/* 
	 * Safety Check: 
	 * Do not attempt to render or initialize if the engine is in the middle 
	 * of a registration sequence (e.g., loading a map). The world model 
	 * data may be invalid or in a transitional state.
	 */
	if (ri.reg.inSequence) {
		return;
	}

	/* Initialize on first call */
	if (!rm_worldInitialized) {
		RM_World_Init();
		rm_worldInitialized = qTrue;
	}
	
	if (!ri.scn.worldModel || ri.scn.worldModel->type == MODEL_BAD || !ri.scn.worldModel->bspModel.numSurfaces) {
		glClearColor(0.1f, 0.1f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		return;
	}
	/* world = ri.scn.worldModel; */

	/* Setup Framebuffer State */
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_BLEND);
	#ifdef GL_ALPHA_TEST
	glDisable(GL_ALPHA_TEST);
	#endif
	glDepthMask(GL_TRUE);
	glClearDepth(1.0);
	glDepthRange(0.0, 1.0);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/* Build Matrices */
	aspect = (float)def->width / (float)def->height;
	if (aspect < 0.1f) aspect = 4.0f / 3.0f;
	float fovY = def->fovY < 1.0f ? 90.0f : def->fovY;
	
	vec3_t camOrigin, camAngles;
	Vec3Copy(def->viewOrigin, camOrigin);
	Vec3Copy(def->viewAngles, camAngles);
	
	/* 1. VISIBILITY MARKING (PVS) */
	RM_MarkVisibleSurfaces(def);
	
	RM_BuildProjectionMatrix(fovY, aspect, 4.0f, 8192.0f, projMatrix);
	RM_BuildViewMatrix(camOrigin, camAngles, viewMatrix);
	
	glViewport(0, 0, def->width > 0 ? def->width : 800, def->height > 0 ? def->height : 600);
	
	/* Draw Sky - Depth 1.0 forces it to background */
	if (RM_Sky_IsLoaded()) {
		glEnable(GL_DEPTH_TEST);
		RM_Sky_Draw(projMatrix, viewMatrix);
	}
	
	/* 
	 * STATIC WORLD RENDERING 
	 */
	if (rm_renderMode == 0 && rm_solidShader && rm_staticWorldVAO) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		
		glUseProgram(rm_solidShader->program);
		glUniformMatrix4fv(rm_solidLoc_Projection, 1, GL_FALSE, projMatrix);
		glUniformMatrix4fv(rm_solidLoc_View, 1, GL_FALSE, viewMatrix);
		glUniformMatrix4fv(rm_solidLoc_Model, 1, GL_FALSE, rm_identityMatrix);
		if (rm_solidLoc_NormalMatrix >= 0) glUniformMatrix4fv(rm_solidLoc_NormalMatrix, 1, GL_FALSE, rm_identityMatrix);
		if (rm_solidLoc_UseNormalMatrix >= 0) glUniform1f(rm_solidLoc_UseNormalMatrix, 1.0f);
		glUniform3f(rm_solidLoc_LightDir, 0.424f, 0.848f, 0.254f);
		
		/* Dynamic Lights Buffer */
		{
			int numDLights = ri.scn.numDLights;
			if (numDLights > RM_MAX_DLIGHTS) numDLights = RM_MAX_DLIGHTS;
			
			if (numDLights > 0 && rm_solidLoc_NumDLights >= 0) {
				float dLightPos[RM_MAX_DLIGHTS * 4];
				float dLightColor[RM_MAX_DLIGHTS * 4];
				int dl;
				for (dl = 0; dl < numDLights; dl++) {
					refDLight_t *light = &ri.scn.dLightList[dl];
					dLightPos[dl * 4 + 0] = light->origin[0]; dLightPos[dl * 4 + 1] = light->origin[1];
					dLightPos[dl * 4 + 2] = light->origin[2]; dLightPos[dl * 4 + 3] = light->intensity;
					dLightColor[dl * 4 + 0] = light->color[0]; dLightColor[dl * 4 + 1] = light->color[1];
					dLightColor[dl * 4 + 2] = light->color[2]; dLightColor[dl * 4 + 3] = 1.0f;
				}
				glUniform4fv(rm_solidLoc_DLightPos, numDLights, dLightPos);
				glUniform4fv(rm_solidLoc_DLightColor, numDLights, dLightColor);
				glUniform1i(rm_solidLoc_NumDLights, numDLights);
			} else if (rm_solidLoc_NumDLights >= 0) {
				glUniform1i(rm_solidLoc_NumDLights, 0);
			}
		}

		/* Draw Static World using PVS Indexes */
		glUniform1i(rm_solidLoc_Texture, 0);
		glUniform1i(rm_solidLoc_Lightmap, 1);
		glUniform1f(rm_solidLoc_UseLightmap, 0.0f);
		
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, rm_whiteTex);
		
		glBindVertexArray(rm_staticWorldVAO);
		/* Elements from the dynamic IBO */
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rm_dynamicIBO);
		
		glActiveTexture(GL_TEXTURE0);

		/* 
		 * BATCHING LOOP 
		 * Iterate over sorted surfaces. Collect visible ones. Draw when texture changes.
		 */
		int batchCount = 0;
		GLuint currentTex = 0;
		int indexCount = 0;
		
		if (rm_numDrawSurfs > 0) currentTex = rm_drawSurfs[0].texNum;
		
		for (i = 0; i < rm_numDrawSurfs; i++) {
			const rm_drawSurf_t *ds = &rm_drawSurfs[i];

			/* Check texture change or buffer absolute limit */
			if (ds->texNum != currentTex || (indexCount + ds->numIndices > RM_MAX_INDICES)) {
				/* Flush current batch */
				if (indexCount > 0) {
					/* q2pro-style streaming: allocate+upload (driver may orphan internally) */
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), rm_tempIndices, GL_STREAM_DRAW);

					/* Bind texture */
					glBindTexture(GL_TEXTURE_2D, currentTex ? currentTex : rm_whiteTex);

					/* Draw */
					glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)0);
					batchCount++;
				}

				/* Reset */
				indexCount = 0;
				currentTex = ds->texNum;
			}

			if (ds->numIndices <= 0 || ds->numIndices > RM_MAX_INDICES)
				continue;

			/* Visibility Check */
			if (!ds->surf || ds->surf->visFrame != rm_visFrameCount)
				continue;

			if (indexCount + ds->numIndices > RM_MAX_INDICES)
				continue;

			/* Add indices for this surface */
			const mesh_t *mesh = ds->surf->mesh;
			if (!mesh || mesh->numVerts < 3)
				continue;

			if (mesh->indexArray && mesh->numIndexes >= 3) {
				int j;
				for (j = 0; j < mesh->numIndexes; j++) {
					int idx = (int)mesh->indexArray[j];
					if (idx < 0 || idx >= mesh->numVerts)
						idx = 0;

					rm_tempIndices[indexCount++] = (GLuint)(ds->startVert + idx);
				}
			} else {
				int v;
				for (v = 1; v < mesh->numVerts - 1; v++) {
					rm_tempIndices[indexCount++] = (GLuint)ds->startVert;
					rm_tempIndices[indexCount++] = (GLuint)(ds->startVert + v);
					rm_tempIndices[indexCount++] = (GLuint)(ds->startVert + v + 1);
				}
			}
		}

		/* Flush final batch */
		if (indexCount > 0) {
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLuint), rm_tempIndices, GL_STREAM_DRAW);
			glBindTexture(GL_TEXTURE_2D, currentTex ? currentTex : rm_whiteTex);
			glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (void*)0);
			batchCount++;
		}
		
		glBindVertexArray(0); /* Unbind static world */
		
		/*
		 * DYNAMIC BRUSH MODELS (Doors, Platforms)
		 * Usage of Dynamic VBO
		 */
		{
			refEntity_t *ent;
			int entIdx;
			float modelMatrix[16];
			
			/* Setup Dynamic VBO State */
			glBindVertexArray(rm_dynamicVAO);
			glBindBuffer(GL_ARRAY_BUFFER, rm_dynamicVBO);
			
			for (entIdx = 2; entIdx < ri.scn.numEntities; entIdx++) {
				ent = &ri.scn.entityList[entIdx];
				if (!ent->model || ent->model->type != MODEL_Q2BSP) continue;
				if (!ent->model->bspModel.firstModelSurface || ent->model->bspModel.numModelSurfaces <= 0) continue;
				
				/* Build Model Matrix */
				float scale = ent->scale > 0.0f ? ent->scale : 1.0f;
				memset(modelMatrix, 0, sizeof(modelMatrix));
				if (Matrix3_Compare(ent->axis, axisIdentity)) {
					modelMatrix[0] = scale; modelMatrix[5] = scale; modelMatrix[10] = scale;
				} else {
					modelMatrix[0] = ent->axis[0][0]*scale; modelMatrix[4] = ent->axis[1][0]*scale; modelMatrix[8] = ent->axis[2][0]*scale;
					modelMatrix[1] = ent->axis[0][1]*scale; modelMatrix[5] = ent->axis[1][1]*scale; modelMatrix[9] = ent->axis[2][1]*scale;
					modelMatrix[2] = ent->axis[0][2]*scale; modelMatrix[6] = ent->axis[1][2]*scale; modelMatrix[10] = ent->axis[2][2]*scale;
				}
				modelMatrix[12] = ent->origin[0]; modelMatrix[13] = ent->origin[1]; modelMatrix[14] = ent->origin[2]; modelMatrix[15] = 1.0f;
				
				/* Collect Verts for THIS entity */
				rm_tempVertCount = 0;
				rm_numDynamicBatches = 0;
				rm_currentTexNum = 0;
				rm_currentBatchStart = 0;
				
				mBspSurface_t *surf = ent->model->bspModel.firstModelSurface;
				int surfIdx;
				for (surfIdx = 0; surfIdx < ent->model->bspModel.numModelSurfaces; surfIdx++, surf++) {
					mQ2BspTexInfo_t *ti = surf->q2_texInfo;
					GLuint tNum = 0;
					image_t *img = NULL;
					
					if (ti && ti->texName[0]) {
						img = R_RegisterImage(ti->texName, 0);
						if (img) tNum = img->texNum;
					}
					
					/* Batch */
					if (tNum != rm_currentTexNum) {
						if (rm_tempVertCount > rm_currentBatchStart) {
							rm_dynamicBatches[rm_numDynamicBatches].texNum = rm_currentTexNum;
							rm_dynamicBatches[rm_numDynamicBatches].startVert = rm_currentBatchStart;
							rm_dynamicBatches[rm_numDynamicBatches].numVerts = rm_tempVertCount - rm_currentBatchStart;
							rm_numDynamicBatches++;
						}
						rm_currentTexNum = tNum;
						rm_currentBatchStart = rm_tempVertCount;
					}
					RM_AccumulateSurface(surf, tNum, img);
				}
				/* Finalize Batch */
				if (rm_tempVertCount > rm_currentBatchStart) {
					rm_dynamicBatches[rm_numDynamicBatches].texNum = rm_currentTexNum;
					rm_dynamicBatches[rm_numDynamicBatches].startVert = rm_currentBatchStart;
					rm_dynamicBatches[rm_numDynamicBatches].numVerts = rm_tempVertCount - rm_currentBatchStart;
					rm_numDynamicBatches++;
				}
				
				/* Upload & Draw */
				if (rm_tempVertCount > 0) {
					glBufferSubData(GL_ARRAY_BUFFER, 0, rm_tempVertCount * sizeof(solidVertex_t), rm_tempVerts);
					
					glUniformMatrix4fv(rm_solidLoc_Model, 1, GL_FALSE, modelMatrix);
					if (rm_solidLoc_NormalMatrix >= 0) glUniformMatrix4fv(rm_solidLoc_NormalMatrix, 1, GL_FALSE, modelMatrix);
					
					for (i = 0; i < rm_numDynamicBatches; i++) {
						if (rm_dynamicBatches[i].texNum > 0) glBindTexture(GL_TEXTURE_2D, rm_dynamicBatches[i].texNum);
						else glBindTexture(GL_TEXTURE_2D, rm_whiteTex);
						glDrawArrays(GL_TRIANGLES, rm_dynamicBatches[i].startVert, rm_dynamicBatches[i].numVerts);
					}
				}
			}
			glBindVertexArray(0);
		}
		
		glUseProgram(0);
	}
	/* WIREFRAME */
	else if (rm_wireShader && rm_wireVAO && rm_staticWorldVertCount > 0) {
		/* Not implemented optimized wireframe yet - relying on debug */
	}

	RM_SetMatricesForEntities(projMatrix, viewMatrix);
	RM_DrawEntities();
	RM_DrawSprites();
	RM_DrawDecals(projMatrix, viewMatrix);
	RM_Particle_Draw(projMatrix, viewMatrix);
}
