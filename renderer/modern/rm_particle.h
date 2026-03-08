/*
==============================================================================

	MODERN PARTICLE RENDERER

	Renders particles (quads) from refPoly_t submissions.
	
	Particles in EGL are submitted as quads (4 vertices) with texture coordinates
	and per-vertex colors. They use alpha blending and are rendered after world
	geometry and entities.

==============================================================================
*/

#ifndef RM_PARTICLE_H
#define RM_PARTICLE_H

#include "../r_local.h"

/*
 * Initialize particle rendering resources (shader, VAO, VBO).
 * Call once after OpenGL context is ready.
 */
void RM_Particle_Init(void);

/*
 * Shutdown and free particle rendering resources.
 */
void RM_Particle_Shutdown(void);

/*
 * Draw all particles in ri.scn.polyList.
 * Call after world and entity rendering, with blending enabled.
 *
 * projMatrix - 4x4 projection matrix
 * viewMatrix - 4x4 view matrix
 */
void RM_Particle_Draw(float *projMatrix, float *viewMatrix);

#endif /* RM_PARTICLE_H */
