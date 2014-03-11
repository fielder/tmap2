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
camera.pos[1] = 44;
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
#if 0
	float cam2world[3][3];
	float v[3];
	struct viewplane_s *p;
	float ang;

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
#endif
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
PutPixel (int x, int y, int c)
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
	v[0] = x;
	v[1] = y;
	v[2] = z;

#if 0
	{
		struct viewplane_s *p;
		p = &camera.vplanes[VPLANE_LEFT];
		if (Vec_Dot(v, p->normal) - p->dist < (1 / 32.0))
			return;
		p = &camera.vplanes[VPLANE_RIGHT];
		if (Vec_Dot(v, p->normal) - p->dist < (1 / 32.0))
			return;
	}
#endif

	Vec_Subtract (v, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	if (out[2] > 0.0)
	{
		int sx = camera.center_x - camera.dist * (out[0] / out[2]) + 0.5;
		int sy = camera.center_y - camera.dist * (out[1] / out[2]) + 0.5;
		PutPixel (sx, sy, 0xffff);
	}
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
