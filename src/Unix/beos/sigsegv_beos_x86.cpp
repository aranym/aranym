/*
 * sigsegv_beos_x86.cpp - x86/x86_64 BeOS/Haiku SIGSEGV handler
 *
 * Copyright (c) 2018 Thorsten Otto of ARAnyM dev team (see AUTHORS)
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
 *
 * 2013-06-16 : Adapted to 64 Bit Linux - Jens Heitmann
 * 2014-07-05 : Merged with 64bit version,
 *              lots of fixes - Thorsten Otto
 *
 */

#include "sysdeps.h"
#include "cpu_emulation.h"

#define DEBUG 1
#include "debug.h"

#if defined(__BEOS__) || defined(__HAIKU__)

#include <csignal>

#ifdef CPU_i386
#define CONTEXT_REGS    (&CONTEXT_NAME->uc_mcontext.eip)
#define REG_EFL	(offsetof(mcontext_t, eflags) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RIP	(offsetof(mcontext_t, eip) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RAX	(offsetof(mcontext_t, eax) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RBX	(offsetof(mcontext_t, ebx) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RCX	(offsetof(mcontext_t, ecx) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RDX	(offsetof(mcontext_t, edx) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RBP	(offsetof(mcontext_t, ebp) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RSI	(offsetof(mcontext_t, esi) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RDI	(offsetof(mcontext_t, edi) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#define REG_RSP	(offsetof(mcontext_t, esp) / sizeof(CONTEXT_NAME->uc_mcontext.eip))
#endif
#if defined(CPU_x86_64)
#define CONTEXT_REGS    (&CONTEXT_NAME->uc_mcontext.rax)
#define REG_EFL	(offsetof(mcontext_t, rflags) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RIP	(offsetof(mcontext_t, rip) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RAX	(offsetof(mcontext_t, rax) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RBX	(offsetof(mcontext_t, rbx) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RCX	(offsetof(mcontext_t, rcx) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RDX	(offsetof(mcontext_t, rdx) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RBP	(offsetof(mcontext_t, rbp) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RSI	(offsetof(mcontext_t, rsi) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RDI	(offsetof(mcontext_t, rdi) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_RSP	(offsetof(mcontext_t, rsp) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R8	(offsetof(mcontext_t, r8)  / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R9	(offsetof(mcontext_t, r9)  / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R10	(offsetof(mcontext_t, r10) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R11	(offsetof(mcontext_t, r11) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R12	(offsetof(mcontext_t, r12) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R13	(offsetof(mcontext_t, r13) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R14	(offsetof(mcontext_t, r14) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#define REG_R15	(offsetof(mcontext_t, r15) / sizeof(CONTEXT_NAME->uc_mcontext.rax))
#endif
#if defined(CPU_i386) || defined(CPU_x86_64)
#define CONTEXT_NAME	ucp
#define CONTEXT_TYPE	volatile ucontext_t
#define CONTEXT_ATYPE	CONTEXT_TYPE *
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
	uintptr faultaddr = (uintptr)sip->si_addr;	/* CONTEXT_REGS[REG_CR2] */
	memptr addr = (memptr)(faultaddr - fixed_memory_offset);
#if DEBUG
	if (addr >= 0xff000000)
		addr &= 0x00ffffff;
	if (addr < 0x00f00000 || addr > 0x00ffffff) // YYY
		bug("\nsegfault: pc=%08x, " REG_RIP_NAME " =%p, addr=%p (0x%08x)", m68k_getpc(), (void *)CONTEXT_AEIP, sip->si_addr, addr);
	if (faultaddr < (uintptr)(fixed_memory_offset - 0x1000000UL)
#ifdef CPU_x86_64
		|| faultaddr >= ((uintptr)fixed_memory_offset + 0x100000000UL)
#endif
		)
	{
#ifdef HAVE_DISASM_X86
		if (CONTEXT_AEIP != 0)
		{
			char buf[256];
			
			x86_disasm((const uint8 *)CONTEXT_AEIP, buf, 1);
			panicbug("%s", buf);
		}
#endif
		// raise(SIGBUS);
	}
#endif
	if (faultaddr == 0 || CONTEXT_AEIP == 0)
	{
		real_segmentationfault();
		/* not reached (hopefully) */
		return;
	}
	handle_access_fault(CONTEXT_NAME, addr);
}

void install_sigsegv() {
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	sigemptyset(&act.sa_mask);
	act.sa_sigaction = segfault_vec;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, NULL);
#if defined(CPU_x86_64) /* XXX is this really neccessary? */
	sigaction(SIGILL, &act, NULL);
#endif
}

void uninstall_sigsegv()
{
	signal(SIGSEGV, SIG_DFL);
#ifdef HW_SIGSEGV_STATISTICS
	for (unsigned int i = 0; i < 256; i++)
		if (x86_opcodes[i] != 0)
			bug("opcodes: %02x = %lu", i, x86_opcodes[i]);
#endif
}

#endif
