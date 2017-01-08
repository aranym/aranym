/*
 * hostfs.h - The CLIPBRD access driver - NF API definitions.
 *
 * Copyright (c) 2006 Standa Opichal, ARAnyM team
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _CLIPBRD_NFAPI_H
#define _CLIPBRD_NFAPI_H

/*
 * general CLIPBRD driver version
 */
#define CLIPBRD_DRIVER_VERSION   0
#define BETA

/* if you change anything in the enum {} below you have to increase
   this CLIPBRD_NFAPI_VERSION!
*/
#define CLIPBRD_NFAPI_VERSION    1

enum {
	GET_VERSION = 0,	/* subID = 0 */
	CLIP_OPEN,	        /* open clipboard */
	CLIP_CLOSE,	        /* close clipboard */
	CLIP_READ,	        /* read clipboard data */
	CLIP_WRITE	        /* write data to clipboard */
};

extern unsigned long nf_clipbrd_id;

#define NFCLIP(a)	(nf_clipbrd_id + a)

#endif /* _CLIPBRD_NFAPI_H */

