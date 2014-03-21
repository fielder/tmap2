#include "vec.h"
#include "clip.h"

enum
{
	SIDE_ON,
	SIDE_FRONT,
	SIDE_BACK,
};

#define CLIP_EPSILON (1.0 / 16.0)

float c_verts[2][MAX_VERTS][3];
int c_idx;
int c_numverts;


static int
ClassifyDist (float d)
{
	if (d < -CLIP_EPSILON)
		return SIDE_BACK;
	else if (d > CLIP_EPSILON)
		return SIDE_FRONT;
	else
		return SIDE_ON;
}


int
C_ClipWithPlane (const float normal[3], float dist)
{
	int sides[MAX_VERTS + 1];
	float dots[MAX_VERTS + 1];
	int counts[3];
	int i, numout;
	float *v, *next;
	float frac;

	counts[SIDE_ON] = counts[SIDE_FRONT] = counts[SIDE_BACK] = 0;

	for (i = 0; i < c_numverts; i++)
	{
		dots[i] = Vec_Dot (c_verts[c_idx][i], normal) - dist;
		sides[i] = ClassifyDist (dots[i]);
		counts[sides[i]]++;
	}
	dots[i] = dots[0];
	sides[i] = sides[0];

	if (!counts[SIDE_FRONT])
	{
		/* all verts on or behind the plane */
		c_numverts = 0;
		return 0;
	}
	if (!counts[SIDE_BACK])
	{
		/* all verts on the front */
		return 1;
	}

	numout = 0;
	for (i = 0; i < c_numverts; i++)
	{
		v = c_verts[c_idx][i];
		next = ((i + 1) < c_numverts) ? c_verts[c_idx][i + 1] : c_verts[c_idx][0];

		if (sides[i] == SIDE_FRONT || sides[i] == SIDE_ON)
			Vec_Copy (v, c_verts[c_idx ^ 1][numout++]);

		if (	(sides[i] == SIDE_FRONT && sides[i + 1] == SIDE_BACK) ||
			(sides[i] == SIDE_BACK && sides[i + 1] == SIDE_FRONT) )
		{
			frac = dots[i] / (dots[i] - dots[i + 1]);
			c_verts[c_idx ^ 1][numout][0] = v[0] + frac * (next[0] - v[0]);
			c_verts[c_idx ^ 1][numout][1] = v[1] + frac * (next[1] - v[1]);
			c_verts[c_idx ^ 1][numout][2] = v[2] + frac * (next[2] - v[2]);
			numout++;
		}
	}

	if (numout < 3)
	{
		c_numverts = 0;
		return 0;
	}

	c_idx ^= 1;
	c_numverts = numout;

	return 1;
}


int
C_ClipLineWithPlane (const float normal[3], float dist)
{
	float d1, d2, frac;

	d1 = Vec_Dot (c_verts[c_idx][0], normal) - dist;
	d2 = Vec_Dot (c_verts[c_idx][1], normal) - dist;

	if (d1 >= 0.0)
	{
		if (d2 < 0.0)
		{
			/* edge runs from front -> back */
			frac = d1 / (d1 - d2);
			c_verts[c_idx][1][0] = c_verts[c_idx][0][0] + frac * (c_verts[c_idx][1][0] - c_verts[c_idx][0][0]);
			c_verts[c_idx][1][1] = c_verts[c_idx][0][1] + frac * (c_verts[c_idx][1][1] - c_verts[c_idx][0][1]);
			c_verts[c_idx][1][2] = c_verts[c_idx][0][2] + frac * (c_verts[c_idx][1][2] - c_verts[c_idx][0][2]);
		}
		else
		{
			/* both vertices on the front side */
		}
	}
	else
	{
		if (d2 < 0.0)
		{
			/* both vertices behind a plane; the edge is
			 * fully clipped away */
			return 0;
		}
		else
		{
			/* edge runs from back -> front */
			frac = d1 / (d1 - d2);
			c_verts[c_idx][0][0] = c_verts[c_idx][0][0] + frac * (c_verts[c_idx][1][0] - c_verts[c_idx][0][0]);
			c_verts[c_idx][0][1] = c_verts[c_idx][0][1] + frac * (c_verts[c_idx][1][1] - c_verts[c_idx][0][1]);
			c_verts[c_idx][0][2] = c_verts[c_idx][0][2] + frac * (c_verts[c_idx][1][2] - c_verts[c_idx][0][2]);
		}
	}

	return 1;
}
