/*
	ROM / OS loader, NetBSD/m68k

	Copyright (c) 2022 Thorsten Otto

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

#ifndef BOOTOSNETBSD_H
#define BOOTOSNETBSD_H

#include "aranym_exception.h"
#include "bootos.h"

/*--- Some defines ---*/

#define CL_SIZE		256

/* NetBSD/m68k loader class */

class NetbsdBootOs : public BootOs
{
	private:
		struct mem_info {
			uint32 addr;		/* physical address of memory chunk */
			uint32 size;		/* length of memory chunk (in bytes) */
		};

		struct atari_bootinfo {
			memptr		kp;			/* 00: Kernel load address	*/
			uint32_t	ksize;		/* 04: Size of loaded kernel	*/
			uint32_t	entry;		/* 08: Kernel entry point	*/
			uint32_t	stmem_size;	/* 12: Size of st-ram		*/
			uint32_t	ttmem_size;	/* 16: Size of tt-ram		*/
			uint32_t	bootflags;	/* 20: Various boot flags	*/
			uint32_t	boothowto;	/* 24: How to boot		*/
			uint32_t	ttmem_start;	/* 28: Start of tt-ram		*/
			uint32_t	esym_loc;	/* 32: End of symbol table	*/
		} bi;

		void *kernel;
		unsigned long kernel_length;
		
		void cleanup(void);
		void init(bool cold);
		void *loadFile(const char *filename, unsigned long *length);
		int checkKernel(void);

	public:
		NetbsdBootOs(void) ARANYM_THROWS(AranymException);
		virtual ~NetbsdBootOs(void);

		virtual void reset(bool cold) ARANYM_THROWS(AranymException);
		virtual const char *type() { return "NETBSD"; };
};

#endif /* BOOTOSNETBSD_H */
