//
// $Header$ 
//
// 2001 STanda
//
// The basic file layout was taken from Julian Reschke's Cookies
//
	
	.globl	_init

	.globl	_FunctionTable, _DriverName, _ShowBanner, _InitDevice
	.globl	_initfun, _fs_dfree, _fs_fsfirst, _fs_fsnext, _fs_fopen
	.globl	_fs_fclose, _fs_fdatime, _fs_fwrite, _fs_fread
	.globl	_fs_fattrib, _fs_fcreate, _fs_fseek, _fs_fcntl
	.globl  _fs_dcreate, _fs_ddelete, _fs_frename
	.globl  _fs_dgetpath, _fs_dsetpath
	.globl	_fs_dpathconf, _fs_fxattr, _fs_dopendir, _fs_dreaddir
	.globl	_fs_dclosedir, _fs_dxreaddir, _fs_drewinddir
	.globl	_fs_dreadlabel, _fs_fdelete

	.equ	ARA_EMULOP,	0x712d		//M68K_EMUL_OP_EXTFS_COMM

	.text
	
_init:
	bsr		_ShowBanner
	move.l	#_FunctionTable+12,d0
	move.l	#_DriverName,d1
	rts


_initfun:
	movem.l	a0-a6/d1-d7,-(sp)
	move.w	d1,-(sp)
	move.w	d0,-(sp)
	bsr		_InitDevice
	addq	#4,sp
	movem.l	(sp)+,a0-a6/d1-d7
// d1: filename conversion
// Bit 0: 0 = force upper case
// Bit 1: 0 = fully expand
// Bit 2: 0 = critical error handler reports EWRPRO
	moveq	#5,d1
	rts

_fs_dfree:
	move.l  #54,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dcreate:
	move.l  #57,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_ddelete:
	move.l  #58,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dsetpath:
	move.l  #59,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fcreate:
	move.l  #60,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fopen:
	move.l  #61,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fclose:
	move.l  #62,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fread:
	move.l  #63,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fwrite:
	move.l  #64,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fdelete:
	move.l  #65,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fseek:
	move.l  #66,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fattrib:
	move.l  #67,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dgetpath:
	move.l  #71,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fsfirst:
	move.l  #78,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fsnext:
	move.l  #79,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_frename:
	move.l  #86,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fdatime:
	move.l  #87,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fcntl:
	move.l  #260,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dpathconf:
	move.l  #292,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dopendir:
	move.l  #296,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dreaddir:
	move.l  #297,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_drewinddir:
	move.l  #298,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dclosedir:
	move.l  #299,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_fxattr:
	move.l  #300,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dxreaddir:
	move.l  #322,-(sp)
	dc.w    ARA_EMULOP
	rts

_fs_dreadlabel:
	move.l  #338,-(sp)
	dc.w    ARA_EMULOP
	rts


//
// $Log$
// Revision 1.4  2001/10/17 18:03:47  standa
// typo fix
//
// Revision 1.3  2001/10/17 17:59:44  standa
// The fandafs to aranymfs name change and code cleanup.
//
//
