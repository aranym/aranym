#include "cpu_emulation.h"
#include <SDL_endian.h>
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
	TYPE_BYTE,
	TYPE_WORD,
	TYPE_INT
};

/******************************************************************************/

#ifdef OS_linux

#include <csignal>

#ifndef HAVE_SIGHANDLER_T
typedef void (*sighandler_t)(int);
#endif

#define CONTEXT_NAME	sc
#define CONTEXT_TYPE	struct sigcontext
#define CONTEXT_ADDR	& CONTEXT_NAME
#define CONTEXT_ATYPE	CONTEXT_TYPE *
#define CONTEXT_EIP	CONTEXT_NAME.eip
#define CONTEXT_EFLAGS	CONTEXT_NAME.eflags
#define CONTEXT_EAX	CONTEXT_NAME.eax
#define CONTEXT_EBX	CONTEXT_NAME.ebx
#define CONTEXT_ECX	CONTEXT_NAME.ecx
#define CONTEXT_EDX	CONTEXT_NAME.edx
#define CONTEXT_EBP	CONTEXT_NAME.ebp
#define CONTEXT_ESI	CONTEXT_NAME.esi
#define CONTEXT_EDI	CONTEXT_NAME.edi
#define CONTEXT_CR2	CONTEXT_NAME.cr2
#define CONTEXT_AEIP	CONTEXT_NAME->eip
#define CONTEXT_AEFLAGS	CONTEXT_NAME->eflags
#define CONTEXT_AEAX	CONTEXT_NAME->eax
#define CONTEXT_AEBX	CONTEXT_NAME->ebx
#define CONTEXT_AECX	CONTEXT_NAME->ecx
#define CONTEXT_AEDX	CONTEXT_NAME->edx
#define CONTEXT_AEBP	CONTEXT_NAME->ebp
#define CONTEXT_AESI	CONTEXT_NAME->esi
#define CONTEXT_AEDI	CONTEXT_NAME->edi
#define CONTEXT_ACR2	CONTEXT_NAME->cr2

#endif /* OS_linux */

/******************************************************************************/

#ifdef OS_cygwin

#define WIN32_LEAN_AND_MEAN /* avoid including junk */
#include <windows.h>
#include <winerror.h>

#define CONTEXT_NAME   ContextRecord
#define CONTEXT_TYPE   CONTEXT *
#define CONTEXT_ADDR   CONTEXT_NAME
#define CONTEXT_ATYPE  CONTEXT_TYPE
#define CONTEXT_EIP    CONTEXT_NAME->Eip
#define CONTEXT_EFLAGS CONTEXT_NAME->EFlags
#define CONTEXT_EAX    CONTEXT_NAME->Eax
#define CONTEXT_EBX    CONTEXT_NAME->Ebx
#define CONTEXT_ECX    CONTEXT_NAME->Ecx
#define CONTEXT_EDX    CONTEXT_NAME->Edx
#define CONTEXT_EBP    CONTEXT_NAME->Ebp
#define CONTEXT_ESI    CONTEXT_NAME->Esi
#define CONTEXT_EDI    CONTEXT_NAME->Edi
#define CONTEXT_AEIP    CONTEXT_NAME->Eip
#define CONTEXT_AEFLAGS CONTEXT_NAME->EFlags
#define CONTEXT_AEAX    CONTEXT_NAME->Eax
#define CONTEXT_AEBX    CONTEXT_NAME->Ebx
#define CONTEXT_AECX    CONTEXT_NAME->Ecx
#define CONTEXT_AEDX    CONTEXT_NAME->Edx
#define CONTEXT_AEBP    CONTEXT_NAME->Ebp
#define CONTEXT_AESI    CONTEXT_NAME->Esi
#define CONTEXT_AEDI    CONTEXT_NAME->Edi


#endif /* OS_cygwin */




int in_handler = 0;

/******************************************************************************/

#if (CPU_i386)

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
	INSTR_IDIV8,
	INSTR_MUL8,
	INSTR_IMUL8,
	INSTR_NEG8,
	INSTR_NOT8,
	INSTR_TESTIMM8
};

