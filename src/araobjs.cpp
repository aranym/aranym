/*
 * $Header$
 *
 * STanda 2001
 */


#include "sysdeps.h"
#include "araobjs.h"

#ifdef EXTFS_SUPPORT
// External filesystem access object.
ExtFs      extFS;
#endif

// The HostScreen custom fVDI driver implementation
FVDIDriver fVDIDrv;

//The Audio driver
AudioDriver AudioDrv;

// XHDI driver
// XHDIDriver Xhdi;

/*
 * $Log$
 * Revision 1.6  2002/08/01 22:21:13  joy
 * xhdi class added
 *
 * Revision 1.5  2002/04/19 21:58:04  joy
 * audio driver by Didier MEQUIGNON
 *
 * Revision 1.4  2001/11/21 13:29:51  milan
 * cleanning & portability
 *
 * Revision 1.3  2001/11/20 23:29:26  milan
 * Extfs now not needed for ARAnyM's compilation
 *
 * Revision 1.2  2001/10/25 19:56:01  standa
 * The Log and Header CVS tags in the Log removed. Was recursing.
 *
 * Revision 1.1  2001/10/08 21:46:05  standa
 *
 *
 */
