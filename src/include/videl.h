/*
 * $Header$
 *
 * Joy 2001
 */

#ifndef _VIDEL_H
#define _VIDEL_H

#include "icio.h"
#include "hostscreen.h"

#include <SDL/SDL.h>

class VIDEL : public BASE_IO {
protected:
	int width, height, bpp, since_last_change;
	bool hostColorsSync;
	bool doRender; // the HW surface is available -> videl writes directly into the Host videoram

	void renderScreenNoFlag();
	void renderScreenNoZoom();
	void renderScreenZoom();

	/* Autozoom */
	int zoomwidth;
	int zoomheight;
	int *zoomxtable;
	int *zoomytable;
public:
	VIDEL();
	void init();

	virtual void handleWrite(uint32 addr, uint8 value);

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
	doRender = render;
}


#endif


/*
 * $Log$
 * Revision 1.19  2002/12/29 13:54:46  joy
 * linewidth and lineoffset registers emulated
 *
 * Revision 1.18  2002/09/24 16:08:24  pmandin
 * Bugfixes+preliminary autozoom support
 *
 * Revision 1.17  2002/04/22 18:30:50  milan
 * header files reform
 *
 * Revision 1.16  2002/02/28 20:44:22  joy
 * uae_ vars replaced with uint's
 *
 * Revision 1.15  2001/12/17 08:33:00  standa
 * Thread synchronization added. The check_event and fvdidriver actions are
 * synchronized each to other.
 *
 * Revision 1.14  2001/12/17 07:09:44  standa
 * SDL_delay() added to setRendering to avoid SDL access the graphics from
 * both VIDEL and fVDIDriver threads....
 *
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

