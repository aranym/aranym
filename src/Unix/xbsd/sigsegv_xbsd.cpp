
#include "sysdeps.h"
#include "memory.h"
#include <SDL_endian.h>

#include <csignal>

#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

int in_handler = 0;

extern void compiler_status();

#define DEBUG 2
#include "debug.h"

enum transfer_type_t {
	TYPE_UNKNOWN,
	TYPE_LOAD,
	TYPE_STORE
};

#if (__i386__)

/* instruction jump table */
//i386op_func *cpufunctbl[256];

static struct sigaction sigsegv_sa;

enum instruction_t {
	INSTR_UNKNOWN,
	INSTR_MOVZX8,
	INSTR_MOVZX16,
	INSTR_MOVSX8,
	INSTR_MOV8,
	INSTR_MOV32,
	INSTR_MOVIMM8,
	INSTR_MOVIMM32,
	INSTR_OR8,
	INSTR_ORIMM8,
	INSTR_AND8,
	INSTR_ADD8,
	INSTR_CMP8,
	INSTR_DIV8,
	INSTR_TESTIMM8
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

static inline void set_eflags(int i, struct sigcontext *sc) {
	if (i < 0) sc->sc_eflags |= 0x1;
		else sc->sc_eflags &= 0xfffffffe;
	if ((i % 2) == 0) sc->sc_eflags |= 0x4;
		else sc->sc_eflags &= 0xfffffffb;
	if (i == 0) sc->sc_eflags |= 0x40;
		else sc->sc_eflags &= 0xffffffbf;
	if (i > 127) sc->sc_eflags |= 0x80;
		else sc->sc_eflags &= 0xffffff7f;
	if ((i > 255) || (i < 0)) sc->sc_eflags |= 0x1;
		else sc->sc_eflags &= 0xfffffffe;
}

static void segfault_vec(int x, siginfo_t *sip, struct sigcontext *scp) {
	memptr addr = (memptr)(sip->si_addr);
	memptr ainstr = scp->sc_eip;
	uint32 instr = (uint32)*(uint32 *)ainstr;
	uint8 *addr_instr = (uint8 *)(scp->sc_eip);
	int reg = -1;
	int len = 0;
	transfer_type_t transfer_type = TYPE_UNKNOWN;
	int size = 4;
	int imm = 0;
	int pom1, pom2 = 0;
	instruction_t instruction = INSTR_UNKNOWN;
	void *preg;

#if 1
	if (in_handler > 0) {
		panicbug("Segmentation fault in handler :-(");
		abort();
	}
#endif
	in_handler += 1;

#ifdef JIT	/* does not compile with default configure */
	D(compiler_status());
#endif
	D(panicbug("\nBUS ERROR fault address is %08x at %08x", addr, ainstr));
	D2(panicbug("instruction is %08x", instr));

	D2(panicbug("PC %08x", regs.pc)); 

	addr -= FMEMORY;

#ifdef HW_SIGSEGV

	if (addr_instr[0] == 0x66) {
		addr_instr++;
		len++;
		size = 2;
		D(panicbug("Word instr:"));
	}
	
	switch (addr_instr[0]) {
		case 0x02:
			D(panicbug("ADD r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_ADD8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x08:
			D(panicbug("OR m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_OR8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x0a:
			D(panicbug("OR r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_OR8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x0f:
			switch (addr_instr[1]) {
				case 0xb6:
					D(panicbug("MOVZX r32, m8"));
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
				case 0xbe:
					D(panicbug("MOVSX r32, m8"));
					transfer_type = TYPE_LOAD;
					instruction = INSTR_MOVSX8;
					reg = (addr_instr[2] >> 3 ) & 7;
					len += 3 + get_instr_size_add(addr_instr + 2);
					break;
			}
			break;
		case 0x22:
			D(panicbug("AND r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_AND8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x3a:
			D(panicbug("CMP r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_CMP8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x80:
			D(panicbug("OR m8, imm8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_ORIMM8;
			reg = (addr_instr[1] >> 3) & 7;
			// imm = addr_instr[3];
			switch(addr_instr[1] & 0x07) {
				case 0: imm = addr_instr[2]; break;
				case 4: imm = addr_instr[3]; break;
				default:
					panicbug("OR m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
			}
			len += 3 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x8a:
			D(panicbug("MOV r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_MOV8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x8b:
			D(panicbug("MOV r32, m32"));
			transfer_type = TYPE_LOAD;
			instruction = INSTR_MOV32;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x88:
			D(panicbug("MOV m8, r8"));
			transfer_type = TYPE_STORE;
			size = 1;
			instruction = INSTR_MOV8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x89:
			D(panicbug("MOV m32, r32"));
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOV32;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0xc6:
			D(panicbug("MOV m8, imm8"));
			transfer_type = TYPE_STORE;
			size = 1;
			instruction = INSTR_MOVIMM8;
			reg = (addr_instr[1] >> 3) & 7;
			// imm = addr_instr[3];	// JOY: was 2
			switch(addr_instr[1] & 0x07) {
				case 0: imm = addr_instr[2]; break;
				case 4: imm = addr_instr[3]; break;
				case 5: imm = addr_instr[6]; break; // used in JIT raw_mov_b_mi
				default:
					panicbug("MOV m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
			}
			len += 3 + get_instr_size_add(addr_instr + 1);
			break;
		case 0xc7:
			D(panicbug("MOV m32, imm32"));
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOVIMM32;
			reg = (addr_instr[1] >> 3) & 7;
			if (size == 2) {
				imm = ((uae_u16)addr_instr[7] << 8) + addr_instr[6];
			} else {
				imm = ((uae_u32)addr_instr[9] << 24) + ((uae_u32)addr_instr[8] << 16) + ((uae_u32)addr_instr[7] << 8) + addr_instr[6];
			}
			len += 4 + get_instr_size_add(addr_instr + 1);
			if (size == 4) len += 2;
			break;
		case 0xf6:
			reg = (addr_instr[1] >> 3) & 7;
			size = 1;
			switch (addr_instr[1] & 0x07) {
				case 6:
					D(panicbug("DIV m8"));
					transfer_type = TYPE_LOAD;
					instruction = INSTR_DIV8;
					len += 2 + get_instr_size_add(addr_instr + 1);
					break;
				case 0:
					D(panicbug("TEST m8, imm8"));
					transfer_type = TYPE_STORE;
					instruction = INSTR_TESTIMM8;
					imm = addr_instr[2];
					len += 3 + get_instr_size_add(addr_instr + 1);
					break;
				case 4: 
					D(panicbug("TEST m8, imm8"));
					transfer_type = TYPE_STORE;
					instruction = INSTR_TESTIMM8;
					imm = addr_instr[3];
					len += 3 + get_instr_size_add(addr_instr + 1);
					break;
				case 5:
					D(panicbug("TEST m8, imm8"));
					transfer_type = TYPE_STORE;
					instruction = INSTR_TESTIMM8;
					reg = (addr_instr[1] >> 3) & 7;
					imm = addr_instr[6];
					len += 3 + get_instr_size_add(addr_instr + 1);
					break;
				default:
					panicbug("TEST m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
			}
			break;
	}

	if (instruction == INSTR_UNKNOWN) {
		panicbug("Unknown instruction %08x!", instr);
		abort();
	}

	if ((addr < 0x00f00000) || ((addr > 0x00ffffff) && (addr < 0xfff00000))) goto buserr;

	switch (reg) {
		case 0: preg = &(scp->sc_eax); break;
		case 1: preg = &(scp->sc_ecx); break;
		case 2: preg = &(scp->sc_edx); break;
		case 3: preg = &(scp->sc_ebx); break;
		case 4: preg = (((uae_u8*)&(scp->sc_eax)) + 1); break;
		case 5: preg = (size > 1) ? (void *)(&(scp->sc_ebp)) : (void*)(((uae_u8*)&(scp->sc_ecx)) + 1); break;
		case 6: preg = (size > 1) ? (void*)(&(scp->sc_esi)) : (void*)(((uae_u8*)&(scp->sc_edx)) + 1); break;
		case 7: preg = (size > 1) ? (void*)(&(scp->sc_edi)) : (void*)(((uae_u8*)&(scp->sc_ebx)) + 1); break;
		default: abort();

	}

	D2(panicbug("Register %d, place %08x, address %08x", reg, preg, addr));

	if (addr >= 0xff000000)
		addr -= 0xff000000;

	if (transfer_type == TYPE_LOAD) {
		switch (instruction) {
			case INSTR_MOVZX16:
				*((uae_u32 *)preg) = 0;
				*((uae_u16 *)preg) = SDL_SwapBE16((uae_u16)HWget_w(addr));
				break;
			case INSTR_MOV8:
				*((uae_u8 *)preg) = HWget_b(addr);
				break;
			case INSTR_MOV32:
				if (size == 4) {
					*((uae_u32 *)preg) = SDL_SwapBE32(HWget_l(addr));
				} else {
					*((uae_u16 *)preg) = SDL_SwapBE16(HWget_w(addr));
				}
				break;
			case INSTR_OR8:
				*((uae_u8 *)preg) |= HWget_b(addr);
				set_eflags(*((uae_u8 *)preg), scp);
				break;
			case INSTR_AND8:
				*((uae_u8 *)preg) &= HWget_b(addr);
				imm = *((uae_u8 *)preg);
				set_eflags(*((uae_u8 *)preg), scp);
				break;
			case INSTR_MOVZX8:
				if (size == 4) {
					*((uae_u32 *)preg) = (uae_u8)HWget_b(addr);
				} else {
					*((uae_u16 *)preg) = (uae_u8)HWget_b(addr);
				}
				break;
			case INSTR_MOVSX8:
				if (size == 4) {
					*((uae_s32 *)preg) = (uae_s8)HWget_b(addr);
				} else {
					*((uae_s16 *)preg) = (uae_s8)HWget_b(addr);
				}
				break;
			case INSTR_ADD8:
				*((uae_u8 *)preg) += HWget_b(addr);
				break;
			case INSTR_CMP8:
				imm = *((uae_u8 *)preg);
				imm -= HWget_b(addr);
				set_eflags(imm, scp);
				break;
			case INSTR_DIV8:
				pom1 = scp->sc_eax & 0xffff;
				pom2 = HWget_b(addr);
				scp->sc_eax = scp->sc_eax & 0xffff0000 + ((pom1 / pom2) << 8) + (pom1 / pom2);
				break;
			default: abort();
		}
	} else {
		switch (instruction) {
			case INSTR_MOV8:
				D2(panicbug("MOV value = $%x\n", *((uae_u8 *)preg)));
				HWput_b(addr, *((uae_u8 *)preg));
				break;
			case INSTR_MOV32:
				if (size == 4) {
					HWput_l(addr, SDL_SwapBE32(*((uae_u32 *)preg)));
				} else {
					HWput_w(addr, SDL_SwapBE16(*((uae_u16 *)preg)));
				}
				break;
			case INSTR_OR8:
				imm = HWget_b(addr);
				imm |= *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_eflags(imm, scp);
				break;
			case INSTR_ORIMM8:
				imm |= HWget_b(addr);
				HWput_b(addr, imm);
				set_eflags(imm, scp);
				break;
			case INSTR_MOVIMM8:
				HWput_b(addr, (uae_u8)imm);
				break;
			case INSTR_MOVIMM32:
				if (size == 4) {
					HWput_l(addr, (uae_u32)imm);
				} else {
					HWput_w(addr, (uae_u16)imm);
				}
				break;
			case INSTR_TESTIMM8:
				imm &= HWget_b(addr);
				set_eflags(imm, scp);
				break;
			default: abort();
		}
	}

	D2(panicbug("Access handled"));
	D2(panicbug("Next instruction on %08x", scp->sc_eip + len));
	scp->sc_eip += len;

	in_handler -= 1;
	return;
buserr:
	D(panicbug("Atari bus error"));

#endif /* HW_SIGSEGV */

	regs.mmu_fault_addr = addr;
	in_handler = 0;
	THROW(2);
}

#endif

void install_sigsegv() {
	sigemptyset(&sigsegv_sa.sa_mask);
	sigsegv_sa.sa_handler = segfault_vec;
	sigsegv_sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sigsegv_sa, NULL);
//	signal(SIGSEGV, (sighandler_t)segfault_vec);
}
