/*
 * $Header$
 *
 * STanda 2001
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "hostscreen.h"
#include "parameters.h"

#define DEBUG 0
#include "debug.h"

#include "gfxprimitives.h"


/*
void SelectVideoMode()
{
	SDL_Rect **modes;
	uint32 i;

	// Get available fullscreen/hardware modes
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);

	// Check is there are any modes available
	if (modes == (SDL_Rect **) 0) {
		printf("No modes available!\n");
		exit(-1);
	}

	// Check if or resolution is restricted
	if (modes == (SDL_Rect **) - 1) {
		printf("All resolutions available.\n");
	}
	else {
		// Print valid modes
		printf("Available Modes\n");
		for (i = 0; modes[i]; ++i)
			printf("  %d x %d\n", modes[i]->w, modes[i]->h);
	}
}
*/


void HostScreen::setWindowSize(	uint32 width, uint32 height )
{
	this->width  = width;
	this->height = height;

	// SelectVideoMode();
	sdl_videoparams = SDL_HWSURFACE;
	if (fullscreen)
		sdl_videoparams |= SDL_FULLSCREEN;

	surf = SDL_SetVideoMode(width, height, 16, sdl_videoparams);
	SDL_WM_SetCaption(VERSION_STRING, "ARAnyM");
	D(bug("Line Length = %d", surf->pitch));
	D(bug("Must Lock? %s", SDL_MUSTLOCK(surf) ? "YES" : "NO"));

	// is the SDL_update needed?
	doUpdate = ( surf->flags & SDL_HWSURFACE ) == 0;

	renderBegin();

	VideoRAMBaseHost = (uint8 *) surf->pixels;
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	D(bug("VideoRAM starts at %p (%08x)", VideoRAMBaseHost, VideoRAMBase));
	D(bug("surf->pixels = %x, getVideoSurface() = %x",
			VideoRAMBaseHost, SDL_GetVideoSurface()->pixels));

	renderEnd();

	D(bug("Pixel format:bitspp=%d, tmasks r=%04x g=%04x b=%04x"
			", tshifts r=%d g=%d b=%d"
			", tlosses r=%d g=%d b=%d",
		    surf->format->BitsPerPixel,
			surf->format->Rmask, surf->format->Gmask, surf->format->Bmask,
			surf->format->Rshift, surf->format->Gshift, surf->format->Bshift,
			surf->format->Rloss, surf->format->Gloss, surf->format->Bloss));
}


extern "C" {
	static void getBinary( uint16 data, char *buffer ) {
		for( uint16 i=0; i<=15; i++ ) {
			buffer[i] = (data & 1)?'1':' ';
			data>>=1;
		}
		buffer[16]='\0';
	}
}




void HostScreen::gfxHLineColor ( int16 x1, int16 x2, int16 y, uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	uint8 *pixel,*pixellast;
	int dx;
	int pixx, pixy;
	int16 w;
	int16 xtmp;
	uint8 ppos;

	/* Swap x1, x2 if required */
	if (x1>x2) {
		xtmp=x1; x1=x2; x2=xtmp;
	}

	/* Calculate width */
	w=x2-x1;

	/* Sanity check on width */
	if (w<0)
		return;

	/* More variable setup */
	dx=w;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (int)x1 + pixy * (int)y;
	ppos = 0;

	/* Draw */
	switch(surf->format->BytesPerPixel) {
		case 1:
			memset (pixel, fgColor, dx);
			break;
		case 2:
			pixellast = pixel + dx + dx;
			for (; pixel<=pixellast; pixel += pixx) {
				switch (logOp) {
					case 1:
						*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor); // STanda
						break;
					case 2:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = fgColor; // STanda
						break;
					case 3:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = ~(*(uint16*)pixel);
						break;
					case 4:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint16*)pixel = fgColor; // STanda
						break;
				}
			}
			break;
		case 3:
			pixellast = pixel + dx + dx + dx;
			for (; pixel<=pixellast; pixel += pixx)
				putBpp24Pixel( pixel, fgColor );
			break;
		default: /* case 4*/
			dx = dx + dx;
			pixellast = pixel + dx + dx;
			for (; pixel<=pixellast; pixel += pixx) {
				*(uint32*)pixel = fgColor;
			}
			break;
	}
}


