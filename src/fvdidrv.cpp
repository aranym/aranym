/*
 * $Header$
 *
 * STanda 2001
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "main.h"

#include "videl.h"
#include "hostscreen.h"

#include "fvdidrv.h"

#define DEBUG 1
#include "debug.h"


extern HostScreen hostScreen;

extern VIDEL videl;
extern bool updateScreen;


void FVDIDriver::dispatch( uint32 fncode, M68kRegisters *r )
{
	MFDB dst;
	MFDB src;
	MFDB *pSrc = NULL;
	MFDB *pDst = NULL;

	// fix the stack (the fncode was pushed onto the stack)
	r->a[7] += 4;

	switch (fncode) {
		case 1:	// read_pixel:
			D(fprintf(stderr, "fVDI: %s\n", "read_pixel"));
			r->d[0] = read_pixel( (void*)get_long( (uint32)r->a[1], true ),
								  fetchMFDB( &src, get_long( (uint32)r->a[1] + 4, true ) ),
								  r->d[1], r->d[2] );
			break;
		case 2:	// write_pixel:
			D(fprintf(stderr, "fVDI: %s %d,%d (%d)\n", "write_pixel", (uint16)r->d[1], (uint16)r->d[2], (uint32)r->d[0] ));
			r->d[0] = write_pixel( (void*)get_long( (uint32)r->a[1], true ),
								   fetchMFDB( &dst, get_long( (uint32)r->a[1] + 4, true ) ),
								   r->d[1], r->d[2], r->d[0] );
			break;
		default:
			D(fprintf(stderr, "fVDI: Unknown %d\n", fncode));
	}
}


FVDIDriver::MFDB* FVDIDriver::fetchMFDB( FVDIDriver::MFDB* mfdb, uint32 pmfdb )
{
	if ( pmfdb != 0 ) {
		mfdb->address = (uint16*)get_long( pmfdb, true );
		return mfdb;
	}
  
	return NULL;
}


/**
 *
 * destination MFDB (odd address marks table operation)
 * x or table address
 * y or table length (high) and type (0 - coordinates)
 */
uint32 FVDIDriver::write_pixel(void *vwk, MFDB *dst, uint32 x, uint32 y, uint32 colour)
{
	uint32 offset;
	
	if ((uint32)vwk & 1)
		return 0;

	updateScreen = false;

	if (!dst || !dst->address) {
		//	Workstation *wk;
		//wk = vwk->real_address;
		//|| (dst->address == wk->screen.mfdb.address)) {

		D(fprintf(stderr, "\nfVDI: %s %d,%d (%d %08x)\n", "write_pixel", (uint16)x, (uint16)y, (uint32)colour, (uint32)colour));

		if (!hostScreen.renderBegin())
			return 0;

		((uint16*)hostScreen.getVideoramAddress())[((uint16)y)*videl.getScreenWidth()+(uint16)x] = (uint16)colour;
		//((uint16*)surf->pixels)[(uint16)y*640+(uint16)x] = (uint16)sdl_colors[(uint16)colour];

		hostScreen.renderEnd();
		hostScreen.update();// (uint16)x, (uint16)y, 1, 1);

		//	  if ( (uint16)y > 479 || (uint16)x > 639 )

	} else {
		D(fprintf(stderr, "fVDI: %s %d,%d (%d)\n", "write_pixel no screen"));

		offset = (dst->wdwidth * 2 * dst->bitplanes) * y + x * sizeof(uint16);
		put_word( (uint32)dst->address + offset, colour );
	}
	
	return 1;
}


uint32 FVDIDriver::read_pixel(void *vwk, MFDB *src, uint32 x, uint32 y)
{
	uint32 offset;
	uint32 colour;
	
	if (!src || !src->address) {
		//|| (src->address == wk->screen.mfdb.address)) {
		
		if (!hostScreen.renderBegin())
			return 0;
		colour = ((uint16*)hostScreen.getVideoramAddress())[((uint16)y)*videl.getScreenWidth()+(uint16)x];

		//	  if ( (uint16)y > 479 || (uint16)x > 639 )
		//D(fprintf(stderr, "fVDI: %s %d,%d %04x\n", "read_pixel", (uint16)x, (uint16)y, (uint16)colour ));

		hostScreen.renderEnd();
		//	  colour = (colour >> 8) | ((colour & 0xff) << 8);	// byteswap
	} else {
		D(fprintf(stderr, "fVDI: %s %d,%d (%d)\n", "read_pixel no screen"));

		offset = (src->wdwidth * 2 * src->bitplanes) * y + x * sizeof(uint16);
		colour = get_word((uint32)src->address + offset, true);
	}
	
	return colour;
}


/*
 * $Log$
 *
 */
