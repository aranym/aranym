/* 
 * A 16 bit graphics blit routine, by Johan Klockars.
 *
 * This file is an example of how to write an
 * fVDI device driver routine in C.
 *
 * You are encouraged to use this file as a starting point
 * for other accelerated features, or even for supporting
 * other graphics modes. This file is therefore put in the
 * public domain. It's not copyrighted or under any sort
 * of license.
 */

#if 1
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

#include "fvdi.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

#define DEBUG

#ifdef DEBUG
#include "relocate.h"
extern short debug;
extern Access *access;
extern char err_msg[];

void debug_out(char *text1, int w, int old_w, int h, int src_x, int src_y, int dst_x, int dst_y)
{
	char buf[10];
	access->funcs.puts(text1);
	access->funcs.ltoa(buf, w, 10);
	access->funcs.puts(buf);
	if (old_w > 0) {
		access->funcs.puts("(");
		access->funcs.ltoa(buf, old_w, 10);
		access->funcs.puts(buf);
		access->funcs.puts(")");
	}
	access->funcs.puts(",");
	access->funcs.ltoa(buf, h, 10);
	access->funcs.puts(buf);
	access->funcs.puts(" from ");
	access->funcs.ltoa(buf, src_x, 10);
	access->funcs.puts(buf);
	access->funcs.puts(",");
	access->funcs.ltoa(buf, src_y, 10);
	access->funcs.puts(buf);
	access->funcs.puts(" to ");
	access->funcs.ltoa(buf, dst_x, 10);
	access->funcs.puts(buf);
	access->funcs.puts(",");
	access->funcs.ltoa(buf, dst_y, 10);
	access->funcs.puts(buf);
	access->funcs.puts("\x0d\x0a");
}
#endif


/*
 * Make it as easy as possible for the C compiler.
 * The current code is written to produce reasonable results with Lattice C.
 * (long integers, optimize: [x xx] time)
 * - One function for each operation -> more free registers
 * - 'int' is the default type
 * - some compilers aren't very smart when it comes to *, / and %
 * - some compilers can't deal well with *var++ constructs
 */

