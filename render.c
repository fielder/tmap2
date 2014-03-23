#include <stdint.h>
#include <string.h>
#include <math.h>

#include "tmap2.h"
#include "vec.h"
#include "clip.h"
#include "render.h"

const float fov_x = 90.0;


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
}

/* ================================================================== */
/* ================================================================== */

//FIXME: likely overflows when the poly has max verts (or close) and is clipped
struct poly_s
{
	float verts[MAX_VERTS][3];
	int num_verts;

	float normal[3];
	float dist;
};

struct span_s
{
	int u, v, len;
};

struct edge_s
{
	struct edge_s *next;
	int top, bottom;
	int u, du; /* 12.20 fixed-point format */
};

struct emit_poly_s
{
	struct span_s *spans;
	int num_spans;
	pixel_t color;
};

static struct span_s r_spans_pool[2048];
static struct span_s *r_spans = r_spans_pool;
static struct span_s *r_spans_end = r_spans_pool + (sizeof(r_spans_pool) / sizeof(r_spans_pool[0]));

static struct emit_poly_s r_epolys_pool[100];
static struct emit_poly_s *r_epolys = r_epolys_pool;
static struct emit_poly_s *r_epolys_end = r_epolys_pool + (sizeof(r_epolys_pool) + sizeof(r_epolys_pool[0]));

static struct edge_s *r_edges_left = NULL;
static struct edge_s *r_edges_right = NULL;


static pixel_t
PtrToPixel (const void *ptr)
{
	uintptr_t a = (uintptr_t)ptr;
//(uintptr_t)p & ((1 << (sizeof(pixel_t) * 8)) - 1);
	return 0xffff;
}


static void
EmitSpan (int v, int x1, int x2)
{
	if (r_spans != r_spans_end)
	{
		r_spans->u = x1;
		r_spans->v = v;
		r_spans->len = x2 - x1 + 1;
		r_spans++;
	}
}


static void
ScanEdges (void)
{
	EmitSpan (50, 5, video.w - 5);
	//TODO: create spans
}


static int
EmitEdge (struct edge_s *e, const float v1[3], const float v2[3])
{
	float u1_f, v1_f;
	int v1_i;

	float u2_f, v2_f;
	int v2_i;

	float du;

	float local[3], out[3];
	float scale;

	/* the pixel containment rule says an edge point
	 * exactly on the center of a pixel will be
	 * considered to cover that pixel */

	Vec_Subtract (v1, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	u1_f = camera.center_x - scale * out[0];
	v1_f = camera.center_y - scale * out[1];
	v1_i = ceil (v1_f - 0.5);

	Vec_Subtract (v2, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	u2_f = camera.center_x - scale * out[0];
	v2_f = camera.center_y - scale * out[1];
	v2_i = ceil (v2_f - 0.5);

	if (v1_i == v2_i)
	{
		/* doesn't cross a pixel center vertically */
		return 0;
	}
	else if (v1_i < v2_i)
	{
#if 0
		du = (u2_f - u1_f) / (v2_f - v1_f);
		r_edges->u = (u1_f + du * (v1_i + 0.5 - v1_f)) * 0x100000 + (1 << 19);
		r_edges->du = (du) * 0x100000;
		r_edges->top = v1_i;
		/* our fill rule says the bottom vertex/pixel goes to
		 * the next poly */
		r_edges->bottom = v2_i - 1;
#endif
		e->next = r_edges_left;
		r_edges_left = e;
	}
	else
	{
#if 0
		du = (u1_f - u2_f) / (v1_f - v2_f);
		r_edges->u = (u2_f + du * (v2_i + 0.5 - v2_f)) * 0x100000 + (1 << 19);
		r_edges->du = (du) * 0x100000;
		r_edges->top = v2_i;
		/* our fill rule says the bottom vertex/pixel goes to
		 * the next poly */
		r_edges->bottom = v1_i - 1;
#endif
		e->next = r_edges_right;
		r_edges_right = e;
	}

	return 1;
}


static void
DrawPoly (const struct poly_s *p)
{
	int i;

	/* no more emit polys */
	if (r_epolys == r_epolys_end)
		return;

	/* back-face check */
	if (Vec_Dot(p->normal, camera.pos) - p->dist < BACKFACE_EPSILON)
		return;

	/* set up for clipping */
	c_idx = 0;
	for (i = 0; i < p->num_verts; i++)
		Vec_Copy (p->verts[i], c_verts[c_idx][i]);
	c_numverts = i;

	/* clip against the view planes */
	for (i = 0; i < 4; i++)
	{
		if (!C_ClipWithPlane(camera.vplanes[i].normal, camera.vplanes[i].dist))
			return;
	}

	{
		struct edge_s edges[MAX_VERTS + 8];
		struct edge_s *end = edges + sizeof(edges) / sizeof(edges[0]);
		struct edge_s *next = edges;

		r_edges_left = NULL;
		r_edges_right = NULL;

		/* project and set up edges */
		for (i = 0; i < c_numverts; i++)
		{
			if (next == end)
				return;
			if (EmitEdge(	next,
					c_verts[c_idx][i],
					c_verts[c_idx][(i + 1) < c_numverts ? (i + 1) : 0]) )
			{
				next++;
			}
		}

		/* scan the edges, creating create drawable spans */

		r_epolys->spans = r_spans;

		ScanEdges ();

		/* if spans were generated, the poly is visible */
		if (r_spans != r_epolys->spans)
		{
			r_epolys->num_spans = r_spans - r_epolys->spans;
			r_epolys->color = PtrToPixel (p);
			r_epolys++;
		}
	}
}



static struct poly_s test_poly =
{
	{	{ 0.0, 0.0, 512.0 },
		{ 0.0, 128.0, 512.0 },
		{ 64.0, 128.0, 512.0 },
		{ 64.0, 0.0, 512.0 } },
	4,
	{0, 0, -1},
	-512
};

/*
 * Traverse the world and set up structures to prepare drawing to the
 * frame buffer.
 */
void
R_DrawScene (void)
{
	r_spans = r_spans_pool;
	r_epolys = r_epolys_pool;

	CalcViewPlanes ();

	DrawPoly (&test_poly);

	//...
}


static void
Render3DPoint (float x, float y, float z)
{
	float v[3], local[3], out[3];

	v[0] = x; v[1] = y; v[2] = z;

	Vec_Subtract (v, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	if (out[2] > 0.0)
	{
		int sx = camera.center_x - camera.dist * (out[0] / out[2]) + 0.5;
		int sy = camera.center_y - camera.dist * (out[1] / out[2]) + 0.5;
		if (sx >= 0 && sx < video.w && sy >= 0 && sy < video.h)
			video.rows[sy][sx] = 0xffff;
	}
}


static void
RenderPolySpans (const struct emit_poly_s *ep)
{
	const struct span_s *s;
	for (s = ep->spans; s != ep->spans + ep->num_spans; s++)
		memset (video.rows[s->v] + s->u, ep->color, s->len * sizeof(pixel_t));
}


/*
 * Draw to the frame buffer.
 */
void
R_RenderScene (void)
{
	struct emit_poly_s *ep;

	if (1)
	{
		int x, z;
		for (x = -50; x < 50; x++)
		{
			float scale = 4.0;
			for (z = -50; z < 50; z++)
				Render3DPoint (x*scale, 0, z*scale);
		}
	}

	for (ep = r_epolys_pool; ep != r_epolys; ep++)
		RenderPolySpans (ep);
}
