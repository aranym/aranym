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
#include "hostfs_xfs.h"
#include "hostfs_dev.h"

#define ARANYM

extern FILESYS * _cdecl init(struct kerinfo *k);

/*
 * global kerinfo structure
 */
struct kerinfo *KERNEL;


/*
 * filesystem basic description
 */
static struct fs_descr hostfs_descr =
    {
	NULL,
	0, /* this is filled in by MiNT at FS_MOUNT */
	0, /* FIXME: what about flags? */
	{0,0,0,0}  /* reserved */
    };


static
FILESYS *hostfs_mount_drives(FILESYS *fs)
{
	long r;
	int succ;
	int keep = 0;
	int rollback_failed = 0;

	/**
	 * FIXME: TODO: If there are really different settings in the filesystem
	 * mounting to provide full case sensitive filesystem (like ext2) then
	 * we need to strip the FS_NOXBIT from the filesystem flags. For now
	 * every mount is half case sensitive and behaves just like mint's fatfs
	 * implementation in this sense.
	 *
	 * We would also need to allocate a duplicate copy if there are different
	 * styles of case sensitivity used in order to provide the kernel with
	 * appropriate filesystem information. Also some NF function would be needed
	 * to go though the configuration from the host side (at this point there
	 * is only the fs_drive_bits() which doesn't say much).
	 **/

	hostfs_descr.file_system = fs;

	/* Install the filesystem */
	r = d_cntl (FS_INSTALL, "u:\\", (long) &hostfs_descr);
	DEBUG(("hostfs: Dcntl(FS_INSTALL) descr=%x", &hostfs_descr));
	if (r != 0 && r != (long)kernel)
	{
		DEBUG(("Return value was %li", r));

		/* Nothing installed, so nothing to stay resident */
		return NULL;
	}

	{
		ulong drv_mask = fs_drive_bits();
		ushort drv_number = 0;
		char mount_point[] = "u:\\XX";

		succ |= 1;

		c_conws("\r\nMounts: ");

		while ( drv_mask ) {
			/* search the 1st log 1 bit position -> drv_number */
			while( ! (drv_mask & 1) ) { drv_number++; drv_mask>>=1; }

			/* ready */
			succ = 0;

			mount_point[3] = drv_number+'a';
			mount_point[4] = '\0';

			DEBUG(("hostfs: drive: %d", drv_number));

			c_conws( (const char*)mount_point );
			c_conws(" ");

			/* mount */
			r = d_cntl(FS_MOUNT, mount_point, (long) &hostfs_descr);
			DEBUG(("hostfs: Dcnt(FS_MOUNT) dev_no: %d", hostfs_descr.dev_no));
			if (r != hostfs_descr.dev_no )
			{
				DEBUG(("hostfs: return value was %li", r));
			} else {
				succ |= 2;
				/* init */

				r = hostfs_native_init( hostfs_descr.dev_no, mount_point, "/", 0 /*caseSensitive*/,
										   fs, &hostfs_fs_devdrv );
				DEBUG(("hostfs: native_init mount_point: %s", mount_point));
				if ( r < 0 ) {
					DEBUG(("hostfs: return value was %li", r));
				} else {
					succ = 0; /* do not unmount */
					keep = 1; /* at least one is mounted */
				}
			}

			/* Try to uninstall, if necessary */
			if ( succ & 2 ) {
				/* unmount */
				r = d_cntl(FS_UNMOUNT, mount_point,
						   (long) &hostfs_descr);
				DEBUG(("hostfs: Dcntl(FS_UNMOUNT) descr=%x", &hostfs_descr));
				if ( r < 0 ) {
					DEBUG(("hostfs: return value was %li", r));
					/* Can't uninstall, because unmount failed */
					rollback_failed |= 2;
				}
			}

			drv_number++; drv_mask>>=1;
		}

		c_conws("\r\n");
	}

	/* everything OK */
	if ( keep )
		return fs; /* We where successfull */

	/* Something went wrong here -> uninstall the filesystem */
	r = d_cntl(FS_UNINSTALL, "u:\\", (long) &hostfs_descr);
	DEBUG(("hostfs: Dcntl(FS_UNINSTALL) descr=%x", &hostfs_descr));
	if ( r < 0 ) {
		DEBUG(("hostfs: return value was %li", r));
		/* Can't say NULL,
		 * because uninstall failed */
		rollback_failed |= 1;
	}

       	/* Can't say NULL, if IF_UNINSTALL or FS_UNMOUNT failed */
	if ( rollback_failed ) return (FILESYS *) 1;

       	/* Nothing installed, so nothing to stay resident */
	return NULL;
}


FILESYS * _cdecl init(struct kerinfo *k)
{
	FILESYS *fs = NULL;

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
	if ( MINT_MAJOR < 1 || (MINT_MAJOR == 1 && MINT_MINOR < 16))
	{
		c_conws (MSG_OLDMINT);
		c_conws (MSG_FAILURE("This filesystem requires MiNT 1.16.x"));

		return NULL;
	}

	/* install filesystem */
	fs = hostfs_init();
	if ( fs ) {
		/* mount the drives according to
		 * the [HOSTFS] hostfsnfig section.
		 */
		fs = hostfs_mount_drives( fs );
	}

	return fs;
}


/*
 * $Log$
 * Revision 1.4  2006/01/31 15:57:39  standa
 * More of the initialization reworked to work with the new nf_ops. This time
 * I have tested it.
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
