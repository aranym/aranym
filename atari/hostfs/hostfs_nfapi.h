/*
 * joy 2003
 *
 * trying to clean up the NFAPI mess
 *
 * GPL
 */

#ifndef _HOSTFS_NFAPI_H
#define _HOSTFS_NFAPI_H

/*
 * General versioning rules:
 * The driver version is literaly HOSTFS_XFS_VERSION.HOSTFS_NFAPI_VERSION
 *
 * note: therefore you need to be careful and set the NFAPI_VERSION like
 * 0x or something if needed.
 */

/*
 * general XFS driver version
 */
#define HOSTFS_XFS_VERSION       0
#define BETA

/* if you change anything in the enum {} below you have to increase
   this HOSTFS_NFAPI_VERSION!
*/
#define HOSTFS_NFAPI_VERSION    03

enum {
	GET_VERSION = 0,	/* subID = 0 */
	GET_DRIVE_BITS,        /* get mapped drive bits */
	/* hostfs_xfs */
	XFS_INIT, XFS_ROOT, XFS_LOOKUP, XFS_CREATE, XFS_GETDEV, XFS_GETXATTR,
	XFS_CHATTR, XFS_CHOWN, XFS_CHMOD, XFS_MKDIR, XFS_RMDIR, XFS_REMOVE,
	XFS_GETNAME, XFS_RENAME, XFS_OPENDIR, XFS_READDIR, XFS_REWINDDIR,
	XFS_CLOSEDIR, XFS_PATHCONF, XFS_DFREE, XFS_WRITELABEL, XFS_READLABEL,
	XFS_SYMLINK, XFS_READLINK, XFS_HARDLINK, XFS_FSCNTL, XFS_DSKCHNG,
	XFS_RELEASE, XFS_DUPCOOKIE, XFS_SYNC, XFS_MKNOD, XFS_UNMOUNT,
	/* hostfs_dev */
	DEV_OPEN, DEV_WRITE, DEV_READ, DEV_LSEEK, DEV_IOCTL, DEV_DATIME,
	DEV_CLOSE, DEV_SELECT, DEV_UNSELECT
};

extern unsigned long nfHostFsId;
#define HOSTFS(a)	(nfHostFsId + a)

#endif /* _HOSTFS_NFAPI_H */
