;
; $Header$ 
;
; 2001 STanda
;
; The basic file layout was taken from Julian Reschke's Cookies
;
	
	.globl	FunctionTable, DriverName, ShowBanner, InitDevice

	.globl	initfun, fs_dfree, fs_fsfirst, fs_fsnext, fs_fopen
	.globl	fs_fclose, fs_fdatime, fs_fwrite, fs_fread
	.globl	fs_fattrib, fs_fcreate, fs_fseek, fs_fcntl
	.globl  fs_dcreate, fs_ddelete, fs_frename
	.globl  fs_dgetpath, fs_dsetpath
	.globl	fs_dpathconf, fs_fxattr, fs_dopendir, fs_dreaddir
	.globl	fs_dclosedir, fs_dxreaddir, fs_drewinddir
	.globl	fs_dreadlabel, fs_fdelete

	.text
	
	bsr		ShowBanner
	move.l	#FunctionTable+12,d0
	move.l	#DriverName,d1
	rts

MACRO	METADOS_DISPATCH.w	opcode
	move.l	#opcode,-(sp)
	dc.w	0x712d				;M68K_EMUL_OP_EXTFS_COMM
ENDM

initfun:
	movem.l	a0-a6/d1-d7,-(sp)
	move.w	d1,-(sp)
	move.w	d0,-(sp)
	bsr		InitDevice
	addq	#4,sp
	movem.l	(sp)+,a0-a6/d1-d7
; d1: filename conversion
; Bit 0: 0 = force upper case
; Bit 1: 0 = fully expand
; Bit 2: 0 = critical error handler reports EWRPRO
	moveq	#5,d1
	rts

fs_dfree:
	METADOS_DISPATCH 54
	rts

fs_dcreate:
	METADOS_DISPATCH 57
	rts

fs_ddelete:
	METADOS_DISPATCH 58
	rts

fs_dsetpath:
	METADOS_DISPATCH 59
	rts

fs_fcreate:
	METADOS_DISPATCH 60
	rts

fs_fopen:
	METADOS_DISPATCH 61
	rts

fs_fclose:
	METADOS_DISPATCH 62
	rts

fs_fread:
	METADOS_DISPATCH 63
	rts

fs_fwrite:
	METADOS_DISPATCH 64
	rts

fs_fdelete:
	METADOS_DISPATCH 65
	rts

fs_fseek:
	METADOS_DISPATCH 66
	rts

fs_fattrib:
	METADOS_DISPATCH 67
	rts

fs_dgetpath:
	METADOS_DISPATCH 71
	rts

fs_fsfirst:
	METADOS_DISPATCH 78
	rts

fs_fsnext:
	METADOS_DISPATCH 79
	rts

fs_frename:
	METADOS_DISPATCH 86
	rts

fs_fdatime:
	METADOS_DISPATCH 87
	rts

fs_fcntl:
	METADOS_DISPATCH 260
	rts

fs_dpathconf:
	METADOS_DISPATCH 292
	rts

fs_dopendir:
	METADOS_DISPATCH 296
	rts

fs_dreaddir:
	METADOS_DISPATCH 297
	rts

fs_drewinddir:
	METADOS_DISPATCH 298
	rts

fs_dclosedir:
	METADOS_DISPATCH 299
	rts

fs_fxattr:
	METADOS_DISPATCH 300
	rts

fs_dxreaddir:
	METADOS_DISPATCH 322
	rts

fs_dreadlabel:
	METADOS_DISPATCH 338
	rts


;
; $Log$
; Revision 1.4  2001/10/17 18:03:47  standa
; typo fix
;
; Revision 1.3  2001/10/17 17:59:44  standa
; The fandafs to aranymfs name change and code cleanup.
;
;
;