/*
	ROM / OS loader, Linux/m68k

	Copyright (c) 2005-2006 Patrice Mandin

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

#ifndef BOOTOSLINUX_H
#define BOOTOSLINUX_H

#include "aranym_exception.h"
#include "bootos.h"

/*--- Some defines ---*/

#define NUM_MEMINFO	4
#define CL_SIZE		256

/* Linux/m68k loader class */

class LinuxBootOs : public BootOs
{
	private:
		struct mem_info {
			uint32 addr;		/* physical address of memory chunk */
			uint32 size;		/* length of memory chunk (in bytes) */
		};

		struct atari_bootinfo {
		    uint32 machtype;		/* machine type */
		    uint32 cputype;		/* system CPU */
		    uint32 fputype;		/* system FPU */
		    uint32 mmutype;		/* system MMU */
		    int32 num_memory;		/* # of memory blocks found */
		    struct mem_info memory[NUM_MEMINFO];  /* memory description */
		    struct mem_info ramdisk;	/* ramdisk description */
		    char command_line[CL_SIZE];	/* kernel command line parameters */
		    uint32 mch_cookie;		/* _MCH cookie from TOS */
		    uint32 mch_type;		/* special machine types */
		};

		void *kernel;
		unsigned long kernel_length;
		void *ramdisk;
		unsigned long ramdisk_length;
		struct atari_bootinfo bi;
		unsigned long bi_size;

		void cleanup(void);
		void init(bool cold);
		void *loadFile(const char *filename, unsigned long *length);
		int checkKernel(void);
		int create_bootinfo(void);
		int add_bi_record(
			unsigned short tag, unsigned short size, const void *data);
		int add_bi_string(unsigned short tag, const char *s);

	public:
		LinuxBootOs(void) ARANYM_THROWS(AranymException);
		virtual ~LinuxBootOs(void);

		virtual void reset(bool cold) ARANYM_THROWS(AranymException);
		virtual const char *type() { return "LILO"; };
};

#endif /* BOOTOSLINUX_H */
/* vim:ts=4:sw=4
 */
