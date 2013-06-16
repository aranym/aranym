/*
 * sigsegv_darwin_x86.cpp - x86 Darwin SIGSEGV handler
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
 *
 * Last modified: 2013-06-16 Jens Heitmann
 *
 */

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "memory.h"
#include <SDL_endian.h>
#define DEBUG 0
#include "debug.h"

#ifdef USE_JIT
extern void compiler_status();
# ifdef JIT_DEBUG
extern void compiler_dumpstate();
# endif
#endif

//
//
//  Darwin segmentation violation handler
//  based on the code of Basilisk II
//
#include <pthread.h>

// Address type
typedef char * sigsegv_address_t;

// SIGSEGV handler return state
enum sigsegv_return_t {
  SIGSEGV_RETURN_SUCCESS,
  SIGSEGV_RETURN_FAILURE,
//  SIGSEGV_RETURN_SKIP_INSTRUCTION,
};

// Define an address that is bound to be invalid for a program counter
const sigsegv_address_t SIGSEGV_INVALID_PC = (sigsegv_address_t)(-1);

extern "C" {
#include <mach/mach.h>
#include <mach/mach_error.h>
	
#ifdef CPU_i386
#	undef MACH_EXCEPTION_CODES
#	define MACH_EXCEPTION_CODES						0
#	define MACH_EXCEPTION_DATA_T					exception_data_t
#	define MACH_EXCEPTION_DATA_TYPE_T				exception_data_type_t
#	define MACH_EXC_SERVER							exc_server
#	define CATCH_MACH_EXCEPTION_RAISE				catch_exception_raise
#	define MACH_EXCEPTION_RAISE						exception_raise
#	define MACH_EXCEPTION_RAISE_STATE				exception_raise_state
#	define MACH_EXCEPTION_RAISE_STATE_IDENTITY		exception_raise_state_identity

#else

#	define MACH_EXCEPTION_DATA_T					mach_exception_data_t
#	define MACH_EXCEPTION_DATA_TYPE_T				mach_exception_data_type_t
#	define MACH_EXC_SERVER							mach_exc_server
#	define CATCH_MACH_EXCEPTION_RAISE				catch_mach_exception_raise
#	define MACH_EXCEPTION_RAISE						mach_exception_raise
#	define MACH_EXCEPTION_RAISE_STATE				mach_exception_raise_state
#	define MACH_EXCEPTION_RAISE_STATE_IDENTITY		mach_exception_raise_state_identity

#endif

	
// Extern declarations of mach functions
// dependend on the underlying architecture this are extern declarations
// for "mach" or "non mach" function names. 64 Bit requieres "mach_xxx"
//	functions
//
extern boolean_t MACH_EXC_SERVER(mach_msg_header_t *, mach_msg_header_t *);

extern kern_return_t CATCH_MACH_EXCEPTION_RAISE(mach_port_t, mach_port_t,
	mach_port_t, exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t);

extern kern_return_t MACH_EXCEPTION_RAISE(mach_port_t, mach_port_t, mach_port_t,
	exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t);

extern kern_return_t MACH_EXCEPTION_RAISE_STATE(mach_port_t, exception_type_t,
	MACH_EXCEPTION_DATA_T, mach_msg_type_number_t, thread_state_flavor_t *,
	thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);
	
extern kern_return_t MACH_EXCEPTION_RAISE_STATE_IDENTITY(mach_port_t, mach_port_t, mach_port_t,
	exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t, thread_state_flavor_t *,
	thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);
	
extern kern_return_t catch_mach_exception_raise_state(mach_port_t,
	exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t,
	int *, thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);
	   
extern kern_return_t catch_mach_exception_raise_state_identity(mach_port_t,
	mach_port_t, mach_port_t, exception_type_t, MACH_EXCEPTION_DATA_T, mach_msg_type_number_t,
    int *, thread_state_t, mach_msg_type_number_t, thread_state_t, mach_msg_type_number_t *);

}

// Could make this dynamic by looking for a result of MIG_ARRAY_TOO_LARGE
#define HANDLER_COUNT 64

// structure to tuck away existing exception handlers
typedef struct _ExceptionPorts {
	mach_msg_type_number_t maskCount;
	exception_mask_t masks[HANDLER_COUNT];
	exception_handler_t handlers[HANDLER_COUNT];
	exception_behavior_t behaviors[HANDLER_COUNT];
	thread_state_flavor_t flavors[HANDLER_COUNT];
} ExceptionPorts;

