|||
||| $Header$
|||
||| 2001/2002 STanda
|||
||| This is a part of the ARAnyM project sources. Originaly taken from the STonX
||| CVS repository and adjusted to our needs.
|||

||| ================================================================
||| Part of ARAnyM-fs (see main.c)
||| This module calls the natvie functions
||| Copyright (c) Markus Kohm, 1998
||| Version 0.1
|||
||| You have to assemble this at TOS or with a cross-assembler!
||| ================================================================

.DATA
	.extern	_aranym_filesys, _aranym_fs_devdrv
	 
.TEXT

|||	dc.w	0x712d				;M68K_EMUL_OP_EXTFS_HFS


	.equ	OPC_NATIVE,	0x712e		| Opcode of native call
	.equ	SBC_ARA_FS,	0x00010000	| Subcodes for ARAnyM-fs
	.equ	SBC_ARA_FS_DEV,	0x00010100	| Subcodes for ARAnyM-fs dev
	.equ	SBC_ARA_SER,	0x00010200	| Subcodes for ARAnyM-fs Serial
	.equ	SBC_ARA_COM,	0x00010300	| Subcodes for ARAnyM-fs communication

	.globl _ara_fs_native_init, _ara_fs_root, _ara_fs_lookup, _ara_fs_creat
	.globl _ara_fs_getdev, _ara_fs_getxattr, _ara_fs_chattr, _ara_fs_chown
	.globl _ara_fs_chmode, _ara_fs_mkdir, _ara_fs_rmdir, _ara_fs_remove,
	.globl _ara_fs_getname, _ara_fs_rename, _ara_fs_opendir, _ara_fs_readdir
	.globl _ara_fs_rewinddir, _ara_fs_closedir, _ara_fs_pathconf
	.globl _ara_fs_dfree, _ara_fs_writelabel, _ara_fs_readlabel 
	.globl _ara_fs_symlink, _ara_fs_readlink, _ara_fs_hardlink
	.globl _ara_fs_fscntl, _ara_fs_dskchng, _ara_fs_release
	.globl _ara_fs_dupcookie, _ara_fs_sync, _ara_fs_mknod, _ara_fs_unmount

	.globl _ara_fs_dev_open, _ara_fs_dev_write, _ara_fs_dev_read, _ara_fs_dev_lseek
	.globl _ara_fs_dev_ioctl, _ara_fs_dev_datime, _ara_fs_dev_close
	.globl _ara_fs_dev_select, _ara_fs_dev_unselect

	.globl _ara_ser_open, _ara_ser_write, _ara_ser_read, _ara_ser_lseek
	.globl _ara_ser_ioctl, _ara_ser_datime, _ara_ser_close
	.globl _ara_ser_select, _ara_ser_unselect
	.globl _ara_ser_b_instat,  _ara_ser_b_in, _ara_ser_b_outstat
	.globl _ara_ser_b_out, _ara_ser_b_rsconf

	.globl _ara_com_open, _ara_com_write, _ara_com_read, _ara_com_close
	.globl _ara_com_ioctl

_ara_fs_native_init:		| (struct kerinfo *kernel,UW fs_devnum)
	pea	_aranym_fs_devdrv
	pea	_aranym_filesys
	moveq	#-32,d0		| if ARAnyMfs4MiNT not installed!
	move.l	#SBC_ARA_FS,-(a7)
	.dc.w	OPC_NATIVE
	lea.l	0x8(a7),a7
	rts			| if ok then d0 = 0 else d0 = -32
	
