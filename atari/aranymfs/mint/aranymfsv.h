/*
 * $Header$
 *
 * 2001/2002 STanda
 *
 * This is a part of the ARAnyM project sources. Originaly taken from the STonX
 * CVS repository and adjusted to our needs.
 *
 */

/*
 * Use this at ARAnyM and the Atari part
 */

#ifndef _aranymfsv_h_
#define _aranymfsv_h_

/* version of XFS (major.minor) */
#define MINT_FS_V_MAJOR    0
#define MINT_FS_V_MINOR    1

/* definitions for Atari part */
#define MINT_FS_NAME    "nativefs"   /* mount point at u:\\ */
#define MINT_SER_NAME   "serial2"    /* device name in u:\\dev\\ */
#define MINT_COM_NAME   "aranym"      /* device name in u:\\dev\\ */

/* Following shouldn't be here but we don't want to share many files with
 * aranym
 */
#ifndef UL
#  define UL    unsigned long
#endif
#ifndef UW
#  define UW    unsigned short
#endif
#ifndef W
#  define W     short
#endif
#ifndef CHAR
#  define CHAR  char
#endif

#endif /* _aranymfsv_h_ */


/*
 * $Log$
 *
 */
