/*
 * The ARAnyM BetaDOS driver.
 *
 * 2001/2002 STan
 *
 * Based on:
 * @(#)cookiefs/dosmain.c
 *
 * Copyright (c) Julian F. Reschke, 28. November 1995
 * All rights reserved.
 *
 **/

#include "hostfs.h"
#include "nf_ops.h"
#include <osbind.h>
#include "hostfs/hostfs_dev.h"
#include "hostfs/hostfs_xfs.h"
#include "global.h"


#define DEVNAME "ARAnyM Host Filesystem"
#define VERSION "0.60"

char DriverName[] = DEVNAME" "VERSION;
long ldp;

long _cdecl (*nf_call)(long id, ...) = 0L;

void _cdecl ShowBanner( void );
void* _cdecl InitDevice( long bosDevID, long dosDevID );

/* Diverse Utility-Funktionen */

void _cdecl ShowBanner( void )
{
	(void) Cconws (
            "\r\n\033p "DEVNAME" "VERSION" \033q "
            "\r\nCopyright (c) ARAnyM Development Team, "__DATE__"\r\n"
            );
}

struct cookie
{
	long tag;
	long value;
};

static ulong get_cookie (ulong tag)
{
	struct cookie *cookie = *(struct cookie **)0x5a0;
	if (!cookie) return 0;

	while (cookie->tag) {
		if (cookie->tag == tag) return cookie->value;
		cookie++;
	}

	return 0;
}

static long set_cookie (ulong tag, ulong val)
{
	struct cookie *cookie = *(struct cookie **)0x5a0;
	long count, size;

	if (!cookie) return 0;
	
	count = 0;
	while (cookie->tag)
	{
		cookie++;
		count++;
	}
	count += 2;
	size = cookie->value;
	if (count >= size)
		return 0;
	cookie->tag = tag;
	cookie->value = val;
	cookie++;
	cookie->tag = 0;
	cookie->value = size;
	return 1;
}


void* _cdecl InitDevice( long bosDevID, long dosDevID )
{
	char mountPoint[] = "A:";
	mountPoint[0] += (dosDevID = (dosDevID&0x1f)); /* mask out bad values of the dosDevID */
	
	/* check the NF HOSTFS avialability */
	if ( ! hostfs_init() ) {
		return (void*)-1;
	}

	/*
	 * Hack to get the drive table the same for all hostfs.dos
	 * instances loaded by BetaDOS into memory.
     *
	 * Note: This is definitely not MP friendly, but FreeMiNT
	 *       doesn't support B(M)etaDOS anyway, so: Do not do
	 *       this when using 'MiNT' or 'MagX' it crashes then.
	 */	
	if (!get_cookie(0x4D694E54L /*'MiNT'*/) &&
	    !get_cookie(0x4D616758L /*'MagX'*/))
	{
 		/* BetaDOS Host Filesystem cookie */
		ulong p = get_cookie(0x42446866L /*'BDfh'*/);
		if ( p ) curproc = (void*)p;
		else set_cookie(0x42446866L /*'BDfh'*/, (ulong)curproc);
	}

	/*
	 * We _must_ use the bosDevID to define the drive letter here
	 * because MetaDOS (in contrary to BetaDOS) does not provide
	 * the dosDevID
	 */
	DEBUG(("InitDevice: %s [dosDev=%ld, bosDev=%ld] addr: %lx", mountPoint, dosDevID, bosDevID, &hostfs_filesys ));

	/* map the BetaDOS drive to some bosDrive | 0x6000 so that the mapping would
	   not colide with the MiNT one */
	fs_native_init( dosDevID | 0x6000, mountPoint, "/tmp", 1,
					&hostfs_filesys, &hostfs_fs_devdrv );
	timezone = hostfs_filesys.res1;
	
	hostfs_filesys.root( dosDevID | 0x6000, &curproc->p_cwd->root[dosDevID] );

#ifdef DEBUG_INFO
	{
		fcookie *relto = &curproc->p_cwd->root[dosDevID];
		DEBUG (("InitDevice: root (%08lx, %08lx, %04x)", relto->fs, relto->index, relto->dev));
	}
#endif

	return &ldp;
}



/**
 * Revision 1.12  2006/02/06 20:58:17  standa
 * Sync with the FreeMiNT CVS. The make.sh now only builds the BetaDOS
 * hostfs.dos.
 *
 * Revision 1.11  2006/02/04 21:03:03  standa
 * Complete isolation of the metados fake mint VFS implemenation in the
 * metados folder. No #ifdef ARAnyM_MetaDOS in the hostfs folder files
 * themselves.
 *
 * Revision 1.10  2006/01/31 15:57:39  standa
 * More of the initialization reworked to work with the new nf_ops. This time
 * I have tested it.
 *
 * Revision 1.9  2005/09/26 22:18:05  standa
 * Build warnings removal.
 *
 * Revision 1.8  2004/05/07 11:31:07  standa
 * The BDhf cookie is not used in MagiC or MiNT because it was causing
 * ARAnyM to crash.
 *
 * Revision 1.7  2004/04/26 07:14:04  standa
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
 * Revision 1.6  2003/03/24 08:58:53  joy
 * aranymfs.xfs renamed to hostfs.xfs
 *
 * Revision 1.5  2003/03/20 21:27:22  standa
 * The .xfs mapping to the U:\G mountpouints (single letter) implemented.
 *
 * Revision 1.4  2003/03/05 09:30:45  standa
 * mountPath declaration fixed.
 *
 * Revision 1.3  2003/03/03 20:39:44  standa
 * Parameter passing fixed.
 *
 * Revision 1.2  2002/12/11 08:05:54  standa
 * The /tmp/calam host fs mount point changed to /tmp one.
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 *
 **/
