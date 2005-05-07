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

#ifndef NFVDI_H
#define NFVDI_H

/*--- Includes ---*/

#include "nf_base.h"
#include "parameters.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- Class ---*/

class VdiDriver : public NF_Base
{
	protected:
		void setResolution(int32 width, int32 height, int32 depth);
		int32 getWidth(void);
		int32 getHeight(void);
		int32 openWorkstation(void);
		int32 closeWorkstation(void);

		virtual int32 getPixel(memptr vwk, memptr src, int32 x, int32 y);
		virtual int32 putPixel(memptr vwk, memptr dst, int32 x, int32 y,
			uint32 color);
		virtual int32 drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
			uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor,
			uint32 bgColor, uint32 mouse_type);
		virtual int32 expandArea(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp,
			uint32 fgColor, uint32 bgColor);
		virtual int32 fillArea(memptr vwk, uint32 x_, uint32 y_, int32 w,
			int32 h, memptr pattern_address, uint32 fgColor, uint32 bgColor,
			uint32 logOp, uint32 interior_style);
		virtual int32 blitArea(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);
		virtual int32 drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
			uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
			uint32 logOp, memptr clip);
		virtual int32 fillPoly(memptr vwk, memptr points_addr, int n,
			memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
			uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip);
		virtual void getHwColor(uint16 index, uint32 red, uint32 green,
			uint32 blue, memptr hw_value);
		virtual void setColor(memptr vwk, uint32 paletteIndex, uint32 red,
			uint32 green, uint32 blue);
		virtual int32 getFbAddr(void);
		
	public:
		char *name();
		bool isSuperOnly();
		int32 dispatch(uint32 fncode);

		virtual ~VdiDriver();
};

#endif /* NFVDI_H */
