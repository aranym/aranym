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

#include <SDL.h>
#include <string.h>

#include "dirty_rects.h"

/*--- Constructor/destructor ---*/

DirtyRects::DirtyRects(int width, int height)
{
	dirtyMarker=NULL;
	resizeDirty(width,height);
}

DirtyRects::~DirtyRects(void)
{
	if (dirtyMarker) {
		delete dirtyMarker;
	}
}

/*--- Public functions ---*/

void DirtyRects::resizeDirty(int width, int height)
{
	if (dirtyMarker) {
		delete dirtyMarker;
	}

	dirtyW = width>>4;
	if (width & 15) {
		dirtyW++;
	}
	dirtyH = height>>4;
	if (height & 15) {
		dirtyH++;
	}
	dirtyMarker = new Uint8[dirtyW * dirtyH];

	/* Will refresh everything */
	memset(dirtyMarker, 1, dirtyW * dirtyH);
}

Uint8 *DirtyRects::getDirtyRects(void)
{
	return dirtyMarker;
}

void DirtyRects::setDirtyRect(int x, int y, int w, int h)
{
	int x2 = x+w;
	if (x2 & 15) {
		x2 = (x2|15)+1;
	}
	x2>>=4;
	int y2 = y+h;
	if (y2 & 15) {
		y2 = (y2|15)+1;
	}
	y2>>=4;
	int x1 = x>>4, y1 = y>>4;
    
	for (y=y1;y<y2;y++) {
		for(x=x1;x<x2;x++) {
			if ((x>=0) && (x<dirtyW) && (y>=0) && (y<dirtyH)) {
				dirtyMarker[y*dirtyW+x]=1;
			}
		}
	}
}

void DirtyRects::setDirtyLine(int x1, int y1, int x2, int y2)
{
	int min_x,min_y, max_x, max_y;
	if (x1<=x2) {
		min_x=x1;
		max_x=x2;
	}
	else {
		min_x=x2;
		max_x=x1;
	}
	if (y1<=y2) {
		min_y=y1;
		max_y=y2;
	}
	else {
		min_y=y2;
		max_y=y1;
	}
	setDirtyRect(min_x,min_y,max_x-min_x+1,max_y-min_y+1);
}


void DirtyRects::clearDirtyRects(void)
{
	memset(dirtyMarker, 0, dirtyW * dirtyH);
}

int DirtyRects::getDirtyWidth(void)
{
	return dirtyW;
}

int DirtyRects::getDirtyHeight(void)
{
	return dirtyH;
}