void HostScreen::gfxVLineColor( int16 x, int16 y1, int16 y2,
								uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	uint8 *pixel, *pixellast;
	int dy;
	int pixx, pixy;
	int16 h;
	int16 ytmp;
	uint8 ppos;

	/* Swap y1, y2 if required */
	if (y1>y2) {
		ytmp=y1; y1=y2; y2=ytmp;
	}

	/* Calculate height */
	h=y2-y1;

	/* Sanity check on height */
	if (h<0)
		return;

	ppos = 0;

	/* More variable setup */
	dy=h;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (int)x + pixy * (int)y1;
	pixellast = pixel + pixy*dy;

	/* Draw */
	switch(surf->format->BytesPerPixel) {
		case 1:
			for (; pixel<=pixellast; pixel += pixy) {
				*(uint8*)pixel = fgColor;
			}
			break;
		case 2:
			for (; pixel<=pixellast; pixel += pixy) {
				switch (logOp) {
					case 1:
						*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor); // STanda
						break;
					case 2:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = fgColor; // STanda
						break;
					case 3:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = ~(*(uint16*)pixel);
						break;
					case 4:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint16*)pixel = fgColor; // STanda
						break;
				}
			}
			break;
		case 3:
			for (; pixel<=pixellast; pixel += pixy) {
				putBpp24Pixel( pixel, fgColor );
			}
			break;
		default: /* case 4*/
			for (; pixel<=pixellast; pixel += pixy) {
				*(uint32*)pixel = fgColor;
			}
			break;
	}
}


/* ----- Line */

/* Non-alpha line drawing code adapted from routine          */
/* by Pete Shinners, pete@shinners.org                       */
/* Originally from pygame, http://pygame.seul.org            */

void HostScreen::gfxLineColor( int16 x1, int16 y1, int16 x2, int16 y2,
							   uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp )
{
	uint32 pixx, pixy;
	uint32 x,y;
	uint32 dx,dy;
	uint32 sx,sy;
	uint32 swaptmp;
	uint8 *pixel;
	uint8 ppos;

	/* Test for special cases of straight lines or single point */
	if (x1==x2) {
		if (y1<y2) {
			gfxVLineColor(x1, y1, y2, pattern, fgColor, bgColor, logOp);
			return;
		} else if (y1>y2) {
			gfxVLineColor(x1, y2, y1, pattern, fgColor, bgColor, logOp);
			return;
		} else {
			if ( pattern & 0x8000 != 0 ) // FIXME: The logOp is _not_ handled here
				gfxFastPixelColorNolock( x1, y1, fgColor );
		}
	}
	if (y1==y2) {
		if (x1<x2) {
			gfxHLineColor(x1, x2, y1, pattern, fgColor, bgColor, logOp);
			return;
		} else if (x1>x2) {
			gfxHLineColor(x2, x1, y1, pattern, fgColor, bgColor, logOp);
			return;
		}
	}

	/* Variable setup */
	dx = x2 - x1;
	dy = y2 - y1;
	sx = (dx >= 0) ? 1 : -1;
	sy = (dy >= 0) ? 1 : -1;
	ppos = 0;

	/* More variable setup */
	dx = sx * dx + 1;
	dy = sy * dy + 1;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (uint32)x1 + pixy * (uint32)y1;
	pixx *= sx;
	pixy *= sy;
	if (dx < dy) {
		swaptmp = dx; dx = dy; dy = swaptmp;
		swaptmp = pixx; pixx = pixy; pixy = swaptmp;
	}

	/* Draw */
	x=0;
	y=0;
	switch(surf->format->BytesPerPixel) {
		case 1:
			for(; x < dx; x++, pixel += pixx) {
				*pixel = fgColor;
				y += dy;
				if (y >= dx) {
					y -= dx; pixel += pixy;
				}
			}
			break;
		case 2:
			for (; x < dx; x++, pixel += pixx) {
				switch (logOp) {
					case 1:
						*(uint16*)pixel = (( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 ) ? fgColor : bgColor); // STanda
						break;
					case 2:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = fgColor; // STanda
						break;
					case 3:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) != 0 )
							*(uint16*)pixel = ~(*(uint16*)pixel);
						break;
					case 4:
						if ( ( pattern & ( 1 << ( (ppos++) & 0xf ) )) == 0 )
							*(uint16*)pixel = fgColor; // STanda
						break;
				}
				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
			break;
		case 3:
			for(; x < dx; x++, pixel += pixx) {
				putBpp24Pixel( pixel, fgColor );
				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
			break;
		default: /* case 4 */
			for(; x < dx; x++, pixel += pixx) {
				*(uint32*)pixel = fgColor;
				y += dy;
				if (y >= dx) {
					y -= dx;
					pixel += pixy;
				}
			}
			break;
	}
}




