/*
 * The ARAnyM MetaDOS driver.
 *
 * This file is a fake header created from several
 * FreeMiNT CVS headers (mainly sys/mint/proc.h)
 *
 * 2002 STan
 */

#ifndef _mintproc_h_
#define _mintproc_h_

/** from kerinfo.h **/
# define DEFAULT_MODE		(0666)
# define DEFAULT_DIRMODE	(0755)


/** from proc.h **/
# define MIN_OPEN	6	/* 0..MIN_OPEN-1 are reserved for system */

# define DOM_TOS	0	/* TOS process domain */
# define DOM_MINT	1	/* MiNT process domain */

struct filedesc;
struct cwd;

struct proc
{
	struct pcred	*p_cred;	/* owner identity */
	struct filedesc	*p_fd;		/* open files */
	struct cwd		*p_cwd;		/* path stuff */

	short   pid, ppid, pgrp;

	short	domain;			/* process domain (TOS or UNIX)	*/

	ushort  debug_level;            /* debug-level of the process   */
};


extern PROC *curproc;			/* current process		*/
extern PROC *rootproc;			/* pid 0 -- MiNT itself		*/

/** from time.h **/
extern long timezone;

extern DEVDRV fakedev;

#define copy_cred(ucr) ucr
#define free_cred(ucr)


#endif /* _mintproc_h_ */
