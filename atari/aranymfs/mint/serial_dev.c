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
 * Serial device driver routines and bios emulation
 */

#include "serial_dev.h"

/*
 * serial device driver routines (see callaranym.s)
 */
extern long _cdecl ara_ser_open	     (FILEPTR *f);
extern long _cdecl ara_ser_write     (FILEPTR *f, const char *buf, long bytes);
extern long _cdecl ara_ser_read	     (FILEPTR *f, char *buf, long bytes);
extern long _cdecl ara_ser_lseek     (FILEPTR *f, long where, int whence);
extern long _cdecl ara_ser_ioctl     (FILEPTR *f, int mode, void *buf);
extern long _cdecl ara_ser_datime    (FILEPTR *f, ushort *timeptr, int rwflag);
extern long _cdecl ara_ser_close     (FILEPTR *f, int pid);
extern long _cdecl ara_ser_select    (FILEPTR *f, long proc, int mode);
extern void _cdecl ara_ser_unselect  (FILEPTR *f, long proc, int mode);


/*
 * serial device bios emulation (see callaranym.s)
 */
extern long _cdecl ara_ser_b_instat  (int dev);
extern long _cdecl ara_ser_b_in	     (int dev);
extern long _cdecl ara_ser_b_outstat (int dev);
extern long _cdecl ara_ser_b_out     (int dev, int c);
extern long _cdecl ara_ser_b_rsconf  (int dev, int speed, int flowctl, int ucr,
									  int rsr, int tsr, int scr);


/*
 * device driver map
 */
static DEVDRV serial_devdrv = {
    ara_ser_open, ara_ser_write, ara_ser_read, ara_ser_lseek, ara_ser_ioctl,
    ara_ser_datime, ara_ser_close, ara_ser_select, ara_ser_unselect,
    NULL, NULL /* writeb, readb not needed */
};


/*
 * bios device driver map
 */
static BDEVMAP serial_bdevmap = {
    ara_ser_b_instat, ara_ser_b_in, ara_ser_b_outstat, ara_ser_b_out,
    ara_ser_b_rsconf
};


/*
 * serial device basic description
 */
static struct dev_descr serial_dev_descr =
    {
		&serial_devdrv,
		0,              /* dinfo -> fc.aux */
		0,              /* flags */
		NULL,           /* struct tty * */
		0,              /* drvsize */
		S_IFCHR |
		S_IRUSR |
		S_IWUSR |
		S_IRGRP |
		S_IWGRP |
		S_IROTH |
		S_IWOTH,        /* fmode */
		&serial_bdevmap,  /* bdevmap */
		BDEV_OFFSET,    /* bdev */
		0               /* reserved */
    };


DEVDRV * serial_init(void)
{
    if ( 0 /* FIXME: aranym_cookie->flags & STNX_IS_SERIAL */ ) {
		long r;
		long mode;

		if (MINT_KVERSION == 1) {
			c_conws(MSG_OLDKERINFO);

			serial_dev_descr.bdevmap = NULL; /* at this version: reserved */
			serial_dev_descr.bdev    = 0;    /* at this version: reserved */
			mode = DEV_INSTALL;
		} else {
			mode = DEV_INSTALL2;
		}

		r = d_cntl(mode, "u:\\dev\\"MINT_SER_NAME,
				   (long) &serial_dev_descr);
		if ( r >= 0 ) {
			if ( MINT_KVERSION >= 2 && add_rsvfentry ) {
				add_rsvfentry (MINT_SER_NAME,
							   RSVF_PORT | RSVF_GEMDOS | RSVF_BIOS,
							   serial_dev_descr.bdev);
			}
			return &serial_devdrv; /* successfull installed */
		} else {
			if ( MINT_KVERSION == 1 ) {
				c_conws (MSG_PFAILURE("u:\\dev\\"MINT_SER_NAME,
									  "Dcntl(DEV_INSTALL,...) failed"));
			} else {
				c_conws (MSG_PFAILURE("u:\\dev\\"MINT_SER_NAME,
									  "Dcntl(DEV_INSTALL2,...) failed"));
			}
			DEBUG(("Return value was %li", r));
		}
    } else {
		c_conws (MSG_PFAILURE("u:\\dev\\"MINT_SER_NAME,
							  "not activated at STonX"));
    }

    return NULL; /* nothing installed, so nothing to stay resident */
}


/*
 * $Log$
 *
 */
