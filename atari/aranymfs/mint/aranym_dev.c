/*
 * $Header$
 *
 * 2001/2002 STanda
 *
 * This is a part of the ARAnyM project sources. Originaly taken from the STonX
 * CVS repository and adjusted to our needs.
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

#include "aranym_dev.h"

/*
 * assembler routines (see callnative.s)
 */
extern long _cdecl ara_fs_dev_open     (FILEPTR *f);
extern long _cdecl ara_fs_dev_write    (FILEPTR *f, const char *buf, long bytes);
extern long _cdecl ara_fs_dev_read     (FILEPTR *f, char *buf, long bytes);
extern long _cdecl ara_fs_dev_lseek    (FILEPTR *f, long where, int whence);
extern long _cdecl ara_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf);
extern long _cdecl ara_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag);
extern long _cdecl ara_fs_dev_close    (FILEPTR *f, int pid);
extern long _cdecl ara_fs_dev_select   (FILEPTR *f, long proc, int mode);
extern void _cdecl ara_fs_dev_unselect (FILEPTR *f, long proc, int mode);

DEVDRV aranym_fs_devdrv =
{
    ara_fs_dev_open, ara_fs_dev_write, ara_fs_dev_read, ara_fs_dev_lseek,
    ara_fs_dev_ioctl, ara_fs_dev_datime, ara_fs_dev_close, ara_fs_dev_select,
    ara_fs_dev_unselect,
    NULL, NULL /* writeb, readb not needed */
};


/*
 * $Log$
 *
 */
