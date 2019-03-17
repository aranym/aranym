/*
	ROM / OS loader, Linux/m68k

	ARAnyM (C) 2005-2008 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "sysdeps.h"
#include "cpu_emulation.h"
#include "bootos_linux.h"
#include "aranym_exception.h"
#include "emul_op.h"

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"

#ifdef ENABLE_LILO

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

/* linux specific include files */
//#include <elf.h>
#include <stdint.h>

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef	int64_t  Elf32_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;


/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  Elf32_Half	e_type;			/* Object file type */
  Elf32_Half	e_machine;		/* Architecture */
  Elf32_Word	e_version;		/* Object file version */
  Elf32_Addr	e_entry;		/* Entry point virtual address */
  Elf32_Off	e_phoff;		/* Program header table file offset */
  Elf32_Off	e_shoff;		/* Section header table file offset */
  Elf32_Word	e_flags;		/* Processor-specific flags */
  Elf32_Half	e_ehsize;		/* ELF header size in bytes */
  Elf32_Half	e_phentsize;		/* Program header table entry size */
  Elf32_Half	e_phnum;		/* Program header table entry count */
  Elf32_Half	e_shentsize;		/* Section header table entry size */
  Elf32_Half	e_shnum;		/* Section header table entry count */
  Elf32_Half	e_shstrndx;		/* Section header string table index */
} Elf32_Ehdr;

/* Program segment header.  */

typedef struct
{
  Elf32_Word	p_type;			/* Segment type */
  Elf32_Off	p_offset;		/* Segment file offset */
  Elf32_Addr	p_vaddr;		/* Segment virtual address */
  Elf32_Addr	p_paddr;		/* Segment physical address */
  Elf32_Word	p_filesz;		/* Segment size in file */
  Elf32_Word	p_memsz;		/* Segment size in memory */
  Elf32_Word	p_flags;		/* Segment flags */
  Elf32_Word	p_align;		/* Segment alignment */
} Elf32_Phdr;

#define EI_MAG0		0		/* File identification byte 0 index */
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4
#define ET_EXEC		2		/* Executable file */
#define EM_68K		 4		/* Motorola m68k family */
#define EV_CURRENT	1		/* Current version */

/* end of rip from elf.h */

/* #include <asm-m68k/bootinfo.h> */
struct bi_record {
    uint16 tag;			/* tag ID */
    uint16 size;		/* size of record (in bytes) */
    uint32 data[0];		/* data */
};

    /*
     *  Tag Definitions
     *
     *  Machine independent tags start counting from 0x0000
     *  Machine dependent tags start counting from 0x8000
     */

#define BI_LAST			0x0000	/* last record (sentinel) */
#define BI_MACHTYPE		0x0001	/* machine type (u_long) */
#define BI_CPUTYPE		0x0002	/* cpu type (u_long) */
#define BI_FPUTYPE		0x0003	/* fpu type (u_long) */
#define BI_MMUTYPE		0x0004	/* mmu type (u_long) */
#define BI_MEMCHUNK		0x0005	/* memory chunk address and size */
					/* (struct mem_info) */
#define BI_RAMDISK		0x0006	/* ramdisk address and size */
					/* (struct mem_info) */
#define BI_COMMAND_LINE		0x0007	/* kernel command line parameters */
					/* (string) */

    /*
     *  Atari-specific tags
     */

#define BI_ATARI_MCH_COOKIE	0x8000	/* _MCH cookie from TOS (u_long) */
#define BI_ATARI_MCH_TYPE	0x8001	/* special machine type (u_long) */

/* mch_type values */
#define ATARI_MACH_AB40		3	/* Afterburner040 on Falcon */

/* #include <asm-m68k/setup.h> */
    /*
     *  Linux/m68k Architectures
     */

#define MACH_ATARI    2

    /*
     *  CPU, FPU and MMU types
     *
     *  Note: we may rely on the following equalities:
     *
     *      CPU_68020 == MMU_68851
     *      CPU_68030 == MMU_68030
     *      CPU_68040 == FPU_68040 == MMU_68040
     *      CPU_68060 == FPU_68060 == MMU_68060
     */

