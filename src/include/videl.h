/*
 * $Header$
 *
 * Joy 2001
 */

#ifndef _VIDEL_H
#define _VIDEL_H

#include "icio.h"
#include <SDL.h>

class VIDEL : public BASE_IO {
protected:
	int width, height, bpp, od_posledni_zmeny;
	bool hostColorsSync;
	bool doRender; // the HW surface is available -> videl writes directly into the Host videoram

	void renderScreenNoFlag();

public:
	VIDEL();
	void init();

	virtual void handleWrite(uaecptr addr, uint8 value);

	void updateColors();
	void renderScreen();
	void setRendering( bool render );

	int getScreenBpp();
	int getScreenWidth();
	int getScreenHeight();

	long getVideoramAddress();
};


inline void VIDEL::renderScreen() {
	if ( doRender )
		renderScreenNoFlag();
}

inline void VIDEL::setRendering( bool render ) {
	bool tempRender = doRender; // FIXME

	doRender = render;

	// STanda: FIXME!!! HACK.... the SDL thread safety problem: VIDEL runs in different thread than fVDIDrv.
	// Either needs to be synchronized or all SDL calls should go from a single thread!!!
	if (doRender == tempRender)
		return;
	SDL_Delay(250);
	// FIXME: HACK
}


#endif


/*
 * $Log$
 * Revision 1.13  2001/11/07 21:18:25  milan
 * SDL_CFLAGS in CXXFLAGS now.
 *
 * Revision 1.12  2001/11/04 23:17:08  standa
 * 8bit destination surface support in VIDEL. Blit routine optimalization.
 * Bugfix in compatibility modes palette copying.
 *
 * Revision 1.11  2001/08/28 23:26:09  standa
 * The fVDI driver update.
 * VIDEL got the doRender flag with setter setRendering().
 *       The host_colors_uptodate variable name was changed to hostColorsSync.
 * HostScreen got the doUpdate flag cleared upon initialization if the HWSURFACE
 *       was created.
 * fVDIDriver got first version of drawLine and fillArea (thanks to SDL_gfxPrimitives).
 *
 * Revision 1.10  2001/06/18 13:21:55  standa
 * Several template.cpp like comments were added.
 * HostScreen SDL encapsulation class.
 *
 * Revision 1.9  2001/06/17 21:56:32  joy
 * updated to reflect changes in .cpp counterpart
 *
 * Revision 1.8  2001/06/15 14:16:06  joy
 * VIDEL palette registers are now processed by the VIDEL object.
 *
 * Revision 1.7  2001/06/13 07:12:39  standa
 * Various methods renamed to conform the sementics.
 * Added videl fuctions needed for VDI driver.
 *
 *
 */

