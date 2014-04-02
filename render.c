#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "tmap2.h"
#include "vec.h"
#include "clip.h"
#include "pic.h"
#include "render.h"

#define BACKFACE_EPSILON 0.5

static const float fov_x = 90.0;

static struct pic_s *texture = NULL;
int r_showtex = 0;
int r_debugframe = 0;


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

	texture = Pic_Load ("TOMW2_1.PCX.rgb"); /* 128x72 */
}


void
R_Shutdown (void)
{
	texture = Pic_Free (texture);

	Pic_FreeAll ();
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

	int textured;

	/* texture-space vecs */
	float texorg[3];
	float texvec_s[3];
	float texvec_t[3];
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
	const struct poly_s *poly;
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
	return (a & 0xff00) | ((a >> 8) & 0xff);
}


static void
EmitSpan (int v, int x1, int x2)
{
	//FIXME: use green spans
	if (r_spans == r_spans_end)
		return;
	if (v < 0 || v >= video.h)
		return;
	if (x1 >= video.w)
		return;
	if (x2 < 0)
		return;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= video.w)
		x2 = video.w - 1;
	if (x1 <= x2)
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
	/* Clipping imprecision can lead to a fex-pixel error when
	 * projecting. For the 2 top edges, skip pixels until both the
	 * edge tops even up. */
	while (r_edges_left->top < r_edges_right->top)
	{
		r_edges_left->u += r_edges_left->du;
		if (++r_edges_left->top > r_edges_left->bottom)
			r_edges_left = r_edges_left->next;
	}
	while (r_edges_right->top < r_edges_left->top)
	{
		r_edges_right->u += r_edges_right->du;
		if (++r_edges_right->top > r_edges_right->bottom)
			r_edges_right = r_edges_right->next;
	}

	if (r_edges_left == NULL || r_edges_right == NULL)
		return;

	int v = r_edges_left->top;

#if 0
	if (v < 0)
	{
		printf ("ltop: %d  rtop: %d\n", r_edges_left->top, r_edges_right->top);
		printf ("(%g %g %g)\n", camera.pos[0], camera.pos[1], camera.pos[2]);
		printf ("angles: %g %g %g\n", camera.angles[0], camera.angles[1], camera.angles[2]);
		printf ("left: (%g %g %g)\n", camera.left[0], camera.left[1], camera.left[2]);
		printf ("up: (%g %g %g)\n", camera.up[0], camera.up[1], camera.up[2]);
		printf ("forward: (%g %g %g)\n", camera.forward[0], camera.forward[1], camera.forward[2]);
		printf ("\n");

	}
#endif

	while (r_edges_left != NULL && r_edges_right != NULL)
	{
		EmitSpan (v++, r_edges_left->u >> 20, r_edges_right->u >> 20);

		r_edges_left->u += r_edges_left->du;
		r_edges_right->u += r_edges_right->du;

		if (v > r_edges_left->bottom)
			r_edges_left = r_edges_left->next;
		if (v > r_edges_right->bottom)
			r_edges_right = r_edges_right->next;
	}
}


