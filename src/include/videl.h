/*
	Falcon VIDEL emulation

	(C) 2001-2007 ARAnyM developer team

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

#ifndef VIDEL_H
#define VIDEL_H

#include <SDL.h>

class BASE_IO;
class HostSurface;

class VIDEL : public BASE_IO
{
	private:
		bool useStPalette(void);

	protected:
		HostSurface *surface;
		bool updatePalette;
		int prevVidelWidth, prevVidelHeight, prevVidelBpp;
		Uint32 *crcList;

		Uint32 getVramAddress(void);
		int getWidth(void);
		int getHeight(void);
		int getBpp(void);

		void refreshPalette(void);
		virtual void refreshScreen(void);

	public:
		VIDEL(memptr, uint32);
		virtual ~VIDEL(void);
		bool isMyHWRegister(memptr addr);
		virtual void reset(void);

		void handleWrite(uint32 addr, uint8 value);

		virtual HostSurface *getSurface(void);
};

#endif /* VIDEL_H */
