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

#ifndef NFVDI_SOFT_H
#define NFVDI_SOFT_H

/*--- Includes ---*/

#include "parameters.h"

/*--- Defines ---*/

/*--- Types ---*/

/*--- Class ---*/

class SoftVdiDriver : public VdiDriver
{
	public:
		SoftVdiDriver();
		~SoftVdiDriver();

	protected:
		void restoreMouseBackground(void);
		void saveMouseBackground(int16 x, int16 y, int16 width, int16 height);

		int32 openWorkstation(void);
		int32 closeWorkstation(void);
		int32 getPixel(memptr vwk, memptr src, int32 x, int32 y);
		int32 putPixel(memptr vwk, memptr dst, int32 x, int32 y,
			uint32 color);
		int32 drawMouse(memptr wk, int32 x, int32 y, uint32 mode,
			uint32 data, uint32 hot_x, uint32 hot_y, uint32 fgColor,
			uint32 bgColor, uint32 mouse_type);
		int32 expandArea(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp,
			uint32 fgColor, uint32 bgColor);
		int32 fillArea(memptr vwk, uint32 x_, uint32 y_, int32 w,
			int32 h, memptr pattern_address, uint32 fgColor, uint32 bgColor,
			uint32 logOp, uint32 interior_style);
		void	fillArea(uint32 x, uint32 y, uint32 w, uint32 h,
		                 uint16* pattern, uint32 fgColor, uint32 bgColor,
		                 uint32 logOp);
		int32 drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
			uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
			uint32 logOp, memptr clip);
		int32 fillPoly(memptr vwk, memptr points_addr, int n,
			memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
			uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip);
		int32 drawText(memptr vwk, memptr text, uint32 length,
			int32 dst_x, int32 dst_y, memptr font,
			uint32 w, uint32 h, uint32 fgColor, uint32 bgColor,
			uint32 logOp, memptr clip);
		void getHwColor(uint16 index, uint32 red, uint32 green,
			uint32 blue, memptr hw_value);
		void setColor(memptr vwk, uint32 paletteIndex, uint32 red,
			uint32 green, uint32 blue);
		int32 getFbAddr(void);

		int32 blitArea_M2S(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);
		int32 blitArea_S2M(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);
		int32 blitArea_S2S(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);

	private:
		bool clipLine(int x1, int y1, int x2, int y2, int cliprect[]);
		int drawSingleLine(int x1, int y1, int x2, int y2, uint16 pattern,
			uint32 fgColor, uint32 bgColor, int logOp, 
			int cliprect[], int minmax[]);
		int drawTableLine(memptr table, int length, uint16 pattern,
			uint32 fgColor, uint32 bgColor, int logOp, int cliprect[],
			int minmax[]);
		int drawMoveLine(memptr table, int length, memptr index, int moves,
			uint16 pattern, uint32 fgColor, uint32 bgColor, int logOp,
			int cliprect[], int minmax[]);

		/* Functions from hostscreen */
		uint32 hsGetPixel( int x, int y );
		void hsPutPixel( int x, int y, uint32 color );
		void hsFillArea( int x, int y, int w, int h,
			uint16 *pattern, uint32 fgColor, uint32 bgColor,
			uint16 logOp );
		void hsGfxBoxColorPattern( int x, int y, int w, int h,
			uint16 *areaPattern, uint32 fgColor, uint32 bgColor,
			uint16 logOp );
		void hsBlitArea( int sx, int sy, int dx, int dy, int w, int h );
		void hsDrawLine( int x1, int y1, int x2, int y2,
			uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp, int cliprect[]);
		void gfxHLineColor ( int16 x1, int16 x2, int16 y, uint16 pattern,
			uint32 fgColor, uint32 bgColor, uint16 logOp, int cliprect[] );
		void gfxVLineColor( int16 x, int16 y1, int16 y2,
			uint16 pattern, uint32 fgColor, uint32 bgColor, uint16 logOp, int cliprect[] );
		inline bool clipped(int x, int y, int cliprect[])
		{
			if (x < cliprect[0] || x > cliprect[2])
				return true;
			if (y < cliprect[1] || y > cliprect[3])
				return true;
			return false;
		}
		/* SDL 1.2.10 to 1.2.13 has a bug when blitting inside same surface */
		int sdl_buggy_blitsurface;
};

#endif /* NFVDI_SOFT_H */