#if (CPU_i386)
#	define STATE_REGISTER_TYPE	uint32
#	ifdef i386_SAVED_STATE
#		define SIGSEGV_THREAD_STATE_TYPE		struct i386_saved_state
#		define SIGSEGV_THREAD_STATE_FLAVOR		i386_SAVED_STATE
#		define SIGSEGV_THREAD_STATE_COUNT		i386_SAVED_STATE_COUNT
#		define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->edi) /* EDI is the first GPR we consider */
#	else
#		ifdef x86_THREAD_STATE32
/* MacOS X 10.5 or newer introduces the new names and deprecates the old ones */
#			define SIGSEGV_THREAD_STATE_TYPE		x86_thread_state32_t
#			define SIGSEGV_THREAD_STATE_FLAVOR		x86_THREAD_STATE32
#			define SIGSEGV_THREAD_STATE_COUNT		x86_THREAD_STATE32_COUNT
#			define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->eax) /* EAX is the first GPR we consider */
#			define SIGSEGV_FAULT_INSTRUCTION		state->__eip

#		else
/* MacOS X 10.4 and below */
#			define SIGSEGV_THREAD_STATE_TYPE		struct i386_thread_state
#			define SIGSEGV_THREAD_STATE_FLAVOR		i386_THREAD_STATE
#			define SIGSEGV_THREAD_STATE_COUNT		i386_THREAD_STATE_COUNT
#			define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->eax) /* EAX is the first GPR we consider */
#			define SIGSEGV_FAULT_INSTRUCTION		state->eip

#		endif
#	endif
#endif

#if CPU_x86_64
#	define STATE_REGISTER_TYPE	uint64
#	ifdef x86_THREAD_STATE64
#		define SIGSEGV_THREAD_STATE_TYPE		x86_thread_state64_t
#		define SIGSEGV_THREAD_STATE_FLAVOR		x86_THREAD_STATE64
#		define SIGSEGV_THREAD_STATE_COUNT		x86_THREAD_STATE64_COUNT
#		define SIGSEGV_REGISTER_FILE			((unsigned long *)&state->__rax) /* EAX is the first GPR we consider */
#		define SIGSEGV_FAULT_INSTRUCTION		state->__rip
#	endif
#endif

#define SIGSEGV_ERROR_CODE				KERN_INVALID_ADDRESS
#define SIGSEGV_ERROR_CODE2				KERN_PROTECTION_FAILURE

#define SIGSEGV_FAULT_ADDRESS			code[1]

enum {
#if (CPU_i386)
#ifdef i386_SAVED_STATE
	// same as FreeBSD (in Open Darwin 8.0.1)
	X86_REG_EIP = 10,
	X86_REG_EAX = 7,
	X86_REG_ECX = 6,
	X86_REG_EDX = 5,
	X86_REG_EBX = 4,
	X86_REG_ESP = 13,
	X86_REG_EBP = 2,
	X86_REG_ESI = 1,
	X86_REG_EDI = 0
#else
	// new layout (MacOS X 10.4.4 for x86)
	X86_REG_EIP = 10,
	X86_REG_EAX = 0,
	X86_REG_ECX = 2,
	X86_REG_EDX = 3,
	X86_REG_EBX = 1,
	X86_REG_ESP = 7,
	X86_REG_EBP = 6,
	X86_REG_ESI = 5,
	X86_REG_EDI = 4
#endif
#else
#if (CPU_x86_64)
	X86_REG_R8  = 8,
	X86_REG_R9  = 9,
	X86_REG_R10 = 10,
	X86_REG_R11 = 11,
	X86_REG_R12 = 12,
	X86_REG_R13 = 13,
	X86_REG_R14 = 14,
	X86_REG_R15 = 15,
	X86_REG_EDI = 4,
	X86_REG_ESI = 5,
	X86_REG_EBP = 6,
	X86_REG_EBX = 1,
	X86_REG_EDX = 3,
	X86_REG_EAX = 0,
	X86_REG_ECX = 2,
	X86_REG_ESP = 7,
	X86_REG_EIP = 16
#endif
#endif
};

// Type of a SIGSEGV handler. Returns boolean expressing successful operation
typedef sigsegv_return_t (*sigsegv_fault_handler_t)(sigsegv_address_t fault_address, sigsegv_address_t instruction_address,
										 SIGSEGV_THREAD_STATE_TYPE *state );

// Install a SIGSEGV handler. Returns boolean expressing success
extern bool sigsegv_install_handler(sigsegv_fault_handler_t handler);

enum transfer_type_t {
	TYPE_UNKNOWN,
	TYPE_LOAD,
	TYPE_STORE
};

enum type_size_t {
	TYPE_BYTE,
	TYPE_WORD,
	TYPE_INT
#ifdef CPU_x86_64
	,TYPE_QUAD
#endif
};

// exception handler thread
static pthread_t exc_thread;

static mach_port_t _exceptionPort = MACH_PORT_NULL;

// place where old exception handler info is stored
static ExceptionPorts ports;

// User's SIGSEGV handler
static sigsegv_fault_handler_t sigsegv_fault_handler = 0;


