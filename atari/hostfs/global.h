/*
 * $Header$
 *
 * 2001-2003 STanda
 *
 * This is a part of the ARAnyM project sources.
 *
 * Originally taken from the STonX CVS repository.
 *
 */

/*
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * See COPYING for details of legal notes.
 *
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Global definitions and variables
 */

#ifndef _global_h_
#define _global_h_

#define __KERNEL_XFS__
#ifdef ARAnyM_MetaDOS

#include "filesys.h"

#else

#include "mint/mint.h"
#include "mint/dcntl.h"
#include "mint/file.h"
#include "mint/stat.h"
#include "libkern/libkern.h"

/* get the version number */
#include "hostfs_nfapi.h"

/*
 * debugging stuff
 */
#ifdef DEV_DEBUG
#  define DEBUG(x)      KERNEL_ALERT x
#  define TRACE(x)      KERNEL_ALERT x
#else
#  define DEBUG(x)      KERNEL_DEBUG x
#  define TRACE(x)      KERNEL_TRACE x
#endif


#ifdef ALPHA
# define VER_ALPHABETA   "\0340"
#elif defined(BETA)
# define VER_ALPHABETA   "\0341"
#else
# define VER_ALPHABETA
#endif

#define MSG_VERSION	str (HOSTFS_XFS_VERSION) "." str (HOSTFS_NFAPI_VERSION) VER_ALPHABETA
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT       \
    "\033p HostFS Filesystem driver version " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
    "½ 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.\r\n" \
    "½ 2000 by Chris Felsch <C.Felsch@gmx.de>\r\n"\
    "½ " MSG_BUILDDATE " by ARAnyM Team\r\n\r\n"

# define MSG_ALPHA      \
    "\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA       \
    "\033p WARNING: This is a test version - BETA! \033q\7\r\n"

#define MSG_OLDMINT	\
    "\033pMiNT to old, this xfs requires at least FreeMiNT 1.15!\033q\r\n"

#define MSG_OLDKERINFO     \
    "\033pMiNT very old, this xfs wants at least FreeMiNT 1.15 with kerinfo version 2\033q\r\n"

#define MSG_FAILURE(s)	\
    "\7Sorry, hostfs.xfs NOT installed: " s "!\r\n\r\n"

#define MSG_PFAILURE(p,s) \
    "\7Sorry, " p " of hostfs.xfs NOT installed: " s "!\r\n"

extern struct kerinfo *KERNEL;


#endif /* ARAnyM_MetaDOS */

#endif /* _global_h_ */


/*
 * $Log$
 * Revision 1.3  2003/03/24 08:58:53  joy
 * aranymfs.xfs renamed to hostfs.xfs
 *
 * Revision 1.2  2003/03/01 11:57:37  joy
 * major HOSTFS NF API cleanup
 *
 * Revision 1.1  2002/12/10 20:47:21  standa
 * The HostFS (the host OS filesystem access via NatFeats) implementation.
 *
 * Revision 1.1  2002/05/22 07:53:22  standa
 * The PureC -> gcc conversion (see the CONFIGVARS).
 * MiNT .XFS sources added.
 *
 *
 */
