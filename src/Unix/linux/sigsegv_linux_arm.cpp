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
#include "disasm-glue.h"

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
	ARM_REG_R0 = 0,
	ARM_REG_R1 = 1,
	ARM_REG_R2 = 2,
	ARM_REG_R3 = 3,
	ARM_REG_R4 = 4,
	ARM_REG_R5 = 5,
	ARM_REG_R6 = 6,
	ARM_REG_R7 = 7,
	ARM_REG_R8 = 8,
	ARM_REG_R9 = 9,
	ARM_REG_R10 = 10,
	ARM_REG_SL = ARM_REG_R10,
	ARM_REG_R11 = 11,
	ARM_REG_FP = ARM_REG_R11,
	ARM_REG_R12 = 12,
	ARM_REG_IP = ARM_REG_R12,
	ARM_REG_R13 = 13,
	ARM_REG_SP = ARM_REG_R13,
	ARM_REG_R14 = 14,
	ARM_REG_LR = ARM_REG_R14,
	ARM_REG_R15 = 15,
	ARM_REG_PC = ARM_REG_R15,
	ARM_REG_CPSR = 16,
	
	ARM_REG_NUM
};

static const char *const reg_names[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "sl", "fp", "ip", "sp", "lr", "pc",
	"cpsr"
};

static inline void unknown_instruction(const unsigned long *pregs)
{
	uint32_t *pc = (uint32_t *)pregs[ARM_REG_PC];
	const uint32 instr = pc[0];
	panicbug("Unknown instruction %08x!", instr);
#ifdef HAVE_DISASM_ARM
	{
		char buf[256];
		const uint8 *ainstr = (const uint8 *)pc;
		
		arm_disasm(ainstr, buf, 1);
		panicbug("%s", buf);
	}
#endif
	for (int i = 0; i < ARM_REG_NUM; i++)
	{
		bug("%s : %08lx", reg_names[i], pregs[i]);
	}
#ifdef USE_JIT
	compiler_status();
# ifdef JIT_DEBUG
	compiler_dumpstate();
# endif
#endif
	abort();
}

#ifdef NO_NESTED_SIGSEGV
static __attribute_noinline__ void handle_arm_instruction2(unsigned long *pregs, memptr faultaddr)
#else
static __attribute_noinline__ void handle_arm_instruction(unsigned long *pregs, memptr faultaddr)
#endif
{
	memptr addr = faultaddr;
	uint32_t *pc = (uint32_t *)pregs[ARM_REG_PC];

	const uint32_t opcode = pc[0];
	D(bug("PC: %p [%08x] %08x", pc, opcode, faultaddr));

	if (in_handler > 0) {
		panicbug("Segmentation fault in handler :-(, faultaddr=0x%08x", faultaddr);
		abort();
	}

	in_handler += 1;

#ifdef USE_JIT	/* does not compile with default configure */
	D(compiler_status());
#endif
	D(bug("BUS ERROR fault address is %08x", addr));

	D(bug("PC %08x", m68k_getpc())); 

#ifdef HW_SIGSEGV

	transfer_type_t transfer_type = TYPE_UNKNOWN;
	int transfer_size = SIZE_UNKNOWN;
	enum { SIGNED, UNSIGNED };
	int style = UNSIGNED;

	// Handle load/store instructions only
	switch ((opcode >> 25) & 7) {
		case 0: // Halfword and Signed Data Transfer (LDRH, STRH, LDRSB, LDRSH)
			// Determine transfer size (S/H bits)
			switch ((opcode >> 5) & 3) {
				case 0: // SWP instruction
					panicbug("FIXME: SWP Instruction");
					unknown_instruction(pregs);
					break;
				case 1: // Unsigned halfwords (LDRH/STRH)
					transfer_size = SIZE_WORD;
					break;
				case 2: // Signed byte (LDRSB)
					style = SIGNED;
					transfer_size = SIZE_BYTE;
					break;
				case 3: // Signed halfwords (LDRSH)
					style = SIGNED;
					transfer_size = SIZE_WORD;
					break;
				}
			break;
		case 2: // Single Data Transfer (LDR, STR immediate)
		case 3: // Single Data Transfer (LDR, STR register)
			style = UNSIGNED;
			// Determine transfer size (B bit)
			if (((opcode >> 22) & 1) == 1)
				transfer_size = SIZE_BYTE;
			else
				transfer_size = SIZE_INT;
			break;
		case 4: // LDM/STM
		case 5: // B
		case 6: // LDC/STC
		case 7: // CDP
		default:
			panicbug("FIXME: support load/store multiple?");
			unknown_instruction(pregs);
			break;
	}

	// Check for invalid transfer size (SWP instruction?)
	if (transfer_size == SIZE_UNKNOWN) {
		panicbug("Invalid transfer size");
		unknown_instruction(pregs);
	}

	// Determine transfer type (L bit)
	if (((opcode >> 20) & 1) == 1)
		transfer_type = TYPE_LOAD;
	else
		transfer_type = TYPE_STORE;

	int rt = (opcode >> 12) & 0xf;
#if DEBUG
	bug("%s %s register %s",
		transfer_size == SIZE_BYTE ? "byte" :
		transfer_size == SIZE_WORD ? "word" :
		transfer_size == SIZE_INT ? "long" : "unknown",
		transfer_type == TYPE_LOAD ? "load to" : "store from",
		reg_names[rt]);
#ifdef HAVE_DISASM_ARM
	{
		char buf[256];
		
		arm_disasm((const uint8_t *)pc, buf, 1);
		bug("%s", buf);
	}
#endif
//	for (int i = 0; i < ARM_REG_NUM; i++) {
//		bug("%s : %08lx", reg_names[i], pregs[i]);
//	}
#endif

	if (addr >= 0xff000000)
		addr &= 0x00ffffff;

	if ((addr < 0x00f00000) || (addr > 0x00ffffff))
		goto buserr;

	if (transfer_type == TYPE_LOAD) {
		switch(transfer_size) {
		case SIZE_BYTE:
			pregs[rt] = style == SIGNED ? (uae_s8)HWget_b(addr) : (uae_u8)HWget_b(addr);
			break;
		case SIZE_WORD:
			pregs[rt] = do_byteswap_16(style == SIGNED ? (uae_s16)HWget_w(addr) : (uae_u16)HWget_w(addr));
			break;
		case SIZE_INT:
			pregs[rt] = do_byteswap_32(HWget_l(addr));
			break;
		}
	} else {
		switch(transfer_size) {
		case SIZE_BYTE:
			HWput_b(addr, pregs[rt]);
			break;
		case SIZE_WORD:
			HWput_w(addr, do_byteswap_16(pregs[rt]));
			break;
		case SIZE_INT:
			HWput_l(addr, do_byteswap_32(pregs[rt]));
			break;
		}
	}

	pregs[ARM_REG_PC] += 4;
	D(bug("processed: %08lx", pregs[ARM_REG_PC]));

	in_handler--;

	return;

buserr:

#endif /* HW_SIGSEGV */

	BUS_ERROR(addr);
} 


