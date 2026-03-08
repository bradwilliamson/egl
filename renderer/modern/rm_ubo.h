#ifndef __RM_UBO_H__
#define __RM_UBO_H__

/*
 * Per-frame UBO layout matching GLSL std140:
 * - mat4 view, mat4 proj (each 4×vec4, 16-byte aligned)
 * - vec4 time (x=time, yzw=padding)
 */
typedef struct {
	float view[16];
	float proj[16];
	float time[4]; /* x=time, yzw=padding */
} ubo_per_frame_t;

struct refDef_s;

void RM_UBO_Init(void);
void RM_UBO_Shutdown(void);
void RM_UBO_Update(struct refDef_s *fd);

#endif /* __RM_UBO_H__ */
