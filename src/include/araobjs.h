/*
 * $Header$
 *
 * The ARAnyM specialities.
 */

#ifndef ARANYM_H
#define ARANYM_H

#include "extfs.h"
#include "fvdidrv.h"

#ifdef EXTFS_SUPPORT
// External filesystem access object.
extern ExtFs      extFS;
#endif

// The HostScreen custom fVDI driver implementation
extern FVDIDriver fVDIDrv;


#endif


/*
 * $Log$
 * Revision 1.2  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.1  2001/10/08 21:46:05  standa
 * The Header and Log CVS tags added.
 *
 *
 *
 */
