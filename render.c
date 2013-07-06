#include <string.h>
#include <math.h>

#include "tmap2.h"
#include "vec.h"
#include "clip.h"
#include "edge.h"
#include "render.h"

struct poly_s test_poly =
{
	{	{ 0.0, 0.0, 512.0 },
		{ 0.0, 128.0, 512.0 },
		{ 64.0, 128.0, 512.0 },
		{ 64.0, 0.0, 512.0 } },
	4,
	{0, 0, -1},
	-512
};


static void
CalcCamera (void)
{
	float cam2world[3][3];
	float v[3];
	struct viewplane_s *p;
	float ang;

	/* make transformation matrix */
	Vec_Copy (cam.angles, v);
	Vec_Scale (v, -1.0);
	Vec_AnglesMatrix (v, cam.xform, "xyz");

	/* get view vectors */
	Vec_Copy (cam.xform[0], cam.left);
	Vec_Copy (cam.xform[1], cam.up);
	Vec_Copy (cam.xform[2], cam.forward);

	/* set up view planes */

	/* view to world transformation matrix */
	Vec_AnglesMatrix (cam.angles, cam2world, "zyx");

	p = &cam.vplanes[VPLANE_LEFT];
	ang = (cam.fov_x / 2.0);
	v[0] = -cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);

	p = &cam.vplanes[VPLANE_RIGHT];
	ang = (cam.fov_x / 2.0);
	v[0] = cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);

	p = &cam.vplanes[VPLANE_TOP];
	ang = (cam.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = -cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);

	p = &cam.vplanes[VPLANE_BOTTOM];
	ang = (cam.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, cam.pos);
}


void
R_RenderPolySpans (void)
{
	struct drawspan_s *s;
	int x;

	for (s = e_spans; s != e_spans + e_numspans; s++)
	{
		for (x = s->left; x <= s->right; x++)
			rowtab[s->y][x] = 0xffff;
	}
}


static void
DrawPoly (struct poly_s *p)
{
	int i;

	/* back-face check */
	if (Vec_Dot(p->normal, cam.pos) - p->dist < BACKFACE_EPSILON)
		return;

	/* set up for clipping */
	c_idx = 0;
	for (i = 0; i < p->num_verts; i++)
		Vec_Copy (p->verts[i], c_verts[c_idx][i]);
	c_numverts = i;

	/* clip against the view planes */
	for (i = 0; i < 4; i++)
	{
		if (!C_ClipWithPlane(cam.vplanes[i].normal, cam.vplanes[i].dist))
			return;
	}

	E_CreateSpans ();
}


void
R_DrawScene (void)
{
	CalcCamera ();

	DrawPoly (&test_poly);

	//...
}
