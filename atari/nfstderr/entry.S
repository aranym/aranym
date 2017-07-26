/*
	NatFeat NF_STDERR /dev/nfstderr, MetaDOS BOS driver

	ARAnyM (C) 2005 STanda Opichal

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

	.globl	_asm_xopen, _xopen
	.globl	_asm_xclose, _xclose
	.globl	_asm_xread, _xread
	.globl	_asm_xwrite, _xwrite
	.globl	_asm_xseek, _xseek
	.globl	_asm_xstatus, _xstatus
	.globl	_asm_xioctl, _xioctl

	.globl	_asm_xstartaudio, _xstartaudio
	.globl	_asm_xstopaudio, _xstopaudio
	.globl	_asm_xsetsongtime, _xsetsongtime
	.globl	_asm_xgettoc, _xgettoc
	.globl	_asm_xdiscinfo, _xdiscinfo

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

_asm_xopen:
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
	bsr		_xopen
	addql	#8,sp
	rts

_asm_xclose:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address

	Forward:
		0(sp)	metados_bosheader_t *device
*/	
	movl	a0,sp@-	
	bsr		_xclose
	addql	#4,sp
	rts
	
_asm_xread:
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
	bsr		_xread
	lea		sp@(16),sp
	rts

_asm_xwrite:
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
	bsr		_xwrite
	lea		sp@(16),sp
	rts

_asm_xseek:
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
	bsr		_xseek
	addql	#8,sp
	rts

_asm_xstatus:
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
	bsr		_xstatus
	addql	#8,sp
	rts

_asm_xioctl:
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
	bsr		_xioctl
	lea		sp@(16),sp
	rts

#if 0
_asm_xstartaudio:
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
	bsr		_xstartaudio
	lea		sp@(12),sp
	rts

_asm_xstopaudio:
/*
	Input:
		a0		metados_bosheader_t *device
		0(sp)	return address

	Forward:
		0(sp)	metados_bosheader_t *device
*/	
	movl	a0,sp@-	
	bsr		_xstopaudio
	addql	#4,sp
	rts

_asm_xsetsongtime:
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
	bsr		_xsetsongtime
	lea		sp@(16),sp
	rts

_asm_xgettoc:
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
	bsr		_xgettoc
	lea		sp@(12),sp
	rts

_asm_xdiscinfo:
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
	bsr		_xdiscinfo
	addql	#8,sp
	rts
#endif
