/*
	ROM / OS loader, NetBSD/m68k

	ARAnyM (C) 2022 Thorsten Otto

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
#include "bootos_netbsd.h"
#include "aranym_exception.h"
#include "emul_op.h"

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"

#ifdef ENABLE_NETBSD

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif
#include "bootos_elf.h"

/*
 * Values for 'bootflags'.
 * Note: These must match the values NetBSD uses!
 */
#define	ATARI_68000	1		/* 68000 CPU			*/
#define	ATARI_68010	(1<<1)		/* 68010 CPU			*/
#define	ATARI_68020	(1<<2)		/* 68020 CPU			*/
#define	ATARI_68030	(1<<3)		/* 68030 CPU			*/
#define	ATARI_68040	(1<<4)		/* 68040 CPU			*/
#define	ATARI_68060	(1<<6)		/* 68060 CPU			*/
#define	ATARI_TT	(1L<<11)	/* This is a TT030		*/
#define	ATARI_FALCON	(1L<<12)	/* This is a Falcon		*/
#define	ATARI_HADES	(1L<<13)	/* This is a Hades		*/
#define	ATARI_MILAN	(1L<<14)	/* This is a Milan		*/

#define	ATARI_CLKBROKEN	(1<<16)		/* GEMDOS has faulty year base	*/

#define	ATARI_ANYCPU	(ATARI_68000|ATARI_68010|ATARI_68020|ATARI_68030|ATARI_68040|ATARI_68060)
#define	ATARI_ANYMACH	(ATARI_TT|ATARI_FALCON|ATARI_HADES|ATARI_MILAN)

/*
 * Definitions for boothowto
 * Note: These must match the values NetBSD uses!
 */
#define	RB_AUTOBOOT	0x00
#define	RB_ASKNAME	0x01
#define	RB_SINGLE	0x02
#define	RB_KDB		0x40


#ifdef PAGE_SIZE
	/*	Might have been defined by vm_param.h. already, indirectly included by sysdeps.h */ 
	#undef PAGE_SIZE
#endif
#define PAGE_SIZE 8192
/*--- Defines ---*/

#define MAXREAD_BLOCK_SIZE	(1<<20)	/* 1 MB */
#define KERNEL_START		PAGE_SIZE	/* Start address of kernel in Atari RAM */

#endif

/*	NetBSD/m68k loader class */

