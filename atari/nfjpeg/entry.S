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

	.globl	_init_driver, _install_driver

	.text

/* Calculate length to keep resident */

_init_driver:
	movl	sp@(4),a0	/* Read basepage pointer */
	movl	#0x100.w,d0	/* Length of basepage */
	addl	a0@(12),d0	/* Add TEXT length */
	addl	a0@(20),d0	/* Add DATA length */
	addl	a0@(28),d0	/* Add BSS length */

	movl	d0,sp@-
	bsr		_install_driver
	addql	#4,sp

	clrw	sp@-	/* If any error, quit program */
	trap	#1
