#define RM_NO_LEGACY_GL
#include "../glad/glad.h"

#include "../r_local.h"
#include "rm_geom.h"
#include "rm_shader.h"

/*
==============================================================================

	MODERN GEOMETRY MODULE
	
	Provides test triangle and dynamic line rendering for wireframe BSP.
	Uses VAO/VBO with the rm_test shader (passthrough position + color).

==============================================================================
*/

static qBool rm_geomInitialized = qFalse;

/* Test triangle resources */
static GLuint rm_testVAO = 0;
static GLuint rm_testVBO = 0;
static rmShader_t *rm_testShader = NULL;

/* Vertex format: position (vec3) + color (vec4) = 7 floats */
typedef struct {
	float x, y, z;
	float r, g, b, a;
} rm_vertex_t;

/* Test triangle in NDC space (screen-centered) */
static rm_vertex_t rm_testTriangle[3] = {
	{ -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f },  /* Red */
	{  0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f },  /* Green */
	{  0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f },  /* Blue */
};

void RM_Geom_Init (void)
{
	FILE *f = fopen("modern.log", "a"); 
	if (f) { 
		fprintf(f, "RM_Geom_Init: Starting geometry system init\n"); 
		fclose(f); 
	}

	/* Load the test shader */
	rm_testShader = RM_LoadShader("rm_test");
	if (!rm_testShader) {
		f = fopen("modern.log", "a");
		if (f) {
			fprintf(f, "RM_Geom_Init: FAILED to load rm_test shader\n");
			fclose(f);
		}
		return;
	}

	/* Create VAO */
	glGenVertexArrays(1, &rm_testVAO);
	glBindVertexArray(rm_testVAO);

	/* Create VBO and upload triangle data */
	glGenBuffers(1, &rm_testVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rm_testVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rm_testTriangle), rm_testTriangle, GL_STATIC_DRAW);

	/* Setup vertex attributes:
	   layout(location = 0) in vec3 in_Position;
	   layout(location = 1) in vec4 in_Color;
	*/
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rm_vertex_t), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(rm_vertex_t), (void*)(3 * sizeof(float)));

	/* Unbind */
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	rm_geomInitialized = qTrue;

	f = fopen("modern.log", "a");
	if (f) {
		fprintf(f, "RM_Geom_Init: SUCCESS - VAO=%u VBO=%u Shader=%u\n", 
			rm_testVAO, rm_testVBO, rm_testShader->program);
		fclose(f);
	}
}

void RM_Geom_Shutdown (void)
{
	FILE *f = fopen("modern.log", "a"); 
	if (f) { 
		fprintf(f, "RM_Geom_Shutdown\n"); 
		fclose(f); 
	}

	if (rm_testVAO) {
		glDeleteVertexArrays(1, &rm_testVAO);
		rm_testVAO = 0;
	}
	if (rm_testVBO) {
		glDeleteBuffers(1, &rm_testVBO);
		rm_testVBO = 0;
	}

	rm_testShader = NULL; /* Shader freed by shader system */
	rm_geomInitialized = qFalse;
}

void RM_Geom_DrawTest (void)
{
	if (!rm_geomInitialized || !rm_testShader || !rm_testVAO) {
		return;
	}

	/* Use the test shader */
	glUseProgram(rm_testShader->program);

	/* Bind VAO and draw */
	glBindVertexArray(rm_testVAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);

	glUseProgram(0);
}