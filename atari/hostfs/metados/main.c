/*
 * $Id$
 *
 * The ARAnyM MetaDOS driver.
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

#include <mintbind.h>
#include <mint/basepage.h>
#include <stdlib.h>
#include <string.h>

#include "mint/mint.h"
#include "mint/filedesc.h"

#include "../hostfs_xfs.h"
#include "../hostfs_dev.h"

#include "mintproc.h"


#define DEVNAME "ARAnyM Host Filesystem"
#define VERSION "0.60"

char DriverName[] = DEVNAME" "VERSION;
long ldp;


void _cdecl ShowBanner( void );
void* _cdecl InitDevice( long bosDevID, long dosDevID );

/* Diverse Utility-Funktionen */

static int Bconws( char *str )
{
    int cnt = 0;

    while (*str) {
        cnt++;
        if (*str == '\n') {
            Bconout (2, '\r');
            cnt++;
        }
        Bconout (2, *str++);
    }

    return cnt;
}

	extern char init;

void _cdecl ShowBanner( void )
{
    Bconws (
            "\n\033p "DEVNAME" "VERSION" \033q "
            "\nCopyright (c) ARAnyM Development Team, "__DATE__"\n"
            );
}


/* ../hostfs_xfs and ../hostfs_dev internals used */
extern long     _cdecl aranym_fs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
											 void *fs, void *fs_dev);
extern void* ara_fs_root;
extern FILESYS aranym_fs;
extern DEVDRV  aranym_fs_devdrv;


void* _cdecl InitDevice( long bosDevID, long dosDevID )
{
	static fcookie root;
	char mountPoint[] = "A:";
	mountPoint[0] += (bosDevID = (bosDevID-'A')&0x1f); // mask out bad values of the bosDevID

	/*
	 * We _must_ use the bosDevID to define the drive letter here
	 * because MetaDOS (in contrary to BetaDOS) does not provide
	 * the dosDevID
	 */
	DEBUG(("InitDevice: %s [%ld - %lx]: [%ld] addr: %lx", mountPoint, dosDevID, dosDevID, bosDevID, &ara_fs_root ));

	aranym_fs_init();

	/* map the MetaDOS drive to some bosDrive | 0x6000 so that the mapping would
	   not colide with the MiNT one */
	aranym_fs_native_init( bosDevID | 0x6000, mountPoint, "/tmp", 1,
						   &aranym_fs, &aranym_fs_devdrv );

	aranym_fs.root( bosDevID | 0x6000, &curproc->p_cwd->root[bosDevID] );

	{
		fcookie *relto = &curproc->p_cwd->root[bosDevID];
		DEBUG (("InitDevice: root (%08lx, %08lx, %04x)", relto->fs, relto->index, relto->dev));
	}

	return &ldp;
}



/**
 * $Log$
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
