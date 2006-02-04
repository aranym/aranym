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

#ifndef _hostfs_dev_h_
#define _hostfs_dev_h_

#include "mint/mint.h"
#include "mint/dcntl.h"
#include "mint/file.h"

extern DEVDRV hostfs_fs_devdrv;

#endif /* _hostfs_dev_h_ */


/*
 * $Log$
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
