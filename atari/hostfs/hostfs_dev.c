/*
 * $Header$
 *
 * 2001-2003 STanda
 *
 * This is a part of the ARAnyM project sources.
 *
 * Originally taken from the STonX CVS repository.
 *
 */

/*
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * See COPYING for details of legal notes.
 *
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Filesystem device driver routines
 */

#include "hostfs_dev.h"
#include "hostfs_nfapi.h"

/* from ../natfeat/natfeat.c */
extern long __CDECL (*nf_call)(long id, ...);

long _cdecl hostfs_fs_dev_open     (FILEPTR *f);
long _cdecl hostfs_fs_dev_write    (FILEPTR *f, const char *buf, long bytes);
long _cdecl hostfs_fs_dev_read     (FILEPTR *f, char *buf, long bytes);
long _cdecl hostfs_fs_dev_lseek    (FILEPTR *f, long where, int whence);
long _cdecl hostfs_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf);
long _cdecl hostfs_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag);
long _cdecl hostfs_fs_dev_close    (FILEPTR *f, int pid);
long _cdecl hostfs_fs_dev_select   (FILEPTR *f, long proc, int mode);
void _cdecl hostfs_fs_dev_unselect (FILEPTR *f, long proc, int mode);



long _cdecl hostfs_fs_dev_open     (FILEPTR *f) {
	return nf_call(HOSTFS(DEV_OPEN), f);
}

long _cdecl hostfs_fs_dev_write    (FILEPTR *f, const char *buf, long bytes) {
	return nf_call(HOSTFS(DEV_WRITE), f, buf, bytes);
}

long _cdecl hostfs_fs_dev_read     (FILEPTR *f, char *buf, long bytes) {
	return nf_call(HOSTFS(DEV_READ), f, buf, bytes);
}

long _cdecl hostfs_fs_dev_lseek    (FILEPTR *f, long where, int whence) {
	return nf_call(HOSTFS(DEV_LSEEK), f, where, (long)whence);
}

long _cdecl hostfs_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf) {
	return nf_call(HOSTFS(DEV_IOCTL), f, (long)mode, buf);
}

long _cdecl hostfs_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag) {
	return nf_call(HOSTFS(DEV_DATIME), f, timeptr, (long)rwflag);
}

long _cdecl hostfs_fs_dev_close    (FILEPTR *f, int pid) {
	return nf_call(HOSTFS(DEV_CLOSE), f, (long)pid);
}

long _cdecl hostfs_fs_dev_select   (FILEPTR *f, long proc, int mode) {
	return nf_call(HOSTFS(DEV_SELECT), f, proc, (long)mode);
}

void _cdecl hostfs_fs_dev_unselect (FILEPTR *f, long proc, int mode) {
	nf_call(HOSTFS(DEV_UNSELECT), f, proc, (long)mode);
}



DEVDRV hostfs_fs_devdrv =
{
    hostfs_fs_dev_open, hostfs_fs_dev_write, hostfs_fs_dev_read, hostfs_fs_dev_lseek,
    hostfs_fs_dev_ioctl, hostfs_fs_dev_datime, hostfs_fs_dev_close, hostfs_fs_dev_select,
    hostfs_fs_dev_unselect,
    NULL, NULL /* writeb, readb not needed */
};


/*
 * $Log$
 * Revision 1.5  2006/02/04 18:17:59  standa
 * The .xfs driver now does not link any atari/natfeat stuff. It uses the
 * FreeMiNT kernel built-in kerinfo struct nf_ops* only.
 *
 * Revision 1.4  2006/01/31 04:48:29  standa
 * Converted to use the new nf_ops.h header introduced.
 *
 * Revision 1.3  2003/03/24 08:58:53  joy
 * aranymfs.xfs renamed to hostfs.xfs
 *
 * Revision 1.2  2003/03/01 11:57:37  joy
 * major HOSTFS NF API cleanup
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 * Revision 1.1  2002/05/22 07:53:22  standa
 * The PureC -> gcc conversion (see the CONFIGVARS).
 * MiNT .XFS sources added.
 *
 *
 */
