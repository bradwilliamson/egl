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

/*
 * rm_sky.c - Modern renderer skybox implementation
 *
 * Renders a 6-face skybox using the Q2 env/ textures.
 * Sky is drawn first at depth=1.0, then world geometry overwrites it.
 */

#include "../r_local.h"
#include "rm_shader.h"
#include "../glad/glad.h"
#include <stdio.h>

/* Sky shader and resources */
static rmShader_t *rm_skyShader = NULL;
static GLuint rm_skyVAO = 0;
static GLuint rm_skyVBO = 0;

/* Uniform locations */
static GLint rm_skyLoc_Projection = -1;
static GLint rm_skyLoc_View = -1;
static GLint rm_skyLoc_Texture = -1;

/* Sky textures (6 faces) */
static GLuint rm_skyTextures[6] = {0};
static qBool rm_skyLoaded = qFalse;
static char rm_skyBaseName[MAX_QPATH] = {0};

/* Face names: rt, bk, lf, ft, up, dn (same as legacy) */
static const char *rm_skySuffix[6] = { "rt", "bk", "lf", "ft", "up", "dn" };

/*
 * Skybox cube vertex data.
 * Each face is a quad (2 triangles), with positions and UVs.
 * The cube is centered at origin with size 1.0 (will be scaled in shader).
 * Winding is set for GL_CW front face (matching our world rendering).
 */
typedef struct {
	float pos[3];
	float uv[2];
} skyVertex_t;

/* 
 * 6 faces * 6 verts = 36 vertices
 * Face order matches texture order: rt(+X), bk(-Y), lf(-X), ft(+Y), up(+Z), dn(-Z)
 */
