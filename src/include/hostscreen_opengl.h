/*
	Hostscreen, OpenGL renderer

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

#ifndef HOSTSCREENOPENGL
#define HOSTSCREENOPENGL 1

#include <SDL.h>

class HostScreen;
class HostSurface;

class HostScreenOpenGL: public HostScreen
{
	private:
		void setVideoMode(int width, int height, int bpp);

		void refreshScreen(void);
		void initScreen(void);
		void clearScreen(void);
		void drawSurfaceToScreen(HostSurface *hsurf, int *dst_x, int *dst_y, int flags);

	public:
		HostScreenOpenGL(void);
		~HostScreenOpenGL();

		int getBpp(void);
		void makeSnapshot(void);

		/* Surface creation */
		HostSurface *createSurface(int width, int height, int bpp);
};

#endif /* HOSTSCREENOPENGL */
