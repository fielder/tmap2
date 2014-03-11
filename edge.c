#include <math.h>

#include "vec.h"
#include "tmap2.h"
#include "clip.h"
#include "edge.h"

struct drawedge_s
{
	struct drawedge_s *next;

	int top, bottom;
	int u, du; /* 12.20 fixed-point */

	int is_right;
};

static struct drawedge_s *edge;

struct drawspan_s e_spans[HEIGHT];
int e_numspans;

//static struct drawspan_s *spans;


static void
EmitProjectedEdge (const float top[2], const float bot[2], int is_right)
{
}


static void
EmitEdge (const float v1[3], const float v2[3])
{
	float local[3], out[3];
	float scale;

	float p1[2], p2[2];

	Vec_Subtract (v1, cam.pos, local);
	Vec_Transform (cam.xform, local, out);
	scale = cam.dist / out[2];
	p1[0] = cam.center_x - scale * out[0];
	p1[1] = cam.center_y - scale * out[1];

	Vec_Subtract (v2, cam.pos, local);
	Vec_Transform (cam.xform, local, out);
	scale = cam.dist / out[2];
	p2[0] = cam.center_x - scale * out[0];
	p2[1] = cam.center_y - scale * out[1];

	if (p1[1] < p2[1])
		EmitProjectedEdge (p1, p2, 0);
	else if (p1[1] > p2[1])
		EmitProjectedEdge (p2, p1, 1);
	else
	{
		/* ignore horizontal edges */
	}
}


static void
FindEdges (void)
{
	int i;

	for (i = 0; i < c_numverts; i++)
	{
		const float *v1 = c_verts[c_idx][i];
		const float *v2 = (i + 1 < c_numverts) ? c_verts[c_idx][i + 1] : c_verts[c_idx][0];
		EmitEdge (v1, v2);
	}
}


static void
ScanSortedEdges (void)
{
	//...
}


void
E_CreateSpans (void)
{
	struct drawedge_s edges[128];

	edge = edges;
	FindEdges ();

	//TODO: sort 'em

	ScanSortedEdges ();
}



#if 0
	float u1_f, v1_f;
	int v1_i;

	float u2_f, v2_f;
	int v2_i;

	float du;

	float local[3], out[3];
	float scale;

	Vec_Subtract (v1, cam.pos, local);
	Vec_Transform (cam.xform, local, out);
	scale = cam.dist / out[2];
	u1_f = cam.center_x - scale * out[0];
	v1_f = cam.center_y - scale * out[1];
	v1_i = floor(v1_f + 0.5);

	Vec_Subtract (v2, cam.pos, local);
	Vec_Transform (cam.xform, local, out);
	scale = cam.dist / out[2];
	u2_f = cam.center_x - scale * out[0];
	v2_f = cam.center_y - scale * out[1];
	v2_i = floor(v2_f + 0.5);

	if (v1_i == v2_i)
	{
		/* ignore horizontal edges */
		return;
	}
	else if (v1_i < v2_i)
	{
		du = (u2_f - u1_f) / (v2_f - v1_f);
		edge->u = (u1_f + du * (v1_i + 0.5 - v1_f)) * 0x100000;

		/* When scannin out, we shift down to get actual pixel
		 * coords (ie: floor the value). So pre-adjust here so
		 * we generate pixels that are properly inside the scan
		 * line */
		edge-u += (0x100000 / 2);

		edge->du = du * 0x100000;
		edge->top = v1_i;
		/* our fill rule says the bottom vertex/pixel goes to
		 * the next poly */
		edge->bottom = v2_i - 1;
		edge->is_right = 0;
		edge++;
	}
	else
	{
		du = (u1_f - u2_f) / (v1_f - v2_f);
		edge->u = (u2_f + du * (v2_i + 0.5 - v2_f)) * 0x100000;

		/* When scannin out, we shift down to get actual pixel
		 * coords (ie: floor the value). So pre-adjust here so
		 * we generate pixels that are properly inside the scan
		 * line */
		edge->u += (0x100000 / 2);

		edge->du = du * 0x100000;
		edge->top = v2_i;
		/* our fill rule says the bottom vertex/pixel goes to
		 * the next poly */
		edge->bottom = v1_i - 1;
		edge->is_right = 1;
		edge++;
	}
#endif
