#ifndef __CLIP_H__
#define __CLIP_H__

#define MAX_VERTS 16

struct poly_s
{
	float verts[MAX_VERTS][3];
	int num_verts;

	float normal[3];
	float dist;
};

extern float c_verts[2][MAX_VERTS][3];
extern int c_idx;
extern int c_numverts;

extern int
C_ClipWithPlane (const float normal[3], float dist);

#endif /* __CLIP_H__ */
