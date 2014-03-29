#ifndef __PIC_H__
#define __PIC_H__

#include "tmap2.h"

struct pic_s
{
	int w, h;
	pixel_t pixels[0];
};

extern struct pic_s *
Pic_Load (const char *path);

extern struct pic_s *
Pic_Free (struct pic_s *pic);

extern void
Pic_FreeAll (void);

#endif /* __PIC_H__ */
