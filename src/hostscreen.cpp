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


void  HostScreen::drawLine( int32 x1, int32 y1, int32 x2, int32 y2, uint16 pattern, uint32 color )
{
	//	linePattern( pattern );
	uint8 r,g,b,a; SDL_GetRGBA( color, surf->format, &r, &g, &b, &a);
	lineRGBA( surf, (int16)x1, (int16)y1, (int16)x2, (int16)y2, r,g,b,a ); // SDL_gfxPrimitives
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
void  HostScreen::fillArea( int32 x1, int32 y1, int32 x2, int32 y2, uint16 *pattern, uint32 color )
{
	areaPattern = pattern;
	/*
	char buffer[30];
	for( uint16 i=0; i<=15; i++ ) {
		getBinary( areaPattern[i], buffer );
		D(fprintf(stderr, "fVDI: fillP:%s\n", buffer ));
	}
	*/
	uint8 r,g,b,a; SDL_GetRGBA( color, surf->format, &r, &g, &b, &a);
	boxRGBA( surf, (int16)x1, (int16)y1, (int16)x2, (int16)y2, r,g,b,a ); // SDL_gfxPrimitives
	areaPattern = NULL;
}


/*
 * $Log$
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


