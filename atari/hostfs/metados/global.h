/*
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

extern long _cdecl (*nf_call)(long id, ...);

/* console output via Cconws */
#include "mint/osbind.h"
#define c_conws (void) Cconws

#define MSG_VERSION	str (HOSTFS_XFS_VERSION) "." str (HOSTFS_NFAPI_VERSION) VER_ALPHABETA
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT       \
    "\033p HostFS Filesystem driver version " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
    "½ " MSG_BUILDDATE " by ARAnyM Team\r\n"

# define MSG_ALPHA      \
    "\033p WARNING: This is an unstable version - ALPHA! \033q\7\r\n"

# define MSG_BETA       \
    "\033p WARNING: This is a test version - BETA! \033q\7\r\n"

#define MSG_PFAILURE(p,s) \
    "\7Sorry, hostfs.dos NOT installed: " s "!\r\n"

#endif /* _global_h_ */


/*
 * Revision 1.2  2006/02/06 20:58:17  standa
 * Sync with the FreeMiNT CVS. The make.sh now only builds the BetaDOS
 * hostfs.dos.
 *
 * Revision 1.1  2006/02/04 21:03:03  standa
 * Complete isolation of the metados fake mint VFS implemenation in the
 * metados folder. No #ifdef ARAnyM_MetaDOS in the hostfs folder files
 * themselves.
 *
 * Revision 1.5  2004/04/26 07:14:04  standa
 * Adjusted to the recent FreeMiNT CVS state to compile and also made
 * BetaDOS only. No more MetaDOS compatibility attempts.
 *
 * Dfree() fix - for Calamus to be able to save its documents.
 *
 * Some minor bugfix backports from the current FreeMiNTs CVS version.
 *
 * The mountpoint entries are now shared among several hostfs.dos instances
 * using a 'BDhf' cookie entry (atari/hostfs/metados/main.c).
 *
 * Revision 1.4  2003/10/02 18:13:41  standa
 * Large HOSTFS cleanup (see the ChangeLog for more)
 *
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
