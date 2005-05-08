/*
	Native features

	ARAnyM (C) 2005 Patrice Mandin

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

#ifndef _NFOBJS_H
#define _NFOBJS_H

/*--- Includes ---*/

#include "nf_base.h"

/*--- Variables ---*/

extern NF_Base *nf_objects[];
extern unsigned int nf_objs_cnt;

/*--- Functions prototypes ---*/

extern void NFCreate(void);
extern void NFDestroy(void);
extern void NFReset(void);

#endif /* _NFOBJS_H */
