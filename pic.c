#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include "pic.h"

static void *
LoadFile (const char *path, int *size);

struct _ipic_s
{
	struct _ipic_s *next;
	struct pic_s pic;
};

static struct _ipic_s ipics = { NULL };


static pixel_t
XLateRGB (int r, int g, int b)
{
	pixel_t p = 0;

	if (sizeof(p) == 1)
	{
		p = (r & 0xe0) | ((g >> 3) & 0x18) | ((b >> 5) & 0x7);
	}
	else if (sizeof(p) == 2)
	{
		//TODO: use video masks instead of hardcoding 565 here
		p = ((r << 8) & 0xf800) | ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f);
	}
	else if (sizeof(p) == 4)
	{
		//TODO: ???
		p = -1;
	}

	return p;
}


struct pic_s *
Pic_Load (const char *path)
{
	uint8_t *file;
	struct _ipic_s *ipic;
	int w, h, i;

	if ((file = LoadFile(path, NULL)) == NULL)
		return NULL;

	w = ((int)file[0] << 0) | ((int)file[1] << 8) | ((int)file[2] << 16) | ((int)file[3] << 24);
	h = ((int)file[4] << 0) | ((int)file[5] << 8) | ((int)file[6] << 16) | ((int)file[7] << 24);

	ipic = malloc (sizeof(*ipic) + (sizeof(pixel_t) * w * h));
	ipic->next = ipics.next;
	ipics.next = ipic;

	ipic->pic.w = w;
	ipic->pic.h = h;
	for (i = 0; i < w * h; i++)
	{
		ipic->pic.pixels[i] = XLateRGB (file[8 + i * 3 + 0],
						file[8 + i * 3 + 1],
						file[8 + i * 3 + 2]);
	}

	free (file);
	file = NULL;

	return &ipic->pic;
}


struct pic_s *
Pic_Free (struct pic_s *pic)
{
	if (pic != NULL)
	{
		struct _ipic_s *ipic;
		for (ipic = &ipics; ipic->next != NULL && &ipic->next->pic != pic; ipic = ipic->next) {}
		if (ipic->next != NULL)
		{
			struct _ipic_s *n = ipic->next;
			ipic->next = ipic->next->next;
			free (n);
			n = NULL;
		}
	}
	return NULL;
}


void
Pic_FreeAll (void)
{
	while (ipics.next != NULL)
		Pic_Free (&ipics.next->pic);
}


static void *
LoadFile (const char *path, int *size)
{
	void *ret = NULL;
	int fd;

	fd = open (path, O_RDONLY);
	if (fd != -1)
	{
		int sz = lseek (fd, 0, SEEK_END);
		if (sz != -1)
		{
			ret = malloc (sz + 1);
			if (ret != NULL)
			{
				lseek (fd, 0, SEEK_SET);
				if (read(fd, ret, sz) != sz)
				{
					free (ret);
					ret = NULL;
				}
				else
				{
					((char *)ret)[sz] = '\0';
					if (size != NULL)
						*size = sz;
				}
			}
		}
		close (fd);
		fd = -1;
	}

	return ret;
}
