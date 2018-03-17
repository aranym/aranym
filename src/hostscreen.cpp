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

#include "newcpu.h"
#include <errno.h>

#include "SDL_compat.h"
#include <vector>
#include <string>

#include "dirty_rects.h"
#include "host_surface.h"
#include "logo.h"
#include "hostscreen.h"
#include "version.h"
#include "parameters.h"	/* bx_options */
#include "main.h"	/* QuitEmulator */
#include "input.h"

#ifdef NFVDI_SUPPORT
# include "nf_objs.h"
# include "nfvdi.h"
#endif

#ifdef SDL_GUI
# include "gui-sdl/sdlgui.h"
#endif

#define DEBUG 0
#include "debug.h"


void HostScreen::SetWMIcon(void)
{
#ifndef OS_darwin
	char path[1024];
	getDataFilename("wm_icon.bmp", path, sizeof(path));
	SDL_Surface *icon = mySDL_LoadBMP_RW(SDL_RWFromFile(path, "rb"), 1);
	if (icon != NULL) {
		SDL_SetColorKey(icon, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_Surface *display_icon = SDL_ConvertSurfaceFormat(icon, SDL_GetWindowPixelFormat(window), 0);
		if (display_icon)
		{
			SDL_FreeSurface(icon);
			icon = display_icon;
		}
		SDL_SetWindowIcon(window, icon);
#else
		SDL_Surface *display_icon = SDL_DisplayFormat(icon);
		if (display_icon)
		{
			SDL_FreeSurface(icon);
			icon = display_icon;
		}
		uint8 mask[] = {0x00, 0x3f, 0xfc, 0x00,
						0x00, 0xff, 0xfe, 0x00,
						0x01, 0xff, 0xff, 0x80,
						0x07, 0xff, 0xff, 0xe0,
						0x0f, 0xff, 0xff, 0xf0,
						0x1f, 0xff, 0xff, 0xf8,
						0x1f, 0xff, 0xff, 0xf8,
						0x3f, 0xff, 0xff, 0xfc,

						0x7f, 0xff, 0xff, 0xfe,
						0x7f, 0xff, 0xff, 0xfe,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,

						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0xff, 0xff, 0xff, 0xff,
						0x7f, 0xff, 0xff, 0xfe,
						0x7f, 0xff, 0xff, 0xfe,
						0x3f, 0xff, 0xff, 0xfe,

						0x3f, 0xff, 0xff, 0xfc,
						0x1f, 0xff, 0xff, 0xf8,
						0x0f, 0xff, 0xff, 0xf8,
						0x0f, 0xff, 0xff, 0xf0,
						0x07, 0xff, 0xff, 0xe0,
						0x01, 0xff, 0xff, 0x80,
						0x00, 0xff, 0xff, 0x00,
						0x00, 0x1f, 0xfc, 0x00};
/*
		uint8 masK[] = {0x01, 0x80, 0x07, 0xfc,
						0x01, 0x80, 0x07, 0xfc,
						0x00, 0x00, 0x3e, 0x03,
						0x00, 0x00, 0x3e, 0x03,
						0x00, 0x00, 0x3e, 0x03,
						0x00, 0x7c, 0xf0, 0x03,
						0x00, 0x7c, 0xf0, 0x03,
						0xc0, 0x7c, 0xc0, 0x1c,

						0xc0, 0x7c, 0xc0, 0x1c,
						0x07, 0x83, 0xc0, 0x63,
						0x07, 0x83, 0xc0, 0x63,
						0x07, 0x9e, 0x31, 0x9c,
						0x07, 0x9e, 0x31, 0x9c,
						0x07, 0x9e, 0x31, 0x9c,
						0x00, 0x7c, 0xfe, 0x00,
						0x00, 0x7c, 0xfe, 0x00,

						0x07, 0xe3, 0x31, 0xe0,
						0x07, 0xe3, 0x31, 0xe0,
						0x3e, 0x1f, 0xff, 0xfc,
						0x3e, 0x1f, 0xff, 0xfc,
						0x38, 0x03, 0x3e, 0x00,
						0xf8, 0x03, 0x3e, 0x00,
						0xf8, 0x03, 0x3e, 0x00,
						0xc0, 0x1c, 0xf1, 0xff,

						0xc0, 0x1c, 0xf1, 0xff,
						0xc0, 0x60, 0xf1, 0xe0,
						0xc0, 0x60, 0xf1, 0xe0,
						0xc1, 0x9c, 0x31, 0x9f,
						0xc1, 0x9c, 0x31, 0x9f,
						0xc1, 0x60, 0x31, 0x9f,
						0x3e, 0x60, 0x01, 0x9c,
						0x3e, 0x60, 0x01, 0x9c};
*/
		SDL_WM_SetIcon(icon, mask);
#endif
		SDL_FreeSurface(icon);
	}
	else {
		infoprint("WM Icon not found at %s", path);
	}
#endif
}


HostScreen::HostScreen(void)
  : DirtyRects(),
  	logo(NULL),
  	clear_screen(true),
	force_refresh(true),
	do_screenshot(false),
	refreshCounter(0),
	grabbedMouse(SDL_FALSE),
	hiddenMouse(SDL_FALSE),
	canGrabMouseAgain(false),
	capslockState(false),
	ignoreMouseMotionEvent(false),
	atari_mouse_xpos(-1),
	atari_mouse_ypos(-1),
	recording(false),
	screen(NULL),
	new_width(0),
	new_height(0),
	PendingConfigureNotifyWidth(-1),
	PendingConfigureNotifyHeight(-1),
	snapCounter(0)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	window = NULL;
	window_id = 0;
	renderer = NULL;
	texture = NULL;
#ifdef SDL_GUI
	gui_window = NULL;
	gui_window_id = 0;
#endif
#endif
}

