/*
	NatFeat host CD-ROM access, MetaDOS BOS driver entry points

	ARAnyM (C) 2003 Patrice Mandin

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

	.text

	.globl	_asm_init_devices, _init_devices

	.globl	_asm_cd_open, _cd_open
	.globl	_asm_cd_close, _cd_close
	.globl	_asm_cd_read, _cd_read
	.globl	_asm_cd_write, _cd_write
	.globl	_asm_cd_seek, _cd_seek
	.globl	_asm_cd_status, _cd_status
	.globl	_asm_cd_ioctl, _cd_ioctl

	.globl	_asm_cd_startaudio, _cd_startaudio
	.globl	_asm_cd_stopaudio, _cd_stopaudio
	.globl	_asm_cd_setsongtime, _cd_setsongtime
	.globl	_asm_cd_gettoc, _cd_gettoc
	.globl	_asm_cd_discinfo, _cd_discinfo

/*--- Function to create a bos header ---*/

_asm_init_devices:
/*
	Input:
		d0.w	physical drive letter
		d1.w	dma channel

	Forward:
		0(sp)	physical drive letter
		4(sp)	dma channel
*/

	andl	#0xffff,d1
	movl	d1,sp@-
	andl	#0xffff,d0
	movl	d0,sp@-
	bsr		_init_devices
	addql	#8,sp
	rts

/*--- MetaDOS BOS functions frontends ---*/

_asm_cd_open:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	metaopen_t *metaopen

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	metaopen_t *metaopen
*/	
	movl	sp,a1
	movl	4(a1),sp@-
	movl	a0,sp@-	
	bsr		_cd_open
	addql	#8,sp
	rts

_asm_cd_close:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address

	Forward:
		0(sp)	metados_bosheader_t *device
*/	
	movl	a0,sp@-	
	bsr		_cd_close
	addql	#4,sp
	rts
	
_asm_cd_read:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	void *buffer;
		8(sp)	unsigned long first
		12(sp)	unsigned short length

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	void *buffer;
		8(sp)	unsigned long first
		12(sp)	unsigned long length
*/	
	movl	sp,a1
	clrl	d0
	movw	12(a1),d0
	movl	d0,sp@-
	movl	8(a1),sp@-
	movl	4(a1),sp@-
	movl	a0,sp@-	
	bsr		_cd_read
	lea		sp@(16),sp
	rts

_asm_cd_write:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	void *buffer;
		8(sp)	unsigned long first
		12(sp)	unsigned short length

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	void *buffer;
		8(sp)	unsigned long first
		12(sp)	unsigned long length
*/	
	movl	sp,a1
	clrl	d0
	movw	12(a1),d0
	movl	d0,sp@-
	movl	8(a1),sp@-
	movl	4(a1),sp@-
	movl	a0,sp@-	
	bsr		_cd_write
	lea		sp@(16),sp
	rts

_asm_cd_seek:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	unsigned long offset

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	unsigned long offset
*/	
	movl	sp,a1
	movl	4(a1),sp@-
	movl	a0,sp@-	
	bsr		_cd_seek
	addql	#8,sp
	rts

_asm_cd_status:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	metados_bosstatus_t *extended_status;

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	metados_bosstatus_t *extended_status;
*/	
	movl	sp,a1
	movl	4(a1),sp@-
	movl	a0,sp@-	
	bsr		_cd_status
	addql	#8,sp
	rts

_asm_cd_ioctl:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	unsigned long magic
		8(sp)	unsigned short opcode
		10(sp)	void *buffer;

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	unsigned long magic
		8(sp)	unsigned long opcode
		12(sp)	void *buffer;
*/	
	movl	sp,a1
	movl	10(a1),sp@-
	clrl	d0
	movw	8(a1),d0
	movl	d0,sp@-	
	movl	4(a1),sp@-
	movl	a0,sp@-	
	bsr		_cd_ioctl
	lea		sp@(16),sp
	rts

_asm_cd_startaudio:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	unknown parameter
		6(sp)	unsigned char *tracks

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	unknown parameter
		8(sp)	unsigned char *tracks
*/	
	movl	sp,a1
	movl	6(a1),sp@-
	clrl	d0
	movw	4(a1),d0
	movl	d0,sp@-
	movl	a0,sp@-	
	bsr		_cd_startaudio
	lea		sp@(12),sp
	rts

_asm_cd_stopaudio:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address

	Forward:
		0(sp)	metados_bosheader_t *device
*/	
	movl	a0,sp@-	
	bsr		_cd_stopaudio
	addql	#4,sp
	rts

_asm_cd_setsongtime:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	unknown parameter
		6(sp)	start_msf
		10(sp)	end_msf

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	unknown parameter
		8(sp)	start_msf
		12(sp)	end_msf
*/	
	movl	sp,a1
	movl	10(a1),sp@-
	movl	6(a1),sp@-
	clrl	d0
	movw	4(a1),d0
	movl	d0,sp@-
	movl	a0,sp@-	
	bsr		_cd_setsongtime
	lea		sp@(16),sp
	rts

_asm_cd_gettoc:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	dummy
		6(sp)	toc_header_t *toc_header

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	dummy
		8(sp)	toc_header_t *toc_header
*/	
	movl	sp,a1
	movl	6(a1),sp@-
	clrl	d0
	movw	4(a1),d0
	movl	d0,sp@-
	movl	a0,sp@-	
	bsr		_cd_gettoc
	lea		sp@(12),sp
	rts

_asm_cd_discinfo:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address
		4(sp)	metados_bosdiscinfo_t *discinfo

	Forward:
		0(sp)	metados_bosheader_t *device
		4(sp)	metados_bosdiscinfo_t *discinfo
*/	
	movl	sp,a1
	movl	4(a1),sp@-
	movl	a0,sp@-	
	bsr		_cd_discinfo
	addql	#8,sp
	rts
