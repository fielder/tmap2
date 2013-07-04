#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <SDL.h>

#include "vec.h"

#define W 320
#define H 240
#define PIXTYPE uint16_t
#define BPP (sizeof(PIXTYPE) * 8)
#define FOV_X 90.0 /* degrees */

#define FLYSPEED 64.0

#if 1
/* wasd-style on a kinesis advantage w/ dvorak */
const int bind_forward = '.';
const int bind_back = 'e';
const int bind_left = 'o';
const int bind_right = 'u';
#else
/* qwerty-style using arrows */
const int bind_forward = SDLK_UP;
const int bind_back = SDLK_DOWN;
const int bind_left = SDLK_LEFT;
const int bind_right = SDLK_RIGHT;
#endif

enum
{
	VPLANE_LEFT,
	VPLANE_RIGHT,
	VPLANE_TOP,
	VPLANE_BOTTOM,
};

enum
{
	PITCH,
	YAW,
	ROLL,
};

struct viewplane_s
{
	float normal[3];
	float dist;
};

struct
{
	float center_x;
	float center_y;

	float fov_x; /* radians */
	float fov_y; /* radians */
	float dist;

	float pos[3];
	float angles[3]; /* radians */

	float xform[3][3]; /* world-to-camera */

	float forward[3];
	float left[3];
	float up[3];

	struct viewplane_s vplanes[4];
} cam;

struct
{
	int mouse_delta[2];
	struct
	{
		int state[SDLK_LAST];
		int press[SDLK_LAST];
		int release[SDLK_LAST];
	} key;
} in;

SDL_Surface *sdl_surf = NULL;

PIXTYPE **rowtab = NULL;

float frametime;

/* ========================================================== */

void
Shutdown (void)
{
	if (sdl_surf != NULL)
	{
		SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
		SDL_WM_GrabInput (SDL_GRAB_OFF);
		SDL_ShowCursor (SDL_ENABLE);

		SDL_FreeSurface (sdl_surf);
		sdl_surf = NULL;
	}

	if (rowtab != NULL)
	{
		free (rowtab);
		rowtab = NULL;
	}

	SDL_Quit ();
}


void
Quit (void)
{
	Shutdown ();
	exit (EXIT_SUCCESS);
}


void
Init (void)
{
	Uint32 flags;
	int y;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		fprintf (stderr, "ERROR: SDL init failed\n");
		exit (EXIT_FAILURE);
	}

	flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_HWPALETTE;
	if ((sdl_surf = SDL_SetVideoMode(W, H, BPP, flags)) == NULL)
	{
		fprintf (stderr, "ERROR: failed setting video mode\n");
		Quit ();
	}

	rowtab = malloc (H * sizeof(*rowtab));

	for (y = 0; y < H; y++)
		rowtab[y] = (PIXTYPE *)((uintptr_t)sdl_surf->pixels + y * sdl_surf->pitch);

	printf ("Set %dx%dx%d video mode\n", sdl_surf->w, sdl_surf->h, sdl_surf->format->BytesPerPixel * 8);

	/* ================== */

	cam.center_x = W / 2.0;
	cam.center_y = H / 2.0;

	cam.fov_x = FOV_X * (M_PI / 180.0);
	cam.dist = (W / 2.0) / tan(cam.fov_x / 2.0);
	cam.fov_y = 2.0 * atan((H / 2.0) / cam.dist);

	Vec_Clear (cam.pos);
	Vec_Clear (cam.angles);
}

static int grabbed = 0;
static int ignore_mousemove = 1;

