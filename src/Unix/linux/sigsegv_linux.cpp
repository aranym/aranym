
#include "sysdeps.h"
#include "memory.h"

#include <csignal>

#define DEBUG 1
#include "debug.h"

static int in_handler = 0;

enum transfer_type_t {
	TYPE_UNKNOWN,
	TYPE_LOAD,
	TYPE_STORE
};

#if (__i386__)

enum instruction_t {
	INSTR_UNKNOWN,
	INSTR_MOVZX8,
	INSTR_MOVZX16,
	INSTR_MOV8L,
	INSTR_MOV32L,
	INSTR_MOV8S,
	INSTR_MOV32S
};

static inline int get_instr_size_add(unsigned char *p)
{
	int mod = (p[0] >> 6) & 3;
	int rm = p[0] & 7;
	int offset = 0;

	// ModR/M Byte
	switch (mod) {
	case 0: // [reg]
		if (rm == 5) return 4; // disp32
		break;
	case 1: // disp8[reg]
		offset = 1;
		break;
	case 2: // disp32[reg]
		offset = 4;
		break;
	case 3: // register
		return 0;
	}
	
	// SIB Byte
	if (rm == 4) {
		if (mod == 0 && (p[1] & 7) == 5)
			offset = 5; // disp32[index]
		else
			offset++;
	}

	return offset;
}


static void segfault_vec(int x, struct sigcontext sc) {
	memptr addr = sc.cr2;
	memptr ainstr = sc.eip;
	uint32 instr = (uint32)*(uint32 *)ainstr;
	uint8 *addr_instr = (uint8 *)sc.eip;
	int reg = -1;
	int len = 0;
	transfer_type_t transfer_type = TYPE_UNKNOWN;
	int size = 4;
	instruction_t instruction = INSTR_UNKNOWN;
	void *preg;

	D(panicbug("BUS ERROR fault address is %08x at %08x", addr, ainstr));
	D(panicbug("instruction is %08x", instr));

	if (in_handler) {
		D(panicbug("In handler sigsegv!"));
		abort();
	}

	in_handler = 1;

	addr -= FMEMORY;

	if (addr_instr[0] == 0x66) {
		addr_instr++;
		len++;
		size = 2;
		D(panicbug("Word instr:"));
	}
	
	switch (addr_instr[0]) {
		case 0x0f:
			switch (addr_instr[1]) {
				case 0xb6:
					D(panicbug("MOVZX r32, m8"));
					size = 1;
					transfer_type = TYPE_LOAD;
					instruction = INSTR_MOVZX8;
					reg = (addr_instr[2] >> 3 ) & 7;
					len += 3 + get_instr_size_add(addr_instr + 2);
					break;
				case 0xb7:
					D(panicbug("MOVZX r32, m16"));
					size = 2;
					transfer_type = TYPE_LOAD;
					instruction = INSTR_MOVZX16;
					reg = (addr_instr[2] >> 3 ) & 7;
					len += 3 + get_instr_size_add(addr_instr + 2);
					break;
			}
			break;
		case 0x8a:
			D(panicbug("MOV r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_MOV8L;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x8b:
			D(panicbug("MOV r32, m32"));
			transfer_type = TYPE_LOAD;
			instruction = INSTR_MOV32L;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x88:
			D(panicbug("MOV m8, r8"));
			transfer_type = TYPE_STORE;
			size = 1;
			instruction = INSTR_MOV8S;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x89:
			D(panicbug("MOV m32, r32"));
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOV32S;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 +get_instr_size_add(addr_instr + 1);
			break;
		case 0x0a:
			D(panicbug(""));
			break;
	}

	if (instruction == INSTR_UNKNOWN) {
		panicbug("Unknown instruction!");
		abort();
	}

	if ((addr < 0x00f00000) || ((addr > 0x00ffffff) && (addr < 0xfff00000))) goto buserr;

	switch (reg) {
		case 0: preg = &(sc.eax); break;
		case 1: preg = &(sc.ecx); break;
		case 2: preg = &(sc.edx); break;
		case 3: preg = &(sc.ebx); break;
		case 4: preg = (((uae_u8*)&(sc.eax)) + 1); break;
		case 5: preg = (size > 1) ? (void *)(&(sc.ebp)) : (void*)(((uae_u8*)&(sc.ecx)) + 1); break;
		case 6: preg = (size > 1) ? (void*)(&(sc.esi)) : (void*)(((uae_u8*)&(sc.edx)) + 1); break;
		case 7: preg = (size > 1) ? (void*)(&(sc.edi)) : (void*)(((uae_u8*)&(sc.ebx)) + 1); break;
		default: abort();

	}

	D(panicbug("Register %d, place %08x, address %08x", reg, preg, addr));

	if (addr >= 0xff000000)
		addr -= 0xff000000;

	switch (instruction) {
		case INSTR_MOVZX16:
			*((uae_u32 *)preg) = 0;
			*((uae_u16 *)preg) = HWget_w(addr);
			break;
		case INSTR_MOV32L:
			if (size == 4) {
				*((uae_u32 *)preg) = HWget_l(addr);
			} else {
				*((uae_u16 *)preg) = HWget_w(addr);
			}
			break;
		case INSTR_MOV32S:
			if (size == 4) {
				HWput_l(addr, *((uae_u32 *)preg));
			} else {
				HWput_w(addr, *((uae_u16 *)preg));
			}
			break;
		default: abort();
	}

	D(panicbug("HWspace access handled!"));
	D(panicbug("Next instruction on %08x", sc.eip += len));
	in_handler = 0;
	return;
buserr:
	D(panicbug("Atari bus error"));
	regs.mmu_fault_addr = addr;
	in_handler = 0;
	longjmp(excep_env, 2);
}

#endif

void install_sigsegv() {
	signal(SIGSEGV, (sighandler_t)segfault_vec);
}
