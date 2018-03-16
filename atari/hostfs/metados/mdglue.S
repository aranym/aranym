/*
 * 2001 STanda
 *
 * The basic file layout was taken from Julian Reschke's Cookies
 */
	
	.globl	_init

	.globl	_sys_d_free, _sys_f_sfirst, _sys_f_snext, _sys_f_open
	.globl	_sys_f_close, _sys_d_atime, _sys_f_write, _sys_f_read
	.globl	_sys_f_attrib, _sys_f_create, _sys_f_seek, _sys_f_cntl
	.globl  _sys_d_create, _sys_d_delete, _sys_f_rename
	.globl  _sys_d_getpath, _sys_d_dsetpath
	.globl	_sys_d_pathconf, _sys_f_xattr, _sys_d_opendir, _sys_d_readdir
	.globl	_sys_d_closedir, _sys_d_xreaddir, _sys_d_rewinddir
	.globl	_sys_d_readlabel, _sys_d_delete
	.globl	_sys_ffstat
	.globl	_sys_f_fchown
	.globl	_sys_f_fchmod
	.globl	_sys_f_link
	.globl	_sys_f_symlink
	.globl	_sys_f_readlink
	.globl	_sys_f_chown
	.globl	_sys_f_chown16
	.globl	_sys_f_chmod
	.globl	_sys_f_stat64

	.globl	_DriverName, _ShowBanner, _InitDevice


.macro PRINT value
/*	nf_call( nf_get_id(NF_STDERR), value); */
#if 0
	move.l  #s_stderr,-(sp)
	move.l  #0,-(sp)		/* fake return address */
	dc.w    0x7300
	lea	8(sp),sp

	move.l  \value,-(sp)
	move.l  #s_frm,-(sp)
	move.l  d0,-(sp)
	move.l  #0,-(sp)		/* fake return address */
	dc.w    0x7301
	lea	16(sp),sp
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
	move.l	sp,temp			/* store the old stack to the new one (do not touch regs) */
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

/*
 * Call to the MetaDOS C function implementation macro
 */
.macro MDBSR framereg,label
	move.w	4(\framereg),-(sp)	/* opcodeMD */
	move.l	(\framereg),-(sp)	/* retMD */
	move.l	a5,-(sp)			/* fpMD */
	move.l	a4,-(sp)			/* pathNameMD */
	move.l	a3,-(sp)			/* devMD */
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
	move.l	d1,-(sp)		/* BetaDOS's dosDevID + some wird word / MetaDOS pure crap */
	move.w	d0,-(sp)		/* bosDevID 16bit */
	move.w	#0,-(sp)		/* pad for gcc (long) */
	bsr	_InitDevice
	addq	#8,sp
	movem.l	(sp)+,a0-a6/d1-d7
/* d1: filename conversion */
/* Bit 0: 0 = force upper case */
/* Bit 1: 0 = fully expand */
/* Bit 2: 0 = critical error handler reports EWRPRO */
	moveq	#5,d1
	rts



