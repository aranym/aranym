/*
 *	Drawing function dispatch
 */

#include "fvdi.h"


#if 0
long ARAnyM_gfx = 0x71384e75L;    /* ARAnyM native graphics subrutines (M68K_EMUL_OP_VIDEO_CONTROL) */

#define ARAnyM(n) (((long CDECL (*)(long, ...))&ARAnyM_gfx)n)
#else
static long NF_getid = 0x73004e75L;
static long NF_call  = 0x73014e75L;
static long NF_fVDI;

#define nfVERSION	(NF_fVDI + 0)
#define nfGET_PIX	(NF_fVDI + 1)
#define nfSET_PIX	(NF_fVDI + 2)
#define nfMOUSE		(NF_fVDI + 3)
#define nfEXPAND	(NF_fVDI + 4)
#define nfFILL		(NF_fVDI + 5)
#define nfBLIT		(NF_fVDI + 6)
#define nfLINE		(NF_fVDI + 7)
#define nfFILLPOLY	(NF_fVDI + 8)
#define nfSET_COL	(NF_fVDI + 9)
#define nfSET_RES	(NF_fVDI + 10)
#define nfGET_FBADDR	(NF_fVDI + 15)
#define nfDEBUG	(NF_fVDI + 20)

#define nfGetID(n)	(((long CDECL (*)(const char *))&NF_getid)n)
#define nfCall(n)	(((long CDECL (*)(long, ...))&NF_call)n)
#define ARAnyM(n)	nfCall(n)

/* NatFeat functions defined */
#define nf_getName(buffer, size) \
	(((long CDECL (*)(long, char *, unsigned long))&NF_call) \
	(nfGetID(("NF_NAME")), (buffer), (unsigned long)(size)))

#define nf_getFullName(buffer, size) \
	(((long CDECL (*)(long, char *, unsigned long))&NF_call) \
	(nfGetID(("NF_NAME"))+1, (buffer), (unsigned long)(size)))

#define nf_getVersion() \
	(((long CDECL (*)(long, ...))&NF_call)(nfGetID(("NF_VERSION"))))
#endif


int
nf_initialize()
{
	if (!(NF_fVDI = nfGetID(("fVDI"))))
		return 0;
	if (ARAnyM((nfVERSION)) != 0x10000960)
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
   return ARAnyM((nfGET_PIX, vwk, simplify(vwk, mfdb), x, y));
}


long CDECL
c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour)
{
   return ARAnyM((nfSET_PIX, vwk, simplify(vwk, mfdb), x, y, colour));
}


long CDECL
c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse)
{
   if ((long)mouse > 3)
      return ARAnyM((nfMOUSE, wk, x, y, &mouse->mask, &mouse->data,
                     (long)mouse->hotspot.x, (long)mouse->hotspot.y,
                     *(long *)&mouse->colour, (long)mouse->type));
   else {
      /* Why is the cast needed for Lattice C? */
      return ARAnyM((nfMOUSE, wk, x, y, (long)mouse));
   }
}


long CDECL
c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
              MFDB *dst, long dst_x, long dst_y, long w, long h,
              long operation, long colour)
{
   return ARAnyM((nfEXPAND, vwk, src, src_x, src_y, simplify(vwk, dst),
                  dst_x, dst_y, w, h, operation, colour));
}


long CDECL
c_fill_area(Virtual *vwk, long x, long y, long w, long h, short *pattern,
            long colour, long mode, long interior_style)
{
   return ARAnyM((nfFILL, vwk, x, y, w, h, pattern, colour, mode, interior_style));
}


long CDECL
c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst,
            long dst_x, long dst_y, long w, long h, long operation)
{
   return ARAnyM((nfBLIT, vwk, simplify(vwk, src), src_x, src_y,
                  simplify(vwk, dst), dst_x, dst_y, w, h, operation));
}


long CDECL
c_line_draw(Virtual *vwk, long x1, long y1, long x2, long y2,
            long pattern, long colour, long mode)
{
   long rect[4];

   /* Why is the cast needed for Lattice C? */   
   return ARAnyM((nfLINE, vwk, x1, y1, x2, y2, pattern, colour, mode,
                  (long)clipping(vwk, rect)));
}


long CDECL
c_fill_polygon(Virtual *vwk, short points[], long n,
               short index[], long moves, short *pattern,
               long colour, long mode, long interior_style)
{
   long rect[4];

   /* Why is the cast needed for Lattice C? */
   return ARAnyM((nfFILLPOLY, vwk, points, n, index, moves, pattern, colour,
                  mode, interior_style, (long)clipping(vwk, rect)));
}


long CDECL
c_set_colour_hook(long index, long red, long green, long blue)
{
   return ARAnyM((nfSET_COL, index, red, green, blue));
}


long CDECL
c_set_resolution(long width, long height, long depth, long frequency)
{
   return ARAnyM((nfSET_RES, width, height, depth, frequency));
}


long CDECL
c_get_videoramaddress()
{
   return ARAnyM((nfGET_FBADDR));
}


long CDECL
c_debug_aranym(long n)
{
   return ARAnyM((nfDEBUG, n));
}