#define MACH_CHECK_ERROR(name,ret) \
if (ret != KERN_SUCCESS) { \
	mach_error(#name, ret); \
	exit (1); \
}

#define MSG_SIZE 512
static char msgbuf[MSG_SIZE];
static char replybuf[MSG_SIZE];

#define CONTEXT_ATYPE	SIGSEGV_THREAD_STATE_TYPE*
#define CONTEXT_NAME	state

#ifdef CPU_x86_64

#define CONTEXT_EIP		CONTEXT_NAME.__rip
#define CONTEXT_EFLAGS	CONTEXT_NAME.__rflags
#define CONTEXT_EAX		CONTEXT_NAME.__rax
#define CONTEXT_EBX		CONTEXT_NAME.__rbx
#define CONTEXT_ECX		CONTEXT_NAME.__rcx
#define CONTEXT_EDX		CONTEXT_NAME.__rdx
#define CONTEXT_EBP		CONTEXT_NAME.__rbp
#define CONTEXT_ESI		CONTEXT_NAME.__rsi
#define CONTEXT_EDI		CONTEXT_NAME.__rdi

#define CONTEXT_R8	CONTEXT_NAME.__r8
#define CONTEXT_R9	CONTEXT_NAME.__r9
#define CONTEXT_R10	CONTEXT_NAME.__r10
#define CONTEXT_R11	CONTEXT_NAME.__r11
#define CONTEXT_R12	CONTEXT_NAME.__r12
#define CONTEXT_R13	CONTEXT_NAME.__r13
#define CONTEXT_R14	CONTEXT_NAME.__r14
#define CONTEXT_R15	CONTEXT_NAME.__r15

#define CONTEXT_CS	CONTEXT_NAME.__cs
#define CONTEXT_FS	CONTEXT_NAME.__fs
#define CONTEXT_GS	CONTEXT_NAME.__gs

#define CONTEXT_AEIP	CONTEXT_NAME->__rip
#define CONTEXT_AEFLAGS	CONTEXT_NAME->__rflags
#define CONTEXT_AEAX	CONTEXT_NAME->__rax
#define CONTEXT_AEBX	CONTEXT_NAME->__rbx
#define CONTEXT_AECX	CONTEXT_NAME->__rcx
#define CONTEXT_AEDX	CONTEXT_NAME->__rdx
#define CONTEXT_AEBP	CONTEXT_NAME->__rbp
#define CONTEXT_AESI	CONTEXT_NAME->__rsi
#define CONTEXT_AEDI	CONTEXT_NAME->__rdi

#define CONTEXT_AR8	CONTEXT_NAME->__r8
#define CONTEXT_AR9	CONTEXT_NAME->__r9
#define CONTEXT_AR10	CONTEXT_NAME->__r10
#define CONTEXT_AR11	CONTEXT_NAME->__r11
#define CONTEXT_AR12	CONTEXT_NAME->__r12
#define CONTEXT_AR13	CONTEXT_NAME->__r13
#define CONTEXT_AR14	CONTEXT_NAME->__r14
#define CONTEXT_AR15	CONTEXT_NAME->__r15

#define CONTEXT_ACS	CONTEXT_NAME->__cs
#define CONTEXT_AFS	CONTEXT_NAME->__fs
#define CONTEXT_AGS	CONTEXT_NAME->__gs

#else

#ifdef x86_THREAD_STATE32

#define CONTEXT_EIP		CONTEXT_NAME.__eip
#define CONTEXT_EFLAGS	CONTEXT_NAME.__eflags;
#define CONTEXT_EAX		CONTEXT_NAME.__eax
#define CONTEXT_EBX		CONTEXT_NAME.__ebx
#define CONTEXT_ECX		CONTEXT_NAME.__ecx
#define CONTEXT_EDX		CONTEXT_NAME.__edx
#define CONTEXT_EBP		CONTEXT_NAME.__ebp
#define CONTEXT_ESI		CONTEXT_NAME.__esi
#define CONTEXT_EDI		CONTEXT_NAME.__edi

#define CONTEXT_AEIP	CONTEXT_NAME->__eip
#define CONTEXT_AEFLAGS	CONTEXT_NAME->__eflags
#define CONTEXT_AEAX	CONTEXT_NAME->__eax
#define CONTEXT_AEBX	CONTEXT_NAME->__ebx
#define CONTEXT_AECX	CONTEXT_NAME->__ecx
#define CONTEXT_AEDX	CONTEXT_NAME->__edx
#define CONTEXT_AEBP	CONTEXT_NAME->__ebp
#define CONTEXT_AESI	CONTEXT_NAME->__esi
#define CONTEXT_AEDI	CONTEXT_NAME->__edi

#else

#define CONTEXT_EIP		CONTEXT_NAME.eip
#define CONTEXT_EFLAGS	CONTEXT_NAME.eflags;
#define CONTEXT_EAX		CONTEXT_NAME.eax
#define CONTEXT_EBX		CONTEXT_NAME.ebx
#define CONTEXT_ECX		CONTEXT_NAME.ecx
#define CONTEXT_EDX		CONTEXT_NAME.edx
#define CONTEXT_EBP		CONTEXT_NAME.ebp
#define CONTEXT_ESI		CONTEXT_NAME.esi
#define CONTEXT_EDI		CONTEXT_NAME.edi

#define CONTEXT_AEIP	CONTEXT_NAME->eip
#define CONTEXT_AEFLAGS	CONTEXT_NAME->eflags
#define CONTEXT_AEAX	CONTEXT_NAME->eax
#define CONTEXT_AEBX	CONTEXT_NAME->ebx
#define CONTEXT_AECX	CONTEXT_NAME->ecx
#define CONTEXT_AEDX	CONTEXT_NAME->edx
#define CONTEXT_AEBP	CONTEXT_NAME->ebp
#define CONTEXT_AESI	CONTEXT_NAME->esi
#define CONTEXT_AEDI	CONTEXT_NAME->edi

#endif
#endif

int in_handler = 0;

#if (CPU_i386) || (CPU_x86_64)

/* instruction jump table */
//i386op_func *cpufunctbl[256];

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
	INSTR_TESTIMM8,
	INSTR_XOR8
};

