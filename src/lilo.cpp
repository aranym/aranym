/*
 *  lilo_k.cpp - Linux kernel loader, part dependent of linux kernel includes
 *
 *  ARAnyM (C) 2003 Patrice Mandin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
	Code mainly taken from the Atari Linux loader, ataboot
*/

/*--- Includes ---*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <SDL_endian.h>
#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

/* linux specific include files */
#include <linux/elf.h>
#include <asm-m68k/bootinfo.h>
#define __KERNEL__
#include <asm-m68k/setup.h>
#undef __KERNEL__

#include "sysdeps.h"
#include "cpu_emulation.h"

#define DEBUG 1
#include "debug.h"

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

struct atari_bootinfo {
    unsigned long machtype;		/* machine type */
    unsigned long cputype;		/* system CPU */
    unsigned long fputype;		/* system FPU */
    unsigned long mmutype;		/* system MMU */
    int num_memory;				/* # of memory blocks found */
    struct mem_info memory[NUM_MEMINFO];  /* memory description */
    struct mem_info ramdisk;	/* ramdisk description */
    char command_line[CL_SIZE];	/* kernel command line parameters */
    unsigned long mch_cookie;	/* _MCH cookie from TOS */
    unsigned long mch_type;		/* special machine types */
};

/*--- Variables ---*/

bool lilo_ready=false;

static void *kernel;
static unsigned long kernel_length;
static void *ramdisk;
static unsigned long ramdisk_length;

static struct atari_bootinfo bi;
static unsigned long bi_size;
static union {
	struct bi_record record;
	unsigned char fake[MAX_BI_SIZE];
} bi_union;

/*--- Functions prototypes ---*/

static void *LoadFile(char *filename, unsigned long *length);
static void LiloFree(void);
static int LiloCheckKernel(
	void *kernel, unsigned long kernel_length,
	void *ramdisk, unsigned long ramdisk_length);

static int create_bootinfo( void);
static int add_bi_record(unsigned short tag, unsigned short size, const void *data);
static int add_bi_string(unsigned short tag, const char *s);

/*--- Functions ---*/

bool LiloInit(void)
{
	D(bug("lilo: init called"));

	kernel=ramdisk=NULL;
	kernel_length=ramdisk_length=0;
	lilo_ready=false;

	/* Load the kernel */
	kernel=LoadFile(bx_options.lilo.kernel, &kernel_length);
	if (kernel==NULL) {
		D(bug("lilo: can not load kernel image"));
		return false;
	}

	/* Load the ramdisk */
	ramdisk=LoadFile(bx_options.lilo.ramdisk, &ramdisk_length);
	if (ramdisk==NULL) {
		D(bug("lilo: can not load ramdisk (maybe useless)"));
	}

	memset(RAMBaseHost, 0, RAMSize);
	memset(FastRAMBaseHost, 0, FastRAMSize);

	/* Check the kernel */
	if (LiloCheckKernel(kernel,kernel_length,ramdisk,ramdisk_length)<0) {
		LiloFree();
		return false;
	}

	/* Kernel and ramdisk copied in Atari RAM, we can free it */
	LiloFree();

	lilo_ready=true;
	return true;
}

static void LiloFree(void)
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

void LiloShutdown(void)
{
	D(bug("lilo: shutdown called"));

#if 0
	{
		int i;

		for (i=0; i<16; i++) {
			unsigned long *tmp;

			tmp = (unsigned long *)(((unsigned char *)FastRAMBaseHost) /*+ 0x14a000*/ + 512);
			D(bug("lilo: ramdisk[%d]=0x%08x",i, SDL_SwapBE32(tmp[i])));
		}
	}
#endif

	LiloFree();
}

