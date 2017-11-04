/*
 * The ARAnyM MetaDOS driver.
 *
 * This file is a fake PROC *curproc and some other
 * structures needed by the FreeMiNT GEMDOS file IO
 * implementation and not real for singleTOS
 *
 * 2002 STan
 */


#include "hostfs.h"
#include "mint/filedesc.h"
#include "mint/credentials.h"


/** proc.c **/

struct ucred ucred0 = {
    100,     /* euid */
    100,     /* egid */

    { 100 }, /* groups */
    1,       /* ngroups */

    1        /* links */
};

struct pcred pcred0 =   {
    &ucred0,
    100,    /* ruid */
	100,    /* rgid */
    100,    /* suid */
	100,    /* sgid */

	1,      /* links */
	1       /* pad   */
};


struct filedesc filedesc0 = {
    NULL, /* struct file **ofiles;     */ /*  file structures for open files */
    NULL, /* char        *ofileflags;  */ /*  per-process open file flags    */
    0,    /* short       nfiles;       */ /*  number of open files allocated */
    0,    /* short       pad2;         */

    1,    /* long        links;        */ /* reference count */


    NULL, /* DIR         *searches;    */ /* open directory searches  */

    /* TOS emulation */

    NULL, /* DTABUF  *dta;             */ /* current DTA          */

# define NUM_SEARCH 10                   /* max. number of searches  */
    /* DTABUF *srchdta[NUM_SEARCH]; */   /* for Fsfirst/next     */
    {
        NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL,
    },

    /* DIR srchdir[NUM_SEARCH]; */       /* for Fsfirst/next     */
    {
        { /* fcookie fc */ { NULL, 0, 0, 0 }, /* index */ 0, /* flags */ 0, /* fsstuf */ "", /* next */ NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
        { { NULL, 0, 0, 0 }, 0, 0, "", NULL },
    },

    /* long    srchtim[NUM_SEARCH];    */ /* for Fsfirst/next     */
    {
        0L, 0L, 0L, 0L, 0L,
        0L, 0L, 0L, 0L, 0L,
    },

    /* XXX total crap
     * there are something like this "ofiles[-3]" over the src
     * before we dynamically alloc the ofiles we have
     * to fix all the places
     */
    0,    /* short       pad1;       */
    0,    /* short       bconmap;    */ /* Bconmap mapping */
    NULL, /* struct file *midiout;   */ /* MIDI output */
    NULL, /* struct file *midiin;    */ /* MIDI input */
    NULL, /* struct file *prn;       */ /* printer */
    NULL, /* struct file *aux;       */ /* auxiliary tty */
    NULL, /* struct file *control;   */ /* control tty */

    /* struct file *dfiles [NDFILE]; */
    {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL
    },
    /* uchar       dfileflags [NDFILE]; */
    {
        '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
        '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
        '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
        '\0', '\0'
    }

};

struct cwd cwd0 = {
	1,      /* links */ /* reference count */
    ~0777,  /* cmask */ /* mask for file creation */
	0,      /* pad   */

    { NULL, 0, 0, 0 },  /* fcookie     currdir; */    /* current directory */
    { NULL, 0, 0, 0 },  /* fcookie     rootdir; */   /* root directory */
	NULL,   /* root_dir */ /* XXX chroot emulation */

    /* DOS emulation */
    'A'-'B',  /* curdrv */ /* current drive */
	0,        /* pad2 */

    /* fcookie     root[NDRIVES];  */ /* root directories */
    {
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }
    },

    /* fcookie     curdir[NDRIVES];*/ /* current directory */
    {
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 },
        { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }, { NULL, 0, 0, 0 }
    }
};


PROC proc0 =    {
    &pcred0,
    &filedesc0,
    &cwd0,

    1, 1, 1,

    DOM_TOS,

    10 /* debug_level */
};

PROC *curproc = &proc0;         /* current process      */
PROC *rootproc = &proc0;        /* pid 0 -- MiNT itself     */

DEVDRV fakedev;

/** time.c **/
long timezone = 0;