#define CPUB_68040     2
#define CPU_68040      (1<<CPUB_68040)
#define FPUB_68040     2                       /* Internal FPU */
#define FPU_68040      (1<<FPUB_68040)
#define MMUB_68040     2                       /* Internal MMU */
#define MMU_68040      (1<<MMUB_68040)

#ifdef PAGE_SIZE
	/*	Might have been defined by vm_param.h. already, indirectly included by sysdeps.h */ 
	#undef PAGE_SIZE
#endif
#define PAGE_SIZE 4096
/*--- Defines ---*/

#define MAXREAD_BLOCK_SIZE	(1<<20)	/* 1 MB */
#define KERNEL_START		PAGE_SIZE	/* Start address of kernel in Atari RAM */
#define RAMDISK_FS_START	0		/* Offset to start of fs in ramdisk file */

#define MAX_BI_SIZE     (4096)

#define GRANULARITY (256*1024) /* min unit for memory */
#define ADD_CHUNK(start,siz)	\
    {	\
		unsigned long _start = (start);	\
		unsigned long _size  = (siz) & ~(GRANULARITY-1);	\
		\
		if (_size > 0) {	\
			bi.memory[bi.num_memory].addr = SDL_SwapBE32(_start);	\
			bi.memory[bi.num_memory].size = SDL_SwapBE32(_size);	\
			bi.num_memory++;	\
		}	\
	}

/*--- Structures ---*/

static union {
	struct bi_record record;
	unsigned char fake[MAX_BI_SIZE];
} bi_union;

#endif

/*	Linux/m68k loader class */

LinuxBootOs::LinuxBootOs(void) ARANYM_THROWS(AranymException)
{
	/* RESET + Linux/m68k boot */
	ROMBaseHost[0x0000] = 0x4e;		/* reset */
	ROMBaseHost[0x0001] = 0x70;
	ROMBaseHost[0x0002] = 0x4e;		/* jmp <abs.addr> */
	ROMBaseHost[0x0003] = 0xf9;

	if (!halt_on_reboot) {
		/* set up a minimal OS for successful Linux/m68k reboot */
		ROMBaseHost[0x0030] = 0x46;		/* move.w #$2700,sr */
		ROMBaseHost[0x0031] = 0xfc;
		ROMBaseHost[0x0032] = 0x27;
		ROMBaseHost[0x0033] = 0x00;
		ROMBaseHost[0x0034] = 0x4e;		/* reset */
		ROMBaseHost[0x0035] = 0x70;
		ROMBaseHost[0x0036] = M68K_EMUL_RESET >> 8;
		ROMBaseHost[0x0037] = M68K_EMUL_RESET & 0xff;
	}
	else {
		/* code that shuts ARAnyM down when Linux/m68k tries to reboot */
		ROMBaseHost[0x0030] = 0x48;		/* pea.l NF_SHUTDOWN(pc) */
		ROMBaseHost[0x0031] = 0x7a;
		ROMBaseHost[0x0032] = 0x00;
		ROMBaseHost[0x0033] = 0x0c;
		ROMBaseHost[0x0034] = 0x59;		/* subq.l #4,sp */
		ROMBaseHost[0x0035] = 0x8f;
		ROMBaseHost[0x0036] = 0x73;		/* NF_ID */
		ROMBaseHost[0x0037] = 0x00;
		ROMBaseHost[0x0038] = 0x2f;		/* move.l d0,-(sp) */
		ROMBaseHost[0x0039] = 0x00;
		ROMBaseHost[0x003a] = 0x59;		/* subq.l #4,sp */
		ROMBaseHost[0x003b] = 0x8f;
		ROMBaseHost[0x003c] = 0x73;		/* NF_CALL */
		ROMBaseHost[0x003d] = 0x01;
		ROMBaseHost[0x003e] = 'N';		/* "NF_SHUTDOWN" */
		ROMBaseHost[0x003f] = 'F';
		ROMBaseHost[0x0040] = '_';
		ROMBaseHost[0x0041] = 'S';
		ROMBaseHost[0x0042] = 'H';
		ROMBaseHost[0x0043] = 'U';
		ROMBaseHost[0x0044] = 'T';
		ROMBaseHost[0x0045] = 'D';
		ROMBaseHost[0x0046] = 'O';
		ROMBaseHost[0x0047] = 'W';
		ROMBaseHost[0x0048] = 'N';
		ROMBaseHost[0x0049] = 0;
	}
	init(true);
}

