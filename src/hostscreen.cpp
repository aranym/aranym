/*
	Hostscreen, base class
	Software renderer

	(C) 2007-2008 ARAnyM developer team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <vector>

#include "dirty_rects.h"
#include "host_surface.h"
#include "logo.h"
#include "hostscreen.h"
#include "version.h"
#include "parameters.h"	/* bx_options */
#include "main.h"	/* QuitEmulator */

#ifdef NFVDI_SUPPORT
# include "nf_objs.h"
# include "nfvdi.h"
#endif

#ifdef SDL_GUI
# include "gui-sdl/sdlgui.h"
#endif

#define DEBUG 0
#include "debug.h"

HostScreen::HostScreen(void)
	: DirtyRects(), logo(NULL), logo_present(true), clear_screen(true),
	force_refresh(true), do_screenshot(false),
	refreshCounter(0), screen(NULL), new_width(0), new_height(0),
	snapCounter(0)
{
}

HostScreen::~HostScreen(void)
{
	if (logo) {
		delete logo;
	}
}

void HostScreen::reset(void)
{
	lastVidelWidth = lastVidelHeight = lastVidelBpp = -1;
	numScreen = SCREEN_BOOT;
	setVidelRendering(true);
	DisableOpenGLVdi();

	setVideoMode(MIN_WIDTH,MIN_HEIGHT,8);

	/* Set window caption */
	char buf[sizeof(VERSION_STRING)+128];
#ifdef SDL_GUI
	char key[80];
	displayKeysym(bx_options.hotkeys.setup, key);
	snprintf(buf, sizeof(buf), "%s  (Press the [%s] key for SETUP)", VERSION_STRING, key);
#else
	safe_strncpy(buf, VERSION_STRING, sizeof(buf));
#endif /* SDL_GUI */
	SDL_WM_SetCaption(buf, "ARAnyM");
}

int HostScreen::getWidth(void)
{
	return screen->w;
}

int HostScreen::getHeight(void)
{
	return screen->h;
}

int HostScreen::getBpp(void)
{
	return screen->format->BitsPerPixel;
}

void HostScreen::doScreenshot(void)
{
	do_screenshot = true;
}

void HostScreen::makeSnapshot(void)
{
	char filename[15];
	sprintf( filename, "snap%03d.bmp", snapCounter++ );

	SDL_SaveBMP(screen, filename);
}

void HostScreen::toggleFullScreen(void)
{
	bx_options.video.fullscreen = !bx_options.video.fullscreen;

	setVideoMode(getWidth(), getHeight(), getBpp());
}