enum case_instr_t {
	CASE_INSTR_ADD8MR	= 0x00,
	CASE_INSTR_ADD8RM	= 0x02,
	CASE_INSTR_OR8MR	= 0x08,
	CASE_INSTR_OR8RM	= 0x0a,
	CASE_INSTR_MOVxX	= 0x0f,
	CASE_INSTR_XOR8MR	= 0x30,
	CASE_INSTR_MOVZX8RM	= 0xb6,
	CASE_INSTR_MOVZX16RM	= 0xb7,
	CASE_INSTR_MOVSX8RM	= 0xbe
};

static inline int get_instr_size_add(unsigned char *p)
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

static inline void set_byte_eflags(int i, CONTEXT_ATYPE CONTEXT_NAME) {
	
	/* MJ - AF and OF not tested, also CF for 32 bit */
	if ((i > 255) || (i < 0)) CONTEXT_AEFLAGS |= 0x1;	// CF
	else CONTEXT_AEFLAGS &= ~0x1;
	
	if (i > 127) CONTEXT_AEFLAGS |= 0x80;			// SF
	else CONTEXT_AEFLAGS &= ~0x80;

	if ((i % 2) == 0) CONTEXT_AEFLAGS |= 0x4;				// PF
	else CONTEXT_AEFLAGS &= ~0x4;
	
	if (i == 0) CONTEXT_AEFLAGS |= 0x40;					// ZF
	else CONTEXT_AEFLAGS &= ~0x40;
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
static sigsegv_return_t sigsegv_handler2(sigsegv_address_t fault_address,
										sigsegv_address_t fault_instruction,
										 SIGSEGV_THREAD_STATE_TYPE *state)
#else
static sigsegv_return_t sigsegv_handler(sigsegv_address_t fault_address,
										sigsegv_address_t fault_instruction,
										 SIGSEGV_THREAD_STATE_TYPE *state)
#endif
{
	D(panicbug("Catched signal %p", fault_address));
	static const int x86_reg_map[] = {
		X86_REG_EAX, X86_REG_ECX, X86_REG_EDX, X86_REG_EBX,
		X86_REG_ESP, X86_REG_EBP, X86_REG_ESI, X86_REG_EDI,
#if defined(__x86_64__) || defined(_M_X64)
		X86_REG_R8,  X86_REG_R9,  X86_REG_R10, X86_REG_R11,
		X86_REG_R12, X86_REG_R13, X86_REG_R14, X86_REG_R15,
#endif
	};
    
	uintptr addr = (uintptr)fault_address;
	
	const uintptr ainstr = (uintptr)fault_instruction;
	const uint32 instr = (uint32)*(uint32 *)ainstr;
	uint8 *addr_instr = (uint8 *)ainstr;
	
	int reg = -1;
	int len = 0;
	transfer_type_t transfer_type = TYPE_UNKNOWN;
	int size = 4;
	int imm = 0;
	int pom1, pom2;
	instruction_t instruction = INSTR_UNKNOWN;
	void *preg;

	if (in_handler > 0) {
		panicbug("Segmentation fault in handler :-(");
		abort();
	}
	in_handler += 1;

#ifdef USE_JIT	/* does not compile with default configure */
	D(compiler_status());
#endif
	D(panicbug("\nBUS ERROR fault address is %p at %p", addr, ainstr));
	D2(panicbug("instruction is %08x", instr));

	D2(panicbug("PC %08x", regs.pc)); 

	addr -= FMEMORY;

	D2(panicbug("op code: %x %x", (int)addr_instr[0], (int)addr_instr[1]));
	
#ifdef HW_SIGSEGV
#if (CPU_x86_64)
	if (addr_instr[0] == 0x67) { // address size override prefix
		addr_instr++; // Skip prefix, seems to be enough
		len++;
	}
#endif

	if (addr_instr[0] == 0x66) { // Precision size override prefix
		addr_instr++;
		len++;
		size = 2;
		D(panicbug("Word instr:"));
	}

#if (CPU_x86_64)	
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
#if DEBUG
		printf("REX: %c,%c,%c,%c\n",
			   rex.W ? 'W' : '_',
			   rex.R ? 'R' : '_',
			   rex.X ? 'X' : '_',
			   rex.B ? 'B' : '_');
#endif
		addr_instr++;
		len++;
		if (rex.W)
			size = 8;
	}
#else
	const bool has_rex = false;
#endif
	
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
		case CASE_INSTR_XOR8MR:
			D(panicbug("XOR m8, r8"));
			size = 1;
			transfer_type = TYPE_STORE;
			instruction = INSTR_XOR8;
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
			panicbug("unknown op code (%p): %x %x", ainstr, (int)addr_instr[0], (int)addr_instr[1]);
			unknown_instruction(instr);
			abort();
	}

	D2(panicbug("address %p", addr));
	
	if (addr >= 0xff000000)
		addr &= 0x00ffffff;

	if ((addr < 0x00f00000) || (addr > 0x00ffffff))
	{
		D2(panicbug("Throwing bus error"));
		goto buserr;
	}
	