LinuxBootOs::~LinuxBootOs(void)
{
	cleanup();
}

void LinuxBootOs::reset(bool cold) ARANYM_THROWS(AranymException)
{
	init(cold);
}

/*--- Private functions ---*/

void LinuxBootOs::cleanup(void)
{
	if (kernel!=NULL) {
		free(kernel);
		kernel=NULL;
	}

	if (ramdisk!=NULL) {
		free(ramdisk);
		ramdisk=NULL;
	}
}

void LinuxBootOs::init(bool cold)
{
	kernel=ramdisk=NULL;
	kernel_length=ramdisk_length=0;
	bi_size = 0;
	bi.ramdisk.addr = 0;
	bi.ramdisk.size = 0;

	UNUSED(cold);
#ifdef ENABLE_LILO
	/* Load the kernel */
	kernel=loadFile(bx_options.lilo.kernel, &kernel_length);
	if (kernel==NULL) {
		throw AranymException("ARAnyM LILO: Error loading kernel '%s'", bx_options.lilo.kernel);
	}

	/* Load the ramdisk */
	if (strlen(bx_options.lilo.ramdisk) > 0) {
		ramdisk=loadFile(bx_options.lilo.ramdisk, &ramdisk_length);
		if (ramdisk==NULL) {
			infoprint("ARAnyM LILO: Error loading ramdisk '%s'", bx_options.lilo.ramdisk);
		}
	}

	memset(RAMBaseHost, 0, RAMSize);
	memset(FastRAMBaseHost, 0, FastRAMSize);

	/* Check the kernel */
	if (checkKernel()<0) {
		cleanup();
		throw AranymException("ARAnyM LILO: Error setting up kernel");
	}

	/* Kernel and ramdisk copied in Atari RAM, we can free it */
	cleanup();
#else
	throw AranymException("ARAnyM LILO: not compiled in");
#endif /* ENABLE_LILO */
}

void *LinuxBootOs::loadFile(const char *filename, unsigned long *length)
{
	void *buffer = NULL;

#ifdef ENABLE_LILO

#ifdef HAVE_LIBZ
	unsigned long unc_len;
	gzFile handle;
#else
	int handle;
#endif

	if (strlen(filename)==0) {
		D(bug("lilo: empty finename"));
		return NULL;
	}

	/* Try to open the file, libz takes care of non-gzipped files */
#ifdef HAVE_LIBZ
	handle = gzopen(filename, "rb");
	if (handle == NULL)
#else
	handle = open(filename, O_RDONLY);
	if (handle<0)
#endif
	{
		D(bug("lilo: unable to open %s", filename));	
		return NULL;
	}

#ifdef HAVE_LIBZ
	/* Search the length of the uncompressed stream */
	buffer = (char *)malloc(MAXREAD_BLOCK_SIZE);
	if (buffer==NULL) {
		D(bug("lilo: unable to allocate %d bytes", MAXREAD_BLOCK_SIZE));
		gzclose(handle);
		return NULL;
	}

	*length = 0;
	unc_len = gzread(handle, buffer, MAXREAD_BLOCK_SIZE);
	while (unc_len>0) {
		*length += unc_len;
		unc_len = gzread(handle, buffer, MAXREAD_BLOCK_SIZE);
	}
	// Avoid gzseek, it is often broken with LFS which we enable by
	// default
	gzrewind(handle);
	D(bug("lilo: uncompressing '%s'", filename));
	D(bug("lilo:  uncompressed length: %lu bytes", *length));

	free(buffer);
	buffer=NULL;
#else
	*length = lseek(handle, 0, SEEK_END);
	lseek(handle, 0, SEEK_SET); 	
#endif

	buffer = (char *)malloc(*length);
	if (buffer==NULL) {
		D(bug("lilo: unable to allocate %ld bytes", *length));
#ifdef HAVE_LIBZ
		gzclose(handle);
#else
		close(handle);
#endif
		return NULL;
	}

#ifdef HAVE_LIBZ
	gzread(handle, buffer, *length);
	gzclose(handle);
#else
	read(handle, buffer, *length);
	close(handle);
#endif

#else
	UNUSED(filename);
	UNUSED(length);
#endif /* ENABLE_LILO */
	return buffer;
}

