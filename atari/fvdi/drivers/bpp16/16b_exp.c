/* 
 * A 16 bit graphics mono-expand routine, by Johan Klockars.
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

extern void CDECL c_get_colour(Virtual *vwk, long colour, short *foreground, short *background);

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
static void s_replace(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (expand_word & mask) {
#ifdef BOTH
				*dst_addr_fast++ = foreground;
#endif
				*dst_addr++ = foreground;
			} else {
#ifdef BOTH
				*dst_addr_fast++ = background;
#endif
				*dst_addr++ = background;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}

static void s_transparent(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (expand_word & mask) {
#ifdef BOTH
				*dst_addr_fast++ = foreground;
#endif
				*dst_addr++ = foreground;
			} else {
#ifdef BOTH
				dst_addr_fast++;
#endif
				dst_addr++;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}

static void s_xor(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j, v;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (expand_word & mask) {
#ifdef BOTH
				v = ~*dst_addr_fast;
#else
				v = ~*dst_addr;
#endif
#ifdef BOTH
				*dst_addr++ = v;
#endif
				*dst_addr++ = v;
			} else {
#ifdef BOTH
				dst_addr_fast++;
#endif
				dst_addr++;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}

static void s_revtransp(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (!(expand_word & mask)) {
#ifdef BOTH
				*dst_addr_fast++ = foreground;
#endif
				*dst_addr++ = foreground;
			} else {
#ifdef BOTH
				dst_addr_fast++;
#endif
				dst_addr++;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
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

static void replace(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (expand_word & mask) {
#ifdef BOTH
				*dst_addr_fast++ = foreground;
#endif
				*dst_addr++ = foreground;
			} else {
#ifdef BOTH
				*dst_addr_fast++ = background;
#endif
				*dst_addr++ = background;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}

static void transparent(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (expand_word & mask) {
#ifdef BOTH
				*dst_addr_fast++ = foreground;
#endif
				*dst_addr++ = foreground;
			} else {
#ifdef BOTH
				dst_addr_fast++;
#endif
				dst_addr++;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}

static void xor(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j, v;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (expand_word & mask) {
#ifdef BOTH
				v = ~*dst_addr_fast;
#else
				v = ~*dst_addr;
#endif
#ifdef BOTH
				*dst_addr++ = v;
#endif
				*dst_addr++ = v;
			} else {
#ifdef BOTH
				dst_addr_fast++;
#endif
				dst_addr++;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
		}
		src_addr += src_line_add;
		dst_addr += dst_line_add;
#ifdef BOTH
		dst_addr_fast += dst_line_add;
#endif
	}
}

static void revtransp(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
	int i, j;
	unsigned int expand_word, mask;

	x = 1 << (15 - (x & 0x000f));

	for(i = h - 1; i >= 0; i--) {
		expand_word = *src_addr++;
		mask = x;
		for(j = w - 1; j >= 0; j--) {
			if (!(expand_word & mask)) {
#ifdef BOTH
				*dst_addr_fast++ = foreground;
#endif
				*dst_addr++ = foreground;
			} else {
#ifdef BOTH
				dst_addr_fast++;
#endif
				dst_addr++;
			}
			if (!(mask >>= 1)) {
				mask = 0x8000;
				expand_word = *src_addr++;
			}
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

long CDECL c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour)
{
	Workstation *wk;
	PIXEL *src_addr, *dst_addr, *dst_addr_fast;
	short foreground, background;
	int src_wrap, dst_wrap;
	int src_line_add, dst_line_add;
	unsigned long src_pos, dst_pos;
	int to_screen;

	wk = vwk->real_address;

	c_get_colour(vwk, colour, &foreground, &background);

	src_wrap = (long)src->wdwidth * 2;		/* Always monochrome */
	src_addr = src->address;
	src_pos = (short)src_y * (long)src_wrap + (src_x >> 4) * 2;
	src_line_add = src_wrap - (((src_x + w) >> 4) - (src_x >> 4) + 1) * 2;

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

	src_addr += src_pos / 2;
	dst_addr += dst_pos / PIXEL_SIZE;
	src_line_add /= 2;
	dst_line_add /= PIXEL_SIZE;			/* Change into pixel count */

	dst_addr_fast = wk->screen.shadow.address;	/* May not really be to screen at all, but... */

#ifdef BOTH
	if (!to_screen || !dst_addr_fast) {
#endif
		switch (operation) {
		case 1:				/* Replace */
			replace(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
			break;
		case 2:				/* Transparent */
			transparent(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
			break;
		case 3:				/* XOR */
			xor(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
			break;
		case 4:				/* Reverse transparent */
			revtransp(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
			break;
		}
#ifdef BOTH
	} else {
		dst_addr_fast += dst_pos / PIXEL_SIZE;
		switch (operation) {
		case 1:				/* Replace */
			s_replace(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
			break;
		case 2:				/* Transparent */
			s_transparent(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
			break;
		case 3:				/* XOR */
			s_xor(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
			break;
		case 4:				/* Reverse transparent */
			s_revtransp(src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
			break;
		}
	}
#endif
	return 1;		/* Return as completed */
}
