#ifndef __TMAP2_H__
#define __TMAP2_H__

#include <stdint.h>
#include <SDL_keysym.h>

typedef uint16_t pixel_t;

enum
{
	VPLANE_LEFT,
	VPLANE_RIGHT,
	VPLANE_TOP,
	VPLANE_BOTTOM,
	VPLANE_NEAR,
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

struct camera_s
{
	float center_x;
	float center_y;

	float fov_x; /* radians */
	float fov_y; /* radians */
	float dist;

	float pos[3];
	float angles[3]; /* radians */

	float forward[3];
	float left[3];
	float up[3];

	float xform[3][3]; /* world-to-camera */

	struct viewplane_s vplanes[5];
};

enum
{
	MBUTTON_LEFT,
	MBUTTON_MIDDLE,
	MBUTTON_RIGHT,
	MBUTTON_WHEEL_UP,
	MBUTTON_WHEEL_DOWN,
	MBUTTON_LAST,
};

struct input_s
{
	struct
	{
		int delta[2];
		struct
		{
			int state[MBUTTON_LAST];
			int press[MBUTTON_LAST];
			int release[MBUTTON_LAST];
		} button;
	} mouse;

	struct
	{
		int state[SDLK_LAST];
		int press[SDLK_LAST];
		int release[SDLK_LAST];
	} key;
};

struct video_s
{
	int w, h;
	pixel_t *bouncebuf;
	pixel_t **rows;
	int scale;
	pixel_t red, green, blue;
};

extern struct camera_s camera;
extern struct input_s input;
extern struct video_s video;

extern void
Quit (void);

#endif /* __TMAP2_H__ */