int LinuxBootOs::checkKernel(void)
{
#ifdef ENABLE_LILO
    Elf32_Ehdr *kexec_elf;	/* header of kernel executable */
    Elf32_Phdr *kernel_phdrs;
	Elf32_Addr min_addr=0xffffffff, max_addr=0;
	Elf32_Addr kernel_size;
	Elf32_Addr mem_ptr;
	Elf32_Addr kernel_offset;
	int i;
	const char *kname, *kernel_name="vmlinux";
	bool load_to_fastram = bx_options.lilo.load_to_fastram && FastRAMSize > 0;

	kexec_elf = (Elf32_Ehdr *) kernel;
	if (memcmp( &kexec_elf->e_ident[EI_MAG0], ELFMAG, SELFMAG ) == 0) {
		if ((SDL_SwapBE16(kexec_elf->e_type) != ET_EXEC) || (SDL_SwapBE16(kexec_elf->e_machine) != EM_68K) ||
			(SDL_SwapBE32(kexec_elf->e_version) != EV_CURRENT)) {
			bug("lilo: Invalid ELF header contents in kernel");
			return -1;
		}
	}

	/*--- Copy the kernel at start of RAM ---*/

	/* Load the program headers */
	kernel_phdrs = (Elf32_Phdr *) (((char *) kexec_elf) + SDL_SwapBE32(kexec_elf->e_phoff));

    /* calculate the total required amount of memory */
	D(bug("lilo: kexec_elf->e_phnum=0x%08x",SDL_SwapBE16(kexec_elf->e_phnum)));

	for (i=0; i<SDL_SwapBE16(kexec_elf->e_phnum); i++) {
		D(bug("lilo: kernel_phdrs[%d].p_vaddr=0x%08x",i,SDL_SwapBE32(kernel_phdrs[i].p_vaddr)));
		D(bug("lilo: kernel_phdrs[%d].p_offset=0x%08x",i,SDL_SwapBE32(kernel_phdrs[i].p_offset)));
		D(bug("lilo: kernel_phdrs[%d].p_filesz=0x%08x",i,SDL_SwapBE32(kernel_phdrs[i].p_filesz)));
		D(bug("lilo: kernel_phdrs[%d].p_memsz=0x%08x",i,SDL_SwapBE32(kernel_phdrs[i].p_memsz)));

		if (min_addr > SDL_SwapBE32(kernel_phdrs[i].p_vaddr)) {
			min_addr = SDL_SwapBE32(kernel_phdrs[i].p_vaddr);
		}
		if (max_addr < SDL_SwapBE32(kernel_phdrs[i].p_vaddr) + SDL_SwapBE32(kernel_phdrs[i].p_memsz)) {
			max_addr = SDL_SwapBE32(kernel_phdrs[i].p_vaddr) + SDL_SwapBE32(kernel_phdrs[i].p_memsz);
		}
	}

	/* This is needed for newer linkers that include the header in
		the first segment.  */
	D(bug("lilo: min_addr=0x%08x",min_addr));
	D(bug("lilo: max_addr=0x%08x",max_addr));

	if (min_addr == 0) {
		D(bug("lilo: new linker"));
		D(bug("lilo:  kernel_phdrs[0].p_vaddr=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_vaddr)));
		D(bug("lilo:  kernel_phdrs[0].p_offset=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_offset)));
		D(bug("lilo:  kernel_phdrs[0].p_filesz=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_filesz)));
		D(bug("lilo:  kernel_phdrs[0].p_memsz=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_memsz)));

		min_addr = PAGE_SIZE;
		/*kernel_phdrs[0].p_vaddr += PAGE_SIZE;*/
		kernel_phdrs[0].p_vaddr = SDL_SwapBE32(SDL_SwapBE32(kernel_phdrs[0].p_vaddr) + PAGE_SIZE);
		/*kernel_phdrs[0].p_offset += PAGE_SIZE;*/
		kernel_phdrs[0].p_offset = SDL_SwapBE32(SDL_SwapBE32(kernel_phdrs[0].p_offset) + PAGE_SIZE);
		/*kernel_phdrs[0].p_filesz -= PAGE_SIZE;*/
		kernel_phdrs[0].p_filesz = SDL_SwapBE32(SDL_SwapBE32(kernel_phdrs[0].p_filesz) - PAGE_SIZE);
		/*kernel_phdrs[0].p_memsz -= PAGE_SIZE;*/
		kernel_phdrs[0].p_memsz = SDL_SwapBE32(SDL_SwapBE32(kernel_phdrs[0].p_memsz) - PAGE_SIZE);

		D(bug("lilo: modified to:"));
		D(bug("lilo:  kernel_phdrs[0].p_vaddr=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_vaddr)));
		D(bug("lilo:  kernel_phdrs[0].p_offset=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_offset)));
		D(bug("lilo:  kernel_phdrs[0].p_filesz=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_filesz)));
		D(bug("lilo:  kernel_phdrs[0].p_memsz=0x%08x",SDL_SwapBE32(kernel_phdrs[0].p_memsz)));
	}
	kernel_size = max_addr - min_addr;
	D(bug("lilo: kernel_size=%u",kernel_size));

	if (load_to_fastram)
	{
		if (KERNEL_START + kernel_size > FastRAMSize)
		{
			bug("lilo: kernel of size %x does not fit in TT-RAM of size %x", kernel_size, FastRAMSize);
			load_to_fastram = false;
		}
	}
	if (!load_to_fastram)
	{
		if (KERNEL_START + kernel_size > RAMSize)
		{
			bug("lilo: kernel of size %x does not fit in RAM of size %x", kernel_size, RAMSize);
			return -1;
		}
	}

	if (load_to_fastram)
		kernel_offset = FastRAMBase;
	else
		kernel_offset = 0;
	mem_ptr = KERNEL_START;
	for (i=0; i<SDL_SwapBE16(kexec_elf->e_phnum); i++) {
		Elf32_Word segment_length;
		Elf32_Addr segment_ptr;
		Elf32_Off segment_offset;

		segment_offset = SDL_SwapBE32(kernel_phdrs[i].p_offset);
		segment_length = SDL_SwapBE32(kernel_phdrs[i].p_filesz);

		if (segment_offset == 0xffffffffUL) {
		    bug("lilo: Failed to seek to segment %d",i);
			return -1;
		}
		segment_ptr = SDL_SwapBE32(kernel_phdrs[i].p_vaddr) - PAGE_SIZE;

		if (load_to_fastram)
			memcpy(FastRAMBaseHost + mem_ptr + segment_ptr, (char *) kexec_elf + segment_offset, segment_length);
		else
			memcpy(RAMBaseHost + mem_ptr + segment_ptr, (char *) kexec_elf + segment_offset, segment_length);

	    D(bug("lilo: Copied segment %d: 0x%08x,0x%08x at 0x%08x",i,segment_offset,segment_length,kernel_offset+mem_ptr+segment_ptr));
	}

	/*--- Copy the ramdisk after kernel (and reserved bootinfo) ---*/
	if (ramdisk && ramdisk_length) {
		Elf32_Addr rd_start;
		Elf32_Word rd_len;
		Elf32_Off rd_offset;

		if (load_to_fastram)
			rd_offset = KERNEL_START + kernel_size + MAX_BI_SIZE;
		else
			rd_offset = 0;
		rd_len = ramdisk_length - RAMDISK_FS_START;
		if (FastRAMSize > rd_offset + rd_len) {
			/* Load in FastRAM */
			rd_start = FastRAMBase + FastRAMSize - rd_len;
			memcpy(FastRAMBaseHost + rd_start - FastRAMBase, (unsigned char *)ramdisk + RAMDISK_FS_START, rd_len);
		} else {
			/* Load in ST-RAM */
			if (load_to_fastram)
				rd_offset = PAGE_SIZE;
			else
				rd_offset = KERNEL_START + kernel_size + MAX_BI_SIZE;
			if (RAMSize < rd_offset + rd_len) {
				bug("lilo: not enough memory to load ramdisk of size %u", rd_len);
				return -1;
			}
			rd_start = RAMSize - rd_len;
			memcpy(RAMBaseHost+rd_start, ((unsigned char *)ramdisk) + RAMDISK_FS_START, rd_len);
		}

		bi.ramdisk.addr = SDL_SwapBE32(rd_start);
		bi.ramdisk.size = SDL_SwapBE32(rd_len);
	    D(bug("lilo: Ramdisk at 0x%08x in RAM, length=0x%08x", rd_start, rd_len));
	} else {
		bi.ramdisk.addr = 0;
		bi.ramdisk.size = 0;
	    D(bug("lilo: No ramdisk"));
	}

	/*--- Create the bootinfo structure ---*/

	/* Command line */
	kname = kernel_name;
    if (strncmp( kernel_name, "local:", 6 ) == 0) {
		kname += 6;
	}
    if (strlen(bx_options.lilo.args) > CL_SIZE-1) {
		bug("lilo: kernel command line too long");
		return -1;
    }
	strcpy(bi.command_line, bx_options.lilo.args);
    if (strlen(bi.command_line)+1+strlen(kname)+12 < CL_SIZE-1) {
		if (*bi.command_line) {
			strcat( bi.command_line, " " );
		}
		strcat( bi.command_line, "BOOT_IMAGE=" );
		strcat( bi.command_line, kname );
    } else
    {
		bug("lilo: kernel command line too long to include kernel name");
    }

    D(bug("lilo: config_file command line: %s", bx_options.lilo.args ));
    D(bug("lilo: kernel command line: %s", bi.command_line ));

	/* Machine type, memory banks */
	bi.machtype = SDL_SwapBE32(MACH_ATARI);
	bi.cputype = SDL_SwapBE32(CPU_68040);
	bi.fputype = SDL_SwapBE32(FPU_68040);
	bi.mmutype = SDL_SwapBE32(MMU_68040);
	bi.mch_cookie = SDL_SwapBE32(0x00030000);
	bi.mch_type = SDL_SwapBE32(ATARI_MACH_AB40);

	bi.num_memory=0;
	/* If loading to FastRAM switch the order of ST and Fast RAM */
	if (!load_to_fastram)
		ADD_CHUNK(0, RAMSize);
	if (FastRAMSize>0) {
		ADD_CHUNK(FastRAMBase, FastRAMSize);
	}
	if (load_to_fastram)
		ADD_CHUNK(0, RAMSize);
	bi.num_memory=SDL_SwapBE32(bi.num_memory);

	if (!create_bootinfo()) {
	    bug("lilo: Can not create bootinfo structure");
		return -1;
	}

	/*--- Copy boot info in RAM ---*/
	if (load_to_fastram)
		memcpy(FastRAMBaseHost + KERNEL_START + kernel_size, &bi_union.record, bi_size);
	else
		memcpy(RAMBaseHost + KERNEL_START + kernel_size, &bi_union.record, bi_size);
	D(bug("lilo: bootinfo at 0x%08x", kernel_offset + KERNEL_START + kernel_size));

#if DEBUG
	for (i=0; i<16; i++) {
		uint32 *tmp;

		if (load_to_fastram)
			tmp = (uint32 *)((unsigned char *)FastRAMBaseHost + KERNEL_START + kernel_size);
		else
			tmp = (uint32 *)((unsigned char *)RAMBaseHost + KERNEL_START + kernel_size);
		D(bug("lilo: bi_union.record[%d]=0x%08x",i, SDL_SwapBE32(tmp[i])));
	}
#endif

	/*--- Init SP & PC ---*/
	uint32 *tmp = (uint32 *)RAMBaseHost;
	tmp[0] = SDL_SwapBE32(kernel_offset + KERNEL_START);	/* SP */
	tmp[1] = SDL_SwapBE32(0x00e00000);		/* PC = ROMBase */
	ROMBaseHost[4] = (kernel_offset + KERNEL_START) >> 24;
	ROMBaseHost[5] = (kernel_offset + KERNEL_START) >> 16;
	ROMBaseHost[6] = (kernel_offset + KERNEL_START) >>  8;
	ROMBaseHost[7] = (kernel_offset + KERNEL_START) & 0xff;
	
	D(bug("lilo: ok"));

	return 0;
#else
	return -1;
#endif /* ENABLE_LILO */
}

    /*
     *  Create the Bootinfo Structure
     */

int LinuxBootOs::create_bootinfo(void)
{
#ifdef ENABLE_LILO
	unsigned int i;
	struct bi_record *record;

	/* Initialization */
	bi_size = 0;

	/* Generic tags */
	if (!add_bi_record(BI_MACHTYPE, sizeof(bi.machtype), &bi.machtype))
		return(0);
	if (!add_bi_record(BI_CPUTYPE, sizeof(bi.cputype), &bi.cputype))
		return(0);
	if (!add_bi_record(BI_FPUTYPE, sizeof(bi.fputype), &bi.fputype))
		return(0);
	if (!add_bi_record(BI_MMUTYPE, sizeof(bi.mmutype), &bi.mmutype))
		return(0);
	for (i = 0; i < SDL_SwapBE32(bi.num_memory); i++) {
		if (!add_bi_record(BI_MEMCHUNK, sizeof(bi.memory[i]), &bi.memory[i]))
			return(0);
	}
	if (SDL_SwapBE32(bi.ramdisk.size)) {
		if (!add_bi_record(BI_RAMDISK, sizeof(bi.ramdisk), &bi.ramdisk))
			return(0);
	}
	if (!add_bi_string(BI_COMMAND_LINE, bi.command_line))
		return(0);

	/* Atari tags */
	if (!add_bi_record(BI_ATARI_MCH_COOKIE, sizeof(bi.mch_cookie), &bi.mch_cookie))
		return(0);
	if (!add_bi_record(BI_ATARI_MCH_TYPE, sizeof(bi.mch_type), &bi.mch_type))
		return(0);

    /* Trailer */
    record = (struct bi_record *)((char *)&bi_union.record+bi_size);
    record->tag = SDL_SwapBE16(BI_LAST);
    bi_size += sizeof(bi_union.record.tag);

    return(1);
#else
	return(0);
#endif /* ENABLE_LILO */
}

    /*
     *  Add a Record to the Bootinfo Structure
     */

int LinuxBootOs::add_bi_record( unsigned short tag, unsigned short size, const void *data)
{
#ifdef ENABLE_LILO
    struct bi_record *record;
    u_short size2;

    size2 = (sizeof(struct bi_record)+size+3)&-4;
    if (bi_size+size2+sizeof(bi_union.record.tag) > MAX_BI_SIZE) {
	bug("Can't add bootinfo record. Ask a wizard to enlarge me.");
	return(0);
    }
    record = (struct bi_record *)((char *)&bi_union.record+bi_size);
    record->tag = SDL_SwapBE16(tag);
    record->size = SDL_SwapBE16(size2);
    memcpy((char *)record + sizeof(struct bi_record), data, size);
    bi_size += size2;

#else
	UNUSED(tag);
	UNUSED(size);
	UNUSED(data);
#endif /* ENABLE_LILO */
    return(1);
}

    /*
     *  Add a String Record to the Bootinfo Structure
     */

int LinuxBootOs::add_bi_string(unsigned short tag, const char *s)
{
#ifdef ENABLE_LILO
    return add_bi_record(tag, strlen(s)+1, (void *)s);
#else
	UNUSED(tag);
	UNUSED(s);
	return 0;
#endif /* ENABLE_LILO */
}