functionTable:
	dc.l	0x4d414749		/* 'MAGI' */
	dc.l	0x434d4554		/* 'CMET' */
	dc.l	389
	dc.l	initfun
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 9 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 19 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 29 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 39 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 49 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1 
	dc.l	wrap_dfree			/* 54 */
	dc.l	-1
	dc.l	-1
	dc.l	wrap_dcreate		/* 57 */
	dc.l	wrap_ddelete		/* 58 */
	dc.l	-1 /* FIXME? wrap_dsetpath */		/* 59 */
	dc.l	wrap_fcreate		/* 60 */
	dc.l	wrap_fopen			/* 61 */
	dc.l	wrap_fclose			/* 62 */
	dc.l	wrap_fread			/* 63 */
	dc.l	wrap_fwrite			/* 64 */
	dc.l	wrap_fdelete		/* 65 */
	dc.l	wrap_fseek			/* 66 */
	dc.l	wrap_fattrib		/* 67 */
	dc.l	-1
	dc.l	-1					/* 69 */
	dc.l	-1
	dc.l	-1					/* 71 */	/* Dgetpath */
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 74 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	wrap_fsfirst		/* 78 */
	dc.l	wrap_fsnext			/* 79 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 84 */
	dc.l	-1
	dc.l	wrap_frename		/* 86 */
	dc.l	wrap_fdatime		/* 87 */
	dc.l	-1
	dc.l	-1					/* 89 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 99 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 109 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 119 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 129 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 139 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 149 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 159 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 169 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 179 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 189 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 199 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 209 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 219 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 229 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 239 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 249 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	wrap_f_fchown		/* 257 */
	dc.l	wrap_f_fchmod		/* 258 */
	dc.l	-1					/* 259 */
	dc.l	wrap_fcntl			/* 260 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 264 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 269 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 279 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 289 */
	dc.l	-1
	dc.l	-1
	dc.l	wrap_dpathconf		/* 292 */
	dc.l	-1
	dc.l	-1					/* 294 */
	dc.l	-1
	dc.l	wrap_dopendir		/* 296 */
	dc.l	wrap_dreaddir		/* 297 */
	dc.l	wrap_drewinddir		/* 298 */
	dc.l	wrap_dclosedir		/* 299 */
	dc.l	wrap_fxattr			/* 300 */
	dc.l	wrap_f_link			/* 301 */
	dc.l	wrap_f_symlink		/* 302 */
	dc.l	wrap_f_readlink		/* 303 */
	dc.l	-1					/* 304 */
	dc.l	wrap_f_chown
	dc.l	wrap_f_chmod
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 309 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 314 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 319 */
	dc.l	-1
	dc.l	-1
	dc.l	wrap_dxreaddir		/* 322 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 329 */
	dc.l	-1
	dc.l	wrap_f_stat64		/* 331 */
	dc.l	-1
	dc.l	-1
	dc.l	-1					/* 334 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	wrap_dreadlabel		/* 338 */
	dc.l	-1					/* 339 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1				 	/* 344 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	wrap_f_fstat64 		/* 349 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1			 		/* 359 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1			 		/* 369 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1			 		/* 379 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	wrap_f_chown16		/* 384 */
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1			 		/* 389 */
	
wrap_dfree:
	PRINT	#54
	SSTCK	a0

	move.w	10(a0),-(sp)	/* drive # */
	move.l	6(a0),-(sp)	/* diskinfo * */
	MDBSR	a0,_sys_d_free
	lea	6(sp),sp

	RSTCK
	rts

wrap_dcreate:
	PRINT	#57
	SSTCK	a0

	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_d_create
	lea	4(sp),sp

	RSTCK
	rts

wrap_ddelete:
	PRINT	#58
	SSTCK	a0

	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_d_delete
	lea	4(sp),sp

	RSTCK
	rts

wrap_dsetpath:
	PRINT	#59
	SSTCK	a0

	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
/*	MDBSR	a0,_sys_d_dsetpath */
	lea	4(sp),sp

	RSTCK
	rts

wrap_fcreate:
	PRINT	#60
	SSTCK	a0

	move.w	10(a0),-(sp)	/* attr */
	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_create
	lea	6(sp),sp

	RSTCK
	rts

wrap_fopen:
	PRINT	#61
	SSTCK	a0

	move.w	10(a0),-(sp)	/* mode */
	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_open
	lea	6(sp),sp

	RSTCK

	cmp.l	#0,d0
	bmi	fopen_error
	move.l	#0,d0		/* reset the fs to 0 (MetaDOS requires this) */
fopen_error:	
	rts

wrap_fclose:
	PRINT	#62
	SSTCK	a0

	move.w	6(a0),-(sp)	/* handle */
	MDBSR	a0,_sys_f_close
	lea	2(sp),sp

	RSTCK
	rts

wrap_fread:
	PRINT	#63
	SSTCK	a0

	move.l	12(a0),-(sp)	/* buffer */
	move.l	8(a0),-(sp)	/* count */
	move.w	6(a0),-(sp)	/* handle */
	MDBSR	a0,_sys_f_read
	lea	10(sp),sp

	RSTCK
	rts

wrap_fwrite:
	PRINT	#64
	SSTCK	a0

	move.l	12(a0),-(sp)	/* buffer */
	move.l	8(a0),-(sp)	/* count */
	move.w	6(a0),-(sp)	/* handle */
	MDBSR	a0,_sys_f_write
	lea	10(sp),sp

	RSTCK
	rts

wrap_fdelete:
	PRINT	#65
	SSTCK	a0

	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_delete
	lea	4(sp),sp

	RSTCK
	rts

wrap_fseek:
	PRINT	#66
#if 1
	SSTCK	a0

	move.w	12(a0),-(sp)	/* mode */
	move.w	10(a0),-(sp)	/* handle */
	move.l	6(a0),-(sp)	/* count */
	MDBSR	a0,_sys_f_seek
	lea	8(sp),sp

	RSTCK
