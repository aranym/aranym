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

#include "../aranym_xfs.h"
#include "../aranym_dev.h"

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


/* ../aranym_xfs and ../aranym_dev internals used */
extern long     _cdecl aranym_fs_native_init(int fs_devnum, char *mountpoint, char *hostroot, int halfsensitive,
											 void *fs, void *fs_dev);
extern FILESYS aranym_fs;
extern DEVDRV  aranym_fs_devdrv;


void* _cdecl InitDevice( long bosDevID, long dosDevID )
{
	static fcookie root;
	char mountPoint[2] = "a:";
	mountPoint[0] += (bosDevID = (bosDevID-'A')&0x1f); // mask out bad values of the bosDevID

	/*
	 * We _must_ use the bosDevID to define the drive letter here
	 * because MetaDOS (in contrary to BetaDOS) does not provide
	 * the dosDevID
	 */
	DEBUG(("InitDevice: %s [%ld - %lx]: [%ld]", mountPoint, dosDevID, dosDevID, bosDevID ));

	aranym_fs_init();
	aranym_fs_native_init(bosDevID, mountPoint, "/tmp", 1,
						  &aranym_fs, &aranym_fs_devdrv );


	aranym_fs.root( bosDevID, &curproc->p_cwd->root[bosDevID] );

	{
		fcookie *relto = &curproc->p_cwd->root[bosDevID];
		DEBUG (("InitDevice: root (%lx, %li, %i)", relto->fs, relto->index, relto->dev));
	}

	return &ldp;
}



/**
 * $Log$
 * Revision 1.2  2002/12/11 08:05:54  standa
 * The /tmp/calam host fs mount point changed to /tmp one.
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 *
 **/
