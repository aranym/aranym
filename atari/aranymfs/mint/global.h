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
#include "mint/mint.h"
#include "mint/dcntl.h"
#include "mint/file.h"
#include "mint/stat.h"
#include "libkern/libkern.h"

#include "aranymfsv.h"

/*
 * debugging stuff
 */
#ifdef DEV_DEBUG
#  define DEBUG(x)      KERNEL_ALERT x
#  define TRACE(x)	KERNEL_ALERT x
#else
#  define DEBUG(x)      KERNEL_DEBUG x
#  define TRACE(x)      KERNEL_TRACE x
#endif

/*
 * version
 */
#define VER_MAJOR	MINT_FS_V_MAJOR
#define VER_MINOR	MINT_FS_V_MINOR
#define BETA

#ifdef ALPHA
# define VER_ALPHABETA   "\0340"
#elif defined(BETA)
# define VER_ALPHABETA   "\0341"
#else
# define VER_ALPHABETA
#endif

#define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) VER_ALPHABETA
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT       \
    "\033p ARAnyM fs/modem/communication version " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
    "½ 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.\r\n" \
    "½ 2000 by Chris Felsch <C.Felsch@gmx.de>\r\n"\
    "½ " MSG_BUILDDATE " by STanda/JAY Software\r\n\r\n"

# define MSG_ALPHA      \
    "\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA       \
    "\033p WARNING: This is a test version - BETA! \033q\7\r\n"

#define MSG_OLDMINT	\
    "\033pMiNT to old, this xfs requires at least FreeMiNT 1.14!\033q\r\n"

#define MSG_OLDKERINFO     \
    "\033pMiNT very old, this xfs wants at least FreeMiNT 1.15 with kerinfo version 2\033q\r\n"

#define MSG_FAILURE(s)	\
    "\7Sorry, aranym.xfs NOT installed: " s "!\r\n\r\n"

#define MSG_PFAILURE(p,s) \
    "\7Sorry, " p " of aranym.xfs NOT installed: " s "!\r\n"

extern struct kerinfo *KERNEL;

#endif /* _global_h_ */


/*
 * $Log$
 *
 */
