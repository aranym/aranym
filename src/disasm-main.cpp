#define MAIN
#include "disasm-glue.cpp"

#ifdef DISASM_USE_BUILTIN
#include "disasm-builtin.cpp"
#endif



#include <stdarg.h>
#include <getopt.h>
#include <errno.h>
#include "vm_alloc.h"
#include "vm_alloc.cpp"
#ifndef HW_SIGSEGV
#include "memory.cpp"
#endif

struct regstruct regs;
struct flag_struct regflags;
#ifdef EXCEPTIONS_VIA_LONGJMP
JMP_BUF excep_env;
#endif
JMP_BUF sigsegv_env;
int in_handler;
void breakpt(void) { }

// RAM and ROM pointers
memptr RAMBase = 0;	// RAM base (Atari address space) gb-- init is important
uint8 *RAMBaseHost;	// RAM base (host address space)
uint32 RAMSize = 0x00e00000;	// Size of RAM

memptr ROMBase = 0x00e00000;	// ROM base (Atari address space)
uint8 *ROMBaseHost;	// ROM base (host address space)
uint32 ROMSize = 0x00100000;	// Size of ROM

uintptr MEMBaseDiff;	// Global offset between a Atari address and its Host equivalent
uintptr ROMBaseDiff;
uintptr FastRAMBaseDiff;
uint32 FastRAMSize;

memptr FastRAMBase = 0x01000000;		// Fast-RAM base (Atari address space)
uint8 *FastRAMBaseHost;	// Fast-RAM base (host address space)

memptr HWBase = 0x00f00000;	// HW base (Atari address space)
uint8 *HWBaseHost;	// HW base (host address space)
uint32 HWSize = 0x00100000;    // Size of HW space

#ifdef FIXED_VIDEORAM
memptr VideoRAMBase = ARANYMVRAMSTART;  // VideoRAM base (Atari address space)
#else
memptr VideoRAMBase;                    // VideoRAM base (Atari address space)
#endif
uint8 *VideoRAMBaseHost;// VideoRAM base (host address space)

static char const progname[] = "m68kdisasm";

static struct {
	const char *name;
	enum m68k_cpu cpu;
} const cpu_names[] = {
	{ "68000", CPU_68000 },
	{ "68008", CPU_68008 },
	{ "68010", CPU_68010 },
	{ "68020", CPU_68020 },
	{ "68030", CPU_68030 },
	{ "68040", CPU_68040 },
	{ "68060", CPU_68060 },
	{ "cpu32", CPU_CPU32 },
	{ "68302", CPU_68302 },
	{ "68331", CPU_68331 },
	{ "68332", CPU_68332 },
	{ "cf5200", CPU_5200 },
	{ "cf5202", CPU_5202 },
	{ "cf5204", CPU_5204 },
	{ "cf5206", CPU_5206 },
	{ "cf5206e", CPU_5206e },
	{ "cf5207", CPU_5207 },
	{ "cf5208", CPU_5208 },
	{ "cf521x", CPU_521x },
	{ "cf5249", CPU_5249 },
	{ "cf528x", CPU_528x },
	{ "cf5307", CPU_5307 },
	{ "cf537x", CPU_537x },
	{ "cf5407", CPU_5407 },
	{ "cf547x", CPU_547x },
	{ "cf548x", CPU_548x },
	{ "cfv4", CPU_CFV4 },
	{ "cfv4e", CPU_CFV4e },
	{ "v4e", CPU_CFV4e },
	{ "5475", CPU_CFV4e },
	{ NULL, CPU_AUTO }
};

static bool arg_to_cpu(const char *val, enum m68k_cpu *cpu)
{
	size_t i;
	
	for (i = 0; cpu_names[i].name != NULL; i++)
		if (strcasecmp(val, cpu_names[i].name) == 0)
		{
			*cpu = cpu_names[i].cpu;
			return true;
		}
	return false;
}


static void fatal(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputs("\n", stderr);
	exit(1);
}



#define OPTION_VMA 256
#define OPTION_CPU 257

static struct option const longopts[] = {
	{ "vma", required_argument, NULL, OPTION_VMA },
	{ "output", required_argument, NULL, 'o' },
	{ "cpu", required_argument, NULL, OPTION_CPU },
	{ NULL, no_argument, NULL, 0 }
};