/**
 * Derived from the SDL_gfxPrimitives::boxColor(). The colors are in the destination surface format here.
 * The trivial cases optimalization removed.
 *
 * @author STanda
 **/
void HostScreen::gfxBoxColorPattern (int16 x1, int16 y1, int16 x2, int16 y2,
									 uint16 *areaPattern, uint32 fgColor, uint32 bgColor, uint16 logOp)
{
	uint8 *pixel, *pixellast;
	int16 x, dx, dy;
	int16 pixx, pixy;
	int16 w,h,tmp;

	/* Order coordinates */
	if (x1>x2) {
		tmp=x1;
		x1=x2;
		x2=tmp;
	}
	if (y1>y2) {
		tmp=y1;
		y1=y2;
		y2=tmp;
	}

	/* Calculate width&height */
	w=x2-x1+1; // STanda //+1
	h=y2-y1;

	/* More variable setup */
	dx=w;
	dy=h;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (int32)x1 + pixy * (int32)y1;
	pixellast = pixel + pixx*dx + pixy*dy;

	// STanda // FIXME here the pattern should be checked out of the loops for performance
	          // but for now it is good enough
	          // FIXME not tested on other then 2 BPP

	/* Draw */
	switch(surf->format->BytesPerPixel) {
		case 1:
			// STanda // the loop is the same as the for the 2 BPP
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				for (x=0; x<dx; x++) {
					*(uint8*)pixel = (( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 ) ? fgColor : bgColor); // STanda
					pixel += pixx;
				}
			}
			break;

			//for (; pixel<=pixellast; pixel += pixy) {
			//	memset(pixel,(uint8)fgColor,dx);
			//}
			// STanda
			break;
		case 2:
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				switch (logOp) {
					case 1:
						for (x=0; x<dx; x++) {
							*(uint16*)pixel = (( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 ) ? fgColor : bgColor); // STanda
							pixel += pixx;
						}; break;
					case 2:
						for (x=0; x<dx; x++) {
							if ( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 )
								*(uint16*)pixel = fgColor; // STanda
							pixel += pixx;
						}; break;
					case 3:
						for (x=0; x<dx; x++) {
							if ( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 )
								*(uint16*)pixel = ~(*(uint16*)pixel);
							pixel += pixx;
						}; break;
					case 4:
						for (x=0; x<dx; x++) {
							if ( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) == 0 )
								*(uint16*)pixel = fgColor; // STanda
							pixel += pixx;
						}; break;
				}
			}
			break;
		case 3:
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				for (x=0; x<dx; x++) {
					uint32 color = (( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 ) ? fgColor : bgColor); // STanda
					putBpp24Pixel( pixel, color );
					pixel += pixx;
				}
			}
			break;
		default: /* case 4*/
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				for (x=0; x<dx; x++) {
					*(uint32*)pixel = (( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 ) ? fgColor : bgColor); // STanda
					pixel += pixx;
				}
			}
			break;
	}  // switch
}


