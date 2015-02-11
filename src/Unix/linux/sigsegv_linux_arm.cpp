/*
 * sigsegv_linux_arm.cpp - x86_64 Linux SIGSEGV handler
 *
 * Copyright (c) 2014 Jens Heitmann ARAnyM dev team (see AUTHORS)
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
 *
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#define DEBUG 0
#include "debug.h"

#ifdef USE_JIT
extern void compiler_status();
# ifdef JIT_DEBUG
extern void compiler_dumpstate();
# endif
#endif

enum transfer_type_t {
	TYPE_UNKNOWN,
	TYPE_LOAD,
	TYPE_STORE
};

enum type_size_t {
	SIZE_UNKNOWN,
	SIZE_BYTE,
	SIZE_WORD,
	SIZE_INT
};

#include <csignal>

#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

int in_handler = 0;

enum {
  ARM_REG_PC = 15,
  ARM_REG_CPSR = 16
};

static inline void unknown_instruction(uint32 instr) {
		panicbug("Unknown instruction %08x!", instr);
#ifdef USE_JIT
		compiler_status();
# ifdef JIT_DEBUG
		compiler_dumpstate();
# endif
#endif
		abort();
}

static bool handle_arm_instruction(unsigned long *pregs, uintptr addr) {
	unsigned int *pc = (unsigned int *)pregs[ARM_REG_PC];

	D(panicbug("IP: %p [%08x] %p\n", pc, pc[0], addr));
	if (pc == 0) return false;

	if (in_handler > 0) {
                panicbug("Segmentation fault in handler :-(");
                abort();
        }

        in_handler += 1;

	transfer_type_t transfer_type = TYPE_UNKNOWN;
	int transfer_size = SIZE_UNKNOWN;
	enum { SIGNED, UNSIGNED };
	int style = UNSIGNED;

	// Handle load/store instructions only
	const unsigned int opcode = pc[0];
	switch ((opcode >> 25) & 7) {
		case 0: // Halfword and Signed Data Transfer (LDRH, STRH, LDRSB, LDRSH)
			// Determine transfer size (S/H bits)
			switch ((opcode >> 5) & 3) {
				case 0: // SWP instruction
					panicbug("FIXME: SWP Instruction");
					break;
				case 1: // Unsigned halfwords
	  				transfer_size = SIZE_WORD;
	  				break;
				case 3: // Signed halfwords
					style = SIGNED;
	  				transfer_size = SIZE_WORD;
	  				break;
				case 2: // Signed byte
					style = SIGNED;
				  	transfer_size = SIZE_BYTE;
	  				break;
				}
			break;
  		case 2:
  		case 3: // Single Data Transfer (LDR, STR)
			style = UNSIGNED;
			// Determine transfer size (B bit)
			if (((opcode >> 22) & 1) == 1)
	  			transfer_size = SIZE_BYTE;
			else
	  			transfer_size = SIZE_INT;
			break;
  		default:
			panicbug("FIXME: support load/store mutliple?");
                	abort();
  	}

  	// Check for invalid transfer size (SWP instruction?)
  	if (transfer_size == SIZE_UNKNOWN) {
		panicbug("Invalid transfer size");
                abort();
	}

	// Determine transfer type (L bit)
	if (((opcode >> 20) & 1) == 1)
		transfer_type = TYPE_LOAD;
	else
		transfer_type = TYPE_STORE;

  int rd = (opcode >> 12) & 0xf;
#if DEBUG
  static const char * reg_names[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc"
  };
  panicbug("%s %s register %s\n",
		 transfer_size == SIZE_BYTE ? "byte" :
		 transfer_size == SIZE_WORD ? "word" :
		 transfer_size == SIZE_INT ? "long" : "unknown",
		 transfer_type == TYPE_LOAD ? "load to" : "store from",
		 reg_names[rd]);
//  for (int i = 0; i < 16; i++) {
//  	panicbug("%s : %p", reg_names[i], pregs[i]);
//  }
#endif

    	if (addr >= 0xff000000)
          addr &= 0x00ffffff;

        if ((addr < 0x00f00000) || (addr > 0x00ffffff))
          goto buserr;

	if (transfer_type == TYPE_LOAD) {
		switch(transfer_size) {
		  case SIZE_BYTE: {
		    pregs[rd] = style == SIGNED ? (uae_s8)HWget_b(addr) : (uae_u8)HWget_b(addr);
		    break;
		  }
		  case SIZE_WORD: {
		    pregs[rd] = do_byteswap_16(style == SIGNED ? (uae_s16)HWget_w(addr) : (uae_u16)HWget_w(addr));
		    break;
		  }
		  case SIZE_INT: {
		    pregs[rd] = do_byteswap_32(HWget_l(addr));
		    break;
		  }
		}
	} else {
		switch(transfer_size) {
		  case SIZE_BYTE: {
		    HWput_b(addr, pregs[rd]);
		    break;
		  }
		  case SIZE_WORD: {
		    HWput_w(addr, do_byteswap_16(pregs[rd]));
		    break;
		  }
		  case SIZE_INT: {
		    HWput_l(addr, do_byteswap_32(pregs[rd]));
		    break;
		  }
		}
	}

  	pregs[ARM_REG_PC] += 4;
	D(panicbug("processed: %p \n", pregs[ARM_REG_PC]));

        in_handler--;

	return true;

buserr:
        D(panicbug("Atari bus error"));

        BUS_ERROR(addr);
	return true;
} 

static void segfault_vec(int /*sig*/, siginfo_t *sip, void *uc) {
	mcontext_t *context = &(((struct ucontext *)uc)->uc_mcontext);
	unsigned long *regs = &context->arm_r0;
	uintptr addr = (uintptr)sip->si_addr;
        addr -= fixed_memory_offset;

	if (handle_arm_instruction(regs, addr)) {
	}
}

void install_sigsegv() {
	struct sigaction sigsegv_sa;
	sigemptyset(&sigsegv_sa.sa_mask);
	sigsegv_sa.sa_handler = (sighandler_t) segfault_vec;
	sigsegv_sa.sa_flags = SA_RESTART|SA_SIGINFO;
	sigaction(SIGSEGV, &sigsegv_sa, NULL);
	sigaction(SIGILL, &sigsegv_sa, NULL);
	D(panicbug("Installed sigseg handler"));
//	signal(SIGSEGV, (sighandler_t)segfault_vec);
}

void uninstall_sigsegv()
{
	signal(SIGSEGV, SIG_DFL);
}
