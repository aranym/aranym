/*
 *	Draw in 16 bit true colour mode			*	
 */
#define BOTH	/* Write in both FastRAM and on screen */

#include "fvdi.h"
#include "relocate.h"
	

/* destination MFDB (odd address marks table operation)
 * x or table address
 * y or table length (high) and type (0 - coordinates)
 */
long CDECL
c_write_pixel(Virtual *vwk, MFDB *dst, long x, long y, long colour)
{
	Workstation *wk;
	long offset;
	
	if ((long)vwk & 1)
		return 0;

	wk = vwk->real_address;
	if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address)) {
		offset = wk->screen.wrap * y + x * sizeof(short);
#ifdef BOTH
		if (wk->screen.shadow.address) {
			*(short *)((long)wk->screen.shadow.address + offset) = colour;
		}
#endif
		*(short *)((long)wk->screen.mfdb.address + offset) = colour;
	} else {
		offset = (dst->wdwidth * 2 * dst->bitplanes) * y + x * sizeof(short);
		*(short *)((long)dst->address + offset) = colour;
	}
	
	return 1;
}


long CDECL
c_read_pixel(Virtual *vwk, MFDB *src, long x, long y)
{
	Workstation *wk;
	long offset;
	unsigned long colour;
	
	wk = vwk->real_address;
	if (!src || !src->address || (src->address == wk->screen.mfdb.address)) {
		offset = wk->screen.wrap * y + x * sizeof(short);
#ifdef BOTH
		if (wk->screen.shadow.address) {
			colour = *(unsigned short *)((long)wk->screen.shadow.address + offset);
		} else {
#endif
			colour = *(unsigned short *)((long)wk->screen.mfdb.address + offset);
#ifdef BOTH
		}
#endif
	} else {
		offset = (src->wdwidth * 2 * src->bitplanes) * y + x * sizeof(short);
		colour = *(unsigned short *)((long)src->address + offset);
	}
	
	return colour;
}