#if (CPU_x86_64)
	if (rex.R) {
		reg += 8;
		D2(panicbug("Extended register %d", reg));
	}
#endif

	// Get register pointer
	if (size == 1) {
		if (has_rex || reg < 4) {
			preg = ((STATE_REGISTER_TYPE *)state) + x86_reg_map[reg];
		} else {
			preg = (uae_u8*)(((STATE_REGISTER_TYPE *)state) + x86_reg_map[reg - 4]) + 1; // AH, BH, CH, DH
		}
	} else {
		preg = ((STATE_REGISTER_TYPE *)state) + x86_reg_map[reg];
	}
	// preg = get_preg(reg, CONTEXT_NAME, size);

	D2(panicbug("Register %d, place %p, address %p", reg, preg, addr));

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
				set_byte_eflags(*((uae_u8 *)preg), CONTEXT_NAME);
				break;
			case INSTR_AND8:
				*((uae_u8 *)preg) &= HWget_b(addr);
				set_byte_eflags(*((uae_u8 *)preg), CONTEXT_NAME);
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
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_DIV8:
				pom1 = CONTEXT_AEAX & 0xffff;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = CONTEXT_AEAX & ~0xffff + ((pom1 / pom2) << 8) + (pom1 / pom2);
				break;
			case INSTR_IDIV8:
				pom1 = CONTEXT_AEAX & 0xffff;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = CONTEXT_AEAX & ~0xffff + (((uae_s8)pom1 / (uae_s8)pom2) << 8) + ((uae_s8)pom1 / (uae_s8)pom2);
				break;
			case INSTR_MUL8:
				pom1 = CONTEXT_AEAX & 0xff;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = CONTEXT_AEAX & ~0xffff + pom1 * pom2;
				if ((CONTEXT_AEAX & 0xff00) == 0) CONTEXT_AEFLAGS &= ~0x401;	// CF + OF
					else CONTEXT_AEFLAGS |= 0x401;
				break;
			case INSTR_IMUL8:
				pom1 = CONTEXT_AEAX & 0xff;
				pom2 = HWget_b(addr);
				CONTEXT_AEAX = CONTEXT_AEAX & ~0xffff + (uae_s8)pom1 * (uae_s8)pom2;
				if ((CONTEXT_AEAX & 0xff00) == 0) CONTEXT_AEFLAGS &= ~0x401;	// CF + OF
					else CONTEXT_AEFLAGS |= 0x401;
				break;
			default:
				D2(panicbug("Unknown load instruction %X", instruction));
				abort();
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
					HWput_l(addr, SDL_SwapBE32(*((uae_u32 *)preg)));
				} else {
					HWput_w(addr, SDL_SwapBE16(*((uae_u16 *)preg)));
				}
				break;
			case INSTR_AND8:
				imm = HWget_b(addr);
				imm &= *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_ADD8:
				imm = HWget_b(addr);
				imm += *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_OR8:
				imm = HWget_b(addr);
				imm |= *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_ORIMM8:
				imm |= HWget_b(addr);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
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
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			case INSTR_NOT8:
				HWput_b(addr, ~(uae_u8)HWget_b(addr));
				break;
			case INSTR_NEG8:
				imm = ~(uae_u8)HWget_b(addr) + 1;
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				if (imm == 0)
					CONTEXT_AEFLAGS &= ~0x1;
				else
					CONTEXT_AEFLAGS |= 0x1;
				break;
			case INSTR_XOR8:
				imm = HWget_b(addr);
				imm ^= *((uae_u8 *)preg);
				HWput_b(addr, imm);
				set_byte_eflags(imm, CONTEXT_NAME);
				break;
			default: abort();
		}
	}

	D2(panicbug("Access handled"));
	D2(panicbug("Next instruction on %p", CONTEXT_AEIP + len));
	CONTEXT_AEIP += len;

	in_handler -= 1;
	
	D2(panicbug("Return from handler"));
	return SIGSEGV_RETURN_SUCCESS;
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
	THROW(2);
}

