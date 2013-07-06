#ifndef __CLIP_H__
#define __CLIP_H__

enum
{
	SIDE_ON,
	SIDE_FRONT,
	SIDE_BACK,
};

#define CLIP_EPSILON (1.0 / 16.0)

#define MAX_VERTS 16

extern float c_verts[2][MAX_VERTS][3];
extern int c_idx;
extern int c_numverts;

extern int
C_ClipWithPlane (const float normal[3], float dist);

#endif /* __CLIP_H__ */
