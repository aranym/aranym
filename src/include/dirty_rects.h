/*
	Dirty rectangle markers

	(C) 2006 ARAnyM developer team

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

#ifndef DIRTYRECTS_H
#define DIRTYRECTS_H 1

/*--- DirtyRects class ---*/

class DirtyRects
{
	protected:
		/* Dirty rectangle list */
		Uint8 *dirtyMarker;
		int dirtyW, dirtyH;
		int areaW, areaH;
		int minDirtX, minDirtY, maxDirtX, maxDirtY;

	public:
		DirtyRects(int width = 16, int height = 16);
		virtual ~DirtyRects();

		void resizeDirty(int width, int height);
		Uint8 *getDirtyRects(void);
		void setDirtyRect(int x, int y, int w, int h);
		void setDirtyLine(int x1, int y1, int x2, int y2);
		void clearDirtyRects(void);
		bool hasDirtyRect(void);
		int getDirtyWidth(void);
		int getDirtyHeight(void);
		int getMinDirtX(void);
		int getMinDirtY(void);
		int getMaxDirtX(void);
		int getMaxDirtY(void);
};

#endif /* DIRTYRECTS_H */
