/*
	OSMesa LDG linker, old functions prototypes

	Copyright (C) 2004	Patrice Mandin

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#ifndef LIB_OLDMESA_H
#define LIB_OLDMESA_H

/*--- Defines ---*/

/* Mxalloc parameters */
#define MX_STRAM 0
#define MX_TTRAM 1
#define MX_PREFSTRAM 2
#define MX_PREFTTRAM 3

/*--- Functions prototypes ---*/

void *Atari_MxAlloc(unsigned long size);

void *OSMesaCreateLDG( long format, long type, long width, long height );
void OSMesaDestroyLDG(void);
long max_width(void);
long max_height(void);
void glOrtho6f( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val );

#endif /* OLDMESA_H */