static sigsegv_return_t sigsegv_handler(sigsegv_address_t fault_address,
										sigsegv_address_t fault_instruction,
										 SIGSEGV_THREAD_STATE_TYPE *state) {
	if (SETJMP(sigsegv_env) != 0)
	{
		CONTEXT_AEIP = (unsigned long)atari_bus_fault;
		return SIGSEGV_RETURN_SUCCESS;
	}

	return sigsegv_handler2(fault_address, fault_instruction, state);
}
#endif /* NO_NESTED_SIGSEGV */

#endif

/*
 *  SIGSEGV global handler
 */

// This function handles the badaccess to memory.
// It is called from the signal handler or the exception handler.
static bool handle_badaccess(mach_port_t thread, MACH_EXCEPTION_DATA_T code, SIGSEGV_THREAD_STATE_TYPE *state)
{
	// We must match the initial count when writing back the CPU state registers
	kern_return_t krc;
	mach_msg_type_number_t count;

	D2(panicbug("handle badaccess"));

	count = SIGSEGV_THREAD_STATE_COUNT;
	krc = thread_get_state(thread, SIGSEGV_THREAD_STATE_FLAVOR, (thread_state_t)state, &count);
	MACH_CHECK_ERROR (thread_get_state, krc);

	sigsegv_address_t fault_address = (sigsegv_address_t)SIGSEGV_FAULT_ADDRESS;
	sigsegv_address_t fault_instruction = (sigsegv_address_t)SIGSEGV_FAULT_INSTRUCTION;

	D2(panicbug("code:%lx %lx %lx %lx", (long)code[0], (long)code[1], (long)code[2], (long)code[3]));
	D2(panicbug("regs eax:%lx", CONTEXT_AEAX));
	D2(panicbug("regs ebx:%lx", CONTEXT_AEBX));
	D2(panicbug("regs ecx:%lx", CONTEXT_AECX));
	D2(panicbug("regs edx:%lx", CONTEXT_AEDX));
	D2(panicbug("regs ebp:%lx", CONTEXT_AEBP));
	D2(panicbug("regs esi:%lx", CONTEXT_AESI));
	D2(panicbug("regs edi:%lx", CONTEXT_AEDI));
	D2(panicbug("regs eip:%lx", CONTEXT_AEIP));
	D2(panicbug("regs eflags:%lx", CONTEXT_AEFLAGS));

	// Call user's handler 
	if (sigsegv_fault_handler(fault_address, fault_instruction, state) == SIGSEGV_RETURN_SUCCESS) {
		D2(panicbug("esi:%lx", CONTEXT_AESI));
		krc = thread_set_state(thread,
								SIGSEGV_THREAD_STATE_FLAVOR, (thread_state_t)state,
								count);
		MACH_CHECK_ERROR (thread_set_state, krc);
		D(panicbug("return from handle bad access with true"));
		return true;
	}

	D(panicbug("return from handle bad access with false"));
	return false;
}


/*
 * We need to forward all exceptions that we do not handle.
 * This is important, there are many exceptions that may be
 * handled by other exception handlers. For example debuggers
 * use exceptions and the exception handler is in another
 * process in such a case. (Timothy J. Wood states in his
 * message to the list that he based this code on that from
 * gdb for Darwin.)
 */