/**
 * Derived from the SDL_gfxPrimitives::boxColor(). The color is in the destination surface format here.
 * The trivial cases optimalization removed.
 *
 * @author STanda
 **/
void HostScreen::gfxBoxColorPatternBgTrans(int16 x1, int16 y1, int16 x2, int16 y2, uint16 *areaPattern, uint32 color)
{
	uint8 *pixel, *pixellast;
	int16 x, dx, dy;
	int16 pixx, pixy;
	int16 w,h,tmp;

	/* Order coordinates */
	if (x1>x2) {
		tmp=x1;
		x1=x2;
		x2=tmp;
	}
	if (y1>y2) {
		tmp=y1;
		y1=y2;
		y2=tmp;
	}

	/* Calculate width&height */
	w=x2-x1+1;
	h=y2-y1;

	/* More variable setup */
	dx=w;
	dy=h;
	pixx = surf->format->BytesPerPixel;
	pixy = surf->pitch;
	pixel = ((uint8*)surf->pixels) + pixx * (int32)x1 + pixy * (int32)y1;
	pixellast = pixel + pixx*dx + pixy*dy;

	// STanda // FIXME here the pattern should be checked out of the loops for performance
	          // but for now it is good enough
	          // FIXME not tested on other then 2 BPP

	/* Draw */
	switch(surf->format->BytesPerPixel) {
		case 1:
			// STanda // the loop is the same as the for the 2 BPP
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				for (x=0; x<dx; x++) {
					if ( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 ) // STanda
						*(uint8*)pixel = color;
					pixel += pixx;
				}
			}
			break;

			//for (; pixel<=pixellast; pixel += pixy) {
			//	memset(pixel,(uint8)color,dx);
			//}
			// STanda
			break;
		case 2:
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				for (x=0; x<dx; x++) {
					if ( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 ) // STanda
						*(uint16*)pixel = color;
					pixel += pixx;
				}
			}
			break;
		case 3:
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				for (x=0; x<dx; x++) {
					if ( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 )
						putBpp24Pixel( pixel, color );
					pixel += pixx;
				}
			}
			break;
		default: /* case 4*/
			pixy -= (pixx*dx);
			for (; pixel<=pixellast; pixel += pixy) {
				uint16 pattern = areaPattern ? areaPattern[ y1++ & 0xf ] : 0xffff; // STanda
				for (x=0; x<dx; x++) {
					if ( ( pattern & ( 1 << ( (x1+x) & 0xf ) )) != 0 ) // STanda
						*(uint32*)pixel = color;
					pixel += pixx;
				}
			}
			break;
	}  // switch
}



/*
 * $Log$
 * Revision 1.10  2001/09/30 23:09:23  standa
 * The line logical operation added.
 * The first version of blitArea (screen to screen only).
 *
 * Revision 1.9  2001/09/24 23:16:28  standa
 * Another minor changes. some logical operation now works.
 * fvdidrv/fillArea and fvdidrv/expandArea got the first logOp handling.
 *
 * Revision 1.8  2001/09/20 18:12:09  standa
 * Off by one bug fixed in fillArea.
 * Separate functions for transparent and opaque background.
 * gfxPrimitives methods moved to the HostScreen
 *
 * Revision 1.7  2001/09/05 15:06:41  joy
 * SelectVideoMode() commented out.
 *
 * Revision 1.6  2001/09/04 13:51:45  joy
 * debug disabled
 *
 * Revision 1.5  2001/08/30 14:04:59  standa
 * The fVDI driver. mouse_draw implemented. Partial pattern fill support.
 * Still buggy.
 *
 * Revision 1.4  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *       The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *       was created.
 * fVDIDriver got first version of drawLine and fillArea (thanks to SDL_gfxPrimitives).
 *
 * Revision 1.3  2001/08/13 22:29:06  milan
 * IDE's params from aranymrc file etc.
 *
 * Revision 1.2  2001/07/24 09:36:51  joy
 * D(bug) macro replaces fprintf
 *
 * Revision 1.1  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 *
 */
