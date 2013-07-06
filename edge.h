#ifndef __EDGE_H__
#define __EDGE_H__

struct drawspan_s
{
	int y;
	int left, right;
};

extern void
E_CreateSpans (void);

extern struct drawspan_s *e_spans;
extern int e_numspans;

#endif /* __EDGE_H__ */
