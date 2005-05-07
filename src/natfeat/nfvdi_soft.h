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
		int32 blitArea(memptr vwk, memptr src, int32 sx, int32 sy,
			memptr dest, int32 dx, int32 dy, int32 w, int32 h, uint32 logOp);
		int32 drawLine(memptr vwk, uint32 x1_, uint32 y1_, uint32 x2_,
			uint32 y2_, uint32 pattern, uint32 fgColor, uint32 bgColor,
			uint32 logOp, memptr clip);
		int32 fillPoly(memptr vwk, memptr points_addr, int n,
			memptr index_addr, int moves, memptr pattern_addr, uint32 fgColor,
			uint32 bgColor, uint32 logOp, uint32 interior_style, memptr clip);
		void getHwColor(uint16 index, uint32 red, uint32 green,
			uint32 blue, memptr hw_value);
		void setColor(memptr vwk, uint32 paletteIndex, uint32 red,
			uint32 green, uint32 blue);
		int32 getFbAddr(void);

	private:
		struct _Mouse {
			struct {
				uint16 x, y;
				uint16 width, height;
				uint32 background[16][16]; // The mouse background backup surface for FVDIDriver
				struct {
					uint32 foreground; // The mouse shape color for FVDIDriver
					uint32 background; // The mouse mask color for FVDIDriver
				} color;
			} storage;
			struct hotspot_ {
				int16  x, y;
			} hotspot;
			struct colors_ {
				int16  fgColorIndex, bgColorIndex;
			} colors;

			uint16 mask[16];
			uint16 shape[16];
		} Mouse;

		int drawSingleLine(int x1, int y1, int x2, int y2, uint16 pattern,
			uint32 fgColor, uint32 bgColor, int logOp, bool last_pixel,
			int cliprect[], int minmax[]);
		int drawTableLine(memptr table, int length, uint16 pattern,
			uint32 fgColor, uint32 bgColor, int logOp, int cliprect[],
			int minmax[]);
		int drawMoveLine(memptr table, int length, memptr index, int moves,
			uint16 pattern, uint32 fgColor, uint32 bgColor, int logOp,
			int cliprect[], int minmax[]);

		// fillPoly helpers
		bool AllocIndices(int n);
		bool AllocCrossings(int n);
		bool AllocPoints(int n);

		int16* alloc_index;
		int index_count;
		int16* alloc_crossing;
		int crossing_count;
		int16* alloc_point;
		int point_count;

		void restoreMouseBackground();
		void saveMouseBackground(int16 x, int16 y, int16 width, int16 height);
};

#endif /* NFVDI_SOFT_H */