enum case_instr_t {
	CASE_INSTR_ADD8MR	= 0x00,
	CASE_INSTR_ADD8RM	= 0x02,
	CASE_INSTR_OR8MR	= 0x08,
	CASE_INSTR_OR8RM	= 0x0a,
	CASE_INSTR_MOVxX	= 0x0f,
	CASE_INSTR_MOVZX8RM	= 0xb6,
	CASE_INSTR_MOVZX16RM	= 0xb7,
	CASE_INSTR_MOVSX8RM	= 0xbe
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

static inline void set_eflags(int i, CONTEXT_ATYPE CONTEXT_NAME, type_size_t t) {
/* MJ - AF and OF not tested, also CF for 32 bit */
	switch (t) {
		case TYPE_BYTE:
			if ((i > 255) || (i < 0)) CONTEXT_AEFLAGS |= 0x1;	// CF
				else CONTEXT_AEFLAGS &= 0xfffffffe;
			if (i > 127) CONTEXT_AEFLAGS |= 0x80;			// SF
				else CONTEXT_AEFLAGS &= 0xffffff7f;
		case TYPE_WORD:
			if ((i > 65535) || (i < 0)) CONTEXT_AEFLAGS |= 0x1;	// CF
				else CONTEXT_AEFLAGS &= 0xfffffffe;
			if (i > 32767) CONTEXT_AEFLAGS |= 0x80;			// SF
				else CONTEXT_AEFLAGS &= 0xffffff7f;
		case TYPE_INT:
			if (i > 2147483647) CONTEXT_AEFLAGS |= 0x80;		// SF
				else CONTEXT_AEFLAGS &= 0xffffff7f;

	}
	if ((i % 2) == 0) CONTEXT_AEFLAGS |= 0x4;				// PF
		else CONTEXT_AEFLAGS &= 0xfffffffb;
	if (i == 0) CONTEXT_AEFLAGS |= 0x40;					// ZF
		else CONTEXT_AEFLAGS &= 0xffffffbf;
}

static inline void *get_preg(int reg, CONTEXT_ATYPE CONTEXT_NAME, int size) {
	switch (reg) {
		case 0: return &(CONTEXT_AEAX);
		case 1: return &(CONTEXT_AECX);
		case 2: return &(CONTEXT_AEDX);
		case 3: return &(CONTEXT_AEBX);
		case 4: return (((uae_u8*)&(CONTEXT_AEAX)) + 1);
		case 5: return (size > 1) ? (void*)(&(CONTEXT_AEBP)) : (void*)(((uae_u8*)&(CONTEXT_AECX)) + 1);
		case 6: return (size > 1) ? (void*)(&(CONTEXT_AESI)) : (void*)(((uae_u8*)&(CONTEXT_AEDX)) + 1);
		case 7: return (size > 1) ? (void*)(&(CONTEXT_AEDI)) : (void*)(((uae_u8*)&(CONTEXT_AEBX)) + 1);
		default: abort();
	}
}

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

#ifdef NO_NESTED_SIGSEGV
static inline void handle_access_fault2(CONTEXT_TYPE CONTEXT_NAME, memptr faultaddr) {
#else /* NO_NESTED_SIGSEGV */
static inline void handle_access_fault(CONTEXT_TYPE CONTEXT_NAME, memptr faultaddr) {
#endif /* NO_NESTED_SIGSEGV */
	memptr addr = faultaddr;
	memptr ainstr = CONTEXT_EIP;
	uint32 instr = (uint32)*(uint32 *)ainstr;
	uint8 *addr_instr = (uint8 *)ainstr;
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

#ifdef USE_JIT	/* does not compile with default configure */
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
		case CASE_INSTR_ADD8MR:
			D(panicbug("ADD m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_ADD8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case CASE_INSTR_ADD8RM:
			D(panicbug("ADD r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_ADD8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case CASE_INSTR_OR8MR:
			D(panicbug("OR m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_OR8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case CASE_INSTR_OR8RM:
			D(panicbug("OR r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_OR8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case CASE_INSTR_MOVxX:
			switch (addr_instr[1]) {
				case CASE_INSTR_MOVZX8RM:
					D(panicbug("MOVZX r32, m8"));
					transfer_type = TYPE_LOAD;
					instruction = INSTR_MOVZX8;
					reg = (addr_instr[2] >> 3 ) & 7;
					len += 3 + get_instr_size_add(addr_instr + 2);
					break;
				case CASE_INSTR_MOVZX16RM:
					D(panicbug("MOVZX r32, m16"));
					size = 2;
					transfer_type = TYPE_LOAD;
					instruction = INSTR_MOVZX16;
					reg = (addr_instr[2] >> 3 ) & 7;
					len += 3 + get_instr_size_add(addr_instr + 2);
					break;
				case CASE_INSTR_MOVSX8RM:
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
				case 2: imm = addr_instr[6]; break;
				case 4: imm = addr_instr[3]; break;
				default:
					instruction = INSTR_UNKNOWN;
					panicbug("OR m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction(instr);
					abort();

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
			size = 1;
			transfer_type = TYPE_STORE;
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
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOVIMM8;
			reg = (addr_instr[1] >> 3) & 7;
			switch(addr_instr[1] & 0x07) {
				case 0: imm = addr_instr[2]; break;
				case 2: imm = addr_instr[2]; break;
				case 4: imm = addr_instr[3]; break;
				case 5: imm = addr_instr[6]; break; // used in JIT raw_mov_b_mi
				default:
					instruction = INSTR_UNKNOWN;
					panicbug("MOV m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction(instr);
					abort();

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
				case 0:
					D(panicbug("TEST m8, imm8"));
					transfer_type = TYPE_STORE;
					instruction = INSTR_TESTIMM8;
					imm = addr_instr[2];
					len += 3 + get_instr_size_add(addr_instr + 1);
					break;
				case 2:
					D(panicbug("NOT m8"));
					transfer_type = TYPE_STORE;
					instruction = INSTR_NOT8;
					len += 2 + get_instr_size_add(addr_instr + 1);
					break;
				case 3:
					D(panicbug("NEG m8"));
					transfer_type = TYPE_STORE;
					instruction = INSTR_NEG8;
					len += 2 + get_instr_size_add(addr_instr + 1);
					break;
#if 0
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
#else
				case 4:
					D(panicbug("MUL m8"));
					transfer_type = TYPE_LOAD;
					instruction = INSTR_MUL8;
					len += 2 + get_instr_size_add(addr_instr + 1);
					break;
				case 5:
					D(panicbug("IMUL m8"));
					transfer_type = TYPE_LOAD;
					instruction = INSTR_IMUL8;
					len += 2 + get_instr_size_add(addr_instr + 1);
					break;
#endif
				case 6:
					D(panicbug("DIV m8"));
					transfer_type = TYPE_LOAD;
					instruction = INSTR_DIV8;
					len += 2 + get_instr_size_add(addr_instr + 1);
					break;
				case 7:
					D(panicbug("IDIV m8"));
					transfer_type = TYPE_LOAD;
					instruction = INSTR_IDIV8;
					len += 2 + get_instr_size_add(addr_instr + 1);
					break;
				default:
					instruction = INSTR_UNKNOWN;
					panicbug("TEST m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction(instr);
					abort();
			}
			break;
		default:
			instruction = INSTR_UNKNOWN;
			unknown_instruction(instr);
			abort();
	}

	if (addr >= 0xff000000)
		addr &= 0x00ffffff;

	if ((addr < 0x00f00000) || (addr > 0x00ffffff))
		goto buserr;

	preg = get_preg(reg, CONTEXT_ADDR, size);

	D2(panicbug("Register %d, place %08x, address %08x", reg, preg, addr));

	if (transfer_type == TYPE_LOAD) {
		D2(panicbug("LOAD instruction %X", instruction));
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
				set_eflags(*((uae_u8 *)preg), CONTEXT_ADDR, TYPE_BYTE);
				break;
			case INSTR_AND8:
				*((uae_u8 *)preg) &= HWget_b(addr);
				imm = *((uae_u8 *)preg);
				set_eflags(*((uae_u8 *)preg), CONTEXT_ADDR, TYPE_BYTE);
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
				set_eflags(imm, CONTEXT_ADDR, TYPE_BYTE);
				break;
			case INSTR_DIV8:
				pom1 = CONTEXT_EAX & 0xffff;
				pom2 = HWget_b(addr);
				CONTEXT_EAX = CONTEXT_EAX & 0xffff0000 + ((pom1 / pom2) << 8) + (pom1 / pom2);
				break;
			case INSTR_IDIV8:
				pom1 = CONTEXT_EAX & 0xffff;
				pom2 = HWget_b(addr);
				CONTEXT_EAX = CONTEXT_EAX & 0xffff0000 + (((uae_s8)pom1 / (uae_s8)pom2) << 8) + ((uae_s8)pom1 / (uae_s8)pom2);
				break;
			case INSTR_MUL8:
				pom1 = CONTEXT_EAX & 0xff;
				pom2 = HWget_b(addr);
				CONTEXT_EAX = CONTEXT_EAX & 0xffff0000 + pom1 * pom2;
				if ((CONTEXT_EAX & 0xff00) == 0) CONTEXT_EFLAGS &= 0xfffffbfe;	// CF + OF
					else CONTEXT_EFLAGS |= 0x401;
				break;
			case INSTR_IMUL8:
				pom1 = CONTEXT_EAX & 0xff;
				pom2 = HWget_b(addr);
				CONTEXT_EAX = CONTEXT_EAX & 0xffff0000 + (uae_s8)pom1 * (uae_s8)pom2;
				if ((CONTEXT_EAX & 0xff00) == 0) CONTEXT_EFLAGS &= 0xfffffbfe;	// CF + OF
					else CONTEXT_EFLAGS |= 0x401;
				break;
			default: abort();
		}
	} else {
		D2(panicbug("WRITE instruction %X", instruction));
		switch (instruction) {
			case INSTR_MOV8:
				D2(panicbug("MOV value = $%x\n", *((uae_u8 *)preg)));
				HWput_b(addr, *((uae_u8 *)preg));
				break;
			case INSTR_MOV32:
				if (size == 4) {
				D2(panicbug("before 1"));
					HWput_l(addr, SDL_SwapBE32(*((uae_u32 *)preg)));
				D2(panicbug("after 1"));
				} else {
				D2(panicbug("before 2"));
					HWput_w(addr, SDL_SwapBE16(*((uae_u16 *)preg)));
				D2(panicbug("after 2"));
				}
				break;
			case INSTR_AND8:
				imm = HWget_b(addr);
				imm &= *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_eflags(imm, CONTEXT_ADDR, TYPE_BYTE);
				break;
			case INSTR_ADD8:
				imm = HWget_b(addr);
				imm += *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_eflags(imm, CONTEXT_ADDR, TYPE_BYTE);
				break;
			case INSTR_OR8:
				imm = HWget_b(addr);
				imm |= *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_eflags(imm, CONTEXT_ADDR, TYPE_BYTE);
				break;
			case INSTR_ORIMM8:
				imm |= HWget_b(addr);
				HWput_b(addr, imm);
				set_eflags(imm, CONTEXT_ADDR, TYPE_BYTE);
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
				set_eflags(imm, CONTEXT_ADDR, TYPE_BYTE);
				break;
			case INSTR_NOT8:
				HWput_b(addr, ~(uae_u8)HWget_b(addr));
				break;
			case INSTR_NEG8:
				imm = ~(uae_u8)HWget_b(addr) + 1;
				HWput_b(addr, imm);
				set_eflags(imm, CONTEXT_ADDR, TYPE_BYTE);
				if (imm == 0)
					CONTEXT_EFLAGS &= 0xfffffffe;
				else
					CONTEXT_EFLAGS |= 0x1;
				break;
			default: abort();
		}
	}

	D2(panicbug("Access handled"));
	D2(panicbug("Next instruction on %08x", CONTEXT_EIP + len));
	CONTEXT_EIP += len;

	in_handler -= 1;
	return;
buserr:
	D(panicbug("Atari bus error"));

#endif /* HW_SIGSEGV */

	BUS_ERROR(addr);
}

#ifdef NO_NESTED_SIGSEGV

JMP_BUF sigsegv_env;

static void
atari_bus_fault(void)
{
	breakpt();
	THROW(2);
}

static inline void handle_access_fault(CONTEXT_TYPE CONTEXT_NAME, memptr faultaddr) {
	if (SETJMP(sigsegv_env) != 0)
	{
		CONTEXT_EIP = (long unsigned int)atari_bus_fault;
		return;
	}

	handle_access_fault2(CONTEXT_NAME, faultaddr);
}
#endif /* NO_NESTED_SIGSEGV */

#endif /* (CPU_i386) */

/******************************************************************************/

#ifdef OS_linux

static void segfault_vec(int x, struct sigcontext sc) {
{
  handle_access_fault(&sc, sc->cr2);
}

void install_sigsegv() {
	signal(SIGSEGV, (sighandler_t)segfault_vec);
}

#endif /* OS_linux */

/******************************************************************************/

#ifdef OS_cygwin

static LONG WINAPI
main_exception_filter (EXCEPTION_POINTERS *ExceptionInfo)
{
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
  {
    handle_access_fault(ExceptionInfo->ContextRecord,
                        (memptr)(ExceptionInfo->ExceptionRecord->ExceptionInformation[1]));
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

/* In Cygwin programs, SetUnhandledExceptionFilter has no effect because Cygwin
   installs a global exception handler.  We have to dig deep in order to install
   our main_exception_filter.  */

/* Data structures for the current thread's exception handler chain.
   On the x86 Windows uses register fs, offset 0 to point to the current
   exception handler; Cygwin mucks with it, so we must do the same... :-/ */

/* Magic taken from winsup/cygwin/include/exceptions.h.  */

struct exception_list
  {
    struct exception_list *prev;
    int (*handler) (EXCEPTION_RECORD *, void *, CONTEXT *, void *);
  };
typedef struct exception_list exception_list;

/* Magic taken from winsup/cygwin/exceptions.cc.  */

__asm__ (".equ __except_list,0");

extern exception_list *_except_list __asm__ ("%fs:__except_list");

/* For debugging.  _except_list is not otherwise accessible from gdb.  */
static exception_list *
debug_get_except_list ()
{
  return _except_list;
}

/* Cygwin's original exception handler.  */
static int (*cygwin_exception_handler) (EXCEPTION_RECORD *, void *, CONTEXT *, void *);

/* Our exception handler.  */
static int
aranym_exception_handler (EXCEPTION_RECORD *exception, void *frame, CONTEXT *context, void *dispatch)
{
  EXCEPTION_POINTERS ExceptionInfo;
  ExceptionInfo.ExceptionRecord = exception;
  ExceptionInfo.ContextRecord = context;
  if (main_exception_filter (&ExceptionInfo) == EXCEPTION_CONTINUE_SEARCH)
    return cygwin_exception_handler (exception, frame, context, dispatch);
  else
    return 0;
}

static void
do_install_main_exception_filter ()
{
  /* We cannot insert any handler into the chain, because such handlers
     must lie on the stack (?).  Instead, we have to replace(!) Cygwin's
     global exception handler.  */
  cygwin_exception_handler = _except_list->handler;
  _except_list->handler = aranym_exception_handler;
}

static void
install_main_exception_filter ()
{
  static int main_exception_filter_installed = 0;

  if (!main_exception_filter_installed)
    {
      do_install_main_exception_filter ();
      main_exception_filter_installed = 1;
    }
}

void
install_sigsegv ()
{
  install_main_exception_filter ();
}

#endif /* OS_cygwin */
