/**
 * NatFeats (Native Features)
 *
 * Petr Stehlik (c) 2002
 *
 * GPL
 */

#ifndef _NF_BASE_H
#define _NF_BASE_H

#include "natfeats.h"	/* nf_getparameter is defined there */

#ifdef USE_JIT
extern int in_handler;
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); in_handler = 0; longjmp(excep_env, 2); }
#else
# define BUS_ERROR(a)	{ regs.mmu_fault_addr=(a); longjmp(excep_env, 2); }
#endif

class NF_Base
{
public:
	virtual char *name() { return ""; }
	virtual bool isSuperOnly() { return true; }	// by default requires supervisor mode
	virtual uint32 dispatch(uint32 fncode) { return 0; }
	uint32 getParameterAsInt(int i) { return nf_getparameter(i); }
};

#endif /* _NF_BASE_H */