HostScreen::~HostScreen(void)
{
	if (logo) {
		delete logo;
	}
}

void HostScreen::reset(void)
{
	StopRecording();
	lastVidelWidth = lastVidelHeight = lastVidelBpp = -1;
	numScreen = SCREEN_BOOT;
	setVidelRendering(true);
	DisableOpenGLVdi();

	setVideoMode(MIN_WIDTH,MIN_HEIGHT,8);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	// If double mouse cursor appear in FB Mode do sthg. similar to below code
#else
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
	if (!videoInfo->wm_available) {
		hideMouse(SDL_TRUE);
		SDL_SetWindowGrab(window, SDL_TRUE);
		grabbedMouse = SDL_TRUE;
		canGrabMouseAgain = true;
	}
#endif

	/* Set window caption */
	std::string buf;
#ifdef SDL_GUI
	char key[HOTKEYS_STRING_SIZE];
	keysymToString(key, &bx_options.hotkeys.setup);
	buf = std::string(version_string) + std::string("  (Press the [") + std::string(key) + std::string("] key for SETUP)");
#else
	buf = std::string(version_string);
#endif /* SDL_GUI */
	SetCaption(buf.c_str());
}

void  HostScreen::SetCaption(const char *caption)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetWindowTitle(window, caption);
	/* SDL2FIXME: have not found a way yet to set the window class */
#else
	SDL_WM_SetCaption(caption, "ARAnyM");
#endif
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

static int get_screenshot_counter(void)
{
	char dummy[5];
	int i, num;
	DIR *workingdir;
	struct dirent *file;
	int nScreenShots = 0;
	
	workingdir = opendir(bx_options.snapshot_dir);
	if (workingdir == NULL)
	{
		HostFilesys::makeDir(bx_options.snapshot_dir);
		workingdir = opendir(bx_options.snapshot_dir);
	}
	if (workingdir != NULL)
	{
		file = readdir(workingdir);
		while (file != NULL)
		{
			if (strncmp("snap", file->d_name, 4) == 0 )
			{
				/* copy next 4 numbers */
				for (i = 0; i < 4; i++)
				{
					if (file->d_name[4+i] >= '0' && file->d_name[4+i] <= '9')
						dummy[i] = file->d_name[4+i];
					else
						break;
				}

				dummy[i] = '\0'; /* null terminate */
				num = atoi(dummy);
				if (num >= nScreenShots)
					nScreenShots = num + 1;
			}
			/* next file.. */
			file = readdir(workingdir);
		}

		closedir(workingdir);
	}		
	
	return nScreenShots;
}


