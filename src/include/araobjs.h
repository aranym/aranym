/*
 * $Header$
 *
 * The ARAnyM specialities.
 */

#ifndef ARANYM_H
#define ARANYM_H

#include "extfs.h"
#include "fvdidrv.h"
#include "audio.h"
#include "xhdi.h"

#ifdef EXTFS_SUPPORT
// External filesystem access object.
extern ExtFs      extFS;
#endif

// The HostScreen custom fVDI driver implementation
extern FVDIDriver fVDIDrv;

// The Audio Driver
extern AudioDriver AudioDrv;

// The XHDI Driver
extern XHDIDriver Xhdi;

#endif


/*
 * $Log$
 * Revision 1.4  2002/04/19 22:14:03  joy
 * sound driver support
 *
 * Revision 1.3  2001/11/21 08:20:00  milan
 * Compilation without extfs corrected
 *
 * Revision 1.2  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.1  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 *
 *
 */
