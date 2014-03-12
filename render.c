//#include <string.h>
#include <math.h>

#include "tmap2.h"
#include "vec.h"
//#include "clip.h"
//#include "edge.h"
#include "render.h"

const float fov_x = 90.0;

#if 0
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
#endif


void
R_Init (void)
{
	camera.center_x = video.w / 2.0;
	camera.center_y = video.h / 2.0;

	camera.fov_x = DEG2RAD(fov_x);
	camera.dist = (video.w / 2.0) / tan(camera.fov_x / 2.0);
	camera.fov_y = 2.0 * atan((video.h / 2.0) / camera.dist);

	Vec_Clear (camera.pos);
	Vec_Clear (camera.angles);

	R_CalcViewXForm ();
#if 0
	S_SpanInit ();

	tex_pixels = R_LoadPic ("pics/DOOR2_4.img", &tex_w, &tex_h);
#endif
}


void
R_Shutdown (void)
{
#if 0
	if (tex_pixels != NULL)
	{
		free (tex_pixels);
		tex_pixels = NULL;
	}

	S_SpanCleanup ();
#endif
}


void
R_CalcViewXForm (void)
{
	float v[3];

	/* make transformation matrix */
	Vec_Copy (camera.angles, v);
	Vec_Scale (v, -1.0);
	Vec_AnglesMatrix (v, camera.xform, "xyz");

	/* get view vectors */
	Vec_Copy (camera.xform[0], camera.left);
	Vec_Copy (camera.xform[1], camera.up);
	Vec_Copy (camera.xform[2], camera.forward);
}


static void
CalcViewPlanes (void)
{
	float cam2world[3][3];
	float v[3];
	struct viewplane_s *p;
	float ang;

	/* view to world transformation matrix */
	Vec_AnglesMatrix (camera.angles, cam2world, "zyx");

	/* set up view planes */

	p = &camera.vplanes[VPLANE_LEFT];
	ang = (camera.fov_x / 2.0);
	v[0] = -cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);

	p = &camera.vplanes[VPLANE_RIGHT];
	ang = (camera.fov_x / 2.0);
	v[0] = cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);

	p = &camera.vplanes[VPLANE_TOP];
	ang = (camera.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = -cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);

	p = &camera.vplanes[VPLANE_BOTTOM];
	ang = (camera.fov_y / 2.0);
	v[0] = 0.0;
	v[1] = cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);

	/* planes running through pixel centers */

	float pixel_angle_horiz = camera.fov_x / video.w;
	float pixel_angle_vert = camera.fov_y / video.h;

	p = &camera.vplanes_center[VPLANE_LEFT];
	ang = (camera.fov_x / 2.0) - (pixel_angle_horiz / 2.0);
	v[0] = -cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);

	p = &camera.vplanes_center[VPLANE_RIGHT];
	ang = (camera.fov_x / 2.0) - (pixel_angle_horiz / 2.0);
	v[0] = cos (ang);
	v[1] = 0.0;
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);

	p = &camera.vplanes_center[VPLANE_TOP];
	ang = (camera.fov_y / 2.0) - (pixel_angle_vert / 2.0);
	v[0] = 0.0;
	v[1] = -cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);

	p = &camera.vplanes_center[VPLANE_BOTTOM];
	ang = (camera.fov_y / 2.0) - (pixel_angle_vert / 2.0);
	v[0] = 0.0;
	v[1] = cos (ang);
	v[2] = sin (ang);
	Vec_Transform (cam2world, v, p->normal);
	p->dist = Vec_Dot (p->normal, camera.pos);
}


#if 0
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
#endif


#if 0
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
#endif


static void
RenderLine (int x1, int y1, int x2, int y2, pixel_t c)
{
	int x, y;
	int dx, dy;
	int sx, sy;
	int ax, ay;
	int d;

	if (1)
	{
		if (	x1 < 0 || x1 >= video.w ||
			x2 < 0 || x2 >= video.w ||
			y1 < 0 || y1 >= video.h ||
			y2 < 0 || y2 >= video.h )
		{
			return;
		}
	}

	dx = x2 - x1;
	ax = 2 * (dx < 0 ? -dx : dx);
	sx = dx < 0 ? -1 : 1;

	dy = y2 - y1;
	ay = 2 * (dy < 0 ? -dy : dy);
	sy = dy < 0 ? -1 : 1;

	x = x1;
	y = y1;

	if (ax > ay)
	{
		d = ay - ax / 2;
		while (1)
		{
			video.rows[y][x] = c;
			if (x == x2)
				break;
			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay / 2;
		while (1)
		{
			video.rows[y][x] = c;
			if (y == y2)
				break;
			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}


static void
RenderPixel (int x, int y, int c)
{
	if (x >= 0 && x < video.w && y >= 0 && y < video.h)
	{
		video.rows[y][x] = c & 0xffff;
//		S_ClipAndEmitSpan (x, y, y);
	}
}


static void
RenderPoint (float x, float y, float z)
{
	float v[3], local[3], out[3];

	v[0] = x; v[1] = y; v[2] = z;

	Vec_Subtract (v, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	if (out[2] > 0.0)
	{
		int sx = camera.center_x - camera.dist * (out[0] / out[2]) + 0.5;
		int sy = camera.center_y - camera.dist * (out[1] / out[2]) + 0.5;
		RenderPixel (sx, sy, 0xffff);
	}
}


static void
Render3DLine (const float v1[3], const float v2[3], pixel_t c)
{
}


/*
 * Traverse the world and set up structures to prepare drawing to the
 * frame buffer.
 */
void
R_DrawScene (void)
{
	CalcViewPlanes ();

	/*
	e_numspans = 0;

	DrawPoly (&test_poly);
	*/

	//...
}


/*
 * Draw to the frame buffer.
 */
void
R_RenderScene (void)
{
	if (1)
	{
		int x, z;
		for (x = -50; x < 50; x++)
		{
			float scale = 4.0;
			for (z = -50; z < 50; z++)
				RenderPoint(x*scale, 0, z*scale);
		}
	}
}