_ara_fs_root:	
	move.l	#SBC_ARA_FS+0x01,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_lookup:
	move.l	#SBC_ARA_FS+0x02,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_creat:
	move.l	#SBC_ARA_FS+0x03,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_getdev:
	move.l	#SBC_ARA_FS+0x04,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_getxattr:
	move.l	#SBC_ARA_FS+0x05,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_chattr:
	move.l	#SBC_ARA_FS+0x06,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_chown:
	move.l	#SBC_ARA_FS+0x07,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_chmode:
	move.l	#SBC_ARA_FS+0x08,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_mkdir:
	move.l	#SBC_ARA_FS+0x09,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_rmdir:
	move.l	#SBC_ARA_FS+0x0A,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_remove:
	move.l	#SBC_ARA_FS+0x0B,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_getname:
	move.l	#SBC_ARA_FS+0x0C,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_rename:
	move.l	#SBC_ARA_FS+0x0D,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_opendir:
	move.l	#SBC_ARA_FS+0x0E,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_readdir:
	move.l	#SBC_ARA_FS+0x0F,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_rewinddir:
	move.l	#SBC_ARA_FS+0x10,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_closedir:
	move.l	#SBC_ARA_FS+0x11,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_pathconf:
	move.l	#SBC_ARA_FS+0x12,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_dfree:
	move.l	#SBC_ARA_FS+0x13,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_writelabel:
	move.l	#SBC_ARA_FS+0x14,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_readlabel:
	move.l	#SBC_ARA_FS+0x15,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_symlink:
	move.l	#SBC_ARA_FS+0x16,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_readlink:
	move.l	#SBC_ARA_FS+0x17,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_hardlink:
	move.l	#SBC_ARA_FS+0x18,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_fscntl:
	move.l	#SBC_ARA_FS+0x19,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_dskchng:
	move.l	#SBC_ARA_FS+0x1A,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_release:
	move.l	#SBC_ARA_FS+0x1B,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_dupcookie:
	move.l	#SBC_ARA_FS+0x1C,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_sync:
	move.l	#SBC_ARA_FS+0x1D,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_fs_mknod:
	move.l	#SBC_ARA_FS+0x1E,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_fs_unmount:
	move.l	#SBC_ARA_FS+0x1F,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
|||
||| FS device driver
|||

_ara_fs_dev_open:
	move.l	#SBC_ARA_FS_DEV+0x01,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_dev_write:
	move.l	#SBC_ARA_FS_DEV+0x02,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_dev_read:
	move.l	#SBC_ARA_FS_DEV+0x03,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_fs_dev_lseek:
	move.l	#SBC_ARA_FS_DEV+0x04,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_fs_dev_ioctl:
	move.l	#SBC_ARA_FS_DEV+0x05,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_fs_dev_datime:
	move.l	#SBC_ARA_FS_DEV+0x06,-(a7)
	.dc.w	OPC_NATIVE
	rts
		
_ara_fs_dev_close:
	move.l	#SBC_ARA_FS_DEV+0x07,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_fs_dev_select:
	move.l	#SBC_ARA_FS_DEV+0x08,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_fs_dev_unselect:
	move.l	#SBC_ARA_FS_DEV+0x09,-(a7)
	.dc.w	OPC_NATIVE
	rts


||| ---------------------------------------------------------------------------
||| Serial device driver

_ara_ser_open:
	move.l	#SBC_ARA_SER+0x01,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_ser_write:
	move.l	#SBC_ARA_SER+0x02,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_ser_read:
	move.l	#SBC_ARA_SER+0x03,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_lseek:
	move.l	#SBC_ARA_SER+0x04,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_ser_ioctl:
	move.l	#SBC_ARA_SER+0x05,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_datime:
	move.l	#SBC_ARA_SER+0x06,-(a7)
	.dc.w	OPC_NATIVE
	rts
		
_ara_ser_close:
	move.l	#SBC_ARA_SER+0x07,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_select:
	move.l	#SBC_ARA_SER+0x08,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_unselect:
	move.l	#SBC_ARA_SER+0x09,-(a7)
	.dc.w	OPC_NATIVE
	rts

||| BIOS emulation
_ara_ser_b_instat:
	move.l	#SBC_ARA_SER+0x0A,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_b_in:
	move.l	#SBC_ARA_SER+0x0B,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_b_outstat:
	move.l	#SBC_ARA_SER+0x0C,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_b_out:
	move.l	#SBC_ARA_SER+0x0D,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_ser_b_rsconf:
	move.l	#SBC_ARA_SER+0x0E,-(a7)
	.dc.w	OPC_NATIVE
	rts


||| ---------------------------------------------------------------------------
||| Communication device driver

_ara_com_open:
	move.l	#SBC_ARA_COM+0x01,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_com_write:
	move.l	#SBC_ARA_COM+0x02,-(a7)
	.dc.w	OPC_NATIVE
	rts
	
_ara_com_read:
	move.l	#SBC_ARA_COM+0x03,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_com_ioctl:
	move.l	#SBC_ARA_COM+0x05,-(a7)
	.dc.w	OPC_NATIVE
	rts

_ara_com_close:
	move.l	#SBC_ARA_COM+0x07,-(a7)
	.dc.w	OPC_NATIVE
	rts

||| EOF


|||
||| $Log$
|||
|||