void
FetchInput (void)
{
	SDL_Event sdlev;

	in.mouse_delta[0] = 0;
	in.mouse_delta[1] = 0;

	memset (in.key.press, 0, sizeof(in.key.press));
	memset (in.key.release, 0, sizeof(in.key.release));

	while (SDL_PollEvent(&sdlev))
	{
		switch (sdlev.type)
		{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
			{
				int k = sdlev.key.keysym.sym;
				if (k >= 0 && k < sizeof(in.key.state) / sizeof(in.key.state[0]))
				{
					if (sdlev.type == SDL_KEYDOWN)
					{
						in.key.state[k] = 1;
						in.key.press[k] = 1;
					}
					else
					{
						in.key.state[k] = 0;
						in.key.release[k] = 1;
					}
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (grabbed)
				{
					if (ignore_mousemove == 0)
					{
						in.mouse_delta[0] = sdlev.motion.xrel;
						in.mouse_delta[1] = sdlev.motion.yrel;
					}
					else
						ignore_mousemove--;
				}
				break;
			}

			case SDL_QUIT:
				Quit ();
				break;

			default:
				break;
		}
	}
}


void
SetGrab (int grab)
{
	if (grab && !grabbed)
	{
		SDL_WM_GrabInput (SDL_GRAB_ON);
		SDL_ShowCursor (SDL_DISABLE);
		grabbed = 1;
		ignore_mousemove = 1;
	}
	else if (!grab && grabbed)
	{
		SDL_WM_GrabInput (SDL_GRAB_OFF);
		SDL_ShowCursor (SDL_ENABLE);
		grabbed = 0;
		ignore_mousemove = 1;
	}
}


void
ToggleGrab (void)
{
	SetGrab (!grabbed);
}


void
UpdateCamera (void)
{
	int left, forward;
	float v[2];

	/* movement */

	left = 0;
	left += in.key.state[bind_left] ? 1 : 0;
	left -= in.key.state[bind_right] ? 1 : 0;

	Vec_Copy (cam.left, v);
	Vec_Scale (v, left * FLYSPEED * frametime);
	Vec_Add (cam.pos, v, cam.pos);

	forward = 0;
	forward += in.key.state[bind_forward] ? 1 : 0;
	forward -= in.key.state[bind_back] ? 1 : 0;

	Vec_Copy (cam.forward, v);
	Vec_Scale (v, forward * FLYSPEED * frametime);
	Vec_Add (cam.pos, v, cam.pos);

	/* camera angle */

	cam.angles[YAW] += -in.mouse_delta[0] * (cam.fov_x / W);
	while (cam.angles[YAW] >= 2.0 * M_PI)
		cam.angles[YAW] -= 2.0 * M_PI;
	while (cam.angles[YAW] < 0.0)
		cam.angles[YAW] += 2.0 * M_PI;

	cam.angles[PITCH] += in.mouse_delta[1] * (cam.fov_y / H);
	if (cam.angles[PITCH] > M_PI / 2.0)
		cam.angles[PITCH] = M_PI / 2.0;
	if (cam.angles[PITCH] < -M_PI / 2.0)
		cam.angles[PITCH] = -M_PI / 2.0;
}


void
RunInput (void)
{
	FetchInput ();

	if (in.key.release[27])
		Quit ();

	if (in.key.release['g'])
		ToggleGrab ();

	UpdateCamera ();
}

/* ========================================================== */

void
CalcCamera (void)
{
	float cam2world[3][3];
	float v[3];
	struct viewplane_s *p;
	float ang;

	/* make transformation matrix */
	Vec_Copy (cam.angles, v);
	Vec_Scale (v, -1.0);
	Vec_AnglesMatrix (v, cam.xform, "xyz");

	/* get view vectors */
	Vec_Copy (cam.xform[0], cam.left);
	Vec_Copy (cam.xform[1], cam.up);
	Vec_Copy (cam.xform[2], cam.forward);

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
}


void
DrawScene (void)
{
	CalcCamera ();

	//...
}

/* ========================================================== */

void
Refresh (void)
{
	int y;

	if (SDL_MUSTLOCK(sdl_surf))
	{
		if (SDL_LockSurface(sdl_surf) != 0)
		{
			fprintf (stderr, "ERROR: surface lock failed\n");
			Quit ();
		}
	}

	for (y = 0; y < H; y++)
		memset (rowtab[y], 0x0, W * sizeof(*rowtab[y]));

	DrawScene ();

	if (SDL_MUSTLOCK(sdl_surf))
		SDL_UnlockSurface (sdl_surf);

	SDL_Flip (sdl_surf);
}


int
main (int argc, const char **argv)
{
	Uint32 last, duration, now;

	Init ();

	last = SDL_GetTicks ();
	for (;;)
	{
		do
		{
			now = SDL_GetTicks ();
			duration = now - last;
		} while (duration < 1);
		frametime = duration / 1000.0;
		last = now;

		RunInput ();
		Refresh ();
	}

	return 0;
}
