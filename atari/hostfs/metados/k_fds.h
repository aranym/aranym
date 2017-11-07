/*
 * The ARAnyM MetaDOS driver.
 *
 * 2002 STan
 *
 * Based on:
 * k_fds.h,v 1.4 2001/06/13 20:21:20 fna Exp
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-01-13
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _k_fds_h
# define _k_fds_h

# define FD_ALLOC(p, fd, min)        0
# define FD_REMOVE(p, fd)

# define FP_ALLOC(p, result)         0; *(result) = fpMD
# define FP_DONE(p, fp, fd, flags)
# define FP_FREE(fp)

# define FP_GET(p, fd, fp)           *(fp) = fpMD
# define FP_GET1(p, fd, fp)          *(fp) = fpMD
# define GETFILEPTR(p, fd, fp)       0; (void)p; *(fp) = fpMD

long do_open	(FILEPTR **f, const char *name, int rwmode, int attr, XATTR *x);
long do_close	(struct proc *p, FILEPTR *f);


# endif /* _k_fds_h  */
