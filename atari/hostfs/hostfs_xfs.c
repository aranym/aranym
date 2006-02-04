/*
 * $Header$
 *
 * 2001-2005 STanda
 *
 * This is a part of the ARAnyM project sources.
 *
 * Originaly taken from the STonX CVS repository.
 *
 */

/*
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * See COPYING for details of legal notes.
 *
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Filesystem driver routines
 */

#include "global.h"

#include "hostfs_xfs.h"
#include "hostfs_nfapi.h"

#include "mint/arch/nf_ops.h"


long     _cdecl hostfs_fs_root		 (int drv, fcookie *fc);
long     _cdecl hostfs_fs_lookup	 (fcookie *dir, const char *name, fcookie *fc);
long     _cdecl hostfs_fs_creat	 (fcookie *dir, const char *name,
								  unsigned mode, int attrib, fcookie *fc);
DEVDRV * _cdecl hostfs_fs_getdev	 (fcookie *fc, long *devspecial);
long     _cdecl hostfs_fs_getxattr	 (fcookie *file, XATTR *xattr);
long     _cdecl hostfs_fs_chattr	 (fcookie *file, int attr);
long     _cdecl hostfs_fs_chown		 (fcookie *file, int uid, int gid);
long     _cdecl hostfs_fs_chmode	 (fcookie *file, unsigned mode);
long     _cdecl hostfs_fs_mkdir		 (fcookie *dir, const char *name, unsigned mode);
long     _cdecl hostfs_fs_rmdir		 (fcookie *dir, const char *name);
long     _cdecl hostfs_fs_remove	 (fcookie *dir, const char *name);
long     _cdecl hostfs_fs_getname	 (fcookie *relto, fcookie *dir,
								  char *pathname, int size);
long     _cdecl hostfs_fs_rename	 (fcookie *olddir, char *oldname,
								  fcookie *newdir, const char *newname);
long     _cdecl hostfs_fs_opendir	 (DIR *dirh, int tosflag);
long     _cdecl hostfs_fs_readdir	 (DIR *dirh, char *name, int namelen,
								  fcookie *fc);
long     _cdecl hostfs_fs_rewinddir (DIR *dirh);
long     _cdecl hostfs_fs_closedir	 (DIR *dirh);
long     _cdecl hostfs_fs_pathconf	 (fcookie *dir, int which);
long     _cdecl hostfs_fs_dfree	 (fcookie *dir, long *buf);
long     _cdecl hostfs_fs_writelabel(fcookie *dir, const char *name);
long     _cdecl hostfs_fs_readlabel (fcookie *dir, char *name,
								  int namelen);
long     _cdecl hostfs_fs_symlink	 (fcookie *dir, const char *name,
								  const char *to);
long     _cdecl hostfs_fs_readlink	 (fcookie *dir, char *buf, int len);
long     _cdecl hostfs_fs_hardlink	 (fcookie *fromdir,
								  const char *fromname,
								  fcookie *todir, const char *toname);
long     _cdecl hostfs_fs_fscntl	 (fcookie *dir, const char *name,
								  int cmd, long arg);
long     _cdecl hostfs_fs_dskchng	 (int drv, int mode);
long     _cdecl hostfs_fs_release	 (fcookie *);
long     _cdecl hostfs_fs_dupcookie (fcookie *new, fcookie *old);
long     _cdecl hostfs_fs_sync		 (void);
long     _cdecl hostfs_fs_mknod     (fcookie *dir, const char *name, ulong mode);
long     _cdecl hostfs_fs_unmount	 (int drv);
long     _cdecl hostfs_fs_stat64    (fcookie *file, STAT *xattr);


unsigned long nf_hostfs_id = 0;
long __CDECL (*nf_call)(long id, ...) = 0UL;


ulong     _cdecl fs_drive_bits(void)
{
	return nf_call(HOSTFS(GET_DRIVE_BITS));
}

long     _cdecl hostfs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
									  void *fs, void *fs_dev)
{
	return nf_call(HOSTFS(XFS_INIT), (long)fs_devnum, mountpoint, hostroot, (long)halfsensitive, fs, fs_dev);
}

long     _cdecl hostfs_fs_root       (int drv, fcookie *fc)
{
	return nf_call(HOSTFS(XFS_ROOT), (long)drv, fc);
}

long     _cdecl hostfs_fs_lookup     (fcookie *dir, const char *name, fcookie *fc)
{
	return nf_call(HOSTFS(XFS_LOOKUP), dir, name, fc);
}

