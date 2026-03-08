/*
==============================================================================

	MODERN RENDERER - ALIAS MODEL (MD2) AND SPRITE (SP2) SUPPORT
	
	Phase 12: Entity rendering for items, monsters, weapons etc.
	Phase 13: Sprite rendering for explosions, BFG, pickups etc.
	Uses existing engine structures from rf_model.h and files.h.

==============================================================================
*/

#ifndef __RM_ALIAS_H__
#define __RM_ALIAS_H__

/* Forward declarations - use existing engine structures */
struct refEntity_s;
struct refModel_s;

/*
==================
RM_Alias_Init

Initialize alias model rendering system.
==================
*/
void RM_Alias_Init(void);

/*
==================
RM_Alias_Shutdown

Cleanup alias model rendering resources.
==================
*/
void RM_Alias_Shutdown(void);

/*
==================
RM_DrawAliasModel

Render a single alias model (MD2/MD3) entity.
For Phase 12, just renders frame 0 as static mesh.
==================
*/
void RM_DrawAliasModel(struct refEntity_s *ent);

/*
==================
RM_DrawEntities

Loop through all entities in the scene and render them.
Called from RM_DrawWorld after BSP rendering.
==================
*/
void RM_DrawEntities(void);

/*
==================
RM_Sprite_Init / RM_Sprite_Shutdown

Initialize and cleanup sprite rendering system.
==================
*/
void RM_Sprite_Init(void);
void RM_Sprite_Shutdown(void);

/*
==================
RM_DrawSprites

Render all SP2 sprite entities (explosions, BFG, pickups).
Should be called after RM_DrawEntities with blending enabled.
==================
*/
void RM_DrawSprites(void);

/*
==================
RM_SetMatricesForEntities

Cache projection/view matrices from world rendering for entity use.
==================
*/
void RM_SetMatricesForEntities(float *proj, float *view);

#endif /* __RM_ALIAS_H__ */