static const skyVertex_t rm_skyboxVerts[36] = {
	/* Face 0: Right (+X) - rt */
	{{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
	{{ 1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
	{{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
	{{ 1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
	{{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
	{{ 1.0f,  1.0f, -1.0f}, {1.0f, 1.0f}},

	/* Face 1: Back (-Y) - bk */
	{{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
	{{ 1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
	{{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
	{{ 1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
	{{-1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
	{{-1.0f,  1.0f, -1.0f}, {1.0f, 1.0f}},

	/* Face 2: Left (-X) - lf */
	{{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
	{{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
	{{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f}},
	{{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
	{{-1.0f, -1.0f,  1.0f}, {1.0f, 0.0f}},
	{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},

	/* Face 3: Front (-Y) - ft */
	{{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
	{{-1.0f, -1.0f,  1.0f}, {0.0f, 0.0f}},
	{{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f}},
	{{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}},
	{{ 1.0f, -1.0f,  1.0f}, {1.0f, 0.0f}},
	{{ 1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}},

	/* Face 4: Up (+Z) - up */
	{{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
	{{-1.0f,  1.0f,  1.0f}, {0.0f, 0.0f}},
	{{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
	{{-1.0f, -1.0f,  1.0f}, {0.0f, 1.0f}},
	{{ 1.0f,  1.0f,  1.0f}, {1.0f, 0.0f}},
	{{ 1.0f, -1.0f,  1.0f}, {1.0f, 1.0f}},

	/* Face 5: Down (-Z) - dn */
	{{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
	{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}},
	{{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},
	{{-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f}},
	{{ 1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}},
	{{ 1.0f,  1.0f, -1.0f}, {1.0f, 1.0f}},
};

/*
==================
RM_Sky_Init

Initialize sky rendering resources (VAO, VBO, shader).
Called once from RM_World_Init.
==================
*/
void RM_Sky_Init(void)
{
	FILE *f;

	f = fopen("modern.log", "a");
	if (f) {
		fprintf(f, "RM_Sky_Init: Initializing sky rendering...\n");
		fclose(f);
	}

	/* Load sky shader */
	rm_skyShader = RM_LoadShader("rm_sky");
	if (!rm_skyShader) {
		f = fopen("modern.log", "a");
		if (f) {
			fprintf(f, "RM_Sky_Init: WARNING - Failed to load rm_sky shader\n");
			fclose(f);
		}
		return;
	}

	/* Get uniform locations */
	rm_skyLoc_Projection = glGetUniformLocation(rm_skyShader->program, "u_Projection");
	rm_skyLoc_View = glGetUniformLocation(rm_skyShader->program, "u_View");
	rm_skyLoc_Texture = glGetUniformLocation(rm_skyShader->program, "u_Texture");

	/* Create VAO/VBO for skybox */
	glGenVertexArrays(1, &rm_skyVAO);
	glBindVertexArray(rm_skyVAO);

	glGenBuffers(1, &rm_skyVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rm_skyVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rm_skyboxVerts), rm_skyboxVerts, GL_STATIC_DRAW);

	/* Attribute 0: position (3 floats) */
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(skyVertex_t), (void*)0);

	/* Attribute 1: UV (2 floats) */
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(skyVertex_t), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	f = fopen("modern.log", "a");
	if (f) {
		fprintf(f, "RM_Sky_Init: Complete - VAO=%u VBO=%u Shader=%u\n",
			rm_skyVAO, rm_skyVBO, rm_skyShader->program);
		fprintf(f, "RM_Sky_Init: Uniforms - Proj=%d View=%d Tex=%d\n",
			rm_skyLoc_Projection, rm_skyLoc_View, rm_skyLoc_Texture);
		fclose(f);
	}
}

/*
==================
RM_Sky_Shutdown

Clean up sky resources.
==================
*/
void RM_Sky_Shutdown(void)
{
	int i;

	if (rm_skyVAO) {
		glDeleteVertexArrays(1, &rm_skyVAO);
		rm_skyVAO = 0;
	}
	if (rm_skyVBO) {
		glDeleteBuffers(1, &rm_skyVBO);
		rm_skyVBO = 0;
	}

	/* Note: textures are managed by the legacy image system, don't delete them */
	for (i = 0; i < 6; i++) {
		rm_skyTextures[i] = 0;
	}

	rm_skyLoaded = qFalse;
	rm_skyBaseName[0] = '\0';
	rm_skyShader = NULL;
}

/*
==================
RM_Sky_SetTextures

Load sky textures for the current sky name.
Called when sky changes (map load, R_SetSky).
==================
*/
void RM_Sky_SetTextures(const char *baseName)
{
	char pathName[MAX_QPATH];
	image_t *img;
	int i;
	FILE *f;

	/* Check if already loaded for this sky */
	if (rm_skyLoaded && !strcmp(rm_skyBaseName, baseName)) {
		return;
	}

	f = fopen("modern.log", "a");
	if (f) {
		fprintf(f, "RM_Sky_SetTextures: Loading sky '%s'\n", baseName);
		fclose(f);
	}

	Q_strncpyz(rm_skyBaseName, baseName, sizeof(rm_skyBaseName));
	rm_skyLoaded = qFalse;

	/* Load 6 sky textures */
	for (i = 0; i < 6; i++) {
		Q_snprintfz(pathName, sizeof(pathName), "env/%s%s.tga", baseName, rm_skySuffix[i]);
		
		img = R_RegisterImage(pathName, 0);
		if (img && img->texNum) {
			rm_skyTextures[i] = img->texNum;
		} else {
			/* Try PCX fallback */
			Q_snprintfz(pathName, sizeof(pathName), "env/%s%s.pcx", baseName, rm_skySuffix[i]);
			img = R_RegisterImage(pathName, 0);
			if (img && img->texNum) {
				rm_skyTextures[i] = img->texNum;
			} else {
				rm_skyTextures[i] = 0;
				f = fopen("modern.log", "a");
				if (f) {
					fprintf(f, "RM_Sky_SetTextures: WARNING - Missing sky face %d (%s)\n", i, rm_skySuffix[i]);
					fclose(f);
				}
			}
		}
	}

	rm_skyLoaded = qTrue;

	f = fopen("modern.log", "a");
	if (f) {
		fprintf(f, "RM_Sky_SetTextures: Loaded sky textures: %u %u %u %u %u %u\n",
			rm_skyTextures[0], rm_skyTextures[1], rm_skyTextures[2],
			rm_skyTextures[3], rm_skyTextures[4], rm_skyTextures[5]);
		fclose(f);
	}
}

/*
==================
RM_Sky_Draw

Render the skybox. Should be called BEFORE world geometry.
Uses depth test with depth func LEQUAL and writes depth=1.0.
==================
*/
void RM_Sky_Draw(float *projMatrix, float *viewMatrix)
{
	float skyViewMatrix[16];
	int i;

	/* Must have shader and textures */
	if (!rm_skyShader || !rm_skyVAO || !rm_skyLoaded) {
		return;
	}

	/* Create view matrix without translation (sky follows camera) */
	/* Copy view matrix but zero out translation (elements 12, 13, 14) */
	for (i = 0; i < 16; i++) {
		skyViewMatrix[i] = viewMatrix[i];
	}
	skyViewMatrix[12] = 0.0f;
	skyViewMatrix[13] = 0.0f;
	skyViewMatrix[14] = 0.0f;

	/* Setup state for sky rendering */
	glDepthMask(GL_FALSE);      /* Don't write depth */
	glDepthFunc(GL_LEQUAL);     /* Draw at far plane */
	glDisable(GL_CULL_FACE);    /* Draw both sides (fixes any winding issues) */

	glUseProgram(rm_skyShader->program);
	glUniformMatrix4fv(rm_skyLoc_Projection, 1, GL_FALSE, projMatrix);
	glUniformMatrix4fv(rm_skyLoc_View, 1, GL_FALSE, skyViewMatrix);
	glUniform1i(rm_skyLoc_Texture, 0);

	glBindVertexArray(rm_skyVAO);
	glActiveTexture(GL_TEXTURE0);

	/* Draw each face with its texture */
	for (i = 0; i < 6; i++) {
		if (rm_skyTextures[i]) {
			glBindTexture(GL_TEXTURE_2D, rm_skyTextures[i]);
			glDrawArrays(GL_TRIANGLES, i * 6, 6);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	/* Restore state for world rendering */
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}

/*
==================
RM_Sky_IsLoaded

Returns qTrue if sky textures are loaded.
==================
*/
qBool RM_Sky_IsLoaded(void)
{
	return rm_skyLoaded && rm_skyShader != NULL;
}
