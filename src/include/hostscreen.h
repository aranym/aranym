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

#include <SDL.h>

#include "dirty_rects.h"

class HostSurface;
class Logo;

class HostScreen: public DirtyRects
{
  protected:
	/* How we want the surface to be rendered on screen */
	enum {
		DRAW_CROPPED,	/* Crop if bigger than screen, center it */
		DRAW_RESCALED	/* Rescale it (only with OpenGL) */
	};

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

	void refreshSurface(HostSurface *hsurf, int flags = DRAW_CROPPED);

	void checkSwitchToVidel(void);
	void checkSwitchVidelNfvdi(void);

	SDL_bool OpenGLVdi;	/* Using NF OpenGL VDI renderer ? */

	Logo *logo;
	bool logo_present;
	bool clear_screen, force_refresh;

	int	refreshCounter;
	bool	renderVidelSurface;
	int	lastVidelWidth, lastVidelHeight, lastVidelBpp;
	int	numScreen;

  protected:
	static const int MIN_WIDTH = 640;
	static const int MIN_HEIGHT = 480;

	SDL_Surface *screen;
	int new_width, new_height;
	uint16 snapCounter; // ALT+PrintScreen to make a snap?

	virtual void setVideoMode(int width, int height, int bpp);

	virtual void refreshScreen(void);
	virtual void initScreen(void);
	virtual void clearScreen(void);
	virtual void drawSurfaceToScreen(HostSurface *hsurf, int *dst_x = NULL,
		int *dst_y = NULL, int flags = DRAW_CROPPED);

  public:
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
		uint8 colorValues[16] );

	// Create a BMP file with a snapshot of the screen surface
	virtual void makeSnapshot(void);

	// Toggle Window/FullScreen mode
	void   toggleFullScreen(void);

	void	refresh(void);
	void	forceRefreshScreen(void);

	void	setVidelRendering(bool videlRender);

	void bootDone(void);

	/* Surface creation */
	virtual HostSurface *createSurface(int width, int height, int bpp);
};

#endif
