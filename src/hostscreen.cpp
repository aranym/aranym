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


#if 0
int SelectVideoMode()
{
	SDL_Rect **modes;
	int i;

	/* Get available fullscreen/hardware modes */
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);

	/* Check is there are any modes available */
	if (modes == (SDL_Rect **) 0) {
		printf("No modes available!\n");
		exit(-1);
	}

	/* Check if or resolution is restricted */
	if (modes == (SDL_Rect **) - 1) {
		printf("All resolutions available.\n");
	}
	else {
		/* Print valid modes */
		printf("Available Modes\n");
		for (i = 0; modes[i]; ++i)
			printf("  %d x %d\n", modes[i]->w, modes[i]->h);
	}

	return 0;
}
#endif


void HostScreen::setWindowSize(	int width, int height )
{
	this->width  = width;
	this->height = height;

	// SelectVideoMode();
	sdl_videoparams = SDL_HWSURFACE;
	if (fullscreen)
		sdl_videoparams |= SDL_FULLSCREEN;

	surf = SDL_SetVideoMode(width, height, 16, sdl_videoparams);
	SDL_WM_SetCaption(VERSION_STRING, "ARAnyM");
	fprintf(stderr, "Line Length = %d\n", surf->pitch);
	fprintf(stderr, "Must Lock? %s\n", SDL_MUSTLOCK(surf) ? "YES" : "NO");

	renderBegin();

	VideoRAMBaseHost = (uint8 *) surf->pixels;
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	fprintf(stderr, "VideoRAM starts at %p (%08x)\n", VideoRAMBaseHost, VideoRAMBase);

	fprintf(stderr, "surf->pixels = %x, getVideoSurface() = %x\n",
			VideoRAMBaseHost, SDL_GetVideoSurface()->pixels);

	renderEnd();

	fprintf(stderr,
			"Pixel format:\n"
			"\tmasks  r %04x, g %04x, b %04x\n"
			"\tshifts r %d, g %d, b %d\n"
			"\tlosses r %d, g %d, b %d\n",
			surf->format->Rmask, surf->format->Gmask, surf->format->Bmask,
			surf->format->Rshift, surf->format->Gshift, surf->format->Bshift,
			surf->format->Rloss, surf->format->Gloss, surf->format->Bloss);
}


/*
 * $Log$
 *
 */


