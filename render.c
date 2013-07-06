#include "clip.c"

#define BACKFACE_EPSILON 0.1


void
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


int
ProjectPoint (const float p[3], int *u, int *v)
{
	float local[3], out[3], zi;

	Vec_Subtract (p, cam.pos, local);
	Vec_Transform (cam.xform, local, out);
	if (out[2] <= 0.0)
		return 0;

	zi = 1.0 / out[2];
	*u = (W / 2.0) - cam.dist * zi * out[0];
	*v = (H / 2.0) - cam.dist * zi * out[1];

	if (*u < 0) *u = 0;
	if (*u >= W) *u = W - 1;
	if (*v < 0) *v = 0;
	if (*v >= H) *v = H - 1;

	return 1;
}


float map_verts[MAX_VERTS][3] =
{
	{ 0.0, 0.0, 512.0 },
	{ 0.0, 128.0, 512.0 },
	{ 64.0, 128.0, 512.0 },
	{ 64.0, 0.0, 512.0 }
};
int map_num_verts = 4;


void
DrawPoly (void)
{
	int i;

	/* back-face check */
	{
		float normal[3], dist;
		Vec_MakeNormal (map_verts[0], map_verts[1], map_verts[2], normal, &dist);
		if (Vec_Dot(normal, cam.pos) - dist < BACKFACE_EPSILON)
			return;
	}

	c_idx = 0;
	for (i = 0; i < map_num_verts; i++)
		Vec_Copy (map_verts[i], c_verts[c_idx][i]);
	c_numverts = i;

	for (i = 0; i < 4; i++)
	{
		if (!ClipPolyAgainstPlane(&cam.vplanes[i]))
			return;
	}

	for (i = 0; i < c_numverts; i++)
	{
		int u, v;
		if (ProjectPoint(c_verts[c_idx][i], &u, &v))
			rowtab[v][u] = 0xffff;
	}
}


void
DrawScene (void)
{
	CalcCamera ();

	DrawPoly ();

	//...
}
