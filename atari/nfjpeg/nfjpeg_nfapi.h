/*
	NatFeat JPEG decoder

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

#ifndef NFJPEG_NFAPI_H
#define NFJPEG_NFAPI_H

/* if you change anything in the enum {} below you have to increase 
   this ARANFJPEG_NFAPI_VERSION!
*/
#define ARANFJPEG_NFAPI_VERSION	0x00000000

enum {
	GET_VERSION = 0,	/* no parameters, return NFAPI_VERSION in d0 */
	NFJPEG_OPENDRIVER,
	NFJPEG_CLOSEDRIVER,
	NFJPEG_GETSTRUCTSIZE,
	NFJPEG_GETIMAGEINFO,
	NFJPEG_GETIMAGESIZE,
	NFJPEG_DECODEIMAGE
};

#define NFJPEG(a)	(nfJpegId + a)

#endif /* NFJPEG_NFAPI_H */
