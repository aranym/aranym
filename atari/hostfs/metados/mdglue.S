//
// $Header$ 
//
// 2001 STanda
//
// The basic file layout was taken from Julian Reschke's Cookies
//
	
	.globl	_init

	.globl	_d_free, _f_sfirst, _f_snext, _f_open
	.globl	_f_close, _d_atime, _f_write, _f_read
	.globl	_f_attrib, _f_create, _f_seek, _f_cntl
	.globl  _d_create, _d_delete, _f_rename
	.globl  _d_getpath, _d_dsetpath
	.globl	_d_pathconf, _f_xattr, _d_opendir, _d_readdir
	.globl	_d_closedir, _d_xreaddir, _d_rewinddir
	.globl	_d_readlabel, _d_delete

	.globl	_DriverName, _ShowBanner, _InitDevice


.macro PRINT value
#if 0
	move.l  \value,-(sp)
	move.l  #20,-(sp)
	move.l  #20,-(sp)
        dc.w    0x7138
        lea	12(sp),sp
#endif
.endm PRINT


#if 1
#define	SREGS	movem.l	a0-a6/d1-d7,-(sp)
#define	RREGS	movem.l (sp)+,a0-a6/d1-d7
#else
#define	SREGS
#define	RREGS
#endif
	
.macro SSTCK framereg
	PRINT	sp
	move.l	sp,temp			// store the old stack to the new one (do not touch regs)
	move.l	#stack,sp

	move.l	temp,-(sp)
	SREGS

	move.l	temp,\framereg
.endm SSTCK

.macro RSTCK
	RREGS
	move.l	(sp),sp

	PRINT	sp
.endm RSTCK

//
// Call to the MetaDOS C function implementation macro
//
.macro MDBSR framereg,label
	move.w	4(\framereg),-(sp)
	move.l	(\framereg),-(sp)
	move.l	a5,-(sp)
	move.l	a4,-(sp)
	move.l	a3,-(sp)
	bsr	\label
	lea	18(sp),sp
.endm MDBSR

	
	.text
	
_init:
	bsr	_ShowBanner
	move.l	#functionTable+12,d0
	move.l	#_DriverName,d1
	rts


initfun:
	movem.l	a0-a6/d1-d7,-(sp)
	move.w	d1,-(sp)
	move.w	d0,-(sp)
	bsr	_InitDevice
	addq	#4,sp
	movem.l	(sp)+,a0-a6/d1-d7
// d1: filename conversion
// Bit 0: 0 = force upper case
// Bit 1: 0 = fully expand
// Bit 2: 0 = critical error handler reports EWRPRO
	moveq	#5,d1
	rts



functionTable:
	dc.l	0x4d414749		// 'MAGI'
	dc.l	0x434d4554		// 'CMET'
	dc.l	349
	dc.l	initfun
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1
	dc.l	-1, -1, -1, -1, -1	// 49
	dc.l	-1, -1, -1, -1 
	dc.l	wrap_dfree		// 54
	dc.l	-1
	dc.l	-1
	dc.l	wrap_dcreate		// 57
	dc.l	wrap_ddelete		// 58
	dc.l	-1 // FIXME? wrap_dsetpath		// 59
	dc.l	wrap_fcreate		// 60
	dc.l	wrap_fopen		// 61
	dc.l	wrap_fclose		// 62
	dc.l	wrap_fread		// 63
	dc.l	wrap_fwrite		// 64
	dc.l	wrap_fdelete		// 65
	dc.l	wrap_fseek		// 66
	dc.l	wrap_fattrib		// 67
	dc.l	-1
	dc.l	-1			// 69
	dc.l	-1
	dc.l	-1			// 71	// Dgetpath
	dc.l	-1, -1, -1		// 74
	dc.l	-1, -1, -1
	dc.l	wrap_fsfirst		// 78
	dc.l	wrap_fsnext		// 79
	dc.l	-1, -1, -1, -1, -1	// 84
	dc.l	-1
	dc.l	wrap_frename		// 86
	dc.l	wrap_fdatime		// 87
	dc.l	-1, -1			// 89
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 99
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 109
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 119
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 129
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 139
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 149
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 159
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 169
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 179
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 189
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 199
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 209
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 219
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 229
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 239
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 249
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 259
	dc.l	-1 // FIXME: wrap_fcntl				// 260
	dc.l	-1, -1, -1, -1		// 264
	dc.l	-1, -1, -1, -1, -1	// 269
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 279
	dc.l	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1	// 289
	dc.l	-1, -1
	dc.l	wrap_dpathconf		// 292
	dc.l	-1, -1			// 294
	dc.l	-1
	dc.l	wrap_dopendir		// 296
	dc.l	wrap_dreaddir		// 297
	dc.l	wrap_drewinddir		// 298
	dc.l	wrap_dclosedir		// 299
	dc.l	wrap_fxattr		// 300
	dc.l	-1, -1, -1, -1		// 304
	dc.l	-1, -1, -1, -1, -1	// 309
	dc.l	-1, -1, -1, -1, -1	// 314
	dc.l	-1, -1, -1, -1, -1	// 319
	dc.l	-1, -1
	dc.l	wrap_dxreaddir		// 322
	dc.l	-1, -1
	dc.l	-1, -1, -1, -1, -1	// 329
	dc.l	-1, -1, -1, -1, -1 	// 334
	dc.l	-1, -1, -1
	dc.l	wrap_dreadlabel		// 338
	dc.l	-1			// 339
	dc.l	-1, -1, -1, -1, -1 	// 344
	dc.l	-1, -1, -1, -1, -1 	// 349
	
	