NetbsdBootOs::NetbsdBootOs(void) ARANYM_THROWS(AranymException)
{
	/* RESET + m68k boot */
	ROMBaseHost[0x0000] = 0x4e;		/* reset */
	ROMBaseHost[0x0001] = 0x70;
	ROMBaseHost[0x0002] = 0x4e;		/* jmp <abs.addr> */
	ROMBaseHost[0x0003] = 0xf9;
	/* jmp address will be filled in later */

	if (!halt_on_reboot)
	{
		/* set up a minimal OS for successful netbsd reboot */
		ROMBaseHost[0x0030] = 0x46;		/* move.w #$2700,sr */
		ROMBaseHost[0x0031] = 0xfc;
		ROMBaseHost[0x0032] = 0x27;
		ROMBaseHost[0x0033] = 0x00;
		ROMBaseHost[0x0034] = 0x4e;		/* reset */
		ROMBaseHost[0x0035] = 0x70;
		ROMBaseHost[0x0036] = M68K_EMUL_RESET >> 8;
		ROMBaseHost[0x0037] = M68K_EMUL_RESET & 0xff;
	} else
	{
		/* code that shuts ARAnyM down when netbsd tries to reboot */
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

NetbsdBootOs::~NetbsdBootOs(void)
{
	cleanup();
}

void NetbsdBootOs::reset(bool cold) ARANYM_THROWS(AranymException)
{
	init(cold);
}

/*--- Private functions ---*/

void NetbsdBootOs::cleanup(void)
{
	if (kernel != NULL)
	{
		free(kernel);
		kernel = NULL;
	}
}

void NetbsdBootOs::init(bool cold)
{
	kernel = NULL;
	kernel_length = 0;

	UNUSED(cold);
#ifdef ENABLE_NETBSD
	/* Load the kernel */
	kernel=loadFile(bx_options.netbsd.kernel, &kernel_length);
	if (kernel == NULL)
	{
		throw AranymException("ARAnyM NetBSD Error loading kernel '%s'", bx_options.netbsd.kernel);
	}

	memset(RAMBaseHost, 0, RAMSize);
	memset(FastRAMBaseHost, 0, FastRAMSize);

	/* Check the kernel */
	if (checkKernel() < 0)
	{
		cleanup();
		throw AranymException("ARAnyM NetBSD Error setting up kernel");
	}

	/* Kernel and ramdisk copied in Atari RAM, we can free it */
	cleanup();
#else
	throw AranymException("ARAnyM NetBSD not compiled in");
#endif /* ENABLE_NETBSD */
}

void *NetbsdBootOs::loadFile(const char *filename, unsigned long *length)
{
	void *buffer = NULL;

#ifdef ENABLE_NETBSD

#ifdef HAVE_LIBZ
	unsigned long unc_len;
	gzFile handle;
#else
	int handle;
#endif

	if (strlen(filename) == 0)
	{
		D(bug("netbsd: empty filename"));
		return NULL;
	}

	/* Try to open the file, libz takes care of non-gzipped files */
#ifdef HAVE_LIBZ
	handle = gzopen(filename, "rb");
	if (handle == NULL)
#else
	handle = open(filename, O_RDONLY);
	if (handle < 0)
#endif
	{
		D(bug("netbsd: unable to open %s", filename));	
		return NULL;
	}

#ifdef HAVE_LIBZ
	/* Search the length of the uncompressed stream */
	buffer = (char *)malloc(MAXREAD_BLOCK_SIZE);
	if (buffer == NULL)
	{
		D(bug("netbsd: unable to allocate %d bytes", MAXREAD_BLOCK_SIZE));
		gzclose(handle);
		return NULL;
	}

	*length = 0;
	unc_len = gzread(handle, buffer, MAXREAD_BLOCK_SIZE);
	while (unc_len > 0)
	{
		*length += unc_len;
		unc_len = gzread(handle, buffer, MAXREAD_BLOCK_SIZE);
	}
	// Avoid gzseek, it is often broken with LFS which we enable by
	// default
	gzrewind(handle);
	D(bug("netbsd: uncompressing '%s'", filename));
	D(bug("netbsd:  uncompressed length: %lu bytes", *length));

	free(buffer);
	buffer=NULL;
#else
	*length = lseek(handle, 0, SEEK_END);
	lseek(handle, 0, SEEK_SET); 	
#endif

	buffer = (char *)malloc(*length);
	if (buffer == NULL)
	{
		D(bug("netbsd: unable to allocate %ld bytes", *length));
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
#endif /* ENABLE_NETBSD */
	return buffer;
}

int NetbsdBootOs::checkKernel(void)
{
#ifdef ENABLE_NETBSD
    Elf32_Ehdr *kexec_elf;	/* header of kernel executable */
    Elf32_Phdr *kernel_phdrs;
	Elf32_Addr min_addr=0xffffffff, max_addr=0;
	Elf32_Addr kernel_size;
	Elf32_Addr mem_ptr;
	Elf32_Addr kernel_offset;
	Elf32_Addr symsize, symstart;
	int i;
	int debug_flag;
	int no_symbols;
	int stmem_only;
		
	bi.boothowto = RB_SINGLE;
	debug_flag = 0;
	no_symbols = 0;
	stmem_only = 0;
	
	/* NetBSD does not seem to have a way to pass arguments to the kernel,
	   but we handle a few that loadbsd does */
	char *ptr = bx_options.netbsd.args;
	while (*ptr != '\0')
	{
		while (*ptr == ' ')
			ptr++;
		if (*ptr == '-')
		{
			++ptr;
			switch (*ptr)
			{
			case 'a': /* autoboot: Boot up to multi-user mode. */
				bi.boothowto &= ~(RB_SINGLE);
				bi.boothowto |= RB_AUTOBOOT;
				break;
			case 'b': /* Ask for root device to use. */
				bi.boothowto |= RB_ASKNAME;
				break;
			case 'd': /* Enter kernel debugger. */
				bi.boothowto |= RB_KDB;
				break;
			case 'D': /* printout debug information while loading */
				debug_flag = 1;
				break;
			case 'N':
				no_symbols = 1; /* No symbols must be loaded. */
				break;
			case 's':
				stmem_only = 1;
				break;
			}
		}
		while (*ptr != '\0' && *ptr != ' ')
			ptr++;
	}
	
	kexec_elf = (Elf32_Ehdr *) kernel;
	if (memcmp(&kexec_elf->e_ident[EI_MAG0], ELFMAG, SELFMAG ) == 0)
	{
		if ((SDL_SwapBE16(kexec_elf->e_type) != ET_EXEC) || (SDL_SwapBE16(kexec_elf->e_machine) != EM_68K) ||
			(SDL_SwapBE32(kexec_elf->e_version) != EV_CURRENT))
		{
			bug("netbsd: Invalid ELF header contents in kernel");
			return -1;
		}
	}

	/*--- Copy the kernel at start of RAM ---*/

	/* Load the program headers */
	kernel_phdrs = (Elf32_Phdr *) (((char *) kexec_elf) + SDL_SwapBE32(kexec_elf->e_phoff));

    /* calculate the total required amount of memory */
	D(bug("netbsd: kexec_elf->e_phnum=0x%08x", SDL_SwapBE16(kexec_elf->e_phnum)));

	for (i = 0; i < SDL_SwapBE16(kexec_elf->e_phnum); i++)
	{
		if (debug_flag)
		{
			bug("netbsd: kernel_phdrs[%d].p_vaddr=0x%08x", i, SDL_SwapBE32(kernel_phdrs[i].p_vaddr));
			bug("netbsd: kernel_phdrs[%d].p_offset=0x%08x", i, SDL_SwapBE32(kernel_phdrs[i].p_offset));
			bug("netbsd: kernel_phdrs[%d].p_filesz=0x%08x", i, SDL_SwapBE32(kernel_phdrs[i].p_filesz));
			bug("netbsd: kernel_phdrs[%d].p_memsz=0x%08x", i, SDL_SwapBE32(kernel_phdrs[i].p_memsz));
		}
		
		if (min_addr > SDL_SwapBE32(kernel_phdrs[i].p_vaddr))
		{
			min_addr = SDL_SwapBE32(kernel_phdrs[i].p_vaddr);
		}
		if (max_addr < SDL_SwapBE32(kernel_phdrs[i].p_vaddr) + SDL_SwapBE32(kernel_phdrs[i].p_memsz))
		{
			max_addr = SDL_SwapBE32(kernel_phdrs[i].p_vaddr) + SDL_SwapBE32(kernel_phdrs[i].p_memsz);
		}
	}

	kernel_size = max_addr - min_addr;
	D(bug("netbsd: kernel_size=%u", kernel_size));

	/*
	 * look for symbols and calculate the size
	 * XXX: This increases the load time by a factor 2 for gzipped
	 *      images!
	 */
	symsize = 0;
	symstart = 0;
	if (!no_symbols)
	{
    	Elf32_Shdr *shdr = (Elf32_Shdr *) ((char *) kexec_elf + SDL_SwapBE32(kexec_elf->e_shoff));
		for (i = 0; i < SDL_SwapBE16(kexec_elf->e_shnum); i++)
		{
			if (SDL_SwapBE32(shdr[i].sh_type) == SHT_SYMTAB || SDL_SwapBE32(shdr[i].sh_type) == SHT_STRTAB)
			{
				if (shdr[i].sh_offset != 0)
				{
					Elf32_Addr size = SDL_SwapBE32(shdr[i].sh_size);
					size = (size + 3) & -4;
					symsize += size;
				}
			}
	    }
	}

	if (symsize != 0)
	{
		symstart = kernel_size;
		kernel_size += symsize + sizeof(*kexec_elf) + SDL_SwapBE16(kexec_elf->e_shnum) * sizeof(Elf32_Shdr);
	}

	/*
	 * Note: the kernel copies itself to ST-RAM upon startup,
	 * so there is no point in loading it into FastRAM first
	 */
	if (KERNEL_START + kernel_size > RAMSize)
	{
		bug("netbsd: kernel of size %x does not fit in RAM of size %x", kernel_size, RAMSize);
		return -1;
	}

	kernel_offset = 0;
	mem_ptr = KERNEL_START;
	for (i = 0; i < SDL_SwapBE16(kexec_elf->e_phnum); i++)
	{
		Elf32_Word segment_length;
		Elf32_Addr segment_ptr;
		Elf32_Off segment_offset;

		segment_offset = SDL_SwapBE32(kernel_phdrs[i].p_offset);
		segment_length = SDL_SwapBE32(kernel_phdrs[i].p_filesz);

		if (segment_offset == 0xffffffffUL)
		{
		    bug("netbsd: Failed to seek to segment %d",i);
			return -1;
		}
		segment_ptr = SDL_SwapBE32(kernel_phdrs[i].p_vaddr);
		if (debug_flag)
		{
			bug("netbsd: Copying segment %d: 0x%08x,0x%08x to 0x%08x-0x%08x", i, segment_offset, segment_length, kernel_offset + mem_ptr + segment_ptr, kernel_offset + mem_ptr + segment_ptr + segment_length);
		}
		memcpy(RAMBaseHost + mem_ptr + segment_ptr, (char *) kexec_elf + segment_offset, segment_length);
	}

	/*
	 * Read symbols and strings
	 */
	if (symsize != 0)
	{
		unsigned char *p, *symtab;
		int nhdrs;
		Elf32_Shdr *shp;
		Elf32_Addr size;
		
		symtab = RAMBaseHost + KERNEL_START + symstart;

		p = symtab + sizeof(*kexec_elf);
		nhdrs = SDL_SwapBE16(kexec_elf->e_shnum);
		memcpy(p, (char *) kexec_elf + SDL_SwapBE32(kexec_elf->e_shoff), nhdrs * sizeof(*shp));
		shp = (Elf32_Shdr*)p;
		p += nhdrs * sizeof(*shp);
		for (i = 0; i < nhdrs; i++)
		{
			if (SDL_SwapBE32(shp[i].sh_type) == SHT_SYMTAB || SDL_SwapBE32(shp[i].sh_type) == SHT_STRTAB)
			{
				if (shp[i].sh_offset != 0)
				{
					/* Get the symbol table. */
					size = SDL_SwapBE32(shp[i].sh_size);
					if (debug_flag)
					{
						memptr pmem = (memptr)(p - RAMBaseHost);
						bug("netbsd: Copying %s: 0x%08x,0x%08x to 0x%08x-0x%08x", SDL_SwapBE32(shp[i].sh_type) == SHT_SYMTAB ? "symbol table" : "symbol strings",
							SDL_SwapBE32(shp[i].sh_offset), size, pmem, pmem + SDL_SwapBE32(shp[i].sh_size));
					}
					memcpy(p, (char *) kexec_elf + SDL_SwapBE32(shp[i].sh_offset), size);
					shp[i].sh_offset = SDL_SwapBE32(p - symtab);
					size = (size + 3) & -4;
					p += size;
				}
			}
		}
		kexec_elf->e_shoff = SDL_SwapBE32(sizeof(*kexec_elf));
		memcpy(symtab, kexec_elf, sizeof(*kexec_elf));
	}

	/*--- Create the bootinfo structure ---*/

	/* Machine type, memory banks */
	bi.kp = kernel_offset + KERNEL_START;
	bi.ksize = kernel_size;
	bi.entry = SDL_SwapBE32(kexec_elf->e_entry);
	bi.stmem_size = RAMSize;
	bi.ttmem_size = stmem_only ? 0 : FastRAMSize;
	bi.ttmem_start = FastRAMBase;
	bi.bootflags = ATARI_68040 | ATARI_FALCON;
	bi.esym_loc = symsize ? symstart : 0;

	if (debug_flag)
	{
	    bug("Machine info:");
	    bug("ST-RAM size\t: %10d bytes", bi.stmem_size);
	    bug("TT-RAM size\t: %10d bytes", bi.ttmem_size);
	    bug("TT-RAM start\t: 0x%08x", bi.ttmem_start);
	    bug("Cpu-type\t: 0x%08x", bi.bootflags);
	    bug("Kernel loadaddr\t: 0x%08x", bi.kp);
	    bug("Kernel size\t: %10d (0x%x) bytes", bi.ksize, bi.ksize);
	    bug("Kernel entry\t: 0x%08x", bi.entry);
	    bug("Kernel esym\t: 0x%08x", bi.esym_loc);
	}

	/*--- Init SP & PC ---*/
	uint32 *tmp = (uint32 *)RAMBaseHost;
	tmp[0] = SDL_SwapBE32(kernel_offset + KERNEL_START);	/* SP */
	tmp[1] = SDL_SwapBE32(0x00e00000);		/* PC = ROMBase */

	/* fill in the jmp address at start of ROM, to the kernel entry point */
	tmp = (uint32 *)ROMBaseHost;
	tmp[1] = SDL_SwapBE32(bi.kp + bi.entry);
	
	/*
	 * the BSD kernel wants values into the following registers:
	 * d0:  ttmem-size
	 * d1:  stmem-size
	 * d2:  cputype
	 * d3:  boothowto
	 * d4:  length of loaded kernel
	 * d5:  start of fastram
	 * a0:  start of loaded kernel
	 * a1:  end of symbols (esym)
	 * All other registers zeroed for possible future requirements.
	 */
	regs.regs[0] = bi.ttmem_size;
	regs.regs[1] = bi.stmem_size;
	regs.regs[2] = bi.bootflags;
	regs.regs[3] = bi.boothowto;
	regs.regs[4] = bi.ksize;
	regs.regs[5] = bi.ttmem_start;
	regs.regs[6] = 0;
	regs.regs[7] = 0;
	regs.regs[8] = bi.kp;
	regs.regs[9] = bi.esym_loc;
	regs.regs[10] = 0;
	regs.regs[11] = 0;
	regs.regs[12] = 0;
	regs.regs[13] = 0;
	regs.regs[14] = 0;

	/* beware: make sure those register values remain until Start680x0() is called */
	
	D(bug("netbsd: ok"));

	return 0;
#else
	return -1;
#endif /* ENABLE_NETBSD */
}
