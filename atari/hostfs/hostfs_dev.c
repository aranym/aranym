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

#include "natfeat.h"
#include "aranym_dev.h"
#include "hostfs_nfapi.h"

long _cdecl ara_fs_dev_open     (FILEPTR *f);
long _cdecl ara_fs_dev_write    (FILEPTR *f, const char *buf, long bytes);
long _cdecl ara_fs_dev_read     (FILEPTR *f, char *buf, long bytes);
long _cdecl ara_fs_dev_lseek    (FILEPTR *f, long where, int whence);
long _cdecl ara_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf);
long _cdecl ara_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag);
long _cdecl ara_fs_dev_close    (FILEPTR *f, int pid);
long _cdecl ara_fs_dev_select   (FILEPTR *f, long proc, int mode);
void _cdecl ara_fs_dev_unselect (FILEPTR *f, long proc, int mode);



long _cdecl ara_fs_dev_open     (FILEPTR *f) {
	return nfCall((HOSTFS(DEV_OPEN), f));
}

long _cdecl ara_fs_dev_write    (FILEPTR *f, const char *buf, long bytes) {
	return nfCall((HOSTFS(DEV_WRITE), f, buf, bytes));
}

long _cdecl ara_fs_dev_read     (FILEPTR *f, char *buf, long bytes) {
	return nfCall((HOSTFS(DEV_READ), f, buf, bytes));
}

long _cdecl ara_fs_dev_lseek    (FILEPTR *f, long where, int whence) {
	return nfCall((HOSTFS(DEV_LSEEK), f, where, (long)whence));
}

long _cdecl ara_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf) {
	return nfCall((HOSTFS(DEV_IOCTL), f, (long)mode, buf));
}

long _cdecl ara_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag) {
	return nfCall((HOSTFS(DEV_DATIME), f, timeptr, (long)rwflag));
}

long _cdecl ara_fs_dev_close    (FILEPTR *f, int pid) {
	return nfCall((HOSTFS(DEV_CLOSE), f, (long)pid));
}

long _cdecl ara_fs_dev_select   (FILEPTR *f, long proc, int mode) {
	return nfCall((HOSTFS(DEV_SELECT), f, proc, (long)mode));
}

void _cdecl ara_fs_dev_unselect (FILEPTR *f, long proc, int mode) {
	nfCall((HOSTFS(DEV_UNSELECT), f, proc, (long)mode));
}



DEVDRV aranym_fs_devdrv =
{
    ara_fs_dev_open, ara_fs_dev_write, ara_fs_dev_read, ara_fs_dev_lseek,
    ara_fs_dev_ioctl, ara_fs_dev_datime, ara_fs_dev_close, ara_fs_dev_select,
    ara_fs_dev_unselect,
    NULL, NULL /* writeb, readb not needed */
};


/*
 * $Log$
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 * Revision 1.1  2002/05/22 07:53:22  standa
 * The PureC -> gcc conversion (see the CONFIGVARS).
 * MiNT .XFS sources added.
 *
 *
 */