#else
	move.l	a5,-(sp)
	move.l	a4,-(sp)
	move.l	a3,-(sp)
	bsr	_sys_f_seek
	lea	12(sp),sp
#endif
	PRINT	d0
	rts

wrap_fattrib:
	PRINT	#67
	SSTCK	a0

	move.w	12(a0),-(sp)	/* attrib */
	move.w	10(a0),-(sp)	/* flag */
	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_attrib
	lea	8(sp),sp

	RSTCK
	rts

wrap_fsfirst:
	PRINT	#78
	SSTCK	a0

	move.w	10(a0),-(sp)	/* flags */
	move.l	a4,-(sp)	/* name/mask - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_sfirst
	lea	6(sp),sp

	RSTCK
	rts

wrap_fsnext:
	PRINT	#79
	SSTCK	a0
	MDBSR	a0,_sys_f_snext

	RSTCK
	rts

wrap_frename:
	PRINT	#86
	SSTCK	a0

	move.l	12(a0),-(sp)	/* newname */
	move.l	a4,-(sp)	/* oldname - 8(a0) or a4 MD (yes 8(a0)!!!) */
	move.w	6(a0),-(sp)	/* reserved */
	MDBSR	a0,_sys_f_rename
	lea	10(sp),sp

	RSTCK
	rts

wrap_fdatime:
	PRINT	#87
	SSTCK	a0

	move.w	12(a0),-(sp)	/* flag */
	move.w	10(a0),-(sp)	/* handle */
	move.l	6(a0),-(sp)	/* timeptr	MDBSR	a0,_sys_f_datime */
	MDBSR	a0,_sys_f_datime
	lea	8(sp),sp

	RSTCK
	rts

wrap_f_fchown:
	PRINT	#257
	SSTCK	a0

	move.w	10(a0),-(sp)	/* gid */
	move.w	8(a0),-(sp)	/* uid */
	move.w	6(a0),-(sp)	/* handle */
	MDBSR	a0,_sys_f_fchown
	lea	6(sp),sp

	RSTCK
	rts

wrap_f_fchmod:
	PRINT	#258
	SSTCK	a0

	move.w	8(a0),-(sp)	/* mode */
	move.w	6(a0),-(sp)	/* handle */
	MDBSR	a0,_sys_f_fchmod
	lea	4(sp),sp

	RSTCK
	rts

wrap_fcntl:
	PRINT	#260
	SSTCK	a0

	move.w	12(a0),-(sp)	/* cmd */
	move.l	8(a0),-(sp)	/* argument */
	move.w	6(a0),-(sp)	/* handle */
	MDBSR	a0,_sys_f_cntl
	lea	8(sp),sp

	RSTCK
	rts

wrap_dpathconf:
	PRINT	#292
	SSTCK	a0

	move.w	10(a0),-(sp)	/* mode */
	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_d_pathconf
	lea	6(sp),sp

	RSTCK
	rts

wrap_dopendir:
	PRINT	#296
	SSTCK	a0

	move.w	10(a0),-(sp)	/* flag */
	move.l	a4,-(sp)	/* name 6(a0) or a4 MD */
	MDBSR	a0,_sys_d_opendir
	lea	6(sp),sp

	RSTCK
	rts

wrap_dreaddir:
	PRINT	#297
	SSTCK	a0

	move.l	12(a0),-(sp)	/* buffer */
	move.l	8(a0),-(sp)	/* dirhandle */
	move.w	6(a0),-(sp)	/* len */
	MDBSR	a0,_sys_d_readdir
	lea	10(sp),sp

	RSTCK
	rts

wrap_drewinddir:
	PRINT	#298
	SSTCK	a0

	move.l	6(a0),-(sp)	/* dirhandle */
	MDBSR	a0,_sys_d_rewind
	lea	4(sp),sp

	RSTCK
	rts

wrap_dclosedir:
	PRINT	#299
	SSTCK	a0

	move.l	6(a0),-(sp)	/* dirhandle */
	MDBSR	a0,_sys_d_closedir
	lea	4(sp),sp

	RSTCK
	rts

wrap_fxattr:
	PRINT	#300
	SSTCK	a0

	move.l	12(a0),-(sp)	/* xattr* */
	move.l	a4,-(sp)	/* name 8(a0) or a4 MD */
	move.w	6(a0),-(sp)	/* flag */
	MDBSR	a0,_sys_f_xattr
	lea	10(sp),sp

	RSTCK
	rts

