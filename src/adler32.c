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

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOD_ADLER 65521

/*
	Calc Adler-32 sum in a rows*lines block

	rows = number of bytes in a line
	lines = number of lines
	pitch = length of a line in bytes (to skip a part)
*/

Uint32 calc_adler(Uint8 *base, int rows, int lines, int pitch)
{
	Uint32 a = 1, b = 0;
	int x,y;

	/* we'll go from 16x16,1bit to 16x16,16bits */
	for (y=0;y<lines;y++) {
		Uint8 *base_line = base;
		for(x=0;x<rows;x++) {
			a += *base_line++;
			b += a;
		}
		base += pitch;
	}

	a = (a & 0xffff) + (a >> 16) * (65536-MOD_ADLER);
	b = (b & 0xffff) + (b >> 16) * (65536-MOD_ADLER);
 
	/* It can be shown that a <= 0x1013a here, so a single subtract will do. */
	if (a >= MOD_ADLER)
		a -= MOD_ADLER;

	/* It can be shown that b can reach 0xfff87 here. */
	b = (b & 0xffff) + (b >> 16) * (65536-MOD_ADLER);

	if (b >= MOD_ADLER)
		b -= MOD_ADLER;

	return (b << 16) | a;
}

#ifdef __cplusplus
}
#endif
