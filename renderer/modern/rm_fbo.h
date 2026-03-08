#pragma once
#ifndef __RM_FBO_H__
#define __RM_FBO_H__

/* No q_shared.h to avoid header conflicts */

typedef struct {
	unsigned int id;
	unsigned int color_tex;
	unsigned int depth_tex;
	int width, height;
} rm_fbo_t;

rm_fbo_t *RM_FBO_Create (int width, int height);
void      RM_FBO_Destroy (rm_fbo_t *fbo);

/* Bind FBO and set its viewport.
 * If fbo is NULL, bind Backbuffer AND restore Window Viewport using winWidth/winHeight. */
void      RM_FBO_Bind (rm_fbo_t *fbo, int winWidth, int winHeight);

void      RM_FBO_BlitToBackbuffer (rm_fbo_t *fbo, int screenWidth, int screenHeight);

/* Fullscreen quad: draw FBO color texture to backbuffer (replaces blit for post-process) */
void      RM_FBO_InitQuad (void);
void      RM_FBO_ShutdownQuad (void);
void      RM_FBO_DrawToBackbuffer (rm_fbo_t *fbo);

#endif /* __RM_FBO_H__ */