static void print_char(FILE *out, unsigned char c, int last)
{
	if (c >= 20 && c < 0x7f)
		fprintf(out, "'%c'", c);
	else
		fprintf(out, "%u", c);
	if (last)
		fputc('\n', out);
	else
		fputc(',', out);
}


#include <signal.h>
static void handler(int sig)
{
	(void) sig;
	exit(2);
}


#define GETUB(a) ((uae_u8)phys_get_byte(a))
#define GETUW(a) ((uae_u16)phys_get_word(a))
#define GETW(a) ((uae_s16)phys_get_word(a))
#define GETL(a) ((uae_s32)phys_get_long(a))
#define GETUL(a) ((uae_u32)phys_get_long(a))

#define PUTUL(a, v) (phys_put_long(a, v))

static enum m68k_cpu cpu;

bool InitMEM() {
	InitMEMBaseDiff(RAMBaseHost, RAMBase);
	InitROMBaseDiff(ROMBaseHost, ROMBase);
	InitFastRAMBaseDiff(FastRAMBaseHost, FastRAMBase);
	InitVMEMBaseDiff(VideoRAMBaseHost, VideoRAMBase);
	return true;
}

uae_u32 HWget_l (uaecptr addr) {
	BUS_ERROR(addr);
	return 0;
}

uae_u16 HWget_w (uaecptr addr) {
	BUS_ERROR(addr);
	return 0;
}

uae_u8 HWget_b (uaecptr addr) {
	BUS_ERROR(addr);
	return 0;
}

void HWput_l (uaecptr addr, uae_u32 value) {
	(void) value;
	BUS_ERROR(addr);
}

void HWput_w (uaecptr addr, uae_u16 value) {
	(void) value;
	BUS_ERROR(addr);
}

void HWput_b (uaecptr addr, uae_u8 value) {
	(void) value;
	BUS_ERROR(addr);
}


void MakeFromSR (void)
{
    regs.t1 = (regs.sr >> 15) & 1;
    regs.t0 = (regs.sr >> 14) & 1;
    regs.s = (regs.sr >> 13) & 1;
    // mmu_set_super(regs.s);
    regs.m = (regs.sr >> 12) & 1;
    regs.intmask = (regs.sr >> 8) & 7;
    SET_XFLG ((regs.sr >> 4) & 1);
    SET_NFLG ((regs.sr >> 3) & 1);
    SET_ZFLG ((regs.sr >> 2) & 1);
    SET_VFLG ((regs.sr >> 1) & 1);
    SET_CFLG (regs.sr & 1);
}


