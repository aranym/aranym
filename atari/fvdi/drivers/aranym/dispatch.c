/*
 *	Drawing function dispatch
 */

#include "fvdi.h"

/* NF API constants */
#include "fvdidrv_nfapi.h"


/* from ../common/. */
extern Driver *me;

/* from spec.c */
extern void CDECL (*get_colours_r)(Virtual *vwk, long colour, long *foreground, long *background);

/* NF fVDI ID value */
static long NF_fVDI;


/* general NatFeat stuff */
static long NF_getid = 0x73004e75L;
static long NF_call  = 0x73014e75L;

#define nfGetID(n)	(((long CDECL (*)(const char *))&NF_getid)n)
#define nfCall(n)	(((long CDECL (*)(long, ...))&NF_call)n)
#define ARAnyM(n)	nfCall(n)


int
nf_initialize()
{
	if (!(NF_fVDI = nfGetID(("fVDI"))))
		return 0;
	if (ARAnyM((NF_fVDI+FVDI_GET_VERSION)) != FVDIDRV_NFAPI_VERSION)
		return 0;
	return 1;
}


static MFDB*
simplify(Virtual* vwk, MFDB* mfdb)
{
   vwk = (Virtual *)((long)vwk & ~1);
   if (!mfdb)
      return 0;
   else if (!mfdb->address)
      return 0;
   else if (mfdb->address == vwk->real_address->screen.mfdb.address)
      return 0;
#if 0
   else if (mfdb->address == old_screen)
      return 0;
#endif
   else
      return mfdb;
}


static long*
clipping(Virtual *vwk, long *rect)
{
   vwk = (Virtual *)((long)vwk & ~1);
   if (!vwk->clip.on)
      return 0;
   
   rect[0] = vwk->clip.rectangle.x1;
   rect[1] = vwk->clip.rectangle.y1;
   rect[2] = vwk->clip.rectangle.x2;
   rect[3] = vwk->clip.rectangle.y2;
   
   return rect;
}


long CDECL
c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y)
{
   return ARAnyM((NF_fVDI+FVDI_GET_PIXEL, vwk, simplify(vwk, mfdb), x, y));
}


long CDECL
c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour)
{
   return ARAnyM((NF_fVDI+FVDI_PUT_PIXEL, vwk, simplify(vwk, mfdb), x, y, colour));
}

long CDECL
c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse)
{
   if ((long)mouse > 3) {
   	long foreground;
   	long background;
   	get_colours_r(me->default_vwk, *(long*)&mouse->colour, &foreground, &background);

        return ARAnyM((NF_fVDI+FVDI_MOUSE, wk, x, y, &mouse->mask, &mouse->data,
                      (long)mouse->hotspot.x, (long)mouse->hotspot.y,
                      foreground, background, (long)mouse->type));
   } else {
      /* Why is the cast needed for Lattice C? */
      return ARAnyM((NF_fVDI+FVDI_MOUSE, wk, x, y, (long)mouse));
   }
}

long CDECL
c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
              MFDB *dst, long dst_x, long dst_y, long w, long h,
              long operation, long colour)
{
   long foreground;
   long background;
   get_colours_r((Virtual *)((long)vwk & ~1), colour, &foreground, &background);

   return ARAnyM((NF_fVDI+FVDI_EXPAND_AREA, vwk, src, src_x, src_y, simplify(vwk, dst),
                  dst_x, dst_y, w, h, operation, foreground, background));
}

long CDECL
c_fill_area(Virtual *vwk, long x, long y, long w, long h, short *pattern,
            long colour, long mode, long interior_style)
{
   long foreground;
   long background;
   get_colours_r((Virtual *)((long)vwk & ~1), colour, &foreground, &background);

   return ARAnyM((NF_fVDI+FVDI_FILL_AREA, vwk, x, y, w, h, pattern,
                  foreground, background, mode,
	          interior_style));
}

long CDECL
c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst,
            long dst_x, long dst_y, long w, long h, long operation)
{
   return ARAnyM((NF_fVDI+FVDI_BLIT_AREA, vwk, simplify(vwk, src), src_x, src_y,
                  simplify(vwk, dst), dst_x, dst_y, w, h, operation));
}

long CDECL
c_line_draw(Virtual *vwk, long x1, long y1, long x2, long y2,
            long pattern, long colour, long mode)
{
   long rect[4];

   long foreground;
   long background;
   get_colours_r((Virtual *)((long)vwk & ~1), colour, &foreground, &background);

   /* Why is the cast needed for Lattice C? */   
   return ARAnyM((NF_fVDI+FVDI_LINE, vwk, x1, y1, x2, y2, pattern,
                  foreground, background, mode, (long)clipping(vwk, rect)));
}

long CDECL
c_fill_polygon(Virtual *vwk, short points[], long n,
               short index[], long moves, short *pattern,
               long colour, long mode, long interior_style)
{
   long rect[4];

   long foreground;
   long background;
   get_colours_r((Virtual *)((long)vwk & ~1), colour, &foreground, &background);

   /* Why is the cast needed for Lattice C? */
   return ARAnyM((NF_fVDI+FVDI_FILL_POLYGON, vwk, points, n, index, moves, pattern,
                  foreground, background,
                  mode, interior_style, (long)clipping(vwk, rect)));
}

long CDECL
c_set_colour(Virtual *vwk, long index, long red, long green, long blue)
{
   return ARAnyM((NF_fVDI+FVDI_SET_COLOR, index, red, green, blue, vwk));
}

long CDECL
c_get_hw_colour(short index, long red, long green, long blue, unsigned long *hw_value)
{
   return ARAnyM((NF_fVDI+FVDI_GET_HWCOLOR, (long)index, red, green, blue, hw_value));
}

long CDECL
c_set_resolution(long width, long height, long depth, long frequency)
{
   return ARAnyM((NF_fVDI+FVDI_SET_RESOLUTION, width, height, depth, frequency));
}

long CDECL
c_get_videoramaddress()
{
   return ARAnyM((NF_fVDI+FVDI_GET_FBADDR));
}

long CDECL
c_get_width(void)
{
   return ARAnyM((NF_fVDI+FVDI_GET_WIDTH));
}

long CDECL
c_get_height(void)
{
   return ARAnyM((NF_fVDI+FVDI_GET_HEIGHT));
}

long CDECL
c_openwk(void)
{
   return ARAnyM((NF_fVDI+FVDI_OPENWK));
}

long CDECL
c_closewk(void)
{
   return ARAnyM((NF_fVDI+FVDI_CLOSEWK));
}

