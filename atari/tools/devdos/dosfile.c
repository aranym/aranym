/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * dosfile.c,v 1.20 2001/06/13 20:21:14 fna Exp
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 */

/* DOS file handling routines */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

# include "dosfile.h"
# include "bosmeta.h"

# include "metados.h"

#if 0
#include "nfd.h"
#define TRACE(x) NFD(x)
#define DEBUG(x) NFD(x)
#else
#define TRACE(x)
#define DEBUG(x)
#endif


typedef struct {
	FCOOKIE 	*fc;
	unsigned long	offset;
} FILEPTR;


static inline long
do_open( FILEPTR *f, FCOOKIE *fc, short mode)
{
	metaopen_t metaopen;
	long ret = Metaopen(fc->bos_dev, &metaopen);
	if ( ret ) return ret;

	/* store the FCOOKIE* */
	f->fc = fc;
	f->offset = 0;
	return 0;
}

static inline long
do_close( FILEPTR *f)
{
	Metaclose(f->fc->bos_dev);

	f->fc = NULL;
	return 0;
}


static inline long
tty_read( FILEPTR *f, char *buf, long count)
{
	return Metaread(f->fc->bos_dev, buf, 0, count);
}

static inline long
tty_write( FILEPTR *f, const char *buf, long count)
{
	return Metawrite(f->fc->bos_dev, buf, 0, count);
}


#define BLOCK_SIZE 2048

static inline long
block_read( FILEPTR *f, char *buf, long count)
{
	unsigned long boffset = f->offset / BLOCK_SIZE;
	unsigned long bcount = ( count + BLOCK_SIZE - 1) / BLOCK_SIZE;

	DEBUG(("block_read: '%s'\n", f->fc->name));

	if (	(f->offset % BLOCK_SIZE) == 0 &&
		(count % BLOCK_SIZE) == 0) {

		long ret = Metaread(f->fc->bos_dev, buf, boffset, bcount);
		if ( ret < 0 )
			return ret;
		f->offset += ret * BLOCK_SIZE;
		return ret;
	} else {
		char *buffer = malloc( bcount * BLOCK_SIZE);
		long ret = Metaread(f->fc->bos_dev, buffer, boffset, bcount);
		if ( ret < 0 ) {
			free(buffer);
			return ret;
		}

		count = (ret * BLOCK_SIZE < count) ? ret * BLOCK_SIZE : count;
		memcpy( buf, &buffer[f->offset - (boffset * BLOCK_SIZE)], count);
		free( buffer);
		return ret;
	}
}

static inline long
block_write( FILEPTR *f, const char *buf, long count)
{
	unsigned long boffset = f->offset / BLOCK_SIZE;
	unsigned long bcount = ( count + BLOCK_SIZE - 1) / BLOCK_SIZE;

	DEBUG(("block_write: '%s'\n", f->fc->name));

	if (	(f->offset % BLOCK_SIZE) == 0 &&
		(count % BLOCK_SIZE) == 0) {

		long ret = Metawrite(f->fc->bos_dev, buf, boffset, bcount);
		if ( ret < 0 )
			return ret;
		return count;
	} else {
		char *buffer = malloc( bcount * BLOCK_SIZE);
		long ret = Metaread(f->fc->bos_dev, buffer, boffset, bcount);
		if ( ret < 0 ) {
			free(buffer);
			return ret;
		}

		count = (ret * BLOCK_SIZE < count) ? ret * BLOCK_SIZE : count;
		memcpy( &buffer[f->offset - (boffset * BLOCK_SIZE)], buf, count);

		ret = Metawrite(f->fc->bos_dev, buffer, boffset, bcount);
		free(buffer);
		if ( ret < 0 )
			return ret;
		return count;
	}
}


static inline long
do_lseek( FILEPTR *f, long place, short how)
{
	switch (how) {
		case 0:	/* whence = SEEK_SET; */
			f->offset = place;
			break;
		case 1:	/* whence = SEEK_CUR; */
			f->offset += place;
			break;
		case 2:	/* whence = SEEK_END; */
			/* FIXME? How to find the size of a device? */
		default:
			;
	}
	
	return -ENOSYS;
}



long __CDECL
sys_f_open (MetaDOSFile const char *name, short mode)
{
	FILEPTR *fp = (FILEPTR*)fpMD;
	FCOOKIE *fc;

	long ret = path2cookie( name, NULL, &fc);
	if (ret) return ret;

#if 0
	/* make sure the mode is legal */
	mode &= O_USER;

	/* note: file mode 3 is reserved for the kernel;
	 * for users, transmogrify it into O_RDWR (mode 2)
	 */
	if ((mode & O_RWMODE) == O_EXEC)
		mode = (mode & ~O_RWMODE) | O_RDWR;
#endif

	return do_open (fp, fc, mode);
}

long __CDECL
sys_f_create (MetaDOSFile const char *name, short attrib)
{
	FILEPTR *fp = (FILEPTR*)fpMD;
	FCOOKIE *fc;

	long ret = path2cookie( name, NULL, &fc);
	if (ret) return ret;

	return do_open (fp, fc, 1);
	// return EPERM;
}

long __CDECL
sys_f_close (MetaDOSFile short fd)
{
	FILEPTR *fp = (FILEPTR*)fpMD;
	return do_close (fp);
}

long __CDECL
sys_f_read (MetaDOSFile short fd, long count, char *buf)
{
	FILEPTR *f = (FILEPTR*)fpMD;

	DEBUG(("f_read: '%s' %lx\n", f->fc->name, f->fc->bos_info_flags));
	if (is_terminal (f->fc))
		return tty_read (f, buf, count);

	DEBUG(("f_read: block '%s'\n", f->fc->name));
	return block_read (f, buf, count);
}

long __CDECL
sys_f_write (MetaDOSFile short fd, long count, const char *buf)
{
	long r;
	FILEPTR *f = (FILEPTR*)fpMD;

	if (is_terminal (f->fc))
		return tty_write (f, buf, count);

	/* Prevent broken device drivers from wiping the disk.
	 * We return a zero rather than a negative error code
	 * to help programs those don't handle GEMDOS errors
	 * returned by Fwrite()
	 */
	if (count <= 0)
		return 0;

#if 0
	/* it would be faster to do this in the device driver, but this
	 * way the drivers are easier to write
	 */
	if (f->flags & O_APPEND)
	{
		r = do_lseek (f, 0L, SEEK_END);
		/* ignore errors from unseekable files (e.g. pipes) */
		if (r == EACCES)
			r = 0;
	} else
#endif
		r = 0;

	if (r < 0)
		return r;

	return block_write (f, buf, count);
}

long __CDECL
sys_f_seek (MetaDOSFile long place, short fd, short how)
{
	FILEPTR *f = (FILEPTR*)fpMD;

	if (is_terminal (f->fc))
		return 0;

	return do_lseek (f, place, how);
}


long __CDECL
sys_f_datime (MetaDOSFile unsigned short *timeptr, short fd, short wflag)
{
	FILEPTR *f = (FILEPTR*)fpMD;

	/* some programs use Fdatime to test for TTY devices */
	if (is_terminal (f->fc))
		return EACCES;

	return EACCES; //FIXME: xdd_datime (f, timeptr, wflag);
}