int main(int argc, char **argv)
{
	char buf[256];
	FILE *fp;
	VOLATILE uintptr len;
	int isize;
	memptr vma = 0;
	char *VOLATILE outname = 0;
	FILE *VOLATILE out;
	int c;
	int VOLATILE retval = 0;
	uint8 *dst;
	
	regs.sr = 0x2700;
	MakeFromSR();
	
	/*
	 * we always want disassembly for at least '40, because the BIOS contains
	 * opcodes for it.
	 */
	cpu = CPU_68040;

	while ((c = getopt_long_only(argc, argv, "o", longopts, NULL)) != EOF)
	{
		switch (c)
		{
		case 'o':
			outname = optarg;
			break;
		case OPTION_VMA:
			vma = strtoul(optarg, NULL, 0);
			break;
		case OPTION_CPU:
			if (!arg_to_cpu(optarg, &cpu))
			{
				fatal("unknown cpu type %s in %s", optarg, "command line");
				return 1;
			}
			break;
		}
	}
	
	if ((argc - optind) != 1)
	{
		fprintf(stderr, "%s: invalid number of arguments\n", progname);
		return 1;
	}
	
	vm_init();
	signal(SIGSEGV, handler);
	
	if (ROMBaseHost == NULL) {
		if ((RAMBaseHost = (uint8 *)malloc(RAMSize + ROMSize + HWSize + FastRAMSize)) == NULL) {
			fatal("Not enough free memory.");
			exit(1);
		}
		ROMBaseHost = (uint8 *)(RAMBaseHost + ROMBase);
		HWBaseHost = (uint8 *)(RAMBaseHost + HWBase);
		FastRAMBaseHost = (uint8 *)(RAMBaseHost + FastRAMBase);
	}

	if (!InitMEM())
		exit(1);

	fp = fopen(argv[optind], "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "%s: %s: %s\n", progname, argv[optind], strerror(errno));
		return 1;
	}
	
	vma = vma & 0xffffff;
	len = STRAM_END + ROMSize - vma;
	{
		SAVE_EXCEPTION;
		{
			TRY(prb)
			{
				dst = phys_get_real_address(vma);
				len = fread(dst, 1, len, fp);
			} CATCH(prb)
			{
				fprintf(stderr, "got M68K exception %d%s\n", int(prb), int(prb) == 2 ? " (Bus Error)" : "");
				exit(1);
			}
		}
		RESTORE_EXCEPTION;
	}
	fclose(fp);

	if (outname)
	{
		out = fopen(outname, "w");
		if (out == NULL)
		{
			fprintf(stderr, "%s: %s: %s\n", progname, outname, strerror(errno));
			return 1;
		}
	} else
	{
		out = stdout;
	}
	m68k_disasm_init(&disasm_info, cpu);

	{
		SAVE_EXCEPTION;
		{
			TRY(prb)
			{
				if (len > 540 && GETUW(vma) == 100 && GETUW(vma + 512) == 0x601a)
				{
					unsigned int i;
					
					fprintf(out, "; CPX magic = 0x%04x\n", GETUW(vma));
					fprintf(out, "; CPX flags = 0x%04x\n", GETUW(vma + 2));
					fprintf(out, "; CPX id = 0x%08x\n", GETUL(vma + 4));
					fprintf(out, "; CPX version = 0x%04x\n", GETUW(vma + 8));
					fprintf(out, "; CPX i_text = ");
					for (i = 0; i < 14; i++)
						print_char(out, GETUB(vma + 10 + i), i == 13);
					fprintf(out, "; CPX sm_icon = ");
					for (i = 0; i < 48; i++)
						fprintf(out, "0x%04x%c", GETUW(vma + 24 + 2 * i), i == 47 ? '\n' : ',');
					fprintf(out, "; CPX i_color = 0x%04x\n", GETUW(vma + 120));
					fprintf(out, "; CPX title_text = ");
					for (i = 0; i < 18; i++)
						print_char(out, GETUB(vma + 122 + i), i == 17);
					fprintf(out, "; CPX t_color = 0x%04x\n", GETUW(vma + 140));
					fprintf(out, "; CPX buffer = ");
					for (i = 0; i < 64; i++)
						fprintf(out, "0x%02x%c", GETUW(vma + 142 + i), i == 63 ? '\n' : ',');
					
					fprintf(out, "\n");
					vma += 512;
					len -= 512;
				}

				if (len > 28 && GETUW(vma) == 0x601a)
				{
					fprintf(out, "; ph_branch = 0x%04x\n", GETUW(vma));
					fprintf(out, "; ph_tlen = 0x%08x\n", GETUL(vma + 2));
					fprintf(out, "; ph_dlen = 0x%08x\n", GETUL(vma + 6));
					fprintf(out, "; ph_blen = 0x%08x\n", GETUL(vma + 10));
					fprintf(out, "; ph_slen = 0x%08x\n", GETUL(vma + 14));
					fprintf(out, "; ph_res1 = 0x%08x\n", GETUL(vma + 18));
					fprintf(out, "; ph_prgflags = 0x%08x\n", GETUL(vma + 22));
					fprintf(out, "; ph_absflag = 0x%04x\n", GETUW(vma + 26));
					if (GETUW(vma + 26) == 0)
					{
						memptr offset;
						memptr text_ptr;
						memptr rel_addr;
						memptr reloc_ptr, reloc_start;
						
						/* find start of relocation data */
						offset = GETUL(vma + 2) +
								 GETUL(vma + 6) +
								 GETUL(vma + 14) +
								 28;
						/* must be at least starting offset + terminating zero */
						if (offset + 5 < len)
						{
							reloc_ptr = reloc_start = vma + offset;
							offset = GETUL(reloc_ptr);
							reloc_ptr += 4;
							if (offset != 0)
							{
								text_ptr = vma + 28;
								rel_addr = text_ptr;
								text_ptr += offset;
								offset = 0;
								PUTUL(text_ptr, GETUL(text_ptr) + rel_addr);
								for (;;)
								{
									offset = GETUB(reloc_ptr);
									reloc_ptr++;
									if (offset == 0)
										break;
									if (offset == 1)
									{
										text_ptr += 254;
									} else
									{
										text_ptr += offset;
										PUTUL(text_ptr, GETUL(text_ptr) + rel_addr);
									}
								}
							}
							fprintf(out, "; relocations = 0x%08x\n", reloc_ptr - reloc_start);
						}
					}
					fprintf(out, "\n");

					if (len > 256 &&
						GETUL(vma + 18) == 0x4d694e54l && /* 'MiNT' extended exec header magic */
						((GETUL(vma + 28) == 0x283a001aL && GETUL(vma + 32) == 0x4efb48faL) ||   /* Original binutils */
						 (GETUL(vma + 28) == 0x203a001a && GETUL(vma + 32) == 0x4efb08faL)) &&   /* binutils >= 2.18-mint-20080209 */
						(GETUW(vma + 38) == 0x0108 ||  /* NMAGIC */
						 GETUW(vma + 38) == 0x010b))   /* ZMAGIC */
					{
						fprintf(out, "; a_info = 0x%04x\n", GETUW(vma + 36));
						fprintf(out, "; a_magic = 0x%04x\n", GETUW(vma + 38));
						fprintf(out, "; a_text = 0x%08x\n", GETUL(vma + 40));
						fprintf(out, "; a_data = 0x%08x\n", GETUL(vma + 44));
						fprintf(out, "; a_bss = 0x%08x\n", GETUL(vma + 48));
						fprintf(out, "; a_syms = 0x%08x\n", GETUL(vma + 52));
						fprintf(out, "; a_entry = 0x%08x (0x%08x)\n", GETUL(vma + 56), GETUL(vma + 56) + 28);
						fprintf(out, "; a_trsize = 0x%08x\n", GETUL(vma + 60));
						fprintf(out, "; a_drsize = 0x%08x\n", GETUL(vma + 64));
						fprintf(out, "\n");
						vma += 256;
						len -= 256;
					} else
					{
						vma += 28;
						len -= 28;
					}
					if (len > 72 && GETUL(vma + 0) == 0x70004afc)
					{
						uae_u32 i, funcs;
						memptr names;
						
						fprintf(out, "; slh_magic = 0x%08x\n", GETUL(vma + 0));
						fprintf(out, "; slh_name = 0x%08x\n", GETUL(vma + 4));
						fprintf(out, "; slh_version = %d\n", GETUL(vma + 8));
						fprintf(out, "; slh_flags = 0x%08x\n", GETUL(vma + 12));
						fprintf(out, "; slh_init = 0x%08x\n", GETUL(vma + 16));
						fprintf(out, "; slh_exit = 0x%08x\n", GETUL(vma + 20));
						fprintf(out, "; slh_open = 0x%08x\n", GETUL(vma + 24));
						fprintf(out, "; slh_close = 0x%08x\n", GETUL(vma + 28));
						names = GETUL(vma + 32);
						fprintf(out, "; slh_names = 0x%08x\n", names);
						funcs = GETUL(vma + 68);
						fprintf(out, "; slb_no_funcs = %d\n", funcs);
						vma += 72;
						for (i = 0; i < funcs && len >= 4; i++)
						{
							fprintf(out, "; slb_fn%d = 0x%08x", i, GETUL(vma + 0));
							if (names)
							{
								memptr name = GETUL(names + 4 * i);
								if (name)
									fprintf(out, " %s", (const char *)phys_get_real_address(name));
							}
							fprintf(out, "\n");
							vma += 4;
							len -= 4;
						}
						if (i != funcs)
							fprintf(out, "; SLB function table truncated\n");
						fprintf(out, "\n");
					}
				}
			} CATCH(prb)
			{
				fprintf(stderr, "got M68K exception %d%s: %08x\n", int(prb), int(prb) == 2 ? " (Bus Error)" : "", regs.mmu_fault_addr);
				exit(1);
			}
		}
		RESTORE_EXCEPTION;
	}
	
	disasm_info.memory_vma = vma;
	while (len > 0)
	{
		isize = m68k_disasm_to_buf(&disasm_info, buf, TRUE);
		fputs(buf, out);
		fputs("\n", out);
		if (isize < 0)
		{
			retval = 1;
			break;
		}
		if ((uintptr)isize > len)
			break;
		len -= isize;
	}
	fflush(out);
	if (out != stdout)
		fclose(out);
	
	return retval;
}
