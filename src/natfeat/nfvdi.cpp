/*
	NatFeat VDI driver

	ARAnyM (C) 2005 Patrice Mandin

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

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "parameters.h"
#include "nfvdi.h"
#include "../../atari/fvdi/drivers/aranym/fvdidrv_nfapi.h"
#include "hostscreen.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define EINVFN -32

/*--- Types ---*/

/*--- Variables ---*/

extern HostScreen hostScreen;

/*--- Public functions ---*/

char *VdiDriver::name()
{
	return "fVDI";
}

bool VdiDriver::isSuperOnly()
{
	return false;
}

int32 VdiDriver::dispatch(uint32 fncode)
{
	int32 ret = EINVFN;

	D(bug("nfvdi: dispatch(%u)", fncode));

	// Thread safety patch (remove it once the fVDI screen output is in the main thread)
	hostScreen.lock();

	switch(fncode) {
		case FVDI_GET_VERSION:
    		ret = FVDIDRV_NFAPI_VERSION;
			break;
		case FVDI_GET_PIXEL:
			ret = getPixel((memptr)getParameter(0),(memptr)getParameter(1),getParameter(2),getParameter(3));
			break;
		case FVDI_PUT_PIXEL:
			ret = putPixel((memptr)getParameter(0),(memptr)getParameter(1),getParameter(2),getParameter(3),getParameter(4));
			break;
		case FVDI_MOUSE:
			{
				// mode (0 - move, 1 - hide, 2 - show)
				// These are only valid when not mode
				uint32 mask = getParameter(3);
				if ( mask > 3 ) {
					ret = drawMouse((memptr)getParameter(0),	// wk
						getParameter(1), getParameter(2),	// x, y
						mask,                                // mask*
						getParameter(4),			            // data*
						getParameter(5), getParameter(6),	// hot_x, hot_y
						getParameter(7),			// fgColor
						getParameter(8),			// bgColor
						getParameter(9));			        // type
				} else {
					ret = drawMouse((memptr)getParameter(0),		        // wk
						getParameter(1), getParameter(2),	// x, y
						mask,
						0, 0, 0, 0, 0, 0); // dummy
				}
			}
			break;
		case FVDI_EXPAND_AREA:
			ret = expandArea((memptr)getParameter(0),		// vwk
			                    (memptr)getParameter(1),		// src MFDB*
			                    getParameter(2), getParameter(3),	// sx, sy
			                    (memptr)getParameter(4),		// dest MFDB*
			                    getParameter(5), getParameter(6),	// dx, dy
			                    getParameter(7),			// width
			                    getParameter(8),			// height
			                    getParameter(9),			// logical operation
			                  getParameter(10),			// fgColor
			                  getParameter(11));			// bgColor
			break;
		case FVDI_FILL_AREA:
			ret = fillArea((memptr)getParameter(0),		// vwk
			                  getParameter(1), getParameter(2),	// x, y
			                                               		// table*, table length/type (0 - y/x1/x2 spans)
			                  getParameter(3), getParameter(4),	// width, height
			                  (memptr)getParameter(5),		// pattern*
			                  getParameter(6),			// fgColor
			                  getParameter(7),			// bgColor
			                  getParameter(8),			// mode
			                  getParameter(9));			// interior style
			break;
		case FVDI_BLIT_AREA:
			ret = blitArea((memptr)getParameter(0),		// vwk
			                  (memptr)getParameter(1),		// src MFDB*
			                  getParameter(2), getParameter(3),	// sx, sy
			                  (memptr)getParameter(4),		// dest MFDB*
			                  getParameter(5), getParameter(6),	// dx, dy
			                  getParameter(7),			// width
			                  getParameter(8),			// height
			                  getParameter(9));			// logical operation
			break;
		case FVDI_LINE:
			ret = drawLine((memptr)getParameter(0),		// vwk
			                  getParameter(1), getParameter(2),	// x1, y1
			                                                	// table*, table length/type (0 - coordinate pairs, 1 - pairs + moves)
			                  getParameter(3), getParameter(4),	// x2, y2
			                                                	// move point count, move index*
			                  getParameter(5) & 0xffff,		// pattern
			                  getParameter(6),			// fgColor
			                  getParameter(7),			// bgColor
			                  getParameter(8) & 0xffff,		// logical operation
			                  (memptr)getParameter(9));		// clip rectangle* (0 or long[4])
			break;
		case FVDI_FILL_POLYGON:
			ret = fillPoly((memptr)getParameter(0),		// vwk
			                  (memptr)getParameter(1),		// points*
			                  getParameter(2),			// point count
			                  (memptr)getParameter(3),		// index*
			                  getParameter(4),			// index count
			                  (memptr)getParameter(5),		// pattern*
			                  getParameter(6),			// fgColor
			                  getParameter(7),			// bgColor
			                  getParameter(8),			// logic operation
			                  getParameter(9),			// interior style
			                  (memptr)getParameter(10));		// clip rectangle
			break;
		case FVDI_GET_HWCOLOR:
			getHwColor(getParameter(0), getParameter(1), getParameter(2), getParameter(3), getParameter(4));
			break;
		case FVDI_SET_COLOR:
			setColor(getParameter(4), getParameter(0), getParameter(1), getParameter(2), getParameter(3));
			break;
		case FVDI_GET_FBADDR:
			ret = getFbAddr();
			break;
		case FVDI_SET_RESOLUTION:
			setResolution(getParameter(0),getParameter(1),getParameter(2));
			ret = 0;
			break;
		case FVDI_GET_WIDTH:
			ret = getWidth();
			break;
		case FVDI_GET_HEIGHT:
			ret = getHeight();
			break;
		case FVDI_OPENWK:
			ret = openWorkstation();
			break;
		case FVDI_CLOSEWK:
			ret = closeWorkstation();
			break;
		default:
			D(bug("nfvdi: unimplemented function #%d", fncode));
			break;
	}

	// Thread safety patch (remove it once the fVDI screen output is in the main thread)
	hostScreen.unlock();

	D(bug("nfvdi: function returning with 0x%08x", ret));
	return ret;
}

VdiDriver::~VdiDriver()
{
}

/*--- Protected functions ---*/

void VdiDriver::setResolution(int32 width, int32 height, int32 depth)
{
	hostScreen.setWindowSize(width, height, depth > 8 ? depth : 8);
}

int32 VdiDriver::getWidth(void)
{
	return hostScreen.getWidth();
}

int32 VdiDriver::getHeight(void)
{
	return hostScreen.getHeight();
}

int32 VdiDriver::openWorkstation(void)
{
	getVIDEL()->setRendering(false);
	return 1;
}

int32 VdiDriver::closeWorkstation(void)
{
	getVIDEL()->setRendering(true);
	return 1;
}
