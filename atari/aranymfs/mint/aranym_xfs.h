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
 * Filesystem driver routines
 */

#ifndef _aranym_xfs_h_
#define _aranym_xfs_h_

#include "global.h"

extern FILESYS aranym_filesys;        /* needed by callaranym.s */

extern FILESYS *aranym_fs_init(void); /* init filesystem driver */

#endif _aranym_xfs_h


/*
 * $Log$
 *
 */
