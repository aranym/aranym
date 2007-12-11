/*
	Rendering surface

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

#ifndef HOSTSURFACE_H
#define HOSTSURFACE_H

#include <SDL.h>

class DirtyRects;

class HostSurface: public DirtyRects
{
	protected:
		SDL_Surface *surface;
		int clip_w, clip_h;	/* clipped dimensions */

		int draw_mode;
		int alpha_coef;

		virtual SDL_Surface *createSdlSurface(int width, int height,
			SDL_PixelFormat *pixelFormat);

	public:
		enum {
			SURF_DRAW,	/* drawing mode */
			SURF_ALPHA	/* alpha coefficient */
		};

		enum {
			DRAW_CROP_AND_CENTER,
			DRAW_RESCALE
		};

		/* Create a surface from dimensions, bpp */
		HostSurface(int width, int height, int bpp);

		virtual ~HostSurface();

		SDL_Surface *getSdlSurface(void);

		int getWidth(void);
		int getHeight(void);
		int getBpp(void);

		virtual void update(void);
		virtual void resize(int new_width, int new_height);
		void resize(int new_width, int new_height,
			int surf_width, int surf_height);

		virtual void setPalette(SDL_Color *palette, int first, int count);

		int getParam(int num_param);
		void setParam(int num_param, int value);
};

#endif /* HOSTSURFACE_H */
