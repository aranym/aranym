/*
 * $Header$
 *
 * ARAnyM native features interface.
 * In 2006 updated with FreeMiNT headers and code.
 *
 **/

/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
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
 * Started: 2003-12-13
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include <compiler.h>
# include "nf_ops.h"


#define ARANYM 1
# ifdef ARANYM


/** @deprecated Use the nf_init() function instead and the 'struct nf_ops'
 */

/* NatFeat opcodes */
long _NF_getid = 0x73004e75L;
long _NF_call  = 0x73014e75L;

/** end of @deprecated
 */


static unsigned long nf_get_id_instr = 0x73004e75UL;
static unsigned long nf_call_instr = 0x73014e75UL;

static struct nf_ops _nf_ops = { (void*)&nf_get_id_instr, (void*)&nf_call_instr }; 
static struct nf_ops *nf_ops = 0UL; 


/* the following routine assumes it is running in a supervisor mode
 * and not under FreeMiNT */

struct cookie
{
	long tag;
	long value;
};

static inline
unsigned long get_cookie (unsigned long tag)
{
	struct cookie *cookie = *(struct cookie **)0x5a0;
	if (!cookie) return 0;

	while (cookie->tag) {
		if (cookie->tag == tag) return cookie->value;
		cookie++;
	}

	return 0;
}

static inline int
detect_native_features(void) {
	if (!get_cookie(0x5f5f4e46UL /* '__NF' */)) {
		return 0;
	}
	return 1;
}


struct nf_ops *
nf_init(void)
{
	if (detect_native_features())
	{
		nf_ops = &_nf_ops;
		return nf_ops;
	}
	
	return 0UL;
}


const char *
nf_name(void)
{
	static char buf[64] = "Unknown emulator";
	
	if (nf_ops)
	{
		static int done = 0;
		
		if (!done)
		{
			long nfid_name = nf_ops->get_id("NF_NAME");
			
			if (nfid_name)
				nf_ops->call(nfid_name, buf, sizeof(buf));
			
			done = 1;
		}
	}
        
	return buf;
}

int
nf_debug(const char *msg)
{
	if (nf_ops)
	{
		long nfid_stderr = nf_ops->get_id("NF_STDERR");
		
		if (nfid_stderr)
		{
			nf_ops->call(nfid_stderr, msg);
			return 1;
		}
	}
	
	return 0;
}

void
nf_shutdown(void)
{
	if (nf_ops)
	{
		long shutdown_id = nf_ops->get_id("NF_SHUTDOWN");
		
		if (shutdown_id)
        		nf_ops->call(shutdown_id);
	}
}

# endif
