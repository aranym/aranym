/*
 *  version.h - Version information
 *
 *  Basilisk II (C) 1997-2000 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef VERSION_H
#define VERSION_H

#define VER_MAJOR	0
#define VER_MINOR	8
#define VER_MICRO	6

#ifndef VER_STATUS
//#define VER_STATUS	"alpha"
#define VER_STATUS	"beta"
#endif

#define str(x)		_stringify (x)
#define _stringify(x)	#x

#define NAME_STRING "ARAnyM"

#define VERSION_STRING	NAME_STRING " " str (VER_MAJOR) "." str (VER_MINOR) "." str (VER_MICRO) VER_STATUS 

#endif