long     _cdecl hostfs_fs_creat      (fcookie *dir, const char *name,
								   unsigned int mode, int attrib, fcookie *fc)
{
	return nf_call(HOSTFS(XFS_CREATE), dir, name, (long)mode, (long)attrib, fc);
}

DEVDRV * _cdecl hostfs_fs_getdev     (fcookie *fc, long *devspecial)
{
	return (DEVDRV*) nf_call(HOSTFS(XFS_GETDEV), fc, devspecial);
}

long     _cdecl hostfs_fs_getxattr   (fcookie *file, XATTR *xattr)
{
	return nf_call(HOSTFS(XFS_GETXATTR), file, xattr);
}

long     _cdecl hostfs_fs_chattr     (fcookie *file, int attr)
{
	return nf_call(HOSTFS(XFS_CHATTR), file, (long)attr);
}

long     _cdecl hostfs_fs_chown      (fcookie *file, int uid, int gid)
{
	return nf_call(HOSTFS(XFS_CHOWN), file, (long)uid, (long)gid);
}

long     _cdecl hostfs_fs_chmode     (fcookie *file, unsigned int mode)
{
	return nf_call(HOSTFS(XFS_CHMOD), file, (long)mode);
}

long     _cdecl hostfs_fs_mkdir      (fcookie *dir, const char *name, unsigned int mode)
{
	return nf_call(HOSTFS(XFS_MKDIR), dir, name, (long)mode);
}

long     _cdecl hostfs_fs_rmdir      (fcookie *dir, const char *name)
{
	return nf_call(HOSTFS(XFS_RMDIR), dir, name);
}

long     _cdecl hostfs_fs_remove     (fcookie *dir, const char *name)
{
	return nf_call(HOSTFS(XFS_REMOVE), dir, name);
}

long     _cdecl hostfs_fs_getname    (fcookie *relto, fcookie *dir,
								   char *pathname, int size)
{
	return nf_call(HOSTFS(XFS_GETNAME), relto, dir, pathname, (long)size);
}

long     _cdecl hostfs_fs_rename     (fcookie *olddir, char *oldname,
								   fcookie *newdir, const char *newname)
{
	return nf_call(HOSTFS(XFS_RENAME), olddir, oldname, newdir, newname);
}

long     _cdecl hostfs_fs_opendir    (DIR *dirh, int tosflag)
{
	return nf_call(HOSTFS(XFS_OPENDIR), dirh, (long)tosflag);
}

long     _cdecl hostfs_fs_readdir    (DIR *dirh, char *name, int namelen,
								   fcookie *fc)
{
	return nf_call(HOSTFS(XFS_READDIR), dirh, name, (long)namelen, fc);
}

long     _cdecl hostfs_fs_rewinddir  (DIR *dirh)
{
	return nf_call(HOSTFS(XFS_REWINDDIR), dirh);
}

long     _cdecl hostfs_fs_closedir   (DIR *dirh)
{
	return nf_call(HOSTFS(XFS_CLOSEDIR), dirh);
}

long     _cdecl hostfs_fs_pathconf   (fcookie *dir, int which)
{
	return nf_call(HOSTFS(XFS_PATHCONF), dir, (long)which);
}

long     _cdecl hostfs_fs_dfree      (fcookie *dir, long *buf)
{
	return nf_call(HOSTFS(XFS_DFREE), dir, buf);
}

long     _cdecl hostfs_fs_writelabel (fcookie *dir, const char *name)
{
	return nf_call(HOSTFS(XFS_WRITELABEL), dir, name);
}

long     _cdecl hostfs_fs_readlabel  (fcookie *dir, char *name,
								   int namelen)
{
	return nf_call(HOSTFS(XFS_READLABEL), dir, name, (long)namelen);
}

long     _cdecl hostfs_fs_symlink    (fcookie *dir, const char *name,
								   const char *to)
{
	return nf_call(HOSTFS(XFS_SYMLINK), dir, name, to);
}

long     _cdecl hostfs_fs_readlink   (fcookie *dir, char *buf, int len)
{
	return nf_call(HOSTFS(XFS_READLINK), dir, buf, (long)len);
}

long     _cdecl hostfs_fs_hardlink   (fcookie *fromdir,
								   const char *fromname,
								   fcookie *todir, const char *toname)
{
	return nf_call(HOSTFS(XFS_HARDLINK), fromdir, fromname, todir, toname);
}

long     _cdecl hostfs_fs_fscntl     (fcookie *dir, const char *name,
								   int cmd, long arg)
{
	return nf_call(HOSTFS(XFS_FSCNTL), dir, name, (long)cmd, arg);
}

long     _cdecl hostfs_fs_dskchng    (int drv, int mode)
{
	return nf_call(HOSTFS(XFS_DSKCHNG), (long)drv, (long)mode);
}

