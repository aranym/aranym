/*
 * nf_base.h - NatFeat common base
 *
 * Copyright (c) 2002-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _NF_BASE_H
#define _NF_BASE_H

#include "natfeats.h"	/* nf_getparameter is defined there */

#ifdef USE_JIT
extern int in_handler;
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); in_handler = 0; LONGJMP(excep_env, 2); }
#else
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); LONGJMP(excep_env, 2); }
#endif

class NF_Base
{
protected:
	/**
	 * Host<->Atari mem & str functions
	 **/
	void a2fmemcpy( char *dest, memptr source, size_t count );
	void f2amemcpy( memptr dest, char *source, size_t count );
	void a2fstrcpy( char *dest, memptr source );
	void f2astrcpy( memptr dest, char *source );
	void atari2HostSafeStrncpy( char *dest, memptr source, size_t count );
	void host2AtariSafeStrncpy( memptr dest, char *source, size_t count );

public:
	NF_Base() {};
	virtual ~NF_Base() {};
	virtual void reset() {};
	virtual char *name() = 0;
	virtual bool isSuperOnly() = 0;
	virtual int32 dispatch(uint32 fncode) = 0;
	uint32 getParameter(int i) { return nf_getparameter(i); }
};

#endif /* _NF_BASE_H */