static void *LoadFile(char *filename, unsigned long *length)
{
	void *buffer;
#ifdef ENABLE_ZLIB
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
#ifdef ENABLE_ZLIB
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

#ifdef ENABLE_ZLIB
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
	gzseek(handle, 0, SEEK_SET); 	
	D(bug("lilo: uncompressing '%s'", filename));
	D(bug("lilo:  uncompressed length: %d bytes", *length));

	free(buffer);
	buffer=NULL;
#else
	*length = lseek(handle, 0, SEEK_END);
	lseek(handle, 0, SEEK_SET); 	
#endif

	buffer = (char *)malloc(*length);
	if (buffer==NULL) {
		D(bug("lilo: unable to allocate %d bytes", *length));
#ifdef ENABLE_ZLIB
		gzclose(handle);
#else
		close(handle);
#endif
		return NULL;
	}

#ifdef ENABLE_ZLIB
	gzread(handle, buffer, *length);
	gzclose(handle);
#else
	read(handle, buffer, *length);
	close(handle);
#endif

	return buffer;
}

int LiloCheckKernel(
	void *kernel, unsigned long kernel_length,
	void *ramdisk, unsigned long ramdisk_length)
{
    Elf32_Ehdr *kexec_elf;	/* header of kernel executable */
    Elf32_Phdr *kernel_phdrs;
	unsigned long min_addr=0xffffffff, max_addr=0;
	unsigned long kernel_size, memptr;
	int i;
	char *kname, *kernel_name="vmlinux";

	kexec_elf = (Elf32_Ehdr *) kernel;
    if (memcmp( (void *)(kexec_elf->e_ident[EI_MAG0]), ELFMAG, SELFMAG ) == 0) {
		if ((SDL_SwapBE16(kexec_elf->e_type) != ET_EXEC) || (SDL_SwapBE16(kexec_elf->e_machine) != EM_68K) ||
	    	(SDL_SwapBE32(kexec_elf->e_version) != EV_CURRENT)) {
		    fprintf(stderr, "lilo: Invalid ELF header contents in kernel\n");
		}
		return -1;
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
	D(bug("lilo: kernel_size=%lu",kernel_size));

	memptr = KERNEL_START;
	for (i=0; i<SDL_SwapBE16(kexec_elf->e_phnum); i++) {
		unsigned long segment_length;
		unsigned long segment_ptr;
		unsigned long segment_offset;

		segment_offset = SDL_SwapBE32(kernel_phdrs[i].p_offset);
		segment_length = SDL_SwapBE32(kernel_phdrs[i].p_filesz);

		if (segment_offset == 0xffffffffUL) {
		    fprintf(stderr, "lilo: Failed to seek to segment %d\n",i);
			return -1;
		}
		segment_ptr =  SDL_SwapBE32(kernel_phdrs[i].p_vaddr)-PAGE_SIZE;

		memcpy(((char *)RAMBaseHost) + memptr + segment_ptr, ((char *) kexec_elf) + segment_offset, segment_length);

	    D(bug("lilo: Copied segment %d: 0x%08x,0x%08x at 0x%08x",i,segment_offset,segment_length,memptr+segment_ptr));
	}

	/*--- Copy the ramdisk after kernel (and reserved bootinfo) ---*/
	if (ramdisk && ramdisk_length) {
		unsigned long rd_start;
		unsigned long rd_len;

		rd_len = ramdisk_length - RAMDISK_FS_START;
		if (FastRAMSize>rd_len) {
			/* Load in FastRAM */
			rd_start = FastRAMBase;
			memcpy(FastRAMBaseHost, ((unsigned char *)ramdisk) + RAMDISK_FS_START, rd_len);
		} else {
			/* Load in ST-RAM */
/*			rd_start = RAMSize - rd_len;*/
			rd_start = KERNEL_START + kernel_size + MAX_BI_SIZE;
			rd_start |= PAGE_SIZE-1;	/* Align on page size boundary */
			rd_start++;
			memcpy(RAMBaseHost+rd_start, ((unsigned char *)ramdisk) + RAMDISK_FS_START, rd_len);
		}

		bi.ramdisk.addr = SDL_SwapBE32(rd_start);
		bi.ramdisk.size = SDL_SwapBE32(rd_len);
	    D(bug("lilo: Ramdisk at 0x%08x in RAM, length=0x%08x", rd_start, rd_len));

#if 0
		for (i=0; i<16; i++) {
			unsigned long *tmp;

			tmp = (unsigned long *)(((unsigned char *)FastRAMBaseHost) /*+ rd_start*/ + 512);
			D(bug("lilo: ramdisk[%d]=0x%08x",i, SDL_SwapBE32(tmp[i])));
		}
#endif
	} else {
		bi.ramdisk.addr = 0;
		bi.ramdisk.size = 0;
	    D(bug("lilo: No ramdisk"));
	}

	/*--- Create the bootinfo structure ---*/

	/* Command line */
	strcpy(bi.command_line, bx_options.lilo.args);
	kname = kernel_name;
    if (strncmp( kernel_name, "local:", 6 ) == 0) {
		kname += 6;
	}
    if (strlen(kname)+12 < CL_SIZE-1) {
		if (*bi.command_line) {
			strcat( bi.command_line, " " );
		}
		strcat( bi.command_line, "BOOT_IMAGE=" );
		strcat( bi.command_line, kname );
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
	ADD_CHUNK(0, RAMSize);
	if (FastRAMSize>0) {
		ADD_CHUNK(FastRAMBase, FastRAMSize);
	}
	bi.num_memory=SDL_SwapBE32(bi.num_memory);

	if (!create_bootinfo()) {
	    fprintf(stderr, "lilo: Can not create bootinfo structure\n");
		return -1;
	}

	/*--- Copy boot info in RAM ---*/
	memcpy(RAMBaseHost + KERNEL_START + kernel_size, &bi_union.record, bi_size);
    D(bug("lilo: bootinfo at 0x%08x",KERNEL_START + kernel_size));

	for (i=0; i<16; i++) {
		unsigned long *tmp;

		tmp = (unsigned long *)(((unsigned char *)RAMBaseHost) + KERNEL_START + kernel_size);
		D(bug("lilo: bi_union.record[%d]=0x%08x",i, SDL_SwapBE32(tmp[i])));
	}

	/*--- Init SP & PC ---*/
	{
		unsigned long *tmp;

		tmp = (unsigned long *)RAMBaseHost;
		tmp[0] = SDL_SwapBE32(KERNEL_START);	/* SP */
		tmp[1] = SDL_SwapBE32(KERNEL_START);	/* PC */
	}
	
	D(bug("lilo: ok"));
	return 0;
}

    /*
     *  Create the Bootinfo Structure
     */

static int create_bootinfo(void)
{
	int i;
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
    record = (struct bi_record *)((u_long)&bi_union.record+bi_size);
    record->tag = SDL_SwapBE16(BI_LAST);
    bi_size += sizeof(bi_union.record.tag);

    return(1);
}

    /*
     *  Add a Record to the Bootinfo Structure
     */

static int add_bi_record( unsigned short tag, unsigned short size, const void *data)
{
    struct bi_record *record;
    u_short size2;

    size2 = (sizeof(struct bi_record)+size+3)&-4;
    if (bi_size+size2+sizeof(bi_union.record.tag) > MAX_BI_SIZE) {
	fprintf (stderr, "Can't add bootinfo record. Ask a wizard to enlarge me.\n");
	return(0);
    }
    record = (struct bi_record *)((u_long)&bi_union.record+bi_size);
    record->tag = SDL_SwapBE16(tag);
    record->size = SDL_SwapBE16(size2);
    memcpy(record->data, data, size);
    bi_size += size2;
    return(1);
}

    /*
     *  Add a String Record to the Bootinfo Structure
     */

static int add_bi_string(unsigned short tag, const char *s)
{
    return add_bi_record(tag, strlen(s)+1, (void *)s);
}
