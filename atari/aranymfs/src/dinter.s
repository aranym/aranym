; @(#)cookiefs.dos/dinter.s
; Copyright (c) Julian F. Reschke, 28. November 1995
; All rights reserved
	
	.globl	FunctionTable, DriverName, ShowBanner
	.globl	InitDevice, DiskFree, SearchFirst, SearchNext
	.globl	OpenFile, CloseFile, DateAndTime, WriteFile
	.globl	ReadFile, FileAttributes, CreateFile, SeekFile, FCntl
	.globl	DPathConf, FXAttr, DOpenDir, DReadDir, DXReadDir
	.globl	DCloseDir, DRewindDir, DReadLabel, FDelete

	.globl	initfun, fs_dfree, fs_fsfirst, fs_fsnext, fs_fopen
	.globl	fs_fclose, fs_fdatime, fs_fwrite, fs_fread
	.globl	fs_fattrib, fs_fcreate, fs_fseek, fs_fcntl
	.globl  fs_dcreate, fs_ddelete, fs_frename
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
;	bsr		DiskFree
	rts

fs_dcreate:
	METADOS_DISPATCH 57
	rts

fs_ddelete:
	METADOS_DISPATCH 58
	rts

fs_fsfirst:
	METADOS_DISPATCH 78
;	bsr		SearchFirst
	rts

fs_fsnext:
	METADOS_DISPATCH 79
;	bsr		SearchNext
	rts

fs_fopen:
	METADOS_DISPATCH 61
;	bsr		OpenFile
	rts

fs_fclose:
	METADOS_DISPATCH 62
;	bsr		CloseFile
	rts

fs_frename:
	METADOS_DISPATCH 86
	rts

fs_fdatime:
	METADOS_DISPATCH 87
;	bsr		DateAndTime
	rts

fs_fwrite:
	METADOS_DISPATCH 64
;	bsr		WriteFile
	rts

fs_fread:
	METADOS_DISPATCH 63
;	bsr		ReadFile
	rts

fs_fattrib:
	METADOS_DISPATCH 67
;	bsr		FileAttributes
	rts

fs_fcreate:
	METADOS_DISPATCH 60
;	bsr		CreateFile
	rts

fs_fseek:
	METADOS_DISPATCH 66
;	bsr		SeekFile
	rts

fs_fcntl:
	METADOS_DISPATCH 260
;	bsr		FCntl
	rts

fs_fdelete:
	METADOS_DISPATCH 65
;	bsr		FDelete
	rts

fs_dpathconf:
	METADOS_DISPATCH 292
;	bsr		DPathConf
	rts

fs_fxattr:
	METADOS_DISPATCH 300
;	bsr		FXAttr
	rts

fs_dopendir:
	METADOS_DISPATCH 296
;	bsr		DOpenDir
	rts

fs_dreaddir:
	METADOS_DISPATCH 297
;	bsr		DReadDir
	rts

fs_dxreaddir:
	METADOS_DISPATCH 322
;	bsr		DXReadDir
	rts

fs_dclosedir:
	METADOS_DISPATCH 299
;	bsr		DCloseDir
	rts

fs_drewinddir:
	METADOS_DISPATCH 298
;	bsr		DRewindDir
	rts

fs_dreadlabel:
	METADOS_DISPATCH 338
;	bsr		DReadLabel
	rts


