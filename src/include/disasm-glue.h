#ifndef __DISASM_GLUE_H__
#define __DISASM_GLUE_H__

/*
 * generic interface to various disassemblers that are freely available.
 *
 * currently supported:
 *
 * - DISASM_USE_BUILTIN: uses the builtin disassembler.
 *   This is the default.
 *   It's always available, and rather short.
 * - DISASM_USE_OPCODES: uses GNU opcodes library.
 *   Available for most systems, and can probably handle everything
 *   we can think of. For this reason, rather bloated.
 *   The problem with this is if the systems default
 *   library doesn't contain support for 68k, you have to compile one yourself,
 *   and install it in a non-default place, or it clashes with the system library.
 */

#if defined(MAIN) && !defined(DISASM_USE_BUILTIN) && !defined(DISASM_USE_OPCODES)
#  define DISASM_USE_BUILTIN
#endif

#if defined(DISASM_USE_BUILTIN) || defined(DISASM_USE_OPCODES)
#  define HAVE_DISASM_M68K
#else
#  undef HAVE_DISASM_M68K
#endif
#if (defined(DISASM_USE_BUILTIN) + defined(DISASM_USE_OPCODES)) > 1
  #error only one disassembler may be defined
#endif

#ifdef HAVE_DISASM_M68K

enum m68k_cpu {
	CPU_AUTO,
	CPU_68000,
	CPU_68008,
	CPU_68010,
	CPU_68020,
	CPU_CPU32,
	CPU_68030,
	CPU_68EC030,
	CPU_68040,
	CPU_68EC040,
	CPU_68LC040,
	CPU_68060,
	CPU_68302,
	CPU_68331,
	CPU_68332,
	CPU_68333,
	CPU_68360,
	CPU_5200,
	CPU_5202,
	CPU_5204,
	CPU_5206,
	CPU_5206e,
	CPU_5207,
	CPU_5208,
	CPU_521x,
	CPU_5249,
	CPU_528x,
	CPU_5307,
	CPU_537x,
	CPU_5407,
	CPU_547x,
	CPU_548x,
	CPU_CFV4,
	CPU_CFV4e,
	CPU_CF_FIRST = CPU_5200,
	CPU_CF_LAST = CPU_CFV4e
};

enum m68k_fpu {
	FPU_AUTO,
	FPU_NONE,
	FPU_68881,
	FPU_68882,
	FPU_68040,
	FPU_COLDFIRE
};

enum m68k_mmu {
	MMU_AUTO,
	MMU_NONE,
	MMU_68851,
	MMU_68040
};

typedef struct _m68k_disasm_info {
	enum m68k_cpu cpu;
	enum m68k_fpu fpu;
	enum m68k_mmu mmu;
	
	int is_64bit;

	memptr memory_vma;
	uae_u32 reloffset;
	
	/*
	 * for use by the caller
	 */
	void *application_data;
	
	/*
	 * for use by the disassembler
	 */
	void *disasm_data;
	
	/*
	 * opcode of decoded instruction
	 */
	char opcode[22];
	
	/*
	 * number of operands in decoded instruction, 0-2
	 */
	int num_oper;
	
	/*
	 * the operands
	 */
	char operands[128];
	
	/*
	 * any comments
	 */
	char comments[128];
	
	/*
	 * the number of instruction words, and their values (in host format)
	 */
	int num_insn_words;
	unsigned short insn_words[11];
} m68k_disasm_info;

extern m68k_disasm_info disasm_info;

void m68k_disasm_init(m68k_disasm_info *info, enum m68k_cpu cpu);
void m68k_disasm_exit(m68k_disasm_info *info);
int m68k_disasm_insn(m68k_disasm_info *info);
int m68k_disasm_to_buf(m68k_disasm_info *info, char *buf, int allbytes);

memptr gdb_dis(memptr start, unsigned int count);
void gdb_regs(void);
memptr gdb_pc(void);

#ifdef DISASM_USE_BUILTIN

int m68k_disasm_builtin(m68k_disasm_info *info);

#endif

#endif /* HAVE_DISASM_M68K */

#if (defined(DISASM_USE_BUILTIN) || defined(DISASM_USE_OPCODES)) && (defined(CPU_i386) || defined(CPU_x86_64))

#define HAVE_DISASM_X86 1

const uint8 *x86_disasm(const uint8 *ainstr, char *buf, int allbytes);

#endif

#if defined(DISASM_USE_OPCODES) && (defined(CPU_powerpc))

#define HAVE_DISASM_PPC 1

const uint8 *ppc_disasm(const uint8 *ainstr, char *buf, int allbytes);

#endif

#if (defined(DISASM_USE_BUILTIN) || defined(DISASM_USE_OPCODES)) && (defined(CPU_arm))

#define HAVE_DISASM_ARM 1

const uint8 *arm_disasm(const uint8 *ainstr, char *buf, int allbytes);

#endif

#endif /* __DISASM_GLUE_H__ */