void HostScreen::setVideoMode(int width, int height, int bpp)
{
	if (bx_options.autozoom.fixedsize) {
		width = bx_options.autozoom.width;
		height = bx_options.autozoom.height;
	}
	if (width<MIN_WIDTH) {
		width=MIN_WIDTH;
	}
	if (height<MIN_HEIGHT) {
		height=MIN_HEIGHT;
	}

	int screenFlags = SDL_HWSURFACE|SDL_HWPALETTE|SDL_RESIZABLE;
	if (bx_options.video.fullscreen) {
		screenFlags |= SDL_FULLSCREEN;
	}

	screen = SDL_SetVideoMode(width, height, bpp, screenFlags);
	if (screen==NULL) {
		/* Try with default bpp */
		screen = SDL_SetVideoMode(width, height, 0, screenFlags);
	}
	if (screen==NULL) {
		/* Try with default resolution */
		screen = SDL_SetVideoMode(0, 0, 0, screenFlags);
	}
	if (screen==NULL) {
		panicbug(("Can not set video mode\n"));
		QuitEmulator();
		return;
	}

	SDL_SetClipRect(screen, NULL);

	bx_options.video.fullscreen = ((screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);

	new_width = screen->w;
	new_height = screen->h;
	resizeDirty(screen->w, screen->h);

	force_refresh = true;
}

void HostScreen::resizeWindow(int new_width, int new_height)
{
	this->new_width = new_width;
	this->new_height = new_height;
}

void HostScreen::EnableOpenGLVdi(void)
{
	OpenGLVdi = SDL_TRUE;
}

void HostScreen::DisableOpenGLVdi(void)
{
	OpenGLVdi = SDL_FALSE;
}

/*
 * this is called in VBL, i.e. 50 times per second
 */
void HostScreen::refresh(void)
{
	if (++refreshCounter < bx_options.video.refresh) {
		return;
	}

	refreshCounter = 0;

	if (force_refresh) {
		clear_screen = true;
		forceRefreshLogo();
		forceRefreshVidel();
		forceRefreshNfvdi();
		if (screen) {
			setDirtyRect(0,0, screen->w, screen->h);
		}
		force_refresh = false;
	}

	initScreen();
	if (clear_screen || bx_options.opengl.enabled) {
		clearScreen();
		clear_screen = false;
	}

	/* Render current screen */
	switch(numScreen) {
		case SCREEN_BOOT:
			/* Wait till GUI or reset is done */
			break;
		case SCREEN_LOGO:
			refreshLogo();
			alphaBlendLogo(true);
			checkSwitchToVidel();
			break;
		case SCREEN_VIDEL:
			refreshVidel();
			alphaBlendLogo(false);
			checkSwitchVidelNfvdi();
			break;
		case SCREEN_NFVDI:
			refreshNfvdi();
			checkSwitchVidelNfvdi();
			break;
	}

#ifdef SDL_GUI
	if (!SDLGui_isClosed()) {
		refreshGui();
	}
#endif

	if (do_screenshot) {
		makeSnapshot();
		do_screenshot = false;
	}

	refreshScreen();

	if ((new_width!=screen->w) || (new_height!=screen->h)) {
		setVideoMode(new_width, new_height, getBpp());
	}
}

void HostScreen::setVidelRendering(bool videlRender)
{
	renderVidelSurface = videlRender;
}

void HostScreen::initScreen(void)
{
}

void HostScreen::clearScreen(void)
{
	SDL_FillRect(screen, NULL, 0);
}

void HostScreen::refreshVidel(void)
{
	refreshSurface(getVIDEL()->getSurface());
}

void HostScreen::forceRefreshVidel(void)
{
	if (!getVIDEL()) {
		return;	
	}

	getVIDEL()->forceRefresh();
}

void HostScreen::checkSwitchVidelNfvdi(void)
{
	numScreen = renderVidelSurface ? SCREEN_VIDEL : SCREEN_NFVDI;
}

void HostScreen::refreshLogo(void)
{
	char path[1024];
	getDataFilename(LOGO_FILENAME, path, sizeof(path));

	if (!logo_present) {
		return;
	}
	if (!logo) {
		logo = new Logo(path);
		if (!logo) {
			return;
		}
	}

	HostSurface *logo_hsurf = logo->getSurface();
	if (!logo_hsurf) {
		logo->load(path);
		logo_hsurf = logo->getSurface();
		if (!logo_hsurf) {
			panicbug("Can not load logo from %s file\n", path); 
			logo_present = false;
			return;
		}
	}

	refreshSurface(logo_hsurf);
}

void HostScreen::forceRefreshLogo(void)
{
	if (!logo) {
		return;
	}
	HostSurface *logo_hsurf = logo->getSurface();
	if (!logo_hsurf) {
		return;
	}

	logo_hsurf->setDirtyRect(0,0,
		logo_hsurf->getWidth(), logo_hsurf->getHeight());
}

void HostScreen::alphaBlendLogo(bool init)
{
	static int logo_opacity = 100;

	if (init) {
		logo_opacity = 100;
	}
	else {
		if (bx_options.opengl.enabled && logo != NULL && logo_opacity > 0) {
			HostSurface *logo_hsurf = logo->getSurface();
			if (logo_hsurf != NULL) {
				logo_hsurf->setParam(HostSurface::SURF_ALPHA, logo_opacity);
				logo_hsurf->setDirtyRect(0, 0, logo_hsurf->getWidth(), logo_hsurf->getHeight());
				drawSurfaceToScreen(logo_hsurf);
			}

			logo_opacity -= 5 * bx_options.video.refresh;
			if (logo_opacity <= 0 && getVIDEL() != NULL && getVIDEL()->getSurface() != NULL && logo_hsurf != NULL) {
				getVIDEL()->getSurface()->setDirtyRect(0, 0, logo_hsurf->getWidth(), logo_hsurf->getHeight());
			}
		}
	}
}
void HostScreen::checkSwitchToVidel(void)
{
	/* No logo ? */
	if (!logo_present) {
		numScreen = SCREEN_VIDEL;
		return;
	}

	/* Wait for Videl surface to be ready */
	HostSurface *videl_hsurf = getVIDEL()->getSurface();
	if (!videl_hsurf) {
		return;
	}

	if ((videl_hsurf->getWidth()>64) && (videl_hsurf->getHeight()>64)) {
		numScreen = SCREEN_VIDEL;
	}
}

void HostScreen::forceRefreshNfvdi(void)
{
#ifdef NFVDI_SUPPORT
	/* Force nfvdi surface refresh */
	NF_Base* fvdi = NFGetDriver("fVDI");
	if (!fvdi) {
		return;
	}

	HostSurface *nfvdi_hsurf = ((VdiDriver *) fvdi)->getSurface();
	if (!nfvdi_hsurf) {
		return;
	}

	nfvdi_hsurf->setDirtyRect(0,0,
		nfvdi_hsurf->getWidth(), nfvdi_hsurf->getHeight());
#endif
}

void HostScreen::refreshNfvdi(void)
{
#ifdef NFVDI_SUPPORT
	NF_Base* fvdi = NFGetDriver("fVDI");
	if (!fvdi) {
		return;
	}

	refreshSurface(((VdiDriver *) fvdi)->getSurface());
#endif
}

void HostScreen::refreshGui(void)
{
#ifdef SDL_GUI
	int gui_x, gui_y;

	drawSurfaceToScreen(SDLGui_getSurface(), &gui_x, &gui_y);

	SDLGui_setGuiPos(gui_x, gui_y);
#endif /* SDL_GUI */
}

void HostScreen::refreshSurface(HostSurface *hsurf)
{
	if (!hsurf) {
		return;
	}
	SDL_Surface *sdl_surf = hsurf->getSdlSurface();
	if (!sdl_surf) {
		return;
	}

	int width = hsurf->getWidth();
	int height = hsurf->getHeight();

	int w = (width < 320) ? 320 : width;
	int h = (height < 200) ? 200 : height;
	int bpp = hsurf->getBpp();
	if ((w!=lastVidelWidth) || (h!=lastVidelHeight) || (bpp!=lastVidelBpp)) {
		setVideoMode(w, h, bpp);
		lastVidelWidth = w;
		lastVidelHeight = h;
		lastVidelBpp = bpp;
	}

	/* Set screen palette from surface if needed */
	if (!bx_options.opengl.enabled && (bpp==8) && (getBpp() == 8)) {
		SDL_Color palette[256];
		for (int i=0; i<256; i++) {
			palette[i].r = sdl_surf->format->palette->colors[i].r;
			palette[i].g = sdl_surf->format->palette->colors[i].g;
			palette[i].b = sdl_surf->format->palette->colors[i].b;
		}
		SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, palette, 0,256);
	}

	drawSurfaceToScreen(hsurf);
}

