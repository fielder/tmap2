#ifndef __VEC_H__
#define __VEC_H__

#ifndef M_PI
#define M_PI 3.141592653589793238462643383279
#endif /* M_PI */

#define DEG2RAD(_X) ((_X) * (M_PI / 180.0))
#define RAD2DEG(_X) ((_X) * (180.0 / M_PI))

extern void
Vec_Clear (float v[3]);

extern void
Vec_Copy (const float src[3], float out[3]);

extern void
Vec_Scale (float v[3], float s);

extern void
Vec_Add (const float a[3], const float b[3], float out[3]);

extern void
Vec_Subtract (const float a[3], const float b[3], float out[3]);

extern float
Vec_Dot (const float a[3], const float b[3]);

extern void
Vec_Cross (const float a[3], const float b[3], float out[3]);

extern void
Vec_Normalize (float v[3]);

extern float
Vec_Length (const float v[3]);

extern void
Vec_Transform (float xform[3][3], const float v[3], float out[3]);

extern void
Vec_MakeNormal (const float v1[3],
		const float v2[3],
		const float v3[3],
		float normal[3],
		float *dist);

extern void
Vec_IdentityMatrix (float mat[3][3]);

extern void
Vec_MultMatrix (float a[3][3], float b[3][3], float out[3][3]);

extern void
Vec_AnglesMatrix (const float angles[3], float out[3][3], const char *order);

#endif /* __VEC_H__ */
