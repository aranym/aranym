/*
	Adler-32 algorithm implementation to deal with blocks (rows x lines)
	based on http://en.wikipedia.org/wiki/Adler-32

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

#ifndef ADLER32_H
#define ADLER32_H 1

#include "SDL_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
	Calc Adler-32 sum in a rows*lines block

	rows = number of bytes in a line
	lines = number of lines
	pitch = length of a line in bytes (to skip a part)
*/
Uint32 calc_adler(Uint8 *base, int rows, int lines, int pitch);

#ifdef __cplusplus
}
#endif

#endif /* ADLER32_H */
