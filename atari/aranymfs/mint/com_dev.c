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
 * Copyright 1999, 2000 by Chris Felsch <C.Felsch@gmx.de>
 *
 * See COPYING for details of legal notes.
 *
 * Modified 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 *
 * Communication device driver routines
 */

#include "com_dev.h"

#ifdef COM_DEBUG
#  define DEBUG_COM(x)      KERNEL_ALERT x
#else
#  define DEBUG_COM(x)
#endif

/*
 * routines (for extern see callaranym.s)
 */
extern long _cdecl ara_com_open	    (FILEPTR *f);
extern long _cdecl ara_com_write    (FILEPTR *f, const char *buf, long bytes);
extern long _cdecl ara_com_read	    (FILEPTR *f, char *buf, long bytes);
static long _cdecl ara_com_lseek    (FILEPTR *f, long where, int whence);
extern long _cdecl ara_com_ioctl    (FILEPTR *f, int mode, void *buf);
static long _cdecl ara_com_datime   (FILEPTR *f, ushort *timeptr, int rwflag);
extern long _cdecl ara_com_close    (FILEPTR *f, int pid);
static long _cdecl ara_com_select   (FILEPTR *f, long proc, int mode);
static void _cdecl ara_com_unselect (FILEPTR *f, long proc, int mode);


/*
 * communication device driver map
 */
static DEVDRV com_devdrv = {
	ara_com_open, ara_com_write, ara_com_read, ara_com_lseek, ara_com_ioctl,
	ara_com_datime, ara_com_close, ara_com_select, ara_com_unselect,
	NULL, NULL /* writeb, readb not needed */
};


/*
 * communication device basic description
 */
static struct dev_descr com_dev_descr = {
	&com_devdrv,
	0,
	0,
	NULL,
	0,
	S_IFCHR |
	S_IRUSR |
	S_IWUSR |
	S_IRGRP |
	S_IWGRP |
	S_IROTH |
	S_IWOTH,
	NULL,
	0,
	0
};


DEVDRV * com_init(void)
{
    if ( 0 /* FIXME: aranym_cookie->flags & STNX_IS_COM */ ) {
        long r;

        r = d_cntl(DEV_INSTALL, "u:\\dev\\"MINT_COM_NAME,
                   (long)&com_dev_descr);
        if ( r >= 0) {
            return (DEVDRV *) &com_devdrv;
        } else {
            c_conws( MSG_PFAILURE( "u:\\dev\\"MINT_COM_NAME,
                                   "Dcntl(DEV_INSTALL,...) failed" ) );
            DEBUG(( "Return value was %li", r ));
        }
    } else {
        c_conws (MSG_PFAILURE("u:\\dev\\"MINT_COM_NAME,
                              "not activated at ARAnyM"));
    }

    return NULL;
}


/*
 * communication device
 * Only open(), close(), read(), write() and ioctl() will be
 * redirected to ARAnyM
 */
static long _cdecl ara_com_lseek(FILEPTR *f, long where, int whence)
{
    DEBUG_COM(("ara_com lseek"));

    UNUSED (f);
    UNUSED (whence);

    return (where == 0) ? 0 : EBADARG;
}


static long _cdecl ara_com_datime(FILEPTR *f, ushort *timeptr, int rwflag)
{
    DEBUG_COM(("ara_com datime"));

    UNUSED (f);

    if (rwflag)
        return EACCES;

    *timeptr++ = timestamp;
    *timeptr = datestamp;

    return E_OK;
}


static long _cdecl ara_com_select(FILEPTR *f, long proc, int mode)
{
    DEBUG_COM(("ara_com select: %x", mode));

    UNUSED (f);
    UNUSED (p);

    if ((mode == O_RDONLY) || (mode == O_WRONLY))
    {
        /* we're always ready to read/write */
        return 1;
    }

    /* other things we don't care about */
    return E_OK;
}

static void _cdecl ara_com_unselect(FILEPTR *f, long proc, int mode)
{
    DEBUG_COM(("ara_com unselect"));

    UNUSED (f);
    UNUSED (p);
    UNUSED (mode);
    /* nothing to do */
}


/*
 * $Log$
 *
 */
