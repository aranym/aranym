/*
 * sigsegv_solaris_x86.cpp - x86/x86_64 Solaris SIGSEGV handler
 *
 * Copyright (c) 2006 Milan Jurik of ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Bernie Meyer's UAE-JIT and Gwenole Beauchesne's Basilisk II-JIT
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

#include "sysdeps.h"
#include "cpu_emulation.h"
#define DEBUG 0
#include "debug.h"

#include <csignal>
#include <ucontext.h>
#include <siginfo.h>
#include <sys/regset.h>

#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

#ifdef CPU_i386
#undef REG_RIP
#undef REG_RAX
#undef REG_RBX
#undef REG_RCX
#undef REG_RDX
#undef REG_RBP
#undef REG_RSI
#undef REG_RDI
#undef REG_RSP
#undef REG_EFL
#define REG_RIP EIP
#define REG_RAX EAX
#define REG_RBX EBX
#define REG_RCX ECX
#define REG_RDX EDX
#define REG_RBP EBP
#define REG_RSI ESI
#define REG_RDI EDI
#define REG_RSP ESP
#define REG_EFL EFL
#endif

#ifdef CPU_x86_64
#define REG_EFL REG_RFL
#endif

#if defined(CPU_i386) || defined(CPU_x86_64)
#define CONTEXT_NAME	ucp
#define CONTEXT_TYPE	ucontext_t
#define CONTEXT_ATYPE	CONTEXT_TYPE *
#define CONTEXT_REGS    CONTEXT_NAME->uc_mcontext.gregs
#define CONTEXT_AEFLAGS	CONTEXT_REGS[REG_EFL]
#define CONTEXT_AEIP	CONTEXT_REGS[REG_RIP]
#define CONTEXT_AEAX	CONTEXT_REGS[REG_RAX]
#define CONTEXT_AEBX	CONTEXT_REGS[REG_RBX]
#define CONTEXT_AECX	CONTEXT_REGS[REG_RCX]
#define CONTEXT_AEDX	CONTEXT_REGS[REG_RDX]
#define CONTEXT_AEBP	CONTEXT_REGS[REG_RBP]
#define CONTEXT_AESI	CONTEXT_REGS[REG_RSI]
#define CONTEXT_AEDI	CONTEXT_REGS[REG_RDI]
#endif

#include "sigsegv_common_x86.h"

static void segfault_vec(int /* sig */, siginfo_t *sip, void *_ucp)
{
	CONTEXT_ATYPE CONTEXT_NAME = (CONTEXT_ATYPE) _ucp;
	char *fault_addr = (char *)sip->si_addr;
	memptr addr = (memptr)(uintptr)(fault_addr - fixed_memory_offset);
	if (fault_addr == 0 || CONTEXT_AEIP == 0)
	{
		real_segmentationfault();
		/* not reached (hopefully) */
		return;
	}
	handle_access_fault(CONTEXT_NAME, addr);
}

void install_sigsegv() {
	struct sigaction act;
	struct sigaction *oact = (struct sigaction*)malloc(sizeof(struct sigaction));
	memset(&act, 0, sizeof(act));
	if (oact == NULL) {
		panicbug("Not enough memory");
		exit(EXIT_FAILURE);
	}
	act.sa_sigaction = segfault_vec;
	sigemptyset (&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, oact);
}

void uninstall_sigsegv()
{
	signal(SIGSEGV, SIG_DFL);
}