void HostScreen::drawSurfaceToScreen(HostSurface *hsurf, int *dst_x, int *dst_y)
{
	if (!hsurf) {
		return;
	}
	hsurf->update();

	SDL_Surface *sdl_surf = hsurf->getSdlSurface();
	if (!sdl_surf) {
		return;
	}

	int width = hsurf->getWidth();
	int height = hsurf->getHeight();

	SDL_Rect src_rect = {0,0, width, height};
	SDL_Rect dst_rect = {0,0, screen->w, screen->h};
	if (screen->w > width) {
		dst_rect.x = (screen->w - width) >> 1;
		dst_rect.w = width;
	} else {
		src_rect.w = screen->w;
	}
	if (screen->h > height) {
		dst_rect.y = (screen->h - height) >> 1;
		dst_rect.h = height;
	} else {
		src_rect.h = screen->h;
	}

	Uint8 *dirtyRects = hsurf->getDirtyRects();
	if (!dirtyRects) {
		SDL_BlitSurface(sdl_surf, &src_rect, screen, &dst_rect);

		setDirtyRect(dst_rect.x,dst_rect.y,dst_rect.w,dst_rect.h);
	} else {
		int dirty_w = hsurf->getDirtyWidth();
		/*int dirty_h = hsurf->getDirtyHeight();*/
		for (int y=0; y<height>>4; y++) {
			for (int x=0; x<width>>4; x++) {
				if (dirtyRects[y * dirty_w + x]) {
					SDL_Rect src, dst;

					src.x = src_rect.x + (x<<4);
					src.y = src_rect.y + (y<<4);
					src.w = (1<<4);
					src.h = (1<<4);

					dst.x = dst_rect.x + (x<<4);
					dst.y = dst_rect.y + (y<<4);
					dst.w = (1<<4);
					dst.h = (1<<4);

					SDL_BlitSurface(sdl_surf, &src, screen, &dst);

					setDirtyRect(dst.x,dst.y,dst.w,dst.h);
				}
			}
		}

		hsurf->clearDirtyRects();
	}

	/* GUI need to know where it is */
	if (dst_x) {
		*dst_x = dst_rect.x;
	}
	if (dst_y) {
		*dst_y = dst_rect.y;
	}
}