static struct edge_s *
LinkEdge (struct edge_s *e, struct edge_s *list)
{
	if (list == NULL || e->top < list->top)
	{
		e->next = list;
		list = e;
	}
	else
	{
		struct edge_s *p;
		for (p = list; p->next != NULL && p->next->top < e->top; p = p->next) {}
		e->next = p->next;
		p->next = e;
	}
	return list;
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

	/* the pixel containment rule says an edge point exactly on the
	 * center of a pixel vertically will be considered to cover that
	 * pixel */

	Vec_Subtract (v1, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	u1_f = camera.center_x - scale * out[0];
	v1_f = camera.center_y - scale * out[1];

	Vec_Subtract (v2, camera.pos, local);
	Vec_Transform (camera.xform, local, out);
	scale = camera.dist / out[2];
	u2_f = camera.center_x - scale * out[0];
	v2_f = camera.center_y - scale * out[1];

	v1_i = ceil (v1_f - 0.5);
	v2_i = ceil (v2_f - 0.5);

	if (v1_i == v2_i)
	{
		/* doesn't cross a pixel center vertically */
		return 0;
	}
	else if (v1_i < v2_i)
	{
		/* left-side edge, running down the screen */

		if (v2_i <= 0 || v1_i >= video.h)
		{
			/* math imprecision sometimes results in nearly-horizontal
			 * emitted edges just above or just below the screen */
			return 0;
		}

		du = (u2_f - u1_f) / (v2_f - v1_f);
		e->u = (u1_f + du * (v1_i + 0.5 - v1_f)) * 0x100000;
		e->u += ((1 << 20) / 2) - 1; /* pre-adjust screen x coords so we end up with the correct pixel after shifting down */
		e->du = du * 0x100000;
		e->top = v1_i;
		e->bottom = v2_i - 1; /* this edge's last row is 1 pixel above the top pixel of the next edge */
		r_edges_left = LinkEdge (e, r_edges_left);
	}
	else
	{
		/* right-side edge, running up the screen */

		if (v1_i <= 0 || v2_i >= video.h)
		{
			/* math imprecision sometimes results in nearly-horizontal
			 * emitted edges just above or just below the screen */
			return 0;
		}

		du = (u1_f - u2_f) / (v1_f - v2_f);
		e->u = (u2_f + du * (v2_i + 0.5 - v2_f)) * 0x100000;
		e->u -= ((1 << 20) / 2); /* pre-adjust screen x coords so we end up with the correct pixel after shifting down */
		e->du = du * 0x100000;
		e->top = v2_i;
		e->bottom = v1_i - 1; /* this edge's last row is 1 pixel above the top pixel of the next edge */
		r_edges_right = LinkEdge (e, r_edges_right);
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
		struct edge_s e_edges[MAX_VERTS + 8];
		struct edge_s *e_next = e_edges;
		memset (e_edges, 0xff, sizeof(e_edges));
		e_next = e_edges;

		r_edges_left = NULL;
		r_edges_right = NULL;

		/* project and set up edges */
		for (i = 0; i < c_numverts; i++)
		{
			if (e_next == e_edges + sizeof(e_edges) / sizeof(e_edges[0]))
				return;
			if (EmitEdge(	e_next,
					c_verts[c_idx][i],
					c_verts[c_idx][(i + 1) < c_numverts ? (i + 1) : 0]) )
			{
				e_next++;
			}
		}

		if (r_edges_left == NULL || r_edges_right == NULL)
		{
			/* can happen if the poly was clipped as in-view
			 * but no edges were emitted */
			return;
		}

		/* scan the edges, creating create drawable spans */

		r_epolys->spans = r_spans;

		ScanEdges ();

		/* if spans were generated, the poly is visible */
		if (r_spans != r_epolys->spans)
		{
			r_epolys->poly = p;
			r_epolys->num_spans = r_spans - r_epolys->spans;
			r_epolys->color = PtrToPixel (p);
			r_epolys++;
		}
	}
}


static struct poly_s test_poly =
{
	{
		{ 96, 0, 512 },
		{ 32, 0, 512 },
		{ 0, 64, 512 },
		{ 96, 160, 512 },
		{ 160, 128, 512 },
		{ 160, 64, 512 },
	},
	6,
	{0, 0, -1},
	-512,
	0,
};
#define Z 160
static struct poly_s test_poly2 =
{
	{
		{ 128, 72, Z },
		{ 128, 0, Z },
		{ 0, 0, Z },
		{ 0, 72, Z },
	},
	4,
	{0, 0, -1},
	-Z,
	1,
	{128, 72, Z},
	{-128, 0, 0},
	{0, -72, 0},
};
#undef Z

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
	DrawPoly (&test_poly2);

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
RenderTexturedPixel (int u, int v, int s, int t, const struct pic_s *tex)
{
	if (u >= 0 && u < video.w && v >= 0 && v < video.h)
	{
		if (s >= 0 && s < tex->w && t >= 0 && t < tex->h)
			video.rows[v][u] = tex->pixels[t * tex->w + s];
		else
			video.rows[v][u] = (pixel_t)-1;
	}
}


static void
RenderSolidSpans (const struct span_s *spans, int count, pixel_t color)
{
	int x;
	for (; count > 0; count--, spans++)
	{
		for (x = 0; x < spans->len; x++)
			video.rows[spans->v][spans->u + x] = color;
	}
}


static void
RenderPoly (const struct emit_poly_s *ep)
{
	if (ep->poly == NULL || !ep->poly->textured)
	{
		RenderSolidSpans (ep->spans, ep->num_spans, ep->color);
	}
	else
	{
		float P[3], M[3], N[3];
		float A[3], B[3], C[3];
		float S[3];
		float local[3];

		const struct span_s *span;
		float a, b, c;
		int i, u, s, t;

		Vec_Subtract (ep->poly->texorg, camera.pos, local);
		Vec_Transform (camera.xform, local, P);

		Vec_Transform (camera.xform, ep->poly->texvec_s, M);
		Vec_Transform (camera.xform, ep->poly->texvec_t, N);

		Vec_Cross (P, N, A);
		Vec_Cross (M, P, B);
		Vec_Cross (N, M, C);

		for (i = 0, span = ep->spans; i < ep->num_spans; i++, span++)
		{
			S[1] = camera.center_y - span->v;
			S[2] = camera.dist;
			for (u = span->u; u < span->u + span->len; u++)
			{
				S[0] = camera.center_x - u;
				a = Vec_Dot (S, A);
				b = Vec_Dot (S, B);
				c = Vec_Dot (S, C);
				s = texture->w * (a / c);
				t = texture->h * (b / c);
				RenderTexturedPixel (u, span->v, s, t, texture);
			}
		}

		// A = P x N
		// B = M x P
		// C = N x M
		// S = ( u, v, camera.dist )
		// a = S * A
		// b = S * B
		// c = S * C
		// s = texw * a / c
		// t = texh * b / c
	}
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
		for (x = 0; x < 100; x++)
		{
			float scale = 1.0;
			for (z = 0; z < 100; z++)
				Render3DPoint (-50 + (x * scale), 0, 128 + (z * scale));
		}
		for (x = 0; x < 200; x++)
			Render3DPoint (x, 0, 512);
	}

	for (ep = r_epolys_pool; ep != r_epolys; ep++)
		RenderPoly (ep);

	if (r_showtex)
	{
		int y;
		for (y = 0; y < texture->h; y++)
			memcpy (video.rows[y], texture->pixels + y * texture->w, sizeof(pixel_t) * texture->w);
	}

	r_debugframe = 0;
}
