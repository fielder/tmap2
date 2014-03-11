#ifndef __EDGE_H__
#define __EDGE_H__

#include "tmap2.h"

struct drawspan_s
{
	int y;
	int left, right;
};

extern void
E_CreateSpans (void);

extern struct drawspan_s e_spans[HEIGHT];
extern int e_numspans;

#endif /* __EDGE_H__ */