wrap_dfree:
	PRINT	#54
	SSTCK	a0

	move.w	10(a0),-(sp)	// drive #
	move.l	6(a0),-(sp)	// diskinfo *
	MDBSR	a0,_d_free
	lea	6(sp),sp

	RSTCK
	rts

wrap_dcreate:
	PRINT	#57
	SSTCK	a0

	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
	MDBSR	a0,_d_create
	lea	4(sp),sp

	RSTCK
	rts

wrap_ddelete:
	PRINT	#58
	SSTCK	a0

	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
	MDBSR	a0,_d_delete
	lea	4(sp),sp

	RSTCK
	rts

wrap_dsetpath:
	PRINT	#59
	SSTCK	a0

	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
//	MDBSR	a0,_d_dsetpath
	lea	4(sp),sp

	RSTCK
	rts

wrap_fcreate:
	PRINT	#60
	SSTCK	a0

	move.w	10(a0),-(sp)	// attr
	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
	MDBSR	a0,_f_create
	lea	6(sp),sp

	RSTCK
	rts

wrap_fopen:
	PRINT	#61
	SSTCK	a0

	move.w	10(a0),-(sp)	// mode
	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
	MDBSR	a0,_f_open
	lea	6(sp),sp

	RSTCK

	cmp.l	#0,d0
	bmi	fopen_error
	move.l	#0,d0		// reset the fs to 0 (MetaDOS requires this)
fopen_error:	
	rts

wrap_fclose:
	PRINT	#62
	SSTCK	a0

	move.w	6(a0),-(sp)	// handle
	MDBSR	a0,_f_close
	lea	2(sp),sp

	RSTCK
	rts

wrap_fread:
	PRINT	#63
	SSTCK	a0

	move.l	12(a0),-(sp)	// buffer
	move.l	8(a0),-(sp)	// count
	move.w	6(a0),-(sp)	// handle
	MDBSR	a0,_f_read
	lea	10(sp),sp

	RSTCK
	rts

wrap_fwrite:
	PRINT	#64
	SSTCK	a0

	move.l	12(a0),-(sp)	// buffer
	move.l	8(a0),-(sp)	// count
	move.w	6(a0),-(sp)	// handle
	MDBSR	a0,_f_write
	lea	10(sp),sp

	RSTCK
	rts

wrap_fdelete:
	PRINT	#65
	SSTCK	a0

	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
	MDBSR	a0,_f_delete
	lea	4(sp),sp

	RSTCK
	rts

wrap_fseek:
	PRINT	#66
#if 1
	SSTCK	a0

	move.w	12(a0),-(sp)	// mode
	move.w	10(a0),-(sp)	// handle
	move.l	6(a0),-(sp)	// count
	MDBSR	a0,_f_seek
	lea	8(sp),sp

	RSTCK
#else
	move.l	a5,-(sp)
	move.l	a4,-(sp)
	move.l	a3,-(sp)
	bsr	_f_seek
	lea	12(sp),sp
#endif
	PRINT	d0
	rts

wrap_fattrib:
	PRINT	#67
	SSTCK	a0

	move.w	12(a0),-(sp)	// attrib
	move.w	10(a0),-(sp)	// flag
	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
	MDBSR	a0,_f_attrib
	lea	8(sp),sp

	RSTCK
	rts