void HostScreen::writeSnapshot(SDL_Surface *surf)
{
	char filename[15];
	char path[PATH_MAX];

	if (snapCounter == 0)
		snapCounter = get_screenshot_counter();
	sprintf(filename, "snap%03d.bmp", snapCounter++ );
	safe_strncpy(path, bx_options.snapshot_dir, sizeof(path));
	addFilename(path, filename, sizeof(path));
	if (SDL_SaveBMP(surf, path) < 0)
	{
#ifdef __CYGWIN__
		bug("%s: %s", SDL_GetError(), win32_errstring(GetLastError()));
#else
		bug("%s: %s", SDL_GetError(), strerror(errno));
#endif
	} else
	{
		bug("saved screenshot %s", path);
	}
}

void HostScreen::makeSnapshot(void)
{
	writeSnapshot(screen);
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

	/* Use current fullscreen mode ? */
	if (bx_options.video.fullscreen && bx_options.autozoom.fixedsize
	    && (bx_options.autozoom.width==0) && (bx_options.autozoom.height==0))
	{
		width = height = 0;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)

	int windowFlags = SDL_WINDOW_MOUSE_FOCUS;
	if (!bx_options.autozoom.fixedsize) {
		windowFlags |= SDL_WINDOW_RESIZABLE;
	}
	if (bx_options.video.fullscreen) {
		windowFlags |= SDL_WINDOW_FULLSCREEN;
	}

	// set preferred window position
	const char *wpos = bx_options.video.window_pos;
	int x, y;
	x = y = SDL_WINDOWPOS_UNDEFINED;
	if (strlen(wpos) > 0) {
		if (strncasecmp(wpos, "center", strlen("center")) == 0) {
			x = y = SDL_WINDOWPOS_CENTERED;
		}
		else {
			sscanf(wpos, "%d,%d", &x, &y);
		}
	}

	SDL_DisplayMode mode, oldmode;

	if (window)
		SDL_GetWindowDisplayMode(window, &oldmode);
	else
		memset(&oldmode, 0, sizeof(oldmode));
	
	mode.w = width;
	mode.h = height;
	mode.refresh_rate = 0;
	mode.driverdata = 0;
	if (bpp <= 8)
		mode.format = SDL_PIXELFORMAT_INDEX8;
	else if (bpp <= 16)
		mode.format = SDL_PIXELFORMAT_RGB565;
	else if (bpp <= 24)
		mode.format = SDL_BITSPERPIXEL(oldmode.format) >= 24 ? oldmode.format : (Uint32)SDL_PIXELFORMAT_RGB24;
	else
		mode.format = SDL_BITSPERPIXEL(oldmode.format) >= 24 ? oldmode.format : (Uint32)SDL_PIXELFORMAT_RGBX8888;

	if (window == NULL ||
		width != oldmode.w ||
		height != oldmode.h ||
		mode.format != oldmode.format)
	{
#ifdef USE_FIXED_CONSOLE_FBVIDEOMODE
// Raspberry doesn't allow switching video mode. Keep current mode
#endif
		if (renderer)
		{
			SDL_DestroyRenderer(renderer);
			renderer = NULL;
		}
		if (texture)
		{
			SDL_DestroyTexture(texture);
			texture = NULL;
		}
		
		if (window == NULL)
		{
			window = SDL_CreateWindow(version_string, x, y, width, height, windowFlags);
			if (window)
			{
				SDL_GetWindowDisplayMode(window, &oldmode);
				if (bpp >= 24 && (int)SDL_BITSPERPIXEL(oldmode.format) >= bpp)
					mode.format = oldmode.format;
			}
		}
		
		if (window==NULL) {
			panicbug("Could not create window: %s", SDL_GetError());
			QuitEmulator();
			return;
		}
		window_id = SDL_GetWindowID(window);
		
		if (mode.format != oldmode.format ||
			width != oldmode.w ||
			height != oldmode.h)
		{
			if (SDL_SetWindowDisplayMode(window, &mode) < 0)
			{
				mode.format = oldmode.format;
				SDL_SetWindowDisplayMode(window, &mode);
			}
		}
		SDL_SetWindowSize(window, width, height);
		/* SDL2FIXME: find appropriate renderer for bpp */
		// renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	}
	screen = SDL_GetWindowSurface(window);
	// texture = SDL_CreateTextureFromSurface(renderer, screen);
	
#else

#ifdef __ANDROID__
	int screenFlags = SDL_SWSURFACE|SDL_HWPALETTE;
#else
	int screenFlags = SDL_HWSURFACE|SDL_HWPALETTE;
#endif
	if (!bx_options.autozoom.fixedsize) {
		screenFlags |= SDL_RESIZABLE;
	}
	if (bx_options.video.fullscreen) {
		screenFlags |= SDL_FULLSCREEN;
	}

	// set preferred window position
	const char *wpos = bx_options.video.window_pos;
	if (strlen(wpos) > 0) {
		if (strncasecmp(wpos, "center", strlen("center")) == 0) {
			SDL_putenv((char*)"SDL_VIDEO_CENTERED=1");
		}
		else {
			static char var[64];
			snprintf(var, sizeof(var), "SDL_VIDEO_WINDOW_POS=%s", wpos);
			SDL_putenv(var);
		}
	}

#ifdef USE_FIXED_CONSOLE_FBVIDEOMODE
// Raspberry doesn't allow switching video mode. Keep current mode
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
	if (!videoInfo->wm_available) {
	   width = videoInfo->current_w;
	   height = videoInfo->current_h;
	   bpp = videoInfo->vfmt->BitsPerPixel;
	}
#endif

	{
		SDL_Rect **modes;
		modes = SDL_ListModes(NULL, screenFlags);
		if (modes != (SDL_Rect **)-1 && modes != (SDL_Rect **)0)
		{
			int i;
			for (i = 0; modes[i] != NULL; i++)
			{
			}
			if (i == 1)
			{
				width = modes[0]->w;
				height = modes[0]->h;
				bug("only one video mode, using %dx%d", width, height);
			}
		}
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

#endif

	if (screen==NULL) {
		panicbug("Can not set video mode");
		QuitEmulator();
		return;
	}

	SetWMIcon();

	SDL_SetClipRect(screen, NULL);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	bx_options.video.fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == SDL_WINDOW_FULLSCREEN;
#else
	bx_options.video.fullscreen = ((screen->flags & SDL_FULLSCREEN) == SDL_FULLSCREEN);
#endif

	new_width = screen->w;
	new_height = screen->h;
	resizeDirty(screen->w, screen->h);

	forceRefreshScreen();
}

void HostScreen::resizeWindow(int new_width, int new_height)
{
	if (PendingConfigureNotifyWidth >= 0 &&
		PendingConfigureNotifyHeight >= 0)
	{
		if (PendingConfigureNotifyWidth == new_width &&
			PendingConfigureNotifyHeight == new_height)
		{
			/*
			 * Event is from setVideoMode, so ignore.
			 */
			PendingConfigureNotifyWidth = -1;
			PendingConfigureNotifyHeight = -1;
		}
		return;
	}
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
	if (!SDLGui_isClosed()
#if SDL_VERSION_ATLEAST(2, 0, 0)
		&& window == gui_window
#endif
		)
	{
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
	getVIDEL()->forceRefresh();
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
	if (!logo) {
		char path[1024];
		getDataFilename(LOGO_FILENAME, path, sizeof(path));

		logo = new Logo(path);
		if (!logo) {
			return;
		}
	}

	HostSurface *logo_hsurf = logo->getSurface();
	if (!logo_hsurf) {
		return;
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
	if (logo)
		logo->alphaBlend(init);
}

void HostScreen::checkSwitchToVidel(void)
{
	/* No logo ? */
	if (!logo || !logo->getSurface()) {
		numScreen = SCREEN_VIDEL;
		return;
	}

	/* Wait for Videl surface to be ready */
	HostSurface *videl_hsurf = getVIDEL()->getSurface();
	if (!videl_hsurf) {
		return;
	}

	if ((videl_hsurf->getWidth()>64) && (videl_hsurf->getHeight()>64)) {
#if 0 /* set to 1 for debugging to make logo visible, if machine is too fast */
		if (numScreen == SCREEN_LOGO && !bx_options.opengl.enabled)
		{
			SDL_Flip(screen);
			usleep(1000000);
		}
#endif
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

void HostScreen::WarpMouse(int x, int y)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_WarpMouseInWindow(window, x, y);
#else
	SDL_WarpMouse(x, y);
#endif
}

SDL_bool HostScreen::hideMouse(SDL_bool hide)
{
	SDL_bool current = hiddenMouse;
	if (hide) {
		SDL_SetCursor(empty_cursor);
#if defined(SDL_VIDEO_DRIVER_X11)
		if (bx_options.startup.grabMouseAllowed && SDL_IsVideoDriver("x11"))
			SDL_ShowCursor(SDL_DISABLE);
#endif
#if defined(SDL_VIDEO_DRIVER_QUARTZ)
		if (bx_options.startup.grabMouseAllowed && SDL_IsVideoDriver("Quartz"))
			SDL_ShowCursor(SDL_DISABLE);
#endif
		hiddenMouse = SDL_TRUE;
	}
	else if (!hide) {
		SDL_SetCursor(aranym_cursor);
#if defined(SDL_VIDEO_DRIVER_X11)
		if (bx_options.startup.grabMouseAllowed && SDL_IsVideoDriver("x11"))
			SDL_ShowCursor(SDL_ENABLE);
#endif
#if defined(SDL_VIDEO_DRIVER_QUARTZ)
		if (bx_options.startup.grabMouseAllowed && SDL_IsVideoDriver("Quartz"))
			SDL_ShowCursor(SDL_ENABLE);
#endif
		hiddenMouse = SDL_FALSE;
	}
	return current;
}

SDL_bool HostScreen::grabMouse(SDL_bool grab)
{
	SDL_bool current = SDL_GetWindowGrab(window);
	if (grab && !current) {
		SDL_SetWindowGrab(window, SDL_TRUE);
		grabbedMouse = SDL_TRUE;
		canGrabMouseAgain = true;
		hideMouse(SDL_TRUE);
		if (bx_options.startup.grabMouseAllowed)
			IgnoreMouseMotionEvent(true);
	}
	else if (!grab && current) {
		SDL_SetWindowGrab(window, SDL_FALSE);
		grabbedMouse = SDL_FALSE;
		hideMouse(SDL_FALSE);
	}

	// show hint in the window caption
	if (SDL_GetWindowGrab(window))
	{
		char ungrab_key[HOTKEYS_STRING_SIZE];
		std::string buf;
		keysymToString(ungrab_key, &bx_options.hotkeys.ungrab);
#ifdef SDL_GUI
		char setup_key[HOTKEYS_STRING_SIZE];
		keysymToString(setup_key, &bx_options.hotkeys.setup);
		buf = "ARAnyM: press [" + std::string(setup_key) + std::string("] for SETUP, [") + std::string(ungrab_key) + std::string("] or middle mouse button to release input grab");
#else
		buf = std::string("ARAnyM: press [") + std::string(ungrab_key) + std::string("] or middle mouse button to release input grab");
#endif
		SetCaption(buf.c_str());
	}
	else {
		SetCaption("ARAnyM: no input grab");
	}

	return current;
}


void HostScreen::grabTheMouse()
{
#if DEBUG
	int x,y;
	SDL_GetMouseState(&x, &y);
	D(bug("grabTheMouse: mouse grab at window position [%d,%d]", x, y));
#endif
	hideMouse(SDL_TRUE);
	grabMouse(SDL_TRUE);
	RestoreAtariMouseCursorPosition();
}


void HostScreen::releaseTheMouse()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	// ToDo: If double mouse cursor appear in FB Mode do sthg. similar to below
#else
	const SDL_VideoInfo *videoInfo = SDL_GetVideoInfo();
	if (!videoInfo->wm_available && hiddenMouse) return;
#endif

	D(bug("Releasing the mouse grab"));
	grabMouse(SDL_FALSE);	// release mouse
	hideMouse(SDL_FALSE);	// show it
	ARADATA *ara = getARADATA();
	if (ara && ara->isAtariMouseDriver()) {
		RememberAtariMouseCursorPosition();
		WarpMouse(atari_mouse_xpos, atari_mouse_ypos);
	}
}


// remember the current Atari mouse cursor position
void HostScreen::RememberAtariMouseCursorPosition()
{
	ARADATA *ara = getARADATA();
	if (ara && ara->isAtariMouseDriver()) {
		atari_mouse_xpos = getARADATA()->getAtariMouseX();
		atari_mouse_ypos = getARADATA()->getAtariMouseY();
		D(bug("Atari mouse cursor pointer left at [%d, %d]", atari_mouse_xpos, atari_mouse_ypos));
	}
}

// reposition the Atari mouse cursor to match the host mouse cursor position
void HostScreen::RestoreAtariMouseCursorPosition()
{
	int xpos, ypos;
	SDL_GetMouseState(&xpos, &ypos);
	if (atari_mouse_xpos >= 0 && atari_mouse_ypos >= 0) {
		D(bug("Restoring mouse cursor pointer to [%d, %d]", xpos, ypos));
//		getARADATA()->setAtariMousePosition(xpos, ypos);
		int delta_x = xpos - atari_mouse_xpos;
		int delta_y = ypos - atari_mouse_ypos;
		if (delta_x || delta_y) {
			D(bug("RestoreAtariMouse: %d,%d atari %d,%d delta %d,%d\n", xpos, ypos, atari_mouse_xpos, atari_mouse_ypos, delta_x, delta_y));
			getIKBD()->SendMouseMotion(delta_x, delta_y, 0, true);
		}
	}
}


bool HostScreen::HasInputFocus()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	return (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
#else
	return (SDL_GetAppState() & SDL_APPINPUTFOCUS) != 0;
#endif
}


bool HostScreen::HasMouseFocus()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	Uint32 flags = SDL_GetWindowFlags(window);
	return (flags & (SDL_WINDOW_MOUSE_FOCUS|SDL_WINDOW_SHOWN)) == (SDL_WINDOW_MOUSE_FOCUS|SDL_WINDOW_SHOWN);
#else
	Uint8 state = SDL_GetAppState();
	return (state & (SDL_APPMOUSEFOCUS|SDL_APPACTIVE)) == (SDL_APPMOUSEFOCUS|SDL_APPACTIVE);
#endif
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
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_Color colors[256];
		for (int i=0; i<256; i++) {
			colors[i].r = sdl_surf->format->palette->colors[i].r;
			colors[i].g = sdl_surf->format->palette->colors[i].g;
			colors[i].b = sdl_surf->format->palette->colors[i].b;
			colors[i].a = WINDOW_ALPHA;
		}
		SDL_SetPaletteColors(screen->format->palette, colors, 0, 256);
#else
		SDL_Color palette[256];
		for (int i=0; i<256; i++) {
			palette[i].r = sdl_surf->format->palette->colors[i].r;
			palette[i].g = sdl_surf->format->palette->colors[i].g;
			palette[i].b = sdl_surf->format->palette->colors[i].b;
		}
		SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, palette, 0,256);
#endif
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

	SDL_Rect src_rect = {0,0, Uint16(width), Uint16(height)};
	SDL_Rect dst_rect = {0,0, Uint16(screen->w), Uint16(screen->h)};
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
	} 
	else if(hsurf->hasDirtyRect()) 
	{
		if(bx_options.video.single_blit_composing) {
			// redraw only using one SDL Blit
			
			SDL_Rect src, dst;
			dst.x = dst_rect.x +  hsurf->getMinDirtX();
			dst.y = dst_rect.y +  hsurf->getMinDirtY();
			dst.w = dst_rect.w +  hsurf->getMaxDirtX()-hsurf->getMinDirtX()+1;
			dst.h = dst_rect.h +  hsurf->getMaxDirtY()-hsurf->getMinDirtY()+1;
			src.x = src_rect.x +  hsurf->getMinDirtX();
			src.y = src_rect.y +  hsurf->getMinDirtY();
			src.w = src_rect.w +  hsurf->getMaxDirtX()-hsurf->getMinDirtX()+1;
			src.h = src_rect.h +  hsurf->getMaxDirtY()-hsurf->getMinDirtX()+1;
			
			SDL_BlitSurface(sdl_surf, &src, screen, &dst);
			setDirtyRect(dst.x,dst.y,dst.w,dst.h);
		}
		else {
			// Chunky redraw: blit multiple 16x16 rectangles
			int dirty_w = hsurf->getDirtyWidth();
			int dirty_h = hsurf->getDirtyHeight();
			
			for (int y=0; y<dirty_h; y++) {
				int block_w=0;
				int block_x=0;
				int num_lines = height - (y<<4);
				
				if (num_lines>16) {
					num_lines=16;
				}
				
				for (int x=0; x<dirty_w; x++) {
					int block_update = (x==dirty_w-1);	/* Force update on last column */
					int num_cols = width - (x<<4);
					
					if (num_cols>16) {
						num_cols=16;
					}
					
					if (dirtyRects[y * dirty_w + x]) {
						/* Dirty */
						if (block_w==0) {
							/* First dirty block, mark x pos */
							block_x = x;
						}
						block_w += num_cols;
					} else {
						/* Non dirty, force update of previously merged blocks */
						block_update = 1;
					}
					
					/* Update only if we have a dirty block */
					if (block_update && (block_w>0)) {
						SDL_Rect src, dst;
						
						src.x = src_rect.x + (block_x<<4);
						src.y = src_rect.y + (y<<4);
						src.w = block_w;
						src.h = num_lines;
						
						dst.x = dst_rect.x + (block_x<<4);
						dst.y = dst_rect.y + (y<<4);
						dst.w = block_w;
						dst.h = num_lines;
						
						SDL_BlitSurface(sdl_surf, &src, screen, &dst);
						
						setDirtyRect(dst.x,dst.y,dst.w,dst.h);
						
						block_w = 0;
					}
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
#if !SDL_VERSION_ATLEAST(2, 0, 0)
	if ((screen->flags & SDL_DOUBLEBUF)==SDL_DOUBLEBUF) {
		SDL_Flip(screen);
		return;
	}
#endif

	if (!dirtyMarker) {
		return;
	}

	if(!hasDirtyRect()) 
		return;
	
	if(bx_options.video.single_blit_refresh) {
		/* Only update dirty rect with a single update*/
		SDL_Rect update_rect;
		update_rect.x=minDirtX;
		update_rect.y=minDirtY;
		update_rect.w=maxDirtX-minDirtX+1;
		update_rect.h=maxDirtY-minDirtY+1;
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_UpdateWindowSurfaceRects(window, &update_rect, 1);
		if (renderer)
			SDL_RenderPresent(renderer);
#else
		SDL_UpdateRects(screen, 1, &update_rect);
#endif
	}
	else {
		// Chunky redraw: blit multiple 16x16 rectangles
		/* Only update dirtied rects */
		std::vector<SDL_Rect> update_rects(dirtyW*dirtyH);
		int i = 0;
		for (int y=0; y<dirtyH; y++) {
			int block_w = 0;
			int block_x = 0;
			int maxh = 1<<4;
			
			if (screen->h - (y<<4) < (1<<4)) {
				maxh = screen->h - (y<<4);
			}
			
			for (int x=0; x<dirtyW; x++) {
				int block_update = (x==dirtyW-1);	/* Force update on last column */
				int maxw = 1<<4;
				
				if (screen->w - (x<<4) < (1<<4)) {
					maxw = screen->w - (x<<4);
				}
				
				if (dirtyMarker[y * dirtyW + x]) {
					/* Dirty */
					if (block_w==0) {
						/* First dirty block, mark x pos */
						block_x = x;
					}
					block_w += maxw;
				} else {
					/* Non dirty, force update of previously merged blocks */
					block_update = 1;
				}
				
				/* Update only if we have a dirty block */
				if (block_update && (block_w>0)) {
					update_rects[i].x = block_x<<4;
					update_rects[i].y = y<<4;
					update_rects[i].w = block_w;
					update_rects[i].h = maxh;
					
					i++;
					
					block_w = 0;
				}
			}
		}
				
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_UpdateWindowSurfaceRects(window, &update_rects[0], i);
		if (renderer)
			SDL_RenderPresent(renderer);
#else
		SDL_UpdateRects(screen, i, &update_rects[0]);
#endif
	}	
	clearDirtyRects();
}

void HostScreen::refreshScreenFromSurface(SDL_Surface *surface)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	// SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
	// SDL_RenderClear(renderer);
	// SDL_RenderCopy(renderer, texture, NULL, NULL);
	// SDL_RenderPresent(renderer);
	(void) surface;
	SDL_UpdateWindowSurface(window);
	if (renderer)
		SDL_RenderPresent(renderer);
#else
	SDL_Rect update_rect;
	update_rect.x=minDirtX;
	update_rect.y=minDirtY;
	update_rect.w=maxDirtX-minDirtX+1;
	update_rect.h=maxDirtY-minDirtY+1;
	SDL_UpdateRects(surface, 1, &update_rect);
#endif
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
			{
				uint16 *source16 = (uint16 *) source;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				d = (*source16)<<16;
#else
				d = *source16;
#endif
			}
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
