#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <SDL.h>

#include "vec.h"
#include "render.h"
#include "tmap2.h"

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

struct cam_s cam;
struct input_s in;

static SDL_Surface *sdl_surf = NULL;

pixel_t **rowtab = NULL;

float frametime;

struct
{
	float rate;
	int framecount;
	Uint32 calc_start;
} fps;


static void
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


static void
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
	if ((sdl_surf = SDL_SetVideoMode(WIDTH, HEIGHT, BPP, flags)) == NULL)
	{
		fprintf (stderr, "ERROR: failed setting video mode\n");
		Quit ();
	}

	rowtab = malloc (HEIGHT * sizeof(*rowtab));

	for (y = 0; y < HEIGHT; y++)
		rowtab[y] = (pixel_t *)((uintptr_t)sdl_surf->pixels + y * sdl_surf->pitch);

	printf ("Set %dx%dx%d video mode\n", sdl_surf->w, sdl_surf->h, sdl_surf->format->BytesPerPixel * 8);

	/* ================== */

	cam.center_x = WIDTH / 2.0;
	cam.center_y = HEIGHT / 2.0;

	cam.fov_x = FOV_X * (M_PI / 180.0);
	cam.dist = (WIDTH / 2.0) / tan(cam.fov_x / 2.0);
	cam.fov_y = 2.0 * atan((HEIGHT / 2.0) / cam.dist);

	Vec_Clear (cam.pos);
	Vec_Clear (cam.angles);
}


static int _mouse_grabbed = 0;
static int _mouse_ignore_move = 1;

static void
FetchInput (void)
{
	SDL_Event sdlev;

	memset (in.key.press, 0, sizeof(in.key.press));
	memset (in.key.release, 0, sizeof(in.key.release));

	in.mouse.delta[0] = 0;
	in.mouse.delta[1] = 0;
	memset (in.mouse.button.press, 0, sizeof(in.mouse.button.press));
	memset (in.mouse.button.release, 0, sizeof(in.mouse.button.release));

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
				int b = -1;
				switch (sdlev.button.button)
				{
					case 1: b = MBUTTON_LEFT; break;
					case 2: b = MBUTTON_MIDDLE; break;
					case 3: b = MBUTTON_RIGHT; break;
					case 4: b = MBUTTON_WHEEL_UP; break;
					case 5: b = MBUTTON_WHEEL_DOWN; break;
					default: break;
				}

				if (b != -1)
				{
					if (sdlev.type == SDL_MOUSEBUTTONDOWN)
					{
						in.mouse.button.state[b] = 1;
						in.mouse.button.press[b] = 1;
					}
					else
					{
						in.mouse.button.state[b] = 0;
						in.mouse.button.release[b] = 1;
					}
				}

				break;
			}

			case SDL_MOUSEMOTION:
			{
				if (_mouse_grabbed)
				{
					/* when grabbing the mouse, the initial
					 * delta we get can be HUGE, so ignore
					 * the first mouse movement after grabbing
					 * input */
					if (_mouse_ignore_move == 0)
					{
						in.mouse.delta[0] = sdlev.motion.xrel;
						in.mouse.delta[1] = sdlev.motion.yrel;
					}
					else
						_mouse_ignore_move--;
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


static void
SetGrab (int grab)
{
	if (grab && !_mouse_grabbed)
	{
		SDL_WM_GrabInput (SDL_GRAB_ON);
		SDL_ShowCursor (SDL_DISABLE);
		_mouse_grabbed = 1;
		_mouse_ignore_move = 1;
	}
	else if (!grab && _mouse_grabbed)
	{
		SDL_WM_GrabInput (SDL_GRAB_OFF);
		SDL_ShowCursor (SDL_ENABLE);
		_mouse_grabbed = 0;
		_mouse_ignore_move = 1;
	}
}


static void
ToggleGrab (void)
{
	SetGrab (!_mouse_grabbed);
}


static void
UpdateCamera (void)
{
	int left, forward, up;
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

	up = 0;
	up += in.mouse.button.state[MBUTTON_RIGHT];
	Vec_Copy (cam.up, v);
	Vec_Scale (v, up * FLYSPEED * frametime);
	Vec_Add (cam.pos, v, cam.pos);

	/* camera angle */

	cam.angles[YAW] += -in.mouse.delta[0] * (cam.fov_x / WIDTH);
	while (cam.angles[YAW] >= 2.0 * M_PI)
		cam.angles[YAW] -= 2.0 * M_PI;
	while (cam.angles[YAW] < 0.0)
		cam.angles[YAW] += 2.0 * M_PI;

	cam.angles[PITCH] += in.mouse.delta[1] * (cam.fov_y / HEIGHT);
	if (cam.angles[PITCH] > M_PI / 2.0)
		cam.angles[PITCH] = M_PI / 2.0;
	if (cam.angles[PITCH] < -M_PI / 2.0)
		cam.angles[PITCH] = -M_PI / 2.0;
}


static void
RunInput (void)
{
	FetchInput ();

	if (in.key.release[27])
		Quit ();

	if (in.key.release['g'])
		ToggleGrab ();

	if (in.key.release['f'])
		printf ("%g\n", fps.rate);

	UpdateCamera ();
}

/* ========================================================== */

static void
Refresh (void)
{
	int y;

	/* do as much as we can _before_ touching the frame buffer */
	R_DrawScene ();

	if (SDL_MUSTLOCK(sdl_surf))
	{
		if (SDL_LockSurface(sdl_surf) != 0)
		{
			fprintf (stderr, "ERROR: surface lock failed\n");
			Quit ();
		}
	}

	for (y = 0; y < HEIGHT; y++)
		memset (rowtab[y], 0x0, WIDTH * sizeof(*rowtab[y]));

	/* finally, draw pixels out */
	R_RenderPolySpans ();

	if (SDL_MUSTLOCK(sdl_surf))
		SDL_UnlockSurface (sdl_surf);

	SDL_Flip (sdl_surf);

	fps.framecount++;

	/* calculate the framerate */
	{
		Uint32 now = SDL_GetTicks();

		if ((now - fps.calc_start) > 250)
		{
			fps.rate = fps.framecount / ((now - fps.calc_start) / 1000.0);
			fps.framecount = 0;
			fps.calc_start = now;
		}
	}
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