wrap_fsfirst:
	PRINT	#78
	SSTCK	a0

	move.w	10(a0),-(sp)	// flags
	move.l	a4,-(sp)	// name/mask - 6(a0) or a4 MD
	MDBSR	a0,_f_sfirst
	lea	6(sp),sp

	RSTCK
	rts

wrap_fsnext:
	PRINT	#79
	SSTCK	a0
	MDBSR	a0,_f_snext

	RSTCK
	rts

wrap_frename:
	PRINT	#86
	SSTCK	a0

	move.l	12(a0),-(sp)	// newname
	move.l	a4,-(sp)	// oldname - 8(a0) or a4 MD (yes 8(a0)!!!)
	move.w	6(a0),-(sp)	// reserved
	MDBSR	a0,_f_rename
	lea	10(sp),sp

	RSTCK
	rts

wrap_fdatime:
	PRINT	#87
	SSTCK	a0

	move.w	12(a0),-(sp)	// flag
	move.w	10(a0),-(sp)	// handle
	move.l	6(a0),-(sp)	// timeptr	MDBSR	a0,_f_datime
	MDBSR	a0,_f_datime
	lea	8(sp),sp

	RSTCK
	rts

wrap_fcntl:
	PRINT	#260
	SSTCK	a0

	move.w	12(a0),-(sp)	// cmd
	move.l	8(a0),-(sp)	// argument
	move.w	6(a0),-(sp)	// handle
//	MDBSR	a0,_f_cntl
	lea	8(sp),sp

	RSTCK
	rts

wrap_dpathconf:
	PRINT	#292
	SSTCK	a0

	move.w	10(a0),-(sp)	// mode
	move.l	a4,-(sp)	// name - 6(a0) or a4 MD
	MDBSR	a0,_d_pathconf
	lea	6(sp),sp

	RSTCK
	rts

wrap_dopendir:
	PRINT	#296
	SSTCK	a0

	move.w	10(a0),-(sp)	// flag
	move.l	a4,-(sp)	// name 6(a0) or a4 MD
	MDBSR	a0,_d_opendir
	lea	6(sp),sp

	RSTCK
	rts

wrap_dreaddir:
	PRINT	#297
	SSTCK	a0

	move.l	12(a0),-(sp)	// buffer
	move.l	8(a0),-(sp)	// dirhandle
	move.w	6(a0),-(sp)	// len
	MDBSR	a0,_d_readdir
	lea	10(sp),sp

	RSTCK
	rts

wrap_drewinddir:
	PRINT	#298
	SSTCK	a0

	move.l	6(a0),-(sp)	// dirhandle
	MDBSR	a0,_d_rewind
	lea	4(sp),sp

	RSTCK
	rts

wrap_dclosedir:
	PRINT	#299
	SSTCK	a0

	move.l	6(a0),-(sp)	// dirhandle
	MDBSR	a0,_d_closedir
	lea	4(sp),sp

	RSTCK
	rts

wrap_fxattr:
	PRINT	#300
	SSTCK	a0

	move.l	12(a0),-(sp)	// xattr*
	move.l	a4,-(sp)	// name 8(a0) or a4 MD
	move.w	6(a0),-(sp)	// flag
	MDBSR	a0,_f_xattr
	lea	10(sp),sp

	RSTCK
	rts

wrap_dxreaddir:
	PRINT	#322
	SSTCK	a0

	move.l	20(a0),-(sp)	// xret*
	move.l	16(a0),-(sp)	// xattr*
	move.l	12(a0),-(sp)	// buffer
	move.l	8(a0),-(sp)	// dirhandle
	move.w	6(a0),-(sp)	// len
	MDBSR	a0,_d_xreaddir
	lea	18(sp),sp

	RSTCK
	rts

wrap_dreadlabel:
	PRINT	#338
	SSTCK	a0

	move.w	14(a0),-(sp)	// len
	move.l	10(a0),-(sp)	// buffer
	move.l	a4,-(sp)	// pathname 6(a0) or a4 MD
	MDBSR	a0,_d_readlabel
	lea	10(sp),sp

	RSTCK
	rts


	.bss
temp:
	ds.l	1

	ds.b	8192
stack:	
	
//
// $Log$
// Revision 1.1  2002/05/22 07:53:22  standa
// The PureC -> gcc conversion (see the CONFIGVARS).
// MiNT .XFS sources added.
//
// Revision 1.4  2001/10/17 18:03:47  standa
// typo fix
//
// Revision 1.3  2001/10/17 17:59:44  standa
// The fandafs to aranymfs name change and code cleanup.
//
//
