/*
 *      16 bit colour index handling
 */
#include "fvdi.h"
#include "relocate.h"


long CDECL c_get_hw_colour(short index, long red, long green, long blue, unsigned long *hw_value);


#define ECLIPSE 0
#define NOVA 0		/* 1 - byte swap 16 bit colour value (NOVA etc) */

#undef NORMAL_NAME

#ifdef NORMAL_NAME
long CDECL
c_get_colour(Virtual *vwk, long colour)
#else
long CDECL
GET1NAME(Virtual *vwk, long colour)
#endif
{
	Colour *local_palette, *global_palette;
	Colour *fore_pal, *back_pal;
	unsigned long tc_word, tc_word2;

	local_palette = vwk->palette;
	if (local_palette && !((long)local_palette & 1))	/* Complete local palette? */
		fore_pal = back_pal = local_palette;
	else {						/* Global or only negative local */
		local_palette = (Colour *)((long)local_palette & 0xfffffffeL);
		global_palette = vwk->real_address->screen.palette.colours;
		if (local_palette && ((short)colour < 0))
			fore_pal = local_palette;
		else
			fore_pal = global_palette;
		if (local_palette && ((colour >> 16) < 0))
			back_pal = local_palette;
		else
			back_pal = global_palette;
	}

	tc_word = *(PIXEL *)&fore_pal[(short)colour].real;
	tc_word2 = *(PIXEL *)&back_pal[colour >> 16].real;

#if NOVA
	switch (sizeof(PIXEL)) {
	case 2:
		tc_word = ((tc_word & 0x000000ffL) << 8) | ((tc_word & 0x0000ff00L) >>  8);
		tc_word2 = ((tc_word2 & 0x000000ffL) << 8) | ((tc_word2 & 0x0000ff00L) >>  8);
		break;
	default:
		tc_word = ((tc_word & 0x000000ffL) << 24) | ((tc_word & 0x0000ff00L) <<  8) |
		          ((tc_word & 0x00ff0000L) >>  8) | ((tc_word & 0xff000000L) >> 24);
		break;
	}
#endif

	if (sizeof(PIXEL) > 2)
		return tc_word;
	else
		return (tc_word2 << 16) | tc_word;
}


#ifdef NORMAL_NAME
void CDECL
c_get_colours(Virtual *vwk, long colour, long *foreground, long *background)
#else
void CDECL
GETNAME(Virtual *vwk, long colour, long *foreground, long *background)
#endif
{
	Colour *local_palette, *global_palette;
	Colour *fore_pal, *back_pal;
	unsigned long tc_word;

	local_palette = vwk->palette;
	if (local_palette && !((long)local_palette & 1))	/* Complete local palette? */
		fore_pal = back_pal = local_palette;
	else {						/* Global or only negative local */
		local_palette = (Colour *)((long)local_palette & 0xfffffffeL);
		global_palette = vwk->real_address->screen.palette.colours;
		if (local_palette && ((short)colour < 0))
			fore_pal = local_palette;
		else
			fore_pal = global_palette;
		if (local_palette && ((colour >> 16) < 0))
			back_pal = local_palette;
		else
			back_pal = global_palette;
	}

	tc_word = *(PIXEL *)&fore_pal[(short)colour].real;

#if NOVA
	switch (sizeof(PIXEL)) {
	case 2:
		tc_word = ((tc_word & 0x000000ffL) << 8) | ((tc_word & 0x0000ff00L) >>  8);
		break;
	default:
		tc_word = ((tc_word & 0x000000ffL) << 24) | ((tc_word & 0x0000ff00L) <<  8) |
		          ((tc_word & 0x00ff0000L) >>  8) | ((tc_word & 0xff000000L) >> 24);
		break;
	}
#endif

	*foreground = tc_word;
	tc_word = *(PIXEL *)&back_pal[colour >> 16].real;

#if NOVA
	switch (sizeof(PIXEL)) {
	case 2:
		tc_word = ((tc_word & 0x000000ffL) << 8) | ((tc_word & 0x0000ff00L) >>  8);
		break;
	default:
		tc_word = ((tc_word & 0x000000ffL) << 24) | ((tc_word & 0x0000ff00L) <<  8) |
		          ((tc_word & 0x00ff0000L) >>  8) | ((tc_word & 0xff000000L) >> 24);
		break;
	}
#endif

	*background = tc_word;
}


#ifdef NORMAL_NAME
void CDECL
c_set_colours(Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[])
#else
void CDECL
SETNAME(Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[])
#endif
{
	unsigned long colour;
	unsigned short component;
	unsigned long tc_word;
	int i;
	
	if ((long)requested & 1) {			/* New entries? */
		requested = (short *)((long)requested & 0xfffffffeL);
		for(i = 0; i < entries; i++) {
			requested++;				/* First word is reserved */
			component = *requested++;
			palette[start + i].vdi.red = component;
			palette[start + i].hw.red = component;	/* Not at all correct */
			component = *requested++;
			palette[start + i].vdi.green = component;
			palette[start + i].hw.green = component;	/* Not at all correct */
			component = *requested++;
			palette[start + i].vdi.blue = component;
			palette[start + i].hw.blue = component;	/* Not at all correct */

			c_get_hw_colour(  start + i,
					  palette[start + i].vdi.red,
					  palette[start + i].vdi.green,
		              		  palette[start + i].vdi.blue,
					  &tc_word );

			*(PIXEL *)&palette[start + i].real = (PIXEL)tc_word;
		}
	} else {
		for(i = 0; i < entries; i++) {
			component = *requested++;
			palette[start + i].vdi.red = component;
			palette[start + i].hw.red = component;	/* Not at all correct */
			colour = (component * ((1L << red_bits) - 1) + 500L) / 1000;
			palette[start + i].hw.red = (colour * 1000 + (1L << (red_bits - 1))) / ((1L << red_bits) - 1);
			component = *requested++;
			palette[start + i].vdi.green = component;
			palette[start + i].hw.green = component;	/* Not at all correct */
			colour = (component * ((1L << green_bits) - 1) + 500L) / 1000;
			palette[start + i].hw.green = (colour * 1000 + (1L << (green_bits - 1))) / ((1L << green_bits) - 1);
			component = *requested++;
			palette[start + i].vdi.blue = component;
			palette[start + i].hw.blue = component;	/* Not at all correct */
			colour = (component * ((1L << blue_bits) - 1) + 500L) / 1000;
			palette[start + i].hw.blue = (colour * 1000 + (1L << (blue_bits - 1))) / ((1L << blue_bits) - 1);

			c_get_hw_colour(  start + i,
					  palette[start + i].vdi.red,
					  palette[start + i].vdi.green,
		              		  palette[start + i].vdi.blue,
					  &tc_word );

			*(PIXEL *)&palette[start + i].real = (PIXEL)tc_word;
		}
	}
}
