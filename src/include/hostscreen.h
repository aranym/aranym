/*
	Hostscreen, base class
	Software renderer

	(C) 2007 ARAnyM developer team

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

#ifndef _HOSTSCREEN_H
#define _HOSTSCREEN_H

#include "parameters.h"

#include "SDL_compat.h"

#include "dirty_rects.h"

class HostSurface;
class Logo;

class HostScreen: public DirtyRects
{
  private:
	enum {
		SCREEN_BOOT,
		SCREEN_LOGO,
		SCREEN_VIDEL,
		SCREEN_NFVDI
	};

	void refreshVidel(void);
	void forceRefreshVidel(void);
	void refreshLogo(void);
	void forceRefreshLogo(void);
	void refreshNfvdi(void);
	void forceRefreshNfvdi(void);
	void refreshGui(void);

	void refreshSurface(HostSurface *hsurf);

	void checkSwitchToVidel(void);
	void checkSwitchVidelNfvdi(void);
	void alphaBlendLogo(bool init);

	SDL_bool OpenGLVdi;	/* Using NF OpenGL VDI renderer ? */

	Logo *logo;
	bool clear_screen, force_refresh, do_screenshot;

	int	refreshCounter;
	bool	renderVidelSurface;
	int	lastVidelWidth, lastVidelHeight, lastVidelBpp;
	int	numScreen;

	SDL_bool grabbedMouse;
	SDL_bool hiddenMouse;
	bool canGrabMouseAgain;
	bool capslockState;
	bool ignoreMouseMotionEvent;
	
	int atari_mouse_xpos;
	int atari_mouse_ypos;

	bool recording;
	
  protected:
	static const int MIN_WIDTH = 640;
	static const int MIN_HEIGHT = 480;

	SDL_Surface *screen;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_Renderer *renderer;
	SDL_Texture *texture;
#endif
	int new_width, new_height;
	int PendingConfigureNotifyWidth, PendingConfigureNotifyHeight;

	uint16 snapCounter; // ALT+PrintScreen to make a snap?

	virtual void setVideoMode(int width, int height, int bpp);

	// Create a BMP file with a snapshot of the screen surface
	void writeSnapshot(SDL_Surface *surf);
	virtual void makeSnapshot(void);

	virtual void refreshScreen(void);
	virtual void initScreen(void);
	virtual void clearScreen(void);
  public:
	virtual void drawSurfaceToScreen(HostSurface *hsurf,
		int *dst_x = NULL, int *dst_y = NULL);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_Window *window;
	Uint32 window_id;
#ifdef SDL_GUI
	SDL_Window *gui_window;
	Uint32 gui_window_id;
#endif
#endif

	HostScreen(void);
	virtual ~HostScreen(void);

	void reset(void);
	void resizeWindow(int new_width, int new_height);

	void OpenGLUpdate(void);	/* Full screen update with NF software VDI */
	void EnableOpenGLVdi(void);
	void DisableOpenGLVdi(void);

	int getWidth(void);
	int getHeight(void);
	virtual int getBpp(void);

	/**
	 * Atari bitplane to chunky conversion helper.
	 **/
	static void bitplaneToChunky( uint16 *atariBitplaneData, uint16 bpp,
		uint8 colorValues[16], int horiz_shift = 0);

	void doScreenshot(void);

	// Toggle Window/FullScreen mode
	void   toggleFullScreen(void);

	void	refresh(void);
	void	forceRefreshScreen(void);

	virtual void refreshScreenFromSurface(SDL_Surface *surface);

	void	setVidelRendering(bool videlRender);

	void bootDone(void);

	/* Surface creation */
	virtual HostSurface *createSurface(int width, int height, int bpp);
	virtual HostSurface *createSurface(int width, int height, SDL_PixelFormat *pixelFormat);
	virtual void destroySurface(HostSurface *hsurf);

	void SetWMIcon();
	void SetCaption(const char *caption);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_Window *Window() { return window; }
#ifdef SDL_GUI
	SDL_Window *GuiWindow() { return gui_window; }
#endif
#endif

	// Mouse handling
	void WarpMouse(int x, int y);

	bool IgnoreMouseMotionEvent() { return ignoreMouseMotionEvent; }
	void IgnoreMouseMotionEvent(bool ignore) { ignoreMouseMotionEvent = ignore; }
	bool CanGrabMouseAgain() { return canGrabMouseAgain; }
	void CanGrabMouseAgain(bool enable) { canGrabMouseAgain = enable; }
	bool CapslockState() { return capslockState; }
	void CapslockState(bool state) { capslockState = state; }
	SDL_bool GrabbedMouse() { return grabbedMouse; }
	SDL_bool hideMouse(SDL_bool hide);
	SDL_bool grabMouse(SDL_bool grab);
	void grabTheMouse();
	void releaseTheMouse();
	void RememberAtariMouseCursorPosition();
	void RestoreAtariMouseCursorPosition();
	bool HasInputFocus();
	bool HasMouseFocus();

	virtual bool Recording(void) { return recording; }
	virtual void StartRecording() { recording = true; }
	virtual void StopRecording() { recording = false; }
};

#endif