wrap_f_link:
	PRINT	#301
	SSTCK	a0

	move.l	10(a0),-(sp)	/* newname */
	move.l	a4,-(sp)	/* oldname - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_link
	lea	8(sp),sp

	RSTCK
	rts

wrap_f_symlink:
	PRINT	#302
	SSTCK	a0

	move.l	10(a0),-(sp)	/* newname */
	move.l	a4,-(sp)	/* oldname - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_symlink
	lea	8(sp),sp

	RSTCK
	rts

wrap_f_readlink:
	PRINT	#303
	SSTCK	a0

	move.l	12(a0),-(sp)	/* newname */
	move.l	a4,-(sp)	/* oldname - 8(a0) or a4 MD */
	move.w	6(a0),-(sp)	/* buflen */
	MDBSR	a0,_sys_f_readlink
	lea	10(sp),sp

	RSTCK
	rts

wrap_f_chown:
	PRINT	#305
	SSTCK	a0

	move.w	12(a0),-(sp)	/* gid */
	move.w	10(a0),-(sp)	/* uid */
	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_chown
	lea	8(sp),sp

	RSTCK
	rts

wrap_f_chmod:
	PRINT	#306
	SSTCK	a0

	move.w	10(a0),-(sp)	/* mode */
	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_chmod
	lea	6(sp),sp

	RSTCK
	rts

wrap_dxreaddir:
	PRINT	#322
	SSTCK	a0

	move.l	20(a0),-(sp)	/* xret* */
	move.l	16(a0),-(sp)	/* xattr* */
	move.l	12(a0),-(sp)	/* buffer */
	move.l	8(a0),-(sp)	/* dirhandle */
	move.w	6(a0),-(sp)	/* len */
	MDBSR	a0,_sys_d_xreaddir
	lea	18(sp),sp

	RSTCK
	rts

wrap_f_stat64:
	PRINT	#331
	SSTCK	a0

	move.l	12(a0),-(sp)	/* stat* */
	move.l	a4,-(sp)	/* name 8(a0) or a4 MD */
	move.w	6(a0),-(sp)	/* flag */
	MDBSR	a0,_sys_f_stat64
	lea	10(sp),sp

	RSTCK
	rts

wrap_dreadlabel:
	PRINT	#338
	SSTCK	a0

	move.w	14(a0),-(sp)	/* len */
	move.l	10(a0),-(sp)	/* buffer */
	move.l	a4,-(sp)	/* pathname 6(a0) or a4 MD */
	MDBSR	a0,_sys_d_readlabel
	lea	10(sp),sp

	RSTCK
	rts

wrap_f_fstat64:
	PRINT	#349
	SSTCK	a0

	move.l	8(a0),-(sp)	/* buffer */
	move.w	6(a0),-(sp)	/* handle */
	MDBSR	a0,_sys_ffstat
	lea	6(sp),sp

	RSTCK
	rts

wrap_f_chown16:
	PRINT	#384
	SSTCK	a0

	move.w	14(a0),-(sp)	/* follow_symlinks */
	move.w	12(a0),-(sp)	/* gid */
	move.w	10(a0),-(sp)	/* uid */
	move.l	a4,-(sp)	/* name - 6(a0) or a4 MD */
	MDBSR	a0,_sys_f_chown16
	lea	10(sp),sp

	RSTCK
	rts


s_frm:
	.ascii	"DEV: %ld\n\0"
s_stderr:
	.ascii	"DEBUGPRINTF\0"

	.bss
temp:
	ds.l	1

	ds.b	8192
stack:	
	
/*
 * Revision 1.3  2004/04/26 07:14:04  standa
 * Adjusted to the recent FreeMiNT CVS state to compile and also made
 * BetaDOS only. No more MetaDOS compatibility attempts.
 *
 * Dfree() fix - for Calamus to be able to save its documents.
 *
 * Some minor bugfix backports from the current FreeMiNTs CVS version.
 *
 * The mountpoint entries are now shared among several hostfs.dos instances
 * using a 'BDhf' cookie entry (atari/hostfs/metados/main.c).
 *
 * Revision 1.2  2003/03/03 20:39:44  standa
 * Parameter passing fixed.
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 * Revision 1.1  2002/05/22 07:53:22  standa
 * The PureC -> gcc conversion (see the CONFIGVARS).
 * MiNT .XFS sources added.
 *
 * Revision 1.4  2001/10/17 18:03:47  standa
 * typo fix
 *
 * Revision 1.3  2001/10/17 17:59:44  standa
 * The fandafs to aranymfs name change and code cleanup.
 *
 */