long     _cdecl hostfs_fs_release    (fcookie *fc)
{
	return nf_call(HOSTFS(XFS_RELEASE), fc);
}

long     _cdecl hostfs_fs_dupcookie  (fcookie *new, fcookie *old)
{
	return nf_call(HOSTFS(XFS_DUPCOOKIE), new, old);
}

long     _cdecl hostfs_fs_sync       (void)
{
	return nf_call(HOSTFS(XFS_SYNC));
}

long     _cdecl hostfs_fs_mknod      (fcookie *dir, const char *name, ulong mode)
{
	return nf_call(HOSTFS(XFS_MKNOD), dir, name, mode);
}

long     _cdecl hostfs_fs_unmount    (int drv)
{
	return nf_call(HOSTFS(XFS_UNMOUNT), (long)drv);
}

long     _cdecl hostfs_fs_stat64     (fcookie *file, STAT *xattr)
{
	return nf_call(HOSTFS(XFS_STAT64), file, xattr);
}


/*
 * filesystem driver map
 */
FILESYS hostfs_fs =
{
	(struct filesys *)0,    /* next */
	/*
	 * FS_KNOPARSE         kernel shouldn't do parsing
	 * FS_CASESENSITIVE    file names are case sensitive
	 * FS_NOXBIT           require only 'read' permission for execution
	 *                     (if a file can be read, it can be executed)
	 * FS_LONGPATH         file system understands "size" argument to "getname"
	 * FS_NO_C_CACHE       don't cache cookies for this filesystem
	 * FS_DO_SYNC          file system has a sync function
	 * FS_OWN_MEDIACHANGE  filesystem control self media change (dskchng)
	 * FS_REENTRANT_L1     fs is level 1 reentrant
	 * FS_REENTRANT_L2     fs is level 2 reentrant
	 * FS_EXT_1            extensions level 1 - mknod & unmount
	 * FS_EXT_2            extensions level 2 - additional place at the end
	 * FS_EXT_3            extensions level 3 - stat & native UTC timestamps
	 */
	FS_NOXBIT        |
	FS_CASESENSITIVE |
	/* FS_DO_SYNC       |  not used on the host side (would be
	 *                     called periodically -> commented out) */
	FS_LONGPATH      |
	FS_REENTRANT_L1  |
	FS_REENTRANT_L2  |
	FS_EXT_1         |
	FS_EXT_2         ,
	hostfs_fs_root, hostfs_fs_lookup, hostfs_fs_creat, hostfs_fs_getdev, hostfs_fs_getxattr,
	hostfs_fs_chattr, hostfs_fs_chown, hostfs_fs_chmode, hostfs_fs_mkdir, hostfs_fs_rmdir,
	hostfs_fs_remove, hostfs_fs_getname, hostfs_fs_rename, hostfs_fs_opendir,
	hostfs_fs_readdir, hostfs_fs_rewinddir, hostfs_fs_closedir, hostfs_fs_pathconf,
	hostfs_fs_dfree, hostfs_fs_writelabel, hostfs_fs_readlabel, hostfs_fs_symlink,
	hostfs_fs_readlink, hostfs_fs_hardlink, hostfs_fs_fscntl, hostfs_fs_dskchng,
	hostfs_fs_release, hostfs_fs_dupcookie,
	hostfs_fs_sync,
	/* FS_EXT_1 */
	hostfs_fs_mknod, hostfs_fs_unmount,
	/* FS_EXT_2 */
	/* FS_EXT_3 */
	0L,
	0L, 0L, 0L,         /* reserved 1,2,3 */
	0L, 0L,             /* lock, sleepers */
	0L, 0L              /* block(), deblock() */
};


FILESYS *hostfs_init(void)
{
	if ( !KERNEL->nf_ops ) {
		c_conws("Native Features not present on this system\r\n");
		return NULL;
	}

	/* get the HostFs NatFeat ID */
	nf_hostfs_id = KERNEL->nf_ops->get_id("HOSTFS");
	if (nf_hostfs_id == 0) {
		c_conws(MSG_PFAILURE("hostfs",
					"\r\nThe HOSTFS NatFeat not found\r\n"));
		return NULL;
	}

	nf_call = KERNEL->nf_ops->call;

	/* compare the version */
	if (nf_call(HOSTFS(GET_VERSION)) != HOSTFS_NFAPI_VERSION) {
		c_conws(MSG_PFAILURE("hostfs",
					"\r\nHOSTFS NFAPI version mismatch\n\r"));
		return NULL;
	}

	return &hostfs_fs;
}

