/*
 * $Header$
 *
 * STanda 2001
 */


#include "sysdeps.h"

#include "cpu_emulation.h"

#include "extfs.h"
#include "fvdidrv.h"

#ifdef EXTFS_SUPPORT
// External filesystem access object.
ExtFs      extFS;
#endif

// The HostScreen custom fVDI driver implementation
FVDIDriver fVDIDrv;


/*
 * $Log$
 * Revision 1.2  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.1  2001/10/08 21:46:05  standa
 *
 *
 */
