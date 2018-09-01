/*
 * ARAnyM Native Features suite.
 *
 * This file was taken from FreeMiNT.
 * 
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 2003-12-14
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _m68k_nf_ops_h
# define _m68k_nf_ops_h

#include <compiler.h>	/* for __CDECL */


struct nf_ops
{
	long __CDECL (*get_id)(const char *);
	long __CDECL (*call)(long id, ...);
	long res[3];
};


/**
 * Use this function to intialize Native Features.
 *
 * @return the pointer to 'struct nf_ops' or NULL when
 *         not available.
 **/
const struct nf_ops *nf_init(void);


/* basic set native feature functions */
const char *nf_name(void);
int  nf_debug(const char *msg);
void nf_shutdown(void);


# endif /* _m68k_nf_ops_h */
