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

#include <mintbind.h>
#include <mint/basepage.h>
#include <stdlib.h>
#include <string.h>

#include "mintfake.h"


#define DEVNAME "DEV.DOS Filesystem"
#define VERSION "0.1"

#if 0
#include "nfd.h"
#define TRACE(x) NFD(x)
#define DEBUG(x) NFD(x)
#else
#define TRACE(x)
#define DEBUG(x)
#endif


char DriverName[] = DEVNAME" "VERSION;
long ldp;


void __CDECL ShowBanner( void );
void* __CDECL InitDevice( long bosDevID, long dosDevID );
unsigned long get_cookie (unsigned long tag);

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

void __CDECL ShowBanner( void )
{
    Bconws (
            "\n\033p "DEVNAME" "VERSION" \033q "
            "\nCopyright (c) ARAnyM Development Team, "__DATE__"\n"
            );
}

struct cookie
{
	long tag;
	long value;
};

unsigned long get_cookie (unsigned long tag)
{
	struct cookie *cookie = *(struct cookie **)0x5a0;
	if (!cookie) return 0;

	while (cookie->tag) {
		if (cookie->tag == tag) return cookie->value;
		cookie++;
	}

	return 0;
}

/* FIXME: from bosmeta.c */
int bosfs_initialize();

void* __CDECL InitDevice( long bosDevID, long dosDevID )
{
	if (get_cookie(0x4D694E54L /*'MiNT'*/))
	{
		return (void*)-1;
	}

	/*
	 * We _must_ use the bosDevID to define the drive letter here
	 * because MetaDOS (in contrary to BetaDOS) does not provide
	 * the dosDevID
	 */
	DEBUG(("InitDevice: [dosDev=%ld, bosDev=%ld]", dosDevID, bosDevID ));

	if ( bosfs_initialize() < 0 )
	{
		return (void*)-1;
	}

	return &ldp;
}

