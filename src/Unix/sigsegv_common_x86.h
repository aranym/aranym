#include "disasm-glue.h"

/* for debug output only */
#ifdef CPU_i386
#define REG_RIP_NAME "eip"
#endif
#ifdef CPU_x86_64
#define REG_RIP_NAME "rip"
#endif

enum transfer_type_t {
	TYPE_UNKNOWN,
	TYPE_LOAD,
	TYPE_STORE
};

#ifdef USE_JIT
extern void compiler_status();
# ifdef JIT_DEBUG
extern void compiler_dumpstate();
# endif
#endif

int in_handler = 0;
// #define HW_SIGSEGV_STATISTICS 1
#ifdef HW_SIGSEGV_STATISTICS
static unsigned long x86_opcodes[256];
#endif

enum instruction_t {
	INSTR_UNKNOWN,
	INSTR_MOVZX8,
	INSTR_MOVZX16,
	INSTR_MOVSX8,
	INSTR_MOVSX16,
#ifdef CPU_x86_64
	INSTR_MOVSX32,
#endif
	INSTR_MOV8,
	INSTR_MOV32,
	INSTR_MOVIMM8,
	INSTR_MOVIMM32,
	INSTR_ORMEM,
	INSTR_ADDIMM8,
	INSTR_ORIMM8,
	INSTR_ANDIMM8,
	INSTR_SUBIMM8,
	INSTR_XORIMM8,
	INSTR_CMPIMM8,
	INSTR_ADDIMM32,
	INSTR_ORIMM32,
	INSTR_ANDIMM32,
	INSTR_SUBIMM32,
	INSTR_XORIMM32,
	INSTR_CMPIMM32,
	INSTR_ADDLIMM8,
	INSTR_ORLIMM8,
	INSTR_ANDLIMM8,
	INSTR_SUBLIMM8,
	INSTR_XORLIMM8,
	INSTR_CMPLIMM8,
	INSTR_ANDMEM,
	INSTR_ADDMEM,
	INSTR_SUBMEM,
	INSTR_CMPMEM,
	INSTR_DIV8,
	INSTR_IDIV8,
	INSTR_MUL8,
	INSTR_IMUL8,
	INSTR_NEGMEM,
	INSTR_NOTMEM,
	INSTR_TESTIMM8,
	INSTR_XORMEM
};


