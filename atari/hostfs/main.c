/*
 * $Header$
 *
 * (c) 2001-2003 STanda
 *
 * This is a part of the ARAnyM project sources.
 *
 * Originally taken from the STonX CVS repository.
 *
 */

/*
 * Author:      Markus Kohm, Chris Felsch
 * Started:     1998
 * Target OS:   MiNT running on StonX (Linux)
 * Description: STonX-file-system for MiNT
 *              Most of this file system has to be integrated into STonX.
 *              The native functions in STonX uses the native
 *              Linux/Unix-filesystem.
 *              The MiNT file system part only calls these native functions.
 *
 * Note:        Please send suggestions, patches or bug reports to
 *              Chris Felsch <C.Felsch@gmx.de>
 *
 * Copying:     Copyright 1998 Markus Kohm
 *
 * Copying:     Copyright 1998 Markus Kohm
 *              Copyright 2000 Chris Felsch (C.Felsch@gmx.de)
 *
 * History: 2000-09-10 (CF)  fs_mknod(), fs_unmount() added
 *          2000-11-12 (CF)  ported to freemint 1.15.10b
 *                           stx_com added
 *                           2001-02-02 (MJK) Read the cookie.
 *                           Redesign of initialization and more files
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "global.h"
#include "aranym_xfs.h"

#define ARANYM

extern FILESYS * _cdecl init(struct kerinfo *k);

/*
 * global kerinfo structure
 */
struct kerinfo *KERNEL;


FILESYS * _cdecl init(struct kerinfo *k)
{
	FILESYS *RetVal = NULL;

	KERNEL = k;

	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);

#ifdef ALPHA
	c_conws (MSG_ALPHA);
#elif defined (BETA)
	c_conws (MSG_BETA);
#endif

	DEBUG(("Found MiNT %ld.%ld with kerinfo version %ld",
		   (long)MINT_MAJOR, (long)MINT_MINOR, (long)MINT_KVERSION));

	/* check for MiNT version */
	if ( MINT_MAJOR < 1 || (MINT_MAJOR == 1 && MINT_MINOR < 15))
	{
		c_conws (MSG_OLDMINT);
		c_conws (MSG_FAILURE("MiNT too old"));

		return NULL;
	}

	/* install filesystem */
	RetVal = aranym_fs_init();

#ifndef ARANYM
	{
		void *rval;

		/* install serial device */
		rval = serial_init();
		if ( rval && !RetVal )
			RetVal = (FILESYS *) 1;

		/* install communication device */
		rval = com_init();
		if ( rval && !RetVal )
			RetVal = (FILESYS *) 1;
	}
#endif

	return RetVal;
}


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
