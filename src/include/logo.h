/*
	Logo

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

#ifndef LOGO_H
#define LOGO_H 1

#ifndef LOGO_FILENAME
#define LOGO_FILENAME	"logo.bmp"
#endif

class HostSurface;

/*--- Logo class ---*/

class Logo {
	private:
		SDL_Surface *logo_surf;
		HostSurface *surface;
		int opacity;
		
		void load(const char *filename);

	public:
		Logo(const char *filename);
		virtual ~Logo();

		HostSurface *getSurface(void);
		void alphaBlend(bool init);
};

#endif /* LOGO_H */
