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

	.globl	_OpenDriver
	.globl	_CloseDriver
	.globl	_GetImageInfo
	.globl	_GetImageSize
	.globl	_DecodeImage

	.globl	_JpegDecOpenDriver
	.globl	_JpegDecCloseDriver
	.globl	_JpegDecGetImageInfo
	.globl	_JpegDecGetImageSize
	.globl	_JpegDecDecodeImage

	.text

_OpenDriver:
	moveml	d1/a0-a1,sp@-
	movel	a0,sp@-
	bsr		_JpegDecOpenDriver
	addql	#4,sp
	moveml	sp@+,d1/a0-a1
	rts
	
_CloseDriver:
	moveml	d1/a0-a1,sp@-
	movel	a0,sp@-
	bsr		_JpegDecCloseDriver
	addql	#4,sp
	moveml	sp@+,d1/a0-a1
	rts

_GetImageInfo:
	moveml	d1/a0-a1,sp@-
	movel	a0,sp@-
	bsr		_JpegDecGetImageInfo
	addql	#4,sp
	moveml	sp@+,d1/a0-a1
	rts

_GetImageSize:
	moveml	d1/a0-a1,sp@-
	movel	a0,sp@-
	bsr		_JpegDecGetImageSize
	addql	#4,sp
	moveml	sp@+,d1/a0-a1
	rts

_DecodeImage:
	moveml	d1/a0-a1,sp@-
	movel	a0,sp@-
	bsr		_JpegDecDecodeImage
	addql	#4,sp
	moveml	sp@+,d1/a0-a1
	rts
