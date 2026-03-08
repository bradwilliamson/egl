/*
==============================================================================

	MODERN DECAL RENDERER - HEADER

	Renders decals (bullet holes, scorch marks, blood splats) projected onto
	world surfaces.

==============================================================================
*/

#ifndef __RM_DECAL_H__
#define __RM_DECAL_H__

void RM_Decal_Init(void);
void RM_Decal_Shutdown(void);
void RM_DrawDecals(const float *projMatrix, const float *viewMatrix);

#endif /* __RM_DECAL_H__ */