static inline kern_return_t
forward_exception(mach_port_t thread_port,
				  mach_port_t task_port,
				  exception_type_t exception_type,
				  MACH_EXCEPTION_DATA_T exception_data,
				  mach_msg_type_number_t data_count,
				  ExceptionPorts *oldExceptionPorts)
{
	kern_return_t kret;
	unsigned int portIndex;
	mach_port_t port;
	exception_behavior_t behavior;
	thread_state_flavor_t flavor;
	thread_state_data_t thread_state;
	mach_msg_type_number_t thread_state_count;
	
	D(panicbug("forward_exception\n"));

	for (portIndex = 0; portIndex < oldExceptionPorts->maskCount; portIndex++) {
		if (oldExceptionPorts->masks[portIndex] & (1 << exception_type)) {
			// This handler wants the exception
			break;
		}
	}

	if (portIndex >= oldExceptionPorts->maskCount) {
		panicbug("No handler for exception_type = %d. Not fowarding\n", exception_type);
		return KERN_FAILURE;
	}

	port = oldExceptionPorts->handlers[portIndex];
	behavior = oldExceptionPorts->behaviors[portIndex];
	flavor = oldExceptionPorts->flavors[portIndex];
	
	if (flavor && !VALID_THREAD_STATE_FLAVOR(flavor)) {
		fprintf(stderr, "Invalid thread_state flavor = %d. Not forwarding\n", flavor);
		return KERN_FAILURE;
	}

	/*
	 fprintf(stderr, "forwarding exception, port = 0x%x, behaviour = %d, flavor = %d\n", port, behavior, flavor);
	 */

	if (behavior != EXCEPTION_DEFAULT) {
		thread_state_count = THREAD_STATE_MAX;
		kret = thread_get_state (thread_port, flavor, (natural_t *)&thread_state,
								 &thread_state_count);
		MACH_CHECK_ERROR (thread_get_state, kret);
	}

	switch (behavior) {
	case EXCEPTION_DEFAULT:
	  // fprintf(stderr, "forwarding to exception_raise\n");
	  kret = MACH_EXCEPTION_RAISE(port, thread_port, task_port, exception_type,
							 exception_data, data_count);
	  MACH_CHECK_ERROR (MACH_EXCEPTION_RAISE, kret);
	  break;
	case EXCEPTION_STATE:
	  // fprintf(stderr, "forwarding to exception_raise_state\n");
	  kret = MACH_EXCEPTION_RAISE_STATE(port, exception_type, exception_data,
								   data_count, &flavor,
								   (natural_t *)&thread_state, thread_state_count,
								   (natural_t *)&thread_state, &thread_state_count);
	  MACH_CHECK_ERROR (MACH_EXCEPTION_RAISE_STATE, kret);
	  break;
	case EXCEPTION_STATE_IDENTITY:
	  // fprintf(stderr, "forwarding to exception_raise_state_identity\n");
	  kret = MACH_EXCEPTION_RAISE_STATE_IDENTITY(port, thread_port, task_port,
											exception_type, exception_data,
											data_count, &flavor,
											(natural_t *)&thread_state, thread_state_count,
											(natural_t *)&thread_state, &thread_state_count);
	  MACH_CHECK_ERROR (MACH_EXCEPTION_RAISE_STATE_IDENTITY, kret);
	  break;
	default:
	  panicbug("forward_exception got unknown behavior");
	  kret = KERN_FAILURE;
	  break;
	}

	if (behavior != EXCEPTION_DEFAULT) {
		kret = thread_set_state (thread_port, flavor, (natural_t *)&thread_state,
								 thread_state_count);
		MACH_CHECK_ERROR (thread_set_state, kret);
	}

	return kret;
}

/*
 * This is the code that actually handles the exception.
 * It is called by exc_server. For Darwin 5 Apple changed
 * this a bit from how this family of functions worked in
 * Mach. If you are familiar with that it is a little
 * different. The main variation that concerns us here is
 * that code is an array of exception specific codes and
 * codeCount is a count of the number of codes in the code
 * array. In typical Mach all exceptions have a code
 * and sub-code. It happens to be the case that for a
 * EXC_BAD_ACCESS exception the first entry is the type of
 * bad access that occurred and the second entry is the
 * faulting address so these entries correspond exactly to
 * how the code and sub-code are used on Mach.
 *
 * This is a MIG interface. No code in Basilisk II should
 * call this directley. This has to have external C
 * linkage because that is what exc_server expects.
 */
__attribute__ ((visibility("default")))
kern_return_t
CATCH_MACH_EXCEPTION_RAISE(mach_port_t exception_port,
					  mach_port_t thread,
					  mach_port_t task,
					  exception_type_t exception,
					  MACH_EXCEPTION_DATA_T code,
					  mach_msg_type_number_t codeCount)
{
	SIGSEGV_THREAD_STATE_TYPE state;
	kern_return_t krc;

	D(panicbug("catch_exception_raise: %d", exception));

	if (exception == EXC_BAD_ACCESS) {
		switch (code[0]) {
			case KERN_PROTECTION_FAILURE:
			case KERN_INVALID_ADDRESS:
				if (handle_badaccess(thread, code, &state))
					return KERN_SUCCESS;
				break;
		}
	}
	

	// In Mach we do not need to remove the exception handler.
	// If we forward the exception, eventually some exception handler
	// will take care of this exception.
	krc = forward_exception(thread, task, exception, code, codeCount, &ports);
	
	return krc;
}

/* XXX: borrowed from launchd and gdb */
kern_return_t
catch_mach_exception_raise_state(mach_port_t exception_port,
								 exception_type_t exception,
								 MACH_EXCEPTION_DATA_T code,
								 mach_msg_type_number_t code_count,
								 int *flavor,
								 thread_state_t old_state,
								 mach_msg_type_number_t old_state_count,
								 thread_state_t new_state,
								 mach_msg_type_number_t *new_state_count)
{
	memcpy(new_state, old_state, old_state_count * sizeof(old_state[0]));
	*new_state_count = old_state_count;
	return KERN_SUCCESS;
}

/* XXX: borrowed from launchd and gdb */
kern_return_t
catch_mach_exception_raise_state_identity(mach_port_t exception_port,
										  mach_port_t thread_port,
										  mach_port_t task_port,
										  exception_type_t exception,
										  MACH_EXCEPTION_DATA_T code,
										  mach_msg_type_number_t code_count,
										  int *flavor,
										  thread_state_t old_state,
										  mach_msg_type_number_t old_state_count,
										  thread_state_t new_state,
										  mach_msg_type_number_t *new_state_count)
{
	kern_return_t kret;
	
	memcpy(new_state, old_state, old_state_count * sizeof(old_state[0]));
	*new_state_count = old_state_count;
	
	kret = mach_port_deallocate(mach_task_self(), task_port);
	MACH_CHECK_ERROR(mach_port_deallocate, kret);
	kret = mach_port_deallocate(mach_task_self(), thread_port);
	MACH_CHECK_ERROR(mach_port_deallocate, kret);
	
	return KERN_SUCCESS;
}