static inline int get_instr_size_add(const uint8 *p)
{
	int mod = (p[0] & 0xC0);
	int rm = p[0] & 7;
	int offset = 0;

	// ModR/M Byte
	switch (mod) {
	case 0: // [reg]
		if (rm == 5) return 4; // disp32
		break;
	case 0x40: // disp8[reg]
		offset = 1;
		break;
	case 0x80: // disp32[reg]
		offset = 4;
		break;
	case 0xc0: // register
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

/* MJ - AF and OF not tested, also CF for 32 bit */
static inline void set_byte_eflags(int i, CONTEXT_ATYPE CONTEXT_NAME) {
	if ((i > 255) || (i < 0)) CONTEXT_AEFLAGS |= 0x1;	// CF
	else CONTEXT_AEFLAGS &= ~0x1;
	
	if (i > 127) CONTEXT_AEFLAGS |= 0x80;			// SF
	else CONTEXT_AEFLAGS &= ~0x80;

	if ((i % 2) == 0) CONTEXT_AEFLAGS |= 0x4;				// PF
	else CONTEXT_AEFLAGS &= ~0x4;
	
	if (i == 0) CONTEXT_AEFLAGS |= 0x40;					// ZF
	else CONTEXT_AEFLAGS &= ~0x40;
}

/* MJ - AF and OF not tested, also CF for 32 bit */
static inline void set_word_eflags(int i, CONTEXT_ATYPE CONTEXT_NAME) {
	if ((i > 65535) || (i < 0)) CONTEXT_AEFLAGS |= 0x1;	// CF
	else CONTEXT_AEFLAGS &= ~0x1;
	
	if (i > 32767) CONTEXT_AEFLAGS |= 0x80;			// SF
	else CONTEXT_AEFLAGS &= ~0x80;
	
	if ((i % 2) == 0) CONTEXT_AEFLAGS |= 0x4;				// PF
	else CONTEXT_AEFLAGS &= ~0x4;
	
	if (i == 0) CONTEXT_AEFLAGS |= 0x40;					// ZF
	else CONTEXT_AEFLAGS &= ~0x40;
}

/* MJ - AF and OF not tested, also CF for 32 bit */
static inline void set_long_eflags(int i, CONTEXT_ATYPE CONTEXT_NAME) {
	if (i > 2147483647) CONTEXT_AEFLAGS |= 0x80;		// SF
	else CONTEXT_AEFLAGS &= ~0x80;
	
	if ((i % 2) == 0) CONTEXT_AEFLAGS |= 0x4;				// PF
	else CONTEXT_AEFLAGS &= ~0x4;
	
	if (i == 0) CONTEXT_AEFLAGS |= 0x40;					// ZF
	else CONTEXT_AEFLAGS &= ~0x40;
}

static
#if DEBUG
__attribute_noinline__
#else
inline
#endif
void unknown_instruction() {
#ifdef USE_JIT
	compiler_status();
# ifdef JIT_DEBUG
	compiler_dumpstate();
# endif
#endif
	abort();
}

/**
	Opcode register id list:
 
	8 Bit
		0: AL
		1: CL
		2: DL
		3: BL
		4: AH (SPL, if REX)
		5: CH (BPL, if REX)
		6: DH (SIL, if REX)
		7: BH (DIL, if REX)
		8: R8L
		9: R9L
	   10: R10L
	   11: R11L
	   12: R12L
	   13: R13L
	   14: R14L
	   15: R15L
 
	16 Bit:
		0: AX
		1: CX
		2: DX
		3: BX
		4: SP
		5: BP
		6: SI
		7: DI
		8: R8W
		9: R9W
	   10: R10W
	   11: R11W
	   12: R12W
	   13: R13W
	   14: R14W
	   15: R15W
 
	32 Bit:
		0: EAX
		1: ECX
		2: EDX
		3: EBX
		4: ESP
		5: EBP
		6: ESI
		7: EDI
		8: R8D
		9: R9D
	   10: R10D
	   11: R11D
	   12: R12D
	   13: R13D
	   14: R14D
	   15: R15D
 
 **/

#ifdef NO_NESTED_SIGSEGV
static __attribute_noinline__ void handle_access_fault2(CONTEXT_ATYPE CONTEXT_NAME, memptr faultaddr)
#else
static __attribute_noinline__ void handle_access_fault(CONTEXT_ATYPE CONTEXT_NAME, memptr faultaddr)
#endif
{
	memptr addr = faultaddr;
	const uint8 *ainstr = (const uint8 *)CONTEXT_AEIP;
	const uint8 *addr_instr = ainstr;
	int reg = -1;
	int len = 0;
	transfer_type_t transfer_type = TYPE_UNKNOWN;
	int size = 4;
	uae_u32 imm = 0;
	int pom1, pom2;
	instruction_t instruction = INSTR_UNKNOWN;
	volatile void *preg;

static const int x86_reg_map[] = {
		REG_RAX, REG_RCX, REG_RDX, REG_RBX,
		REG_RSP, REG_RBP, REG_RSI, REG_RDI,
#ifdef CPU_x86_64
		REG_R8,  REG_R9,  REG_R10, REG_R11,
		REG_R12, REG_R13, REG_R14, REG_R15,
#endif
	};

	if (in_handler > 0) {
		panicbug("Segmentation fault in handler :-(, faultaddr=0x%08x", faultaddr);
		abort();
	}
	in_handler += 1;

#ifdef USE_JIT	/* does not compile with default configure */
	D(compiler_status());
#endif
	D(bug("[%08x] BUS ERROR fault address is %08x", m68k_getpc(), addr));

#ifdef HW_SIGSEGV

	/* segment override not handled */
#if DEBUG
	if (addr_instr[0] == 0x2e ||
		addr_instr[0] == 0x3e ||
		addr_instr[0] == 0x26 ||
		addr_instr[0] == 0x64 ||
		addr_instr[0] == 0x65 ||
		addr_instr[0] == 0x36)
	{
		D(panicbug("segment override prefix"));
		addr_instr++;
		len++;
	}
#endif

	if (addr_instr[0] == 0x67) { // address size override prefix
#ifdef CPU_i386
		D(panicbug("address size override prefix in 32bit-mode"));
#endif
		addr_instr++; // Skip prefix, seems to be enough
		len++;
	}

	if (addr_instr[0] == 0x66) { // operand size override prefix
		addr_instr++;
		len++;
		size = 2;
		D(bug("Word instr:"));
	}

	if (addr_instr[0] == 0xf0) { // lock prefix
		addr_instr++;
		len++;
		D(bug("Lock:"));
	}

	/* Repeat prefix not handled */
	if (addr_instr[0] == 0xf3 ||
		addr_instr[0] == 0xf2)
	{
		panicbug("repeat prefix");
		addr_instr++;
		len++;
	}

#ifdef CPU_x86_64
	// REX prefix
	struct rex_t {
		unsigned char W;
		unsigned char R;
		unsigned char X;
		unsigned char B;
	};
	rex_t rex = { 0, 0, 0, 0 };
	bool has_rex = false;
	if ((*addr_instr & 0xf0) == 0x40) {
		has_rex = true;
		const unsigned char b = *addr_instr;
		rex.W = b & (1 << 3);
		rex.R = b & (1 << 2);
		rex.X = b & (1 << 1);
		rex.B = b & (1 << 0);
		addr_instr++;
		len++;
		if (rex.W)
			size = 8;
	}
#endif

#ifdef HW_SIGSEGV_STATISTICS
	x86_opcodes[addr_instr[0]]++;
#endif

	switch (addr_instr[0]) {
		case 0x00:
			D(bug("ADD m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_ADDMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x01:
			if (size == 8)
			{
				D(bug("ADD m64, r64"));
			} else if (size == 2)
			{
				D(bug("ADD m16, r16"));
			} else /* if (size == 4) */
			{
				D(bug("ADD m32, r32"));
			}
			instruction = INSTR_ADDMEM;
			transfer_type = TYPE_STORE;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x02:
			D(bug("ADD r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_ADDMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x03:
			if (size == 8)
			{
				D(bug("ADD r64, m64"));
			} else if (size == 2)
			{
				D(bug("ADD r16, m16"));
			} else if (size == 4)
			{
				D(bug("ADD r32, m32"));
			}
			instruction = INSTR_ADDMEM;
			transfer_type = TYPE_LOAD;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x28:
			D(bug("SUB m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_SUBMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x29:
			if (size == 8)
			{
				D(bug("SUB m64, r64"));
			} else if (size == 2)
			{
				D(bug("SUB m16, r16"));
			} else /* if (size == 4) */
			{
				D(bug("SUB m32, r32"));
			}
			instruction = INSTR_SUBMEM;
			transfer_type = TYPE_STORE;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x2A:
			D(bug("SUB r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_SUBMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x2b:
			if (size == 8)
			{
				D(bug("SUB r64, m64"));
			} else if (size == 2)
			{
				D(bug("SUB r16, m16"));
			} else if (size == 4)
			{
				D(bug("SUB r32, m32"));
			}
			instruction = INSTR_SUBMEM;
			transfer_type = TYPE_LOAD;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x08:
			D(bug("OR m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_ORMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x09:
			if (size == 8)
			{
				D(bug("OR m64, r64"));
			} else if (size == 2)
			{
				D(bug("OR m16, r16"));
			} else /* if (size == 4) */
			{
				D(bug("OR m32, r32"));
			}
			instruction = INSTR_ORMEM;
			transfer_type = TYPE_STORE;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x0a:
			D(bug("OR r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_ORMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x0b:
			if (size == 8)
			{
				D(bug("OR r64, m64"));
			} else if (size == 2)
			{
				D(bug("OR r16, m16"));
			} else if (size == 4)
			{
				D(bug("OR r32, m32"));
			}
			instruction = INSTR_ORMEM;
			transfer_type = TYPE_LOAD;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x0f:
			switch (addr_instr[1])
			{
			case 0xb6:
				D(bug("MOVZX r32, m8"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_MOVZX8;
				reg = (addr_instr[2] >> 3 ) & 7;
				len += 3 + get_instr_size_add(addr_instr + 2);
				break;
			case 0xb7:
				D(bug("MOVZX r32, m16"));
				size = 2;
				transfer_type = TYPE_LOAD;
				instruction = INSTR_MOVZX16;
				reg = (addr_instr[2] >> 3 ) & 7;
				len += 3 + get_instr_size_add(addr_instr + 2);
				break;
			case 0xbe:
				D(bug("MOVSX r32, m8"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_MOVSX8;
				reg = (addr_instr[2] >> 3 ) & 7;
				len += 3 + get_instr_size_add(addr_instr + 2);
				break;
			case 0xbf:
				D(bug("MOVSX r32, m16"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_MOVSX16;
				reg = (addr_instr[2] >> 3 ) & 7;
				len += 3 + get_instr_size_add(addr_instr + 2);
				break;
#ifdef CPU_x86_64
			case 0x63:
				D(bug("MOVSX r64, m32"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_MOVSX32;
				reg = (addr_instr[2] >> 3 ) & 7;
				len += 3 + get_instr_size_add(addr_instr + 2);
				break;
#endif
			default:
				instruction = INSTR_UNKNOWN;
				panicbug("MOVSX - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
				unknown_instruction();
				break;
			}
			break;
		case 0x20:
			D(bug("AND m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_ANDMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x21:
			if (size == 8)
			{
				D(bug("AND m64, r64"));
			} else if (size == 2)
			{
				D(bug("AND m16, r16"));
			} else /* if (size == 4) */
			{
				D(bug("AND m32, r32"));
			}
			instruction = INSTR_ANDMEM;
			transfer_type = TYPE_STORE;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x22:
			D(bug("AND r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_ANDMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x23:
			if (size == 8)
			{
				D(bug("AND r64, m64"));
			} else if (size == 2)
			{
				D(bug("AND r16, m16"));
			} else /* if (size == 4) */
			{
				D(bug("AND r32, m32"));
			}
			instruction = INSTR_ANDMEM;
			transfer_type = TYPE_STORE;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x30:
			D(bug("XOR m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_XORMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x31:
			if (size == 8)
			{
				D(bug("XOR m64, r64"));
			} else if (size == 2)
			{
				D(bug("XOR m16, r16"));
			} else /* if (size == 4) */
			{
				D(bug("XOR m32, r32"));
			}
			instruction = INSTR_XORMEM;
			transfer_type = TYPE_STORE;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x32:
			D(bug("XOR r8,m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_XORMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x33:
			if (size == 8)
			{
				D(bug("XOR r64, m64"));
			} else if (size == 2)
			{
				D(bug("XOR r16, m16"));
			} else /* if (size == 4) */
			{
				D(bug("XOR r32, m32"));
			}
			instruction = INSTR_XORMEM;
			transfer_type = TYPE_LOAD;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x38:
			D(bug("CMP m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_CMPMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x39:
			if (size == 8)
			{
				D(bug("CMP m64, r64"));
			} else if (size == 2)
			{
				D(bug("CMP m16, r16"));
			} else /* if (size == 4) */
			{
				D(bug("CMP m32, r32"));
			}
			transfer_type = TYPE_STORE;
			instruction = INSTR_CMPMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x3a:
			D(bug("CMP r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_CMPMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x3b:
			if (size == 8)
			{
				D(bug("CMP r64, m64"));
			} else if (size == 2)
			{
				D(bug("CMP r16, m16"));
			} else /* if (size == 4) */
			{
				D(bug("CMP r32, m32"));
			}
			transfer_type = TYPE_LOAD;
			instruction = INSTR_CMPMEM;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x80:
			size = 1;
			transfer_type = TYPE_STORE;
			switch ((addr_instr[1] >> 3) & 7)
			{
			case 0:
				D(bug("ADD m8, imm8"));
				instruction = INSTR_ADDIMM8;
				break;
			case 1:
				D(bug("OR m8, imm8"));
				instruction = INSTR_ORIMM8;
				break;
			case 4:
				D(bug("AND m8, imm8"));
				instruction = INSTR_ANDIMM8;
				break;
			case 5:
				D(bug("SUB m8, imm8"));
				instruction = INSTR_SUBIMM8;
				break;
			case 6:
				D(bug("XOR m8, imm8"));
				instruction = INSTR_XORIMM8;
				break;
			case 7:
				D(bug("CMP m8, imm8"));
				instruction = INSTR_CMPIMM8;
				break;
			
			case 2: /* ADC */
			case 3: /* SBB */
			default:
				instruction = INSTR_UNKNOWN;
				panicbug("ADD m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
				unknown_instruction();
				break;
			}
			len += 2 + get_instr_size_add(addr_instr + 1);
			imm = *((uae_u8 *)(ainstr + len));
			len++;
			break;
		case 0x81:
			transfer_type = TYPE_STORE;
			switch ((addr_instr[1] >> 3) & 7)
			{
			case 0:
				D(bug("ADD m32, imm32"));
				instruction = INSTR_ADDIMM32;
				break;
			case 1:
				D(bug("OR m32, imm32"));
				instruction = INSTR_ORIMM32;
				break;
			case 4:
				D(bug("AND m32, imm32"));
				instruction = INSTR_ANDIMM32;
				break;
			case 5:
				D(bug("SUB m32, imm32"));
				instruction = INSTR_SUBIMM32;
				break;
			case 6:
				D(bug("XOR m32, imm32"));
				instruction = INSTR_XORIMM32;
				break;
			case 7:
				D(bug("CMP m32, imm32"));
				instruction = INSTR_CMPIMM32;
				break;
			
			case 2: /* ADC */
			case 3: /* SBB */
			default:
				instruction = INSTR_UNKNOWN;
				panicbug("ADD m32, imm32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
				unknown_instruction();
				break;
			}
			len += 2 + get_instr_size_add(addr_instr + 1);
			if (size == 2)
			{
				imm = *((uae_u16 *)(ainstr + len));
				len += 2;
			} else
			{
				imm = *((uae_u32 *)(ainstr + len));
				len += 4;
			}
			break;
		case 0x83:
			transfer_type = TYPE_STORE;
			switch ((addr_instr[1] >> 3) & 7)
			{
			case 0:
				D(bug("ADDL m32, imm8"));
				instruction = INSTR_ADDLIMM8;
				break;
			case 1:
				D(bug("ORL m32, imm8"));
				instruction = INSTR_ORLIMM8;
				break;
			case 4:
				D(bug("ANDL m32, imm8"));
				instruction = INSTR_ANDLIMM8;
				break;
			case 5:
				D(bug("SUBL m32, imm8"));
				instruction = INSTR_SUBLIMM8;
				break;
			case 6:
				D(bug("XORL m32, imm8"));
				instruction = INSTR_XORLIMM8;
				break;
			case 7:
				D(bug("CMPL m32, imm8"));
				instruction = INSTR_CMPLIMM8;
				break;
			
			case 2: /* ADC */
			case 3: /* SBB */
			default:
				instruction = INSTR_UNKNOWN;
				panicbug("ADD m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
				unknown_instruction();
				break;
			}
			len += 2 + get_instr_size_add(addr_instr + 1);
			imm = *((uae_s8 *)(ainstr + len));
			len++;
			break;
		case 0x88:
			D(bug("MOV m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOV8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x89:
			D(bug("MOV m32, r32"));
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOV32;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x8a:
			D(bug("MOV r8, m8"));
			size = 1;
			transfer_type = TYPE_LOAD;
			instruction = INSTR_MOV8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0x8b:
			D(bug("MOV r32, m32"));
			transfer_type = TYPE_LOAD;
			instruction = INSTR_MOV32;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			break;
		case 0xc6:
			D(bug("MOV m8, imm8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOVIMM8;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			imm = *((uae_u8 *)(ainstr + len));
			len++;
			break;
		case 0xc7:
			D(bug("MOV m32, imm32"));
			transfer_type = TYPE_STORE;
			instruction = INSTR_MOVIMM32;
			reg = (addr_instr[1] >> 3) & 7;
			len += 2 + get_instr_size_add(addr_instr + 1);
			if (size == 2)
			{
				imm = *((uae_u16 *)(ainstr + len));
				len += 2;
			} else
			{
				imm = *((uae_u32 *)(ainstr + len));
				len += 4;
			}
			break;
		case 0xf6:
			size = 1;
			/* fall through */
		case 0xf7:
			len += 2 + get_instr_size_add(addr_instr + 1);
			switch ((addr_instr[1] >> 3) & 7)
			{
			case 0:
				D(bug("TEST m8, imm8"));
				transfer_type = TYPE_STORE;
				instruction = INSTR_TESTIMM8;
				imm = *((uae_u8 *)(ainstr + len));
				len++;
				break;
			case 2:
				D(bug("NOT m8"));
				transfer_type = TYPE_STORE;
				instruction = INSTR_NOTMEM;
				break;
			case 3:
				D(bug("NEG m8"));
				transfer_type = TYPE_STORE;
				instruction = INSTR_NEGMEM;
				break;
			case 4:
				D(bug("MUL m8"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_MUL8;
				reg = 0; /* always EAX */
				break;
			case 5:
				D(bug("IMUL m8"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_IMUL8;
				reg = 0; /* always EAX */
				break;
			case 6:
				D(bug("DIV m8"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_DIV8;
				reg = 0; /* always EAX */
				break;
			case 7:
				D(bug("IDIV m8"));
				transfer_type = TYPE_LOAD;
				instruction = INSTR_IDIV8;
				reg = 0; /* always EAX */
				break;
			default:
				instruction = INSTR_UNKNOWN;
				panicbug("TEST m8, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
				unknown_instruction();
				break;
			}
			break;
		default:
			instruction = INSTR_UNKNOWN;
			panicbug("unsupported instruction: i[0-6]=%02x %02x %02x %02x %02x %02x %02x", addr_instr[0], addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
			unknown_instruction();
			break;
	}

#if DEBUG
	{
		const uint8 *a = ainstr;
		int i;
		char scratch[1024];
		
		strcpy(scratch, "instruction is");
		for (i = 0; i < len && i < 7; i++)
			sprintf(scratch + strlen(scratch), " %02x", a[i]);
		if (i > 7)
			strcat(scratch, "...");
		sprintf(scratch + strlen(scratch), ", len %d", len);
		D(bug("%s", scratch));
	}
#ifdef HAVE_DISASM_X86
	{
		char buf[256];
		
		x86_disasm(ainstr, buf, 1);
		bug("%s", buf);
	}
#endif
#endif

	if (addr >= 0xff000000)
		addr &= 0x00ffffff;

	if ((addr < 0x00f00000) || (addr > 0x00ffffff))
		goto buserr;

#ifdef CPU_x86_64
	if (rex.R) {
		reg += 8;
		D2(bug("Extended register %d", reg));
	}
#endif

	if (reg >= 0)
	{
		// Get register pointer
		if (size == 1) {
			if (
#ifdef CPU_x86_64
				has_rex || 
#endif
				reg < 4)
			{
				preg = &(CONTEXT_REGS[x86_reg_map[reg]]);
			} else {
				preg = (uae_u8*)(&(CONTEXT_REGS[x86_reg_map[reg - 4]])) + 1; // AH, BH, CH, DH
			}
		} else {
			preg = &(CONTEXT_REGS[x86_reg_map[reg]]);
		}
	} else
	{
		preg = NULL;
	}
	
	D2(bug("Register %d, place %08x, address %08x", reg, (unsigned int)((const char *)preg - (const char *)CONTEXT_REGS), addr));

	if (transfer_type == TYPE_LOAD) {
		D(bug("LOAD instruction %X", instruction));
		switch (instruction) {
			case INSTR_MOVZX16:
#ifdef CPU_x86_64
				if (rex.W)
					*((uae_u64 *)preg) = (uae_u16)SDL_SwapBE16(HWget_w(addr));
				else
#endif
					*((uae_u32 *)preg) = (uae_u16)SDL_SwapBE16(HWget_w(addr));
				break;
			case INSTR_MOV8:
				*((uae_u8 *)preg) = HWget_b(addr);
				break;
			case INSTR_MOV32:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("MOV r64, m32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
					*((uae_u32 *)preg) = SDL_SwapBE32(HWget_l(addr));
				else
					*((uae_u16 *)preg) = SDL_SwapBE16(HWget_w(addr));
				break;
			case INSTR_ORMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("OR r64, m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					*((uae_u8 *)preg) |= HWget_b(addr);
					set_byte_eflags(*((uae_u8 *)preg), CONTEXT_NAME);
				} else if (size == 2)
				{
					*((uae_u16 *)preg) |= SDL_SwapBE16(HWget_w(addr));
					set_word_eflags(*((uae_u16 *)preg), CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					*((uae_u32 *)preg) |= SDL_SwapBE32(HWget_l(addr));
					set_long_eflags(*((uae_u32 *)preg), CONTEXT_NAME);
				}
				break;
			case INSTR_ANDMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("AND r64, m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					*((uae_u8 *)preg) &= HWget_b(addr);
					set_byte_eflags(*((uae_u8 *)preg), CONTEXT_NAME);
				} else if (size == 2)
				{
					*((uae_u16 *)preg) &= SDL_SwapBE16(HWget_w(addr));
					set_word_eflags(*((uae_u16 *)preg), CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					*((uae_u32 *)preg) &= SDL_SwapBE32(HWget_l(addr));
					set_long_eflags(*((uae_u32 *)preg), CONTEXT_NAME);
				}
				break;
			case INSTR_XORMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("XOR r64, m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					*((uae_u8 *)preg) ^= HWget_b(addr);
					set_byte_eflags(*((uae_u8 *)preg), CONTEXT_NAME);
				} else if (size == 2)
				{
					*((uae_u16 *)preg) ^= SDL_SwapBE16(HWget_w(addr));
					set_word_eflags(*((uae_u16 *)preg), CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					*((uae_u32 *)preg) ^= SDL_SwapBE32(HWget_l(addr));
					set_long_eflags(*((uae_u32 *)preg), CONTEXT_NAME);
				}
				break;
			case INSTR_MOVZX8:
#ifdef CPU_x86_64
				if (rex.W)
					*((uae_u64 *)preg) = (uae_u8)HWget_b(addr);
				else
#endif
				if (size == 4)
					*((uae_u32 *)preg) = (uae_u8)HWget_b(addr);
				else
					*((uae_u16 *)preg) = (uae_u8)HWget_b(addr);
				break;
			case INSTR_MOVSX8:
#ifdef CPU_x86_64
				if (rex.W)
					*((uae_s64 *)preg) = (uae_s8)HWget_b(addr);
				else
#endif
				if (size == 4)
					*((uae_s32 *)preg) = (uae_s8)HWget_b(addr);
				else
					*((uae_s16 *)preg) = (uae_s8)HWget_b(addr);
				break;
			case INSTR_MOVSX16:
#ifdef CPU_x86_64
				if (rex.W)
					*((uae_s64 *)preg) = (uae_s16)SDL_SwapBE16(HWget_w(addr));
				else
#endif
					*((uae_s32 *)preg) = (uae_s16)SDL_SwapBE16(HWget_w(addr));
				break;
#ifdef CPU_x86_64
			case INSTR_MOVSX32:
				*((uae_s64 *)preg) = (uae_s32)SDL_SwapBE32(HWget_l(addr));
				break;
#endif
			case INSTR_ADDMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("ADD r64, m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					*((uae_u8 *)preg) += HWget_b(addr);
					set_byte_eflags(*((uae_u8 *)preg), CONTEXT_NAME);
				} else if (size == 2)
				{
					*((uae_u16 *)preg) += SDL_SwapBE16(HWget_w(addr));
					set_byte_eflags(*((uae_u16 *)preg), CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					*((uae_u32 *)preg) += SDL_SwapBE32(HWget_l(addr));
					set_byte_eflags(*((uae_u32 *)preg), CONTEXT_NAME);
				}
				break;
			case INSTR_SUBMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("SUB r64, m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					*((uae_u8 *)preg) -= HWget_b(addr);
					set_byte_eflags(*((uae_u8 *)preg), CONTEXT_NAME);
				} else if (size == 2)
				{
					*((uae_u16 *)preg) -= SDL_SwapBE16(HWget_w(addr));
					set_word_eflags(*((uae_u16 *)preg), CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					*((uae_u32 *)preg) -= SDL_SwapBE32(HWget_l(addr));
					set_long_eflags(*((uae_u32 *)preg), CONTEXT_NAME);
				}
				break;
			case INSTR_CMPMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("CMP r64, m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					imm = *((uae_u8 *)preg);
					imm -= HWget_b(addr);
					set_byte_eflags(imm, CONTEXT_NAME);
				} else if (size == 2)
				{
					imm = *((uae_u16 *)preg);
					imm -= SDL_SwapBE16(HWget_w(addr));
					set_word_eflags(imm, CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					imm = *((uae_u32 *)preg);
					imm -= SDL_SwapBE32(HWget_l(addr));
					set_long_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_DIV8:
				pom1 = CONTEXT_AEAX & 0xffffUL;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = (CONTEXT_AEAX & ~0xffffUL) + ((pom1 % pom2) << 8) + (pom1 / pom2);
				break;
			case INSTR_IDIV8:
				pom1 = CONTEXT_AEAX & 0xffffUL;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = (CONTEXT_AEAX & ~0xffffUL) + (((uae_s8)pom1 % (uae_s8)pom2) << 8) + ((uae_s8)pom1 / (uae_s8)pom2);
				break;
			case INSTR_MUL8:
				pom1 = CONTEXT_AEAX & 0xff;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = (CONTEXT_AEAX & ~0xffffUL) + pom1 * pom2;
				if ((CONTEXT_AEAX & 0xff00) == 0)
					CONTEXT_AEFLAGS &= ~0x401;	// CF + OF
				else
					CONTEXT_AEFLAGS |= 0x401;
				break;
			case INSTR_IMUL8:
				pom1 = CONTEXT_AEAX & 0xff;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = (CONTEXT_AEAX & ~0xffffUL) + (uae_s8)pom1 * (uae_s8)pom2;
				if ((CONTEXT_AEAX & 0xff00) == 0)
					CONTEXT_AEFLAGS &= ~0x401;	// CF + OF
				else
					CONTEXT_AEFLAGS |= 0x401;
				break;
			default:
				D2(panicbug("Unknown load instruction %X", instruction));
				abort();
		}
	} else {
		D(bug("WRITE instruction %X", instruction));
		switch (instruction) {
			case INSTR_MOV8:
				D2(bug("MOV value = $%x", *((uae_u8 *)preg)));
				HWput_b(addr, *((uae_u8 *)preg));
				break;
			case INSTR_MOV32:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("MOV m32, r64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
					HWput_l(addr, SDL_SwapBE32(*((uae_u32 *)preg)));
				else
					HWput_w(addr, SDL_SwapBE16(*((uae_u16 *)preg)));
				break;
			case INSTR_ANDMEM:
				imm = HWget_b(addr);
				imm &= *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_ADDMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWput_q */
					panicbug("ADD m64, r64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					imm = HWget_b(addr);
					imm += *((uae_u8 *)preg);
					HWput_b(addr, imm);
					set_byte_eflags(imm, CONTEXT_NAME);
				} else if (size == 2)
				{
					imm = SDL_SwapBE16(HWget_w(addr));
					imm += *((uae_u16 *)preg);
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(SDL_SwapBE16(imm), CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					imm = SDL_SwapBE32(HWget_l(addr));
					imm += *((uae_u32 *)preg);
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_SUBMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWput_q */
					panicbug("SUB m64, r64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					imm = HWget_b(addr);
					imm -= *((uae_u8 *)preg);
					HWput_b(addr, imm);
					set_byte_eflags(imm, CONTEXT_NAME);
				} else if (size == 2)
				{
					imm = SDL_SwapBE16(HWget_w(addr));
					imm -= *((uae_u16 *)preg);
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					imm = SDL_SwapBE32(HWget_l(addr));
					imm -= *((uae_u32 *)preg);
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_CMPMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWget_q */
					panicbug("CMP m64, r64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					imm = HWget_b(addr);
					imm -= *((uae_u8 *)preg);
					set_byte_eflags(imm, CONTEXT_NAME);
				} else if (size == 2)
				{
					imm = SDL_SwapBE16(HWget_w(addr));
					imm -= *((uae_u16 *)preg);
					set_word_eflags(imm, CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					imm = SDL_SwapBE32(HWget_l(addr));
					imm -= *((uae_u32 *)preg);
					set_long_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_XORMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWput_q */
					panicbug("XOR m64, r64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					imm = HWget_b(addr);
					imm ^= *((uae_u8 *)preg);
					HWput_b(addr, imm);
					set_byte_eflags(imm, CONTEXT_NAME);
				} else if (size == 2)
				{
					imm = SDL_SwapBE16(HWget_w(addr));
					imm ^= *((uae_u16 *)preg);
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					imm = SDL_SwapBE32(HWget_l(addr));
					imm ^= *((uae_u32 *)preg);
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_ORMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWput_q */
					panicbug("OR m64, r64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					imm = HWget_b(addr);
					imm |= *((uae_u8 *)preg);
					HWput_b(addr, imm);
					set_byte_eflags(imm, CONTEXT_NAME);
				} else if (size == 2)
				{
					imm = SDL_SwapBE16(HWget_w(addr));
					imm |= *((uae_u16 *)preg);
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					imm = SDL_SwapBE32(HWget_l(addr));
					imm |= *((uae_u32 *)preg);
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_ADDIMM8:
				imm += HWget_b(addr);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_ORIMM8:
				imm |= HWget_b(addr);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_ANDIMM8:
				imm &= HWget_b(addr);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_SUBIMM8:
 				imm = HWget_b(addr) - imm;
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_XORIMM8:
				imm ^= HWget_b(addr);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_CMPIMM8:
				imm = HWget_b(addr) - imm;
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_ADDIMM32:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("ADD m64, imm32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm += SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm += SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_ORIMM32:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("OR m64, imm32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm |= SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm |= SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_ANDIMM32:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("AND m64, imm32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm &= SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm &= SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_SUBIMM32:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("SUB m64, imm32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm = SDL_SwapBE32(HWget_l(addr)) - imm;
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm = SDL_SwapBE16(HWget_w(addr)) - imm;
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_XORIMM32:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("XOR m64, imm32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm ^= SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm ^= SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_CMPIMM32:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("CMP m64, imm32 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm = SDL_SwapBE32(HWget_l(addr)) - imm;
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm = SDL_SwapBE16(HWget_w(addr)) - imm;
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_ADDLIMM8:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("ADDL m64, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm += (uae_s32)SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm += (uae_s16)SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_ORLIMM8:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("ORL m64, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm |= (uae_s32)SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm |= (uae_s16)SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_ANDLIMM8:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("ANDL m64, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm &= (uae_s32)SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm &= (uae_s16)SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_SUBLIMM8:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("SUBL m64, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm = (uae_s32)SDL_SwapBE32(HWget_l(addr)) - imm;
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm = (uae_s16)SDL_SwapBE16(HWget_w(addr)) - imm;
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_XORLIMM8:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("ANDL m64, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm ^= (uae_s32)SDL_SwapBE32(HWget_l(addr));
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm ^= (uae_s16)SDL_SwapBE16(HWget_w(addr));
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_CMPLIMM8:
#ifdef CPU_x86_64
				if (rex.W)
				{
					/* TODO: needs HWput_q */
					panicbug("CMP m64, imm8 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 4)
				{
					imm = (uae_s32)SDL_SwapBE32(HWget_l(addr)) - imm;
					set_long_eflags(imm, CONTEXT_NAME);
				} else
				{
					imm = (uae_s16)SDL_SwapBE16(HWget_w(addr)) - imm;
					set_word_eflags(imm, CONTEXT_NAME);
				}
				break;
			case INSTR_MOVIMM8:
				HWput_b(addr, (uae_u8)imm);
				break;
			case INSTR_MOVIMM32:
				if (size == 2)
					HWput_w(addr, (uae_u16)SDL_SwapBE16(imm));
				else
					HWput_l(addr, (uae_u32)SDL_SwapBE32(imm));
				break;
			case INSTR_TESTIMM8:
				imm &= HWget_b(addr);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_NOTMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWput_q */
					panicbug("NOT m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					HWput_b(addr, ~(uae_u8)HWget_b(addr));
				} else if (size == 2)
				{
					HWput_w(addr, SDL_SwapBE16(~(uae_u16)SDL_SwapBE16(HWget_w(addr))));
				} else /* if (size == 4) */
				{
					HWput_l(addr, SDL_SwapBE32(~(uae_u32)SDL_SwapBE32(HWget_l(addr))));
				}
				/* eflags not affected */
				break;
			case INSTR_NEGMEM:
#ifdef CPU_x86_64
				if (size == 8)
				{
					/* TODO: needs HWput_q */
					panicbug("NEG m64 - unsupported mode: i[1-6]=%02x %02x %02x %02x %02x %02x", addr_instr[1], addr_instr[2], addr_instr[3], addr_instr[4], addr_instr[5], addr_instr[6]);
					unknown_instruction();
				} else
#endif
				if (size == 1)
				{
					imm = ~(uae_u8)HWget_b(addr) + 1;
					HWput_b(addr, imm);
					set_byte_eflags(imm, CONTEXT_NAME);
				} else if (size == 2)
				{
					imm = ~(uae_u16)SDL_SwapBE16(HWget_w(addr)) + 1;
					HWput_w(addr, SDL_SwapBE16(imm));
					set_word_eflags(imm, CONTEXT_NAME);
				} else /* if (size == 4) */
				{
					imm = ~(uae_u32)SDL_SwapBE32(HWget_l(addr)) + 1;
					HWput_l(addr, SDL_SwapBE32(imm));
					set_long_eflags(imm, CONTEXT_NAME);
				}
				if (imm == 0)
					CONTEXT_AEFLAGS &= ~0x1;
				else
					CONTEXT_AEFLAGS |= 0x1;
				break;
			default:
				abort();
		}
	}

	D(bug("Access handled"));
	CONTEXT_AEIP += len;
#ifdef CPU_x86_64
	D2(bug("Next instruction on 0x%016lx", (uintptr)CONTEXT_AEIP));
#else
	D2(bug("Next instruction on 0x%08x", (uintptr)CONTEXT_AEIP));
#endif

	in_handler -= 1;
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

static __attribute_noinline__ void handle_access_fault(CONTEXT_ATYPE CONTEXT_NAME, volatile memptr faultaddr)
{
	if (SETJMP(sigsegv_env) != 0)
	{
		/*
		 * we get back here by a LONGJMP in BUS_ERROR,
		 * triggered by one of the HWget_x/HWput_x calls
		 * in the handler above
		 */
		D(bug("Atari bus error (%s)", (regs.mmu_fault_addr < 0x00f00000) || (regs.mmu_fault_addr > 0x00ffffff) ? "mem" : "hw"));
		CONTEXT_AEIP = (uintptr)atari_bus_fault;
		return;
	}

	handle_access_fault2(CONTEXT_NAME, faultaddr);
}
#endif /* NO_NESTED_SIGSEGV */
