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


void* _cdecl InitDevice( short bosDevID, short dosDevID )
{
	char mountPoint[2] = "a:";
	mountPoint[0]+=dosDevID;

    DEBUG(("InitDevice: %c:%c", 'A'+dosDevID, bosDevID ));

    aranym_fs_init();
	aranym_fs_native_init(dosDevID, mountPoint, "/tmp/calam", 1,
						  &aranym_fs, &aranym_fs_devdrv );


    aranym_fs.root( dosDevID, &curproc->p_cwd->root[dosDevID] );

    {
        fcookie *relto = &curproc->p_cwd->root[dosDevID];
        DEBUG (("InitDevice: root (%lx, %li, %i)", relto->fs, relto->index, relto->dev));
    }

    return &ldp;
}



/**
 * $Log$
 *
 **/