#ifdef BOTH
static void
s_blit_copy(PIXEL *src_addr, int src_line_add,
            PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
            int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *src_addr++;
#ifdef BOTH
			*(volatile PIXEL *)dst_addr_fast++ = v, 0;   /* Silly compiler... */
#endif
			*dst_addr++ = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
s_blit_or(PIXEL *src_addr, int src_line_add,
          PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
          int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *src_addr++;
#ifdef BOTH
			v |= *dst_addr_fast;
			*(volatile PIXEL *)dst_addr_fast++ = v, 0;   /* Silly compiler... */
			*dst_addr++ = v;
#else
			*dst_addr++ |= v;
#endif
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
s_blit(PIXEL *src_addr, int src_line_add,
       PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
       int w, int h, int operation)
{
	int i, j;
	PIXEL v, vs, vd;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			vs = *src_addr++;
#ifdef BOTH
			vd = *dst_addr_fast;
#else
			vd = *dst_addr;
#endif
			switch(operation) {
			case 0:
				v = 0;
				break;
			case 1:
				v = vs & vd;
				break;
			case 2:
				v = vs & ~vd;
				break;
			case 3:
				v = vs;
				break;
			case 4:
				v = ~vs & vd;
				break;
			case 5:
				v = vd;
				break;
			case 6:
				v = vs ^ vd;
				break;
			case 7:
				v = vs | vd;
				break;
			case 8:
				v = ~(vs | vd);
				break;
			case 9:
				v = ~(vs ^ vd);
				break;
			case 10:
				v = ~vd;
				break;
			case 11:
				v = vs | ~vd;
				break;
			case 12:
				v = ~vs;
				break;
			case 13:
				v = ~vs | vd;
				break;
			case 14:
				v = ~(vs & vd);
				break;
			case 15:
				v = 0xffff;
				break;
			}
#ifdef BOTH
			*(volatile PIXEL *)dst_addr_fast++ = v, 0;   /* Silly compiler... */
#endif
			*dst_addr++ = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
s_pan_backwards_copy(PIXEL *src_addr, int src_line_add,
                     PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                     int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *--src_addr;
#ifdef BOTH
			*(volatile PIXEL *)--dst_addr_fast = v, 0;   /* Silly compiler... */
#endif
			*--dst_addr = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
s_pan_backwards_or(PIXEL *src_addr, int src_line_add,
                   PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                   int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *--src_addr;
#ifdef BOTH
			v |= *--dst_addr_fast;
			*(volatile PIXEL *)dst_addr_fast = v, 0;   /* Silly compiler... */
			*--dst_addr = v;
#else
			*--dst_addr |= v;
#endif
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
s_pan_backwards(PIXEL *src_addr, int src_line_add,
                PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                int w, int h, int operation)
{
	int i, j;
	PIXEL v, vs, vd;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			vs = *--src_addr;
#ifdef BOTH
			vd = *--dst_addr_fast;
#else
			vd = *--dst_addr;
#endif
			switch(operation) {
			case 0:
				v = 0;
				break;
			case 1:
				v = vs & vd;
				break;
			case 2:
				v = vs & ~vd;
				break;
			case 3:
				v = vs;
				break;
			case 4:
				v = ~vs & vd;
				break;
			case 5:
				v = vd;
				break;
			case 6:
				v = vs ^ vd;
				break;
			case 7:
				v = vs | vd;
				break;
			case 8:
				v = ~(vs | vd);
				break;
			case 9:
				v = ~(vs ^ vd);
				break;
			case 10:
				v = ~vd;
				break;
			case 11:
				v = vs | ~vd;
				break;
			case 12:
				v = ~vs;
				break;
			case 13:
				v = ~vs | vd;
				break;
			case 14:
				v = ~(vs & vd);
				break;
			case 15:
				v = 0xffff;
				break;
			}
#ifdef BOTH
			*(volatile PIXEL *)dst_addr_fast = v, 0;   /* Silly compiler... */
#endif
			*dst_addr = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}

#define BOTH_WAS_ON
#endif
#undef BOTH

/*
 * The functions below are exact copies of those above.
 * The '#undef BOTH' makes sure that this works as it should
 * when no shadow buffer is available
 */

static void
blit_copy(PIXEL *src_addr, int src_line_add,
          PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
          int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *src_addr++;
#ifdef BOTH
			*dst_addr_fast++ = v;
#endif
			*dst_addr++ = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
blit_or(PIXEL *src_addr, int src_line_add,
        PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
        int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *src_addr++;
#ifdef BOTH
			v |= *dst_addr_fast;
			*dst_addr_fast++ = v;
			*dst_addr++ = v;
#else
			*dst_addr++ |= v;
#endif
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
blit(PIXEL *src_addr, int src_line_add,
     PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
     int w, int h, int operation)
{
	int i, j;
	PIXEL v, vs, vd;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			vs = *src_addr++;
#ifdef BOTH
			vd = *dst_addr_fast;
#else
			vd = *dst_addr;
#endif
			switch(operation) {
			case 0:
				v = 0;
				break;
			case 1:
				v = vs & vd;
				break;
			case 2:
				v = vs & ~vd;
				break;
			case 3:
				v = vs;
				break;
			case 4:
				v = ~vs & vd;
				break;
			case 5:
				v = vd;
				break;
			case 6:
				v = vs ^ vd;
				break;
			case 7:
				v = vs | vd;
				break;
			case 8:
				v = ~(vs | vd);
				break;
			case 9:
				v = ~(vs ^ vd);
				break;
			case 10:
				v = ~vd;
				break;
			case 11:
				v = vs | ~vd;
				break;
			case 12:
				v = ~vs;
				break;
			case 13:
				v = ~vs | vd;
				break;
			case 14:
				v = ~(vs & vd);
				break;
			case 15:
				v = 0xffff;
				break;
			}
#ifdef BOTH
			*dst_addr_fast++ = v;
#endif
			*dst_addr++ = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
pan_backwards_copy(PIXEL *src_addr, int src_line_add,
                   PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                   int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *--src_addr;
#ifdef BOTH
			*--dst_addr_fast = v;
#endif
			*--dst_addr = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
pan_backwards_or(PIXEL *src_addr, int src_line_add,
                 PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                 int w, int h)
{
	int i, j;
	PIXEL v;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			v = *--src_addr;
#ifdef BOTH
			v |= *--dst_addr_fast;
			*dst_addr_fast = v;
			*--dst_addr = v;
#else
			*--dst_addr |= v;
#endif
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


static void
pan_backwards(PIXEL *src_addr, int src_line_add,
              PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
              int w, int h, int operation)
{
	int i, j;
	PIXEL v, vs, vd;
	
	for(i = h - 1; i >= 0; i--) {
		for(j = w - 1; j >= 0; j--) {
			vs = *--src_addr;
#ifdef BOTH
			vd = *--dst_addr_fast;
#else
			vd = *--dst_addr;
#endif
			switch(operation) {
			case 0:
				v = 0;
				break;
			case 1:
				v = vs & vd;
				break;
			case 2:
				v = vs & ~vd;
				break;
			case 3:
				v = vs;
				break;
			case 4:
				v = ~vs & vd;
				break;
			case 5:
				v = vd;
				break;
			case 6:
				v = vs ^ vd;
				break;
			case 7:
				v = vs | vd;
				break;
			case 8:
				v = ~(vs | vd);
				break;
			case 9:
				v = ~(vs ^ vd);
				break;
			case 10:
				v = ~vd;
				break;
			case 11:
				v = vs | ~vd;
				break;
			case 12:
				v = ~vs;
				break;
			case 13:
				v = ~vs | vd;
				break;
			case 14:
				v = ~(vs & vd);
				break;
			case 15:
				v = 0xffff;
				break;
			}
#ifdef BOTH
			*dst_addr_fast = v;
#endif
			*dst_addr = v;
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}


#ifdef BOTH_WAS_ON
#define BOTH
#endif


long CDECL
c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
            MFDB *dst, long dst_x, long dst_y,
            long w, long h, long operation)
{
	Workstation *wk;
	PIXEL *src_addr, *dst_addr, *dst_addr_fast;
	int src_wrap, dst_wrap;
	int src_line_add, dst_line_add;
	unsigned long src_pos, dst_pos;
	int from_screen, to_screen;

	wk = vwk->real_address;

	from_screen = 0;
	if (!src || !src->address || (src->address == wk->screen.mfdb.address)) {		/* From screen? */
		src_wrap = wk->screen.wrap;
		if (!(src_addr = wk->screen.shadow.address))
			src_addr = wk->screen.mfdb.address;
		from_screen = 1;
	} else {
		src_wrap = (long)src->wdwidth * 2 * src->bitplanes;
		src_addr = src->address;
	}
	src_pos = (short)src_y * (long)src_wrap + src_x * PIXEL_SIZE;
	src_line_add = src_wrap - w * PIXEL_SIZE;

	to_screen = 0;
	if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address)) {		/* To screen? */
		dst_wrap = wk->screen.wrap;
		dst_addr = wk->screen.mfdb.address;
		to_screen = 1;
	} else {
		dst_wrap = (long)dst->wdwidth * 2 * dst->bitplanes;
		dst_addr = dst->address;
	}
	dst_pos = (short)dst_y * (long)dst_wrap + dst_x * PIXEL_SIZE;
	dst_line_add = dst_wrap - w * PIXEL_SIZE;

	if (src_y < dst_y) {
		src_pos += (short)(h - 1) * (long)src_wrap;
		src_line_add -= src_wrap * 2;
		dst_pos += (short)(h - 1) * (long)dst_wrap;
		dst_line_add -= dst_wrap * 2;
	}

	src_addr += src_pos / PIXEL_SIZE;
	dst_addr += dst_pos / PIXEL_SIZE;
	src_line_add /= PIXEL_SIZE;		/* Change into pixel count */
	dst_line_add /= PIXEL_SIZE;

	dst_addr_fast = wk->screen.shadow.address;	/* May not really be to screen at all, but... */

#ifdef DEBUG
	if (debug > 1) {
		debug_out("Blitting: ", w, -1, h, src_x, src_y, dst_x, dst_y);
	}
#endif

#ifdef BOTH
	if (!to_screen || !dst_addr_fast) {
#endif
		if ((src_y == dst_y) && (src_x < dst_x)) {
			src_addr += w;		/* To take backward copy into account */
			dst_addr += w;
			src_line_add += 2 * w;
			dst_line_add += 2 * w;
			switch(operation) {
			case 3:
				pan_backwards_copy(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
				break;
			case 7:
				pan_backwards_or(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
				break;
			default:
				pan_backwards(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h, operation);
				break;
			}
		} else {
			switch(operation) {
			case 3:
				blit_copy(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
				break;
			case 7:
				blit_or(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
				break;
			default:
				blit(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h, operation);
				break;
			}
		}
#ifdef BOTH
	} else {
		dst_addr_fast += dst_pos / PIXEL_SIZE;
		if ((src_y == dst_y) && (src_x < dst_x)) {
			src_addr += w;		/* To take backward copy into account */
			dst_addr += w;
			dst_addr_fast += w;
			src_line_add += 2 * w;
			dst_line_add += 2 * w;
			switch(operation) {
			case 3:
				s_pan_backwards_copy(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, w, h);
				break;
			case 7:
				s_pan_backwards_or(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, w, h);
				break;
			default:
				s_pan_backwards(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, w, h, operation);
				break;
			}
		} else {
			switch(operation) {
			case 3:
				s_blit_copy(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, w, h);
				break;
			case 7:
				s_blit_or(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, w, h);
				break;
			default:
				s_blit(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, w, h, operation);
				break;
			}
		}
	}
#endif
	return 1;	/* Return as completed */
}
