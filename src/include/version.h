/*
 *  version.h - Version information
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

#define VER_MAJOR	1
#define VER_MINOR	1
#define VER_MICRO	0

#ifndef VER_STATUS
#define VER_STATUS
//#define VER_STATUS	"+" CVS_DATE
//#define VER_STATUS	"alpha"
//#define VER_STATUS	"beta"
//#define VER_STATUS	"beta+" CVS_DATE
#endif

#define NAME_STRING "ARAnyM"

extern char const name_string[];
extern char const version_string[];

#endif