#ifdef NO_NESTED_SIGSEGV

JMP_BUF sigsegv_env;

static void
__attribute__((__noreturn__))
atari_bus_fault(void)
{
	breakpt();
	THROW(2);
}

static __attribute_noinline__ void handle_arm_instruction(unsigned long *pregs, volatile memptr faultaddr)
{
	if (SETJMP(sigsegv_env) != 0)
	{
		/*
		 * we get back here by a LONGJMP in BUS_ERROR,
		 * triggered by one of the HWget_x/HWput_x calls
		 * in the handler above
		 */
		D(bug("Atari bus error (%s)", (regs.mmu_fault_addr < 0x00f00000) || (regs.mmu_fault_addr > 0x00ffffff) ? "mem" : "hw"));
		pregs[ARM_REG_PC] = (uintptr)atari_bus_fault;
		return;
	}

	handle_arm_instruction2(pregs, faultaddr);
}
#endif /* NO_NESTED_SIGSEGV */


static void segfault_vec(int /*sig*/, siginfo_t *sip, void *uc) {
	mcontext_t *context = &(((ucontext_t *)uc)->uc_mcontext);
	unsigned long *regs = &context->arm_r0;
	uintptr faultaddr = (uintptr)sip->si_addr;
	memptr addr = (memptr)(faultaddr - fixed_memory_offset);

#if DEBUG
	if (addr >= 0xff000000)
		addr &= 0x00ffffff;
	if (addr < 0x00f00000 || addr > 0x00ffffff)
		bug("segfault: m68k pc=%08x, arm PC=%08lx, addr=%08x (0x%08x)", m68k_getpc(), regs[ARM_REG_PC], faultaddr, addr);
	if (faultaddr < (uintptr)(fixed_memory_offset - 0x1000000UL)
#ifdef CPU_aarch64
		|| faultaddr >= ((uintptr)fixed_memory_offset + 0x100000000UL)
#endif
		)
	{
#ifdef HAVE_DISASM_ARM
		if (regs[ARM_REG_PC] != 0)
		{
			char buf[256];
			
			arm_disasm((const uint8 *)regs[ARM_REG_PC], buf, 1);
			panicbug("%s", buf);
		}
#endif
		// raise(SIGBUS);
	}
#endif

	if (faultaddr == 0 || regs[ARM_REG_PC] == 0)
	{
		real_segmentationfault();
		/* not reached (hopefully) */
		return;
	}
	
	handle_arm_instruction(regs, addr);
}

void install_sigsegv() {
	struct sigaction sigsegv_sa;
	memset(&sigsegv_sa, 0, sizeof(sigsegv_sa));
	sigemptyset(&sigsegv_sa.sa_mask);
	sigsegv_sa.sa_handler = (sighandler_t) segfault_vec;
	sigsegv_sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sigsegv_sa, NULL);
	D(bug("Installed sigsegv handler"));
}

void uninstall_sigsegv()
{
	signal(SIGSEGV, SIG_DFL);
}
