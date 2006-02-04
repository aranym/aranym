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
 * Filesystem driver routines
 */

#ifndef _hostfs_xfs_h_
#define _hostfs_xfs_h_

# include "mint/mint.h"
# include "mint/file.h"

extern ulong    fs_drive_bits(void);
extern long     hostfs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
									  void *fs, void *fs_dev);

extern FILESYS *hostfs_init(void); /* init filesystem driver */

extern FILESYS hostfs_fs;

#endif /* _hostfs_xfs_h_ */


/*
 * $Log$
 * Revision 1.3  2006/01/31 15:57:39  standa
 * More of the initialization reworked to work with the new nf_ops. This time
 * I have tested it.
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
