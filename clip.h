#ifndef __CLIP_H__
#define __CLIP_H__

//FIXME: likely overflows when the poly has max verts (or close) and is clipped

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

/* when clipped */
extern float c_back_verts[MAX_VERTS][3];
extern int c_back_numverts;

extern int
C_ClipWithPlane (const float normal[3], float dist);

/* assumes c_verts and c_numverts describes only 2 verts */
extern int
C_ClipLineWithPlane (const float normal[3], float dist);

#endif /* __CLIP_H__ */