/*
 * This is the entry point for the exception handler thread. The job
 * of this thread is to wait for exception messages on the exception
 * port that was setup beforehand and to pass them on to exc_server.
 * exc_server is a MIG generated function that is a part of Mach.
 * Its job is to decide what to do with the exception message. In our
 * case exc_server calls catch_exception_raise on our behalf. After
 * exc_server returns, it is our responsibility to send the reply.
 */
static void *
handleExceptions(void * /*priv*/)
{
	D(panicbug("handleExceptions\n"));

	mach_msg_header_t *msg, *reply;
	kern_return_t krc;

	msg = (mach_msg_header_t *)msgbuf;
	reply = (mach_msg_header_t *)replybuf;
			
	for (;;) {
		krc = mach_msg(msg, MACH_RCV_MSG, MSG_SIZE, MSG_SIZE,
				_exceptionPort, 0, MACH_PORT_NULL);
		MACH_CHECK_ERROR(mach_msg, krc);

		if (!MACH_EXC_SERVER(msg, reply)) {
			fprintf(stderr, "exc_server hated the message\n");
			exit(1);
		}

		krc = mach_msg(reply, MACH_SEND_MSG, reply->msgh_size, 0,
				 msg->msgh_local_port, 0, MACH_PORT_NULL);
		if (krc != KERN_SUCCESS) {
			fprintf(stderr, "Error sending message to original reply port, krc = %d, %s",
				krc, mach_error_string(krc));
			exit(1);
		}
	}
}

static bool sigsegv_do_install_handler(sigsegv_fault_handler_t handler)
{
	D(panicbug("sigsegv_do_install_handler\n"));

	//
	//Except for the exception port functions, this should be
	//pretty much stock Mach. If later you choose to support
	//other Mach's besides Darwin, just check for __MACH__
	//here and __APPLE__ where the actual differences are.
	//
	if (sigsegv_fault_handler != NULL) {
		sigsegv_fault_handler = handler;
		return true;
	}

	kern_return_t krc;

	// create the the exception port
	krc = mach_port_allocate(mach_task_self(),
			  MACH_PORT_RIGHT_RECEIVE, &_exceptionPort);
	if (krc != KERN_SUCCESS) {
		mach_error("mach_port_allocate", krc);
		return false;
	}

	// add a port send right
	krc = mach_port_insert_right(mach_task_self(),
			      _exceptionPort, _exceptionPort,
			      MACH_MSG_TYPE_MAKE_SEND);
	if (krc != KERN_SUCCESS) {
		mach_error("mach_port_insert_right", krc);
		return false;
	}

	// get the old exception ports
	ports.maskCount = sizeof (ports.masks) / sizeof (ports.masks[0]);
	krc = thread_get_exception_ports(mach_thread_self(), EXC_MASK_BAD_ACCESS, ports.masks,
 				&ports.maskCount, ports.handlers, ports.behaviors, ports.flavors);
 	if (krc != KERN_SUCCESS) {
 		mach_error("thread_get_exception_ports", krc);
 		return false;
 	}

	// set the new exception port
	//
	// We could have used EXCEPTION_STATE_IDENTITY instead of
	// EXCEPTION_DEFAULT to get the thread state in the initial
	// message, but it turns out that in the common case this is not
	// neccessary. If we need it we can later ask for it from the
	// suspended thread.
	//
	// Even with THREAD_STATE_NONE, Darwin provides the program
	// counter in the thread state.  The comments in the header file
	// seem to imply that you can count on the GPR's on an exception
	// as well but just to be safe I use MACHINE_THREAD_STATE because
	// you have to ask for all of the GPR's anyway just to get the
	// program counter. In any case because of update effective
	// address from immediate and update address from effective
	// addresses of ra and rb modes (as good an name as any for these
	// addressing modes) used in PPC instructions, you will need the
	// GPR state anyway.
	krc = thread_set_exception_ports(mach_thread_self(), EXC_MASK_BAD_ACCESS, _exceptionPort,
				EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES, SIGSEGV_THREAD_STATE_FLAVOR);
	if (krc != KERN_SUCCESS) {
		mach_error("thread_set_exception_ports", krc);
		return false;
	}

	// create the exception handler thread
	if (pthread_create(&exc_thread, NULL, &handleExceptions, NULL) != 0) {
		panicbug("creation of exception thread failed\n");
		return false;
	}

	// do not care about the exception thread any longer, let is run standalone
	(void)pthread_detach(exc_thread);

	D(panicbug("Sigsegv installed\n"));
	sigsegv_fault_handler = handler;
	return true;
}

void install_sigsegv() {
	sigsegv_do_install_handler(sigsegv_handler);
}