void HostScreen::refreshScreen(void)
{
	if ((screen->flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		SDL_Flip(screen);
		return;
	}

	if (!dirtyMarker) {
		return;
	}

	/* Only update dirtied rects */
	std::vector<SDL_Rect> update_rects(dirtyW*dirtyH);
	int i = 0;
	for (int y=0; y<dirtyH; y++) {
		for (int x=0; x<dirtyW; x++) {
			if (dirtyMarker[y * dirtyW + x]) {
				int maxw = 1<<4, maxh = 1<<4;
				if (screen->w - (x<<4) < (1<<4)) {
					maxw = screen->w - (x<<4);
				}
				if (screen->h - (y<<4) < (1<<4)) {
					maxh = screen->h - (y<<4);
				}

				update_rects[i].x = x<<4;
				update_rects[i].y = y<<4;
				update_rects[i].w = maxw;
				update_rects[i].h = maxh;

				i++;
			}
		}
	}

	SDL_UpdateRects(screen, i, &update_rects[0]);

	clearDirtyRects();
}

void HostScreen::forceRefreshScreen(void)
{
	force_refresh = true;
}

void HostScreen::bootDone(void)
{
	numScreen = SCREEN_LOGO;
}

HostSurface *HostScreen::createSurface(int width, int height, int bpp)
{
	return new HostSurface(width, height, bpp);
}

HostSurface *HostScreen::createSurface(int width, int height, SDL_PixelFormat *pixelFormat)
{
	return new HostSurface(width, height, pixelFormat);
}

void HostScreen::destroySurface(HostSurface *hsurf)
{
	delete hsurf;
}

/**
 * Performs conversion from the TOS's bitplane word order (big endian) data
 * into the native chunky color index.
 */
void HostScreen::bitplaneToChunky( uint16 *atariBitplaneData, uint16 bpp,
	uint8 colorValues[16], int horiz_shift)
{
	uint32 a=0, b=0, c=0, d=0, x;
	uint32 *source = (uint32 *) atariBitplaneData;
	uint16 scrolled[8];

	/* Obviously the different cases can be broken out in various
	 * ways to lessen the amount of work needed for <8 bit modes.
	 * It's doubtful if the usage of those modes warrants it, though.
	 * The branches below should be ~100% correctly predicted and
	 * thus be more or less for free.
	 * Getting the palette values inline does not seem to help
	 * enough to worry about. The palette lookup is much slower than
	 * this code, though, so it would be nice to do something about it.
	 */
	if (horiz_shift) {
		/* Shift the source to deal with horizontal scrolling */
		int i;
		for (i=0; i<bpp; i++) {
			x = (SDL_SwapBE16(atariBitplaneData[i])<<16)
				|SDL_SwapBE16(atariBitplaneData[i+bpp]);
			scrolled[i] = SDL_SwapBE16((x<<horiz_shift)>>16);
		}
		source = (uint32 *) scrolled;
	}

	switch(bpp) {
		case 8:
			d = *source++;
			c = *source++;
			b = *source++;
			a = *source++;
			break;
		case 4:
			d = *source++;
			c = *source++;
			break;
		case 2:
			d = *source++;
			break;
		default:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			d = *source<<16;
#else
			d = *source;
#endif
			break;
	}

	x = a;
	a =  (a & 0xf0f0f0f0)       | ((c & 0xf0f0f0f0) >> 4);
	c = ((x & 0x0f0f0f0f) << 4) |  (c & 0x0f0f0f0f);
	x = b;
	b =  (b & 0xf0f0f0f0)       | ((d & 0xf0f0f0f0) >> 4);
	d = ((x & 0x0f0f0f0f) << 4) |  (d & 0x0f0f0f0f);

	x = a;
	a =  (a & 0xcccccccc)       | ((b & 0xcccccccc) >> 2);
	b = ((x & 0x33333333) << 2) |  (b & 0x33333333);
	x = c;
	c =  (c & 0xcccccccc)       | ((d & 0xcccccccc) >> 2);
	d = ((x & 0x33333333) << 2) |  (d & 0x33333333);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	a = (a & 0x5555aaaa) | ((a & 0x00005555) << 17) | ((a & 0xaaaa0000) >> 17);
	b = (b & 0x5555aaaa) | ((b & 0x00005555) << 17) | ((b & 0xaaaa0000) >> 17);
	c = (c & 0x5555aaaa) | ((c & 0x00005555) << 17) | ((c & 0xaaaa0000) >> 17);
	d = (d & 0x5555aaaa) | ((d & 0x00005555) << 17) | ((d & 0xaaaa0000) >> 17);
	
	colorValues[ 8] = a;
	a >>= 8;
	colorValues[ 0] = a;
	a >>= 8;
	colorValues[ 9] = a;
	a >>= 8;
	colorValues[ 1] = a;
	
	colorValues[10] = b;
	b >>= 8;
	colorValues[ 2] = b;
	b >>= 8;
	colorValues[11] = b;
	b >>= 8;
	colorValues[ 3] = b;
	
	colorValues[12] = c;
	c >>= 8;
	colorValues[ 4] = c;
	c >>= 8;
	colorValues[13] = c;
	c >>= 8;
	colorValues[ 5] = c;
	
	colorValues[14] = d;
	d >>= 8;
	colorValues[ 6] = d;
	d >>= 8;
	colorValues[15] = d;
	d >>= 8;
	colorValues[ 7] = d;
#else
	a = (a & 0xaaaa5555) | ((a & 0x0000aaaa) << 15) | ((a & 0x55550000) >> 15);
	b = (b & 0xaaaa5555) | ((b & 0x0000aaaa) << 15) | ((b & 0x55550000) >> 15);
	c = (c & 0xaaaa5555) | ((c & 0x0000aaaa) << 15) | ((c & 0x55550000) >> 15);
	d = (d & 0xaaaa5555) | ((d & 0x0000aaaa) << 15) | ((d & 0x55550000) >> 15);

	colorValues[ 1] = a;
	a >>= 8;
	colorValues[ 9] = a;
	a >>= 8;
	colorValues[ 0] = a;
	a >>= 8;
	colorValues[ 8] = a;

	colorValues[ 3] = b;
	b >>= 8;
	colorValues[11] = b;
	b >>= 8;
	colorValues[ 2] = b;
	b >>= 8;
	colorValues[10] = b;

	colorValues[ 5] = c;
	c >>= 8;
	colorValues[13] = c;
	c >>= 8;
	colorValues[ 4] = c;
	c >>= 8;
	colorValues[12] = c;

	colorValues[ 7] = d;
	d >>= 8;
	colorValues[15] = d;
	d >>= 8;
	colorValues[ 6] = d;
	d >>= 8;
	colorValues[14] = d;
#endif
}

/*
vim:ts=4:sw=4:
*/
