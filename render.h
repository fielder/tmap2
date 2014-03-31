#ifndef __RENDER_H__
#define __RENDER_H__

extern int r_showtex;
extern int r_debugframe;

extern void
R_Init (void);

extern void
R_Shutdown (void);

extern void
R_CalcViewXForm (void);

extern void
R_DrawScene (void);

extern void
R_RenderScene (void);

#endif /* __RENDER_H__ */
