/* 
 * A 16 bit graphics fill routine, by Johan Klockars.
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
static void s_replace(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0xffff:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				*addr_fast = foreground;
				addr_fast++;
#endif
				*addr = foreground;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (pattern_word & mask) {
#ifdef BOTH
					*addr_fast = foreground;
					addr_fast++;
#endif
					*addr = foreground;
					addr++;
				} else {
#ifdef BOTH
					*addr_fast = background;
					addr_fast++;
#endif
					*addr = background;
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
	}
}

static void s_transparent(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0xffff:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				*addr_fast = foreground;
				addr_fast++;
#endif
				*addr = foreground;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (pattern_word & mask) {
#ifdef BOTH
					*addr_fast = foreground;
					addr_fast++;
#endif
					*addr = foreground;
					addr++;
				} else {
#ifdef BOTH
					addr_fast++;
#endif
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
	}
}

static void s_xor(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask, v;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0xffff:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				v = ~*addr_fast;
#else
				v = ~*addr;
#endif
#ifdef BOTH
				*addr_fast = v;
				addr_fast++;
#endif
				*addr = v;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (pattern_word & mask) {
#ifdef BOTH
					v = ~*addr_fast;
#else
					v = ~*addr;
#endif
#ifdef BOTH
					*addr_fast = v;
					addr_fast++;
#endif
					*addr = v;
					addr++;
				} else {
#ifdef BOTH
					addr_fast++;
#endif
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
	}
}

static void s_revtransp(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0x0000:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				*addr_fast = foreground;
				addr_fast++;
#endif
				*addr = foreground;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (!(pattern_word & mask)) {
#ifdef BOTH
					*addr_fast = foreground;
					addr_fast++;
#endif
					*addr = foreground;
					addr++;
				} else {
#ifdef BOTH
					addr_fast++;
#endif
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
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

static void replace(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0xffff:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				*addr_fast = foreground;
				addr_fast++;
#endif
				*addr = foreground;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (pattern_word & mask) {
#ifdef BOTH
					*addr_fast = foreground;
					addr_fast++;
#endif
					*addr = foreground;
					addr++;
				} else {
#ifdef BOTH
					*addr_fast = background;
					addr_fast++;
#endif
					*addr = background;
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
	}
}

static void transparent(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0xffff:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				*addr_fast = foreground;
				addr_fast++;
#endif
				*addr = foreground;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (pattern_word & mask) {
#ifdef BOTH
					*addr_fast = foreground;
					addr_fast++;
#endif
					*addr = foreground;
					addr++;
				} else {
#ifdef BOTH
					addr_fast++;
#endif
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
	}
}

static void xor(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask, v;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0xffff:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				v = ~*addr_fast;
#else
				v = ~*addr;
#endif
#ifdef BOTH
				*addr_fast = v;
				addr_fast++;
#endif
				*addr = v;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (pattern_word & mask) {
#ifdef BOTH
					v = ~*addr_fast;
#else
					v = ~*addr;
#endif
#ifdef BOTH
					*addr_fast = v;
					addr_fast++;
#endif
					*addr = v;
					addr++;
				} else {
#ifdef BOTH
					addr_fast++;
#endif
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
	}
}

static void revtransp(short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
	int i, j;
	unsigned int pattern_word, mask;

	i = y;
	h = y + h;
	x = 1 << (15 - (x & 0x000f));

	for(; i < h; i++) {
		pattern_word = pattern[i & 0x000f];
		switch (pattern_word) {
		case 0x0000:
			for(j = w - 1; j >= 0; j--) {
#ifdef BOTH
				*addr_fast = foreground;
				addr_fast++;
#endif
				*addr = foreground;
				addr++;
			}
			break;
		default:
			mask = x;
			for(j = w - 1; j >= 0; j--) {
				if (!(pattern_word & mask)) {
#ifdef BOTH
					*addr_fast = foreground;
					addr_fast++;
#endif
					*addr = foreground;
					addr++;
				} else {
#ifdef BOTH
					addr_fast++;
#endif
					addr++;
				}
				if (!(mask >>= 1))
					mask = 0x8000;
			}
			break;
		}
#ifdef BOTH
		addr_fast += line_add;
#endif
		addr += line_add;
	}
}


#ifdef BOTH_WAS_ON
#define BOTH
#endif

long CDECL c_fill_area(Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour)
{
	Workstation *wk;
	short *addr, *addr_fast;
	short foreground, background;
  	int mode, line_add;
	long pos;
	short *table;

	table = 0;
	if ((long)vwk & 1) {
		if ((y & 0xffff) != 0)
			return -1;		/* Don't know about this kind of table operation */
		table = (short *)x;
		h = (y >> 16) & 0xffff;
		vwk = (Virtual *)((long)vwk - 1);
		return -1;			/* Don't know about anything yet */
	}

	c_get_colour(vwk, colour, &foreground, &background);

	wk = vwk->real_address;

	pos = (short)y * (long)wk->screen.wrap + x * 2;
	addr = wk->screen.mfdb.address;
	line_add = (wk->screen.wrap - w * 2) >> 1;

	mode = vwk->mode;

#ifdef BOTH
	if (addr_fast = wk->screen.shadow.address) {

		addr += pos >> 1;
#ifdef BOTH
		addr_fast += pos >> 1;
#endif
		switch (mode) {
		case 1:				/* Replace */
			s_replace(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		case 2:				/* Transparent */
			s_transparent(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		case 3:				/* XOR */
			s_xor(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		case 4:				/* Reverse transparent */
			s_revtransp(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		}
	} else {
#endif
		addr += pos >> 1;
		switch (mode) {
		case 1:				/* Replace */
			replace(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		case 2:				/* Transparent */
			transparent(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		case 3:				/* XOR */
			xor(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		case 4:				/* Reverse transparent */
			revtransp(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
			break;
		}
#ifdef BOTH
	}
#endif
	return 1;		/* Return as completed */
}
