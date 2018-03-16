/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * dosfile.h,v 1.7 2001/06/13 20:21:14 fna Exp
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _dosfile_h
# define _dosfile_h

# include "mintfake.h"

long __CDECL sys_f_open (MetaDOSFile const char *name, short mode);
long __CDECL sys_f_create (MetaDOSFile const char *name, short attrib);
long __CDECL sys_f_close (MetaDOSFile short fd);
long __CDECL sys_f_read (MetaDOSFile short fd, long count, char *buf);
long __CDECL sys_f_write (MetaDOSFile short fd, long count, const char *buf);
long __CDECL sys_f_seek (MetaDOSFile long place, short fd, short how);
long __CDECL sys_f_datime (MetaDOSFile unsigned short *timeptr, short fd, short wflag);


# endif /* _dosfile_h */
