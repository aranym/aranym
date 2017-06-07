/*
 * blitter.cpp - Atari Blitter emulation code
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *
 * Based on work by Martin Griffiths for the STonX, see below.
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
 */
/*
 *	Blitter Emulator,
 *	Martin Griffiths, 1995/96.
 *	
 * 	Here lies the Atari Blitter Emulator - The 'Blitter' chip is found in  
 *	the STE/MegaSTE and provides a very fast BitBlit in hardware.
 *
 *	The hardware registers for this chip lie at addresses $ff8a00 - $ff8a3c,
 * 	There seems to be a mirror for $ff8a30 used in TOS 1.02 at $ff7f30.
 *	
 */


#undef BLITTER_MEMMOVE		// when defined it replaces complicated logic of word-by-word copying by simple and fast memmove()
#undef BLITTER_SDLBLIT		// when defined it accelerates screen to screen blits with host VGA hardware accelerated routines (using SDL_BlitSurface)

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "blitter.h"
#include "icio.h"

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"

#if DEBUG
#define SHOWPARAMS												\
{       fprintf(stderr,"Source Address:%X\n",source_addr);		\
        fprintf(stderr,"  Dest Address:%X\n",dest_addr);		\
        fprintf(stderr,"       X count:%X\n",x_count);			\
        fprintf(stderr,"       Y count:%X\n",y_count);			\
        fprintf(stderr,"Words per line:%X\n",x_count ? x_count : 65536);		\
        fprintf(stderr,"Lines per blok:%X\n",y_count ? y_count : 65536);		\
        fprintf(stderr,"  Source X inc:%X\n",source_x_inc);		\
        fprintf(stderr,"    Dest X inc:%X\n",dest_x_inc);		\
        fprintf(stderr,"  Source Y inc:%X\n",source_y_inc);		\
        fprintf(stderr,"    Dest Y inc:%X\n",dest_y_inc);		\
        fprintf(stderr,"HOP:%2X    OP:%X\n",hop,op);			\
        fprintf(stderr,"   source SKEW:%X\n",skewreg);			\
        fprintf(stderr,"     endmask 1:%X\n",end_mask_1);		\
        fprintf(stderr,"     endmask 2:%X\n",end_mask_2);		\
        fprintf(stderr,"     endmask 3:%X\n",end_mask_3);		\
        fprintf(stderr,"       linenum:%X\n",line_num);			\
        if (NFSR) fprintf(stderr,"NFSR is Set!\n");			\
        if (FXSR) fprintf(stderr,"FXSR is Set!\n");			\
}
#endif

typedef uint16	UW;
typedef uint8	UB;
typedef char	B;

#define ADDR(a)		(a)

BLITTER::BLITTER(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	reset();
}

void BLITTER::reset()
{
	// halftone_ram[16];
	end_mask_1 = end_mask_2 = end_mask_3 = 0;
	NFSR = FXSR = 0;
	x_count = y_count = 0;
	hop = op = line_num = skewreg = 0;
	halftone_curroffset = halftone_direction = 0;
	source_x_inc = source_y_inc = dest_x_inc = dest_y_inc = 0;
	source_addr = 0;
	dest_addr = 0;
	blit = false;
}

UW BLITTER::LM_UW(memptr addr) {
	return ReadAtariInt16(addr);	//??
}

void BLITTER::SM_UW(memptr addr, UW value) {
	if (addr <= 0x800 || (addr >= 0x0e00000 && addr < 0x1000000)) {
		D(bug("Blitter tried to write to %06lx", addr));
		return;
	}
	WriteAtariInt16(addr, value);	//??
}


#define HOP_OPS(_fn_name,_op,_do_source_shift,_get_source_data,_shifted_hopd_data, _do_halftone_inc) \
static void _fn_name ( BLITTER& b ) \
{												\
	register unsigned int skew       = (unsigned int) b.skewreg & 15;		\
	register unsigned int source_buffer=0;				\
	if (b.hop & 1) {										\
		if (b.line_num & 0x20) 		 					\
			b.halftone_curroffset = b.skewreg & 15;			\
		else									\
			b.halftone_curroffset = b.line_num & 15;		\
		if (b.dest_y_inc >= 0)							\
			b.halftone_direction = 1;						\
		else 									\
			b.halftone_direction = -1;					\
	}													\
	do 								\
	{	register UW x,dst_data,opd_data;			\
		if (b.FXSR) 						\
		{ 	_do_source_shift; 				\
			_get_source_data;				\
			b.source_addr += b.source_x_inc; 			\
		} 							\
		_do_source_shift;					\
		_get_source_data;		 			\
		dst_data = b.LM_UW(ADDR(b.dest_addr)); 			\
		opd_data =  _shifted_hopd_data;				\
		b.SM_UW(ADDR(b.dest_addr),(dst_data & ~b.end_mask_1) | (_op & b.end_mask_1)); \
		for(x=0 ; x<b.x_count-2 ; x++) 				\
		{	b.source_addr += b.source_x_inc; 			\
			b.dest_addr += b.dest_x_inc; 			\
			_do_source_shift;				\
			_get_source_data;		 		\
			dst_data = b.LM_UW(ADDR(b.dest_addr)); 		\
			opd_data = _shifted_hopd_data;		 	\
			b.SM_UW(ADDR(b.dest_addr),(dst_data & ~b.end_mask_2) | (_op & b.end_mask_2)); \
		} 							\
		if (b.x_count >= 2)					\
		{	b.dest_addr += b.dest_x_inc;			\
			_do_source_shift;				\
			if ( (!b.NFSR) || ((~(0xffff>>skew)) > b.end_mask_3) )\
			{	b.source_addr += b.source_x_inc;		\
				_get_source_data;			\
			}						\
			dst_data = b.LM_UW(ADDR(b.dest_addr));		\
			opd_data = _shifted_hopd_data;			\
			b.SM_UW(ADDR(b.dest_addr),(((UW)dst_data) & ~b.end_mask_3) | (_op & b.end_mask_3));\
		}							\
		b.source_addr += b.source_y_inc;				\
		b.dest_addr += b.dest_y_inc;				\
		_do_halftone_inc;					\
		(void) opd_data; \
		(void) dst_data; \
	} while (--b.y_count > 0);					\
};


/*
 * Some macros fixed:
 * operations, that don't need a source, may not access the source address;
 * the source address isn't even set up by the BIOS.
 * This includes all HOP_0 operations (source is all ones),
 * all HOP_1 operations (source is halftone only),
 * and operations 0, 5, 10 and 15
 */
#define _HOP_0 0xffff
#define _HOP_1 b.halftone_ram[b.halftone_curroffset]
#define _HOP_2 (source_buffer >> skew)
#define _HOP_3 (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset]

#define _HOP_INC b.halftone_curroffset = (b.halftone_curroffset + b.halftone_direction) & 15

#define _SRC_READ_N source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16)
#define _SRC_READ_P source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)))

HOP_OPS(_HOP_0_OP_00_N,(0),                     source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_01_N,(opd_data & dst_data),   source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_02_N,(opd_data & ~dst_data),  source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_03_N,(opd_data),              source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_04_N,(~opd_data & dst_data),  source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_05_N,(dst_data),              source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_06_N,(opd_data ^ dst_data),   source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_07_N,(opd_data | dst_data),   source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_08_N,(~opd_data & ~dst_data), source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_09_N,(~opd_data ^ dst_data),  source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_10_N,(~dst_data),             source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_11_N,(opd_data | ~dst_data),  source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_12_N,(~opd_data),             source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_13_N,(~opd_data | dst_data),  source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_14_N,(~opd_data | ~dst_data), source_buffer >>=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_15_N,(0xffff),                source_buffer >>=16, ;, _HOP_0, ;)

HOP_OPS(_HOP_1_OP_00_N,(0),                     source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_01_N,(opd_data & dst_data),   source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_02_N,(opd_data & ~dst_data),  source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_03_N,(opd_data),              source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_04_N,(~opd_data & dst_data),  source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_05_N,(dst_data),              source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_06_N,(opd_data ^ dst_data),   source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_07_N,(opd_data | dst_data),   source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_08_N,(~opd_data & ~dst_data), source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_09_N,(~opd_data ^ dst_data),  source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_10_N,(~dst_data),             source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_11_N,(opd_data | ~dst_data),  source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_12_N,(~opd_data),             source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_13_N,(~opd_data | dst_data),  source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_14_N,(~opd_data | ~dst_data), source_buffer >>=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_15_N,(0xffff),                source_buffer >>=16, ;, _HOP_1, _HOP_INC)

HOP_OPS(_HOP_2_OP_00_N,(0),                     source_buffer >>=16, ;,           _HOP_2, ;)
HOP_OPS(_HOP_2_OP_01_N,(opd_data & dst_data),   source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_02_N,(opd_data & ~dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_03_N,(opd_data),              source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_04_N,(~opd_data & dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_05_N,(dst_data),              source_buffer >>=16, ;,           _HOP_2, ;)
HOP_OPS(_HOP_2_OP_06_N,(opd_data ^ dst_data),   source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_07_N,(opd_data | dst_data),   source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_08_N,(~opd_data & ~dst_data), source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_09_N,(~opd_data ^ dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_10_N,(~dst_data),             source_buffer >>=16, ;,           _HOP_2, ;)
HOP_OPS(_HOP_2_OP_11_N,(opd_data | ~dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_12_N,(~opd_data),             source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_13_N,(~opd_data | dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_14_N,(~opd_data | ~dst_data), source_buffer >>=16, _SRC_READ_N, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_15_N,(0xffff),                source_buffer >>=16, ;,           _HOP_2, ;)

HOP_OPS(_HOP_3_OP_00_N,(0),                     source_buffer >>=16, ;,           _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_01_N,(opd_data & dst_data),   source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_02_N,(opd_data & ~dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_03_N,(opd_data),              source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_04_N,(~opd_data & dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_05_N,(dst_data),              source_buffer >>=16, ;,           _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_06_N,(opd_data ^ dst_data),   source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_07_N,(opd_data | dst_data),   source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_08_N,(~opd_data & ~dst_data), source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_09_N,(~opd_data ^ dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_10_N,(~dst_data),             source_buffer >>=16, ;,           _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_11_N,(opd_data | ~dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_12_N,(~opd_data),             source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_13_N,(~opd_data | dst_data),  source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_14_N,(~opd_data | ~dst_data), source_buffer >>=16, _SRC_READ_N, _HOP_3, _HOP_INC) 
HOP_OPS(_HOP_3_OP_15_N,(0xffff),                source_buffer >>=16, ;,           _HOP_3, _HOP_INC)


HOP_OPS(_HOP_0_OP_00_P,(0),                     source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_01_P,(opd_data & dst_data),   source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_02_P,(opd_data & ~dst_data),  source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_03_P,(opd_data),              source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_04_P,(~opd_data & dst_data),  source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_05_P,(dst_data),              source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_06_P,(opd_data ^ dst_data),   source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_07_P,(opd_data | dst_data),   source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_08_P,(~opd_data & ~dst_data), source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_09_P,(~opd_data ^ dst_data),  source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_10_P,(~dst_data),             source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_11_P,(opd_data | ~dst_data),  source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_12_P,(~opd_data),             source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_13_P,(~opd_data | dst_data),  source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_14_P,(~opd_data | ~dst_data), source_buffer <<=16, ;, _HOP_0, ;)
HOP_OPS(_HOP_0_OP_15_P,(0xffff),                source_buffer <<=16, ;, _HOP_0, ;)

HOP_OPS(_HOP_1_OP_00_P,(0),                     source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_01_P,(opd_data & dst_data),   source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_02_P,(opd_data & ~dst_data),  source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_03_P,(opd_data),              source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_04_P,(~opd_data & dst_data),  source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_05_P,(dst_data),              source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_06_P,(opd_data ^ dst_data),   source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_07_P,(opd_data | dst_data),   source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_08_P,(~opd_data & ~dst_data), source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_09_P,(~opd_data ^ dst_data),  source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_10_P,(~dst_data),             source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_11_P,(opd_data | ~dst_data),  source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_12_P,(~opd_data),             source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_13_P,(~opd_data | dst_data),  source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_14_P,(~opd_data | ~dst_data), source_buffer <<=16, ;, _HOP_1, _HOP_INC)
HOP_OPS(_HOP_1_OP_15_P,(0xffff),                source_buffer <<=16, ;, _HOP_1, _HOP_INC)

HOP_OPS(_HOP_2_OP_00_P,(0),                     source_buffer <<=16, ;,           _HOP_2, ;)
HOP_OPS(_HOP_2_OP_01_P,(opd_data & dst_data),   source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_02_P,(opd_data & ~dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_03_P,(opd_data),              source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_04_P,(~opd_data & dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_05_P,(dst_data),              source_buffer <<=16, ;,           _HOP_2, ;)
HOP_OPS(_HOP_2_OP_06_P,(opd_data ^ dst_data),   source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_07_P,(opd_data | dst_data),   source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_08_P,(~opd_data & ~dst_data), source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_09_P,(~opd_data ^ dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_10_P,(~dst_data),             source_buffer <<=16, ;,           _HOP_2, ;)
HOP_OPS(_HOP_2_OP_11_P,(opd_data | ~dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_12_P,(~opd_data),             source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_13_P,(~opd_data | dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_14_P,(~opd_data | ~dst_data), source_buffer <<=16, _SRC_READ_P, _HOP_2, ;)
HOP_OPS(_HOP_2_OP_15_P,(0xffff),                source_buffer <<=16, ;,           _HOP_2, ;)

HOP_OPS(_HOP_3_OP_00_P,(0),                     source_buffer <<=16, ;,           _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_01_P,(opd_data & dst_data),   source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_02_P,(opd_data & ~dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_03_P,(opd_data),              source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_04_P,(~opd_data & dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_05_P,(dst_data),              source_buffer <<=16, ;,           _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_06_P,(opd_data ^ dst_data),   source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_07_P,(opd_data | dst_data),   source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_08_P,(~opd_data & ~dst_data), source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_09_P,(~opd_data ^ dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_10_P,(~dst_data),             source_buffer <<=16, ;,           _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_11_P,(opd_data | ~dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_12_P,(~opd_data),             source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_13_P,(~opd_data | dst_data),  source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_14_P,(~opd_data | ~dst_data), source_buffer <<=16, _SRC_READ_P, _HOP_3, _HOP_INC)
HOP_OPS(_HOP_3_OP_15_P,(0xffff),                source_buffer <<=16, ;,           _HOP_3, _HOP_INC)



void _hop_2_op_03_p( BLITTER& b )
{
#ifdef BLITTER_MEMMOVE
	if (getVIDEL ()->getBpp() == 16) {
#ifdef BLITTER_SDLBLIT
		if (b.source_addr >= ARANYMVRAMSTART && b.dest_addr >= ARANYMVRAMSTART) {
			SDL_Rect src, dest;
			int src_offset = b.source_addr - ARANYMVRAMSTART;
			int dest_offset = b.dest_addr - ARANYMVRAMSTART;
			int VidelScreenWidth = getVIDEL ()->getScreenWidth();
			src.x = (src_offset % (2*VidelScreenWidth))/2;
			src.y = (src_offset / (2*VidelScreenWidth));
			src.w = dest.w = b.x_count;
			src.h = dest.h = b.y_count;
			dest.x = (dest_offset % (2*VidelScreenWidth))/2;
			dest.y = (dest_offset / (2*VidelScreenWidth));
			SDL_Surface *surf = SDL_GetVideoSurface();
			SDL_BlitSurface(surf, &src, surf, &dest);
			b.source_addr += (((b.x_count-1)*b.source_x_inc)+b.source_y_inc)*b.y_count;
			b.dest_addr += (((b.x_count-1)*b.dest_x_inc)+b.dest_y_inc)*b.y_count;
			b.y_count = 0;
			return;
		}
#endif /* BLITTER_SDLBLIT */
		do
		{
			memmove(phys_get_real_address(b.dest_addr), phys_get_real_address(b.source_addr), b.x_count*2);
			b.source_addr += ((b.x_count-1)*b.source_x_inc)+b.source_y_inc;
			b.dest_addr += ((b.x_count-1)*b.dest_x_inc)+b.dest_y_inc;
		} while (--b.y_count > 0);
	}
	else
#endif /* BLITTER_MEMMOVE */
	_HOP_2_OP_03_P( b );
}

void _hop_2_op_03_n( BLITTER& b )
{
#ifdef BLITTER_MEMMOVE
	if (getVIDEL ()->getBpp() == 16) {
		b.source_addr += ((b.x_count-1)*b.source_x_inc);
		b.dest_addr += ((b.x_count-1)*b.dest_x_inc);
#ifdef BLITTER_SDLBLIT
		if (b.source_addr >= ARANYMVRAMSTART && b.dest_addr >= ARANYMVRAMSTART) {
			b.source_addr += (((b.x_count)*b.source_x_inc)+b.source_y_inc)*b.y_count;
			b.dest_addr += (((b.x_count-1)*b.dest_x_inc)+b.dest_y_inc)*b.y_count;
			SDL_Rect src, dest;
			int src_offset = b.source_addr - ARANYMVRAMSTART;
			int dest_offset = b.dest_addr - ARANYMVRAMSTART;
			int VidelScreenWidth = getVIDEL ()->getScreenWidth();
			src.x = (src_offset % (2*VidelScreenWidth))/2;
			src.y = (src_offset / (2*VidelScreenWidth));
			src.w = dest.w = b.x_count;
			src.h = dest.h = b.y_count;
			dest.x = (dest_offset % (2*VidelScreenWidth))/2;
			dest.y = (dest_offset / (2*VidelScreenWidth));
			SDL_Surface *surf = SDL_GetVideoSurface();
			SDL_BlitSurface(surf, &src, surf, &dest);
			b.y_count = 0;
			return;
		}
#endif /* BLITTER_SDLBLIT */
		do
		{
			memmove(phys_get_real_address(b.dest_addr), phys_get_real_address(b.source_addr), b.x_count*2);
			b.source_addr += ((b.x_count)*b.source_x_inc)+b.source_y_inc;
			b.dest_addr += ((b.x_count-1)*b.dest_x_inc)+b.dest_y_inc;
		} while (--b.y_count > 0);
	}
	else
#endif /* BLITTER_MEMMOVE */
	_HOP_2_OP_03_N( b );
}



static void (*const do_hop_op_N[4][16])( BLITTER& ) = {
	{ _HOP_0_OP_00_N, _HOP_0_OP_01_N, _HOP_0_OP_02_N, _HOP_0_OP_03_N, _HOP_0_OP_04_N, _HOP_0_OP_05_N, _HOP_0_OP_06_N, _HOP_0_OP_07_N, _HOP_0_OP_08_N, _HOP_0_OP_09_N, _HOP_0_OP_10_N, _HOP_0_OP_11_N, _HOP_0_OP_12_N, _HOP_0_OP_13_N, _HOP_0_OP_14_N, _HOP_0_OP_15_N },
	{ _HOP_1_OP_00_N, _HOP_1_OP_01_N, _HOP_1_OP_02_N, _HOP_1_OP_03_N, _HOP_1_OP_04_N, _HOP_1_OP_05_N, _HOP_1_OP_06_N, _HOP_1_OP_07_N, _HOP_1_OP_08_N, _HOP_1_OP_09_N, _HOP_1_OP_10_N, _HOP_1_OP_11_N, _HOP_1_OP_12_N, _HOP_1_OP_13_N, _HOP_1_OP_14_N, _HOP_1_OP_15_N },
	{ _HOP_2_OP_00_N, _HOP_2_OP_01_N, _HOP_2_OP_02_N, _hop_2_op_03_n, _HOP_2_OP_04_N, _HOP_2_OP_05_N, _HOP_2_OP_06_N, _HOP_2_OP_07_N, _HOP_2_OP_08_N, _HOP_2_OP_09_N, _HOP_2_OP_10_N, _HOP_2_OP_11_N, _HOP_2_OP_12_N, _HOP_2_OP_13_N, _HOP_2_OP_14_N, _HOP_2_OP_15_N },
	{ _HOP_3_OP_00_N, _HOP_3_OP_01_N, _HOP_3_OP_02_N, _HOP_3_OP_03_N, _HOP_3_OP_04_N, _HOP_3_OP_05_N, _HOP_3_OP_06_N, _HOP_3_OP_07_N, _HOP_3_OP_08_N, _HOP_3_OP_09_N, _HOP_3_OP_10_N, _HOP_3_OP_11_N, _HOP_3_OP_12_N, _HOP_3_OP_13_N, _HOP_3_OP_14_N, _HOP_3_OP_15_N }
};

static void (*const do_hop_op_P[4][16])( BLITTER& ) = {
	{ _HOP_0_OP_00_P, _HOP_0_OP_01_P, _HOP_0_OP_02_P, _HOP_0_OP_03_P, _HOP_0_OP_04_P, _HOP_0_OP_05_P, _HOP_0_OP_06_P, _HOP_0_OP_07_P, _HOP_0_OP_08_P, _HOP_0_OP_09_P, _HOP_0_OP_10_P, _HOP_0_OP_11_P, _HOP_0_OP_12_P, _HOP_0_OP_13_P, _HOP_0_OP_14_P, _HOP_0_OP_15_P },
	{ _HOP_1_OP_00_P, _HOP_1_OP_01_P, _HOP_1_OP_02_P, _HOP_1_OP_03_P, _HOP_1_OP_04_P, _HOP_1_OP_05_P, _HOP_1_OP_06_P, _HOP_1_OP_07_P, _HOP_1_OP_08_P, _HOP_1_OP_09_P, _HOP_1_OP_10_P, _HOP_1_OP_11_P, _HOP_1_OP_12_P, _HOP_1_OP_13_P, _HOP_1_OP_14_P, _HOP_1_OP_15_P },
	{ _HOP_2_OP_00_P, _HOP_2_OP_01_P, _HOP_2_OP_02_P, _hop_2_op_03_p, _HOP_2_OP_04_P, _HOP_2_OP_05_P, _HOP_2_OP_06_P, _HOP_2_OP_07_P, _HOP_2_OP_08_P, _HOP_2_OP_09_P, _HOP_2_OP_10_P, _HOP_2_OP_11_P, _HOP_2_OP_12_P, _HOP_2_OP_13_P, _HOP_2_OP_14_P, _HOP_2_OP_15_P },
	{ _HOP_3_OP_00_P, _HOP_3_OP_01_P, _HOP_3_OP_02_P, _HOP_3_OP_03_P, _HOP_3_OP_04_P, _HOP_3_OP_05_P, _HOP_3_OP_06_P, _HOP_3_OP_07_P, _HOP_3_OP_08_P, _HOP_3_OP_09_P, _HOP_3_OP_10_P, _HOP_3_OP_11_P, _HOP_3_OP_12_P, _HOP_3_OP_13_P, _HOP_3_OP_14_P, _HOP_3_OP_15_P }
};


uint8 BLITTER::handleRead(memptr addr) {
	addr -= getHWoffset();

	D(bug("Blitter read byte from register %x at %06x", addr+getHWoffset(), showPC()));
	switch(addr) {
		case 0x3a: return hop;
		case 0x3b: return op;
		case 0x3c: return LOAD_B_ff8a3c();
		case 0x3d: return skewreg;
	}

	panicbug("Blitter tried to read byte from register %x at %06x", addr+getHWoffset(), showPC());

	return 0;
}

uae_u16 BLITTER::handleReadW(uaecptr addr) {
	addr -= getHWoffset();

	D(bug("Blitter read word from register %x at %06x", addr+getHWoffset(), showPC()));
	if (addr < 0x20) {
		return halftone_ram[addr / 2];
	}

	switch(addr) {
		case 0x20: return source_x_inc;
		case 0x22: return source_y_inc;
		case 0x24: return (source_addr >> 16) & 0xffff;
		case 0x26: return (source_addr) & 0xffff;
		case 0x28: return end_mask_1;
		case 0x2a: return end_mask_2;
		case 0x2c: return end_mask_3;
		case 0x2e: return dest_x_inc;
		case 0x30: return dest_y_inc;
		case 0x32: return (dest_addr >> 16) & 0xffff;
		case 0x34: return (dest_addr) & 0xffff;
		case 0x36: return x_count;
		case 0x38: return y_count;
		case 0x3a: // fallthrough
		case 0x3c: return BASE_IO::handleReadW(addr+getHWoffset());
	}

	panicbug("Blitter tried to read word from register %x at %06x", addr+getHWoffset(), showPC());

	return 0;
}

uae_u32 BLITTER::handleReadL(uaecptr addr) {
	addr -= getHWoffset();

	D(bug("Blitter read long from register %x at %06x", addr+getHWoffset(), showPC()));
	if (addr < 0x20) {
		return BASE_IO::handleReadL(addr+getHWoffset());
	}

	switch(addr) {
		case 0x24: return source_addr;
		case 0x32: return dest_addr;
	}

	panicbug("Blitter tried to read long word from register %x at %06x", addr+getHWoffset(), showPC());

	return 0;
}

void BLITTER::handleWrite(memptr addr, uint8 value) {
	addr -= getHWoffset();

	D(bug("Blitter write byte $%02x to register %x at %06x", value, addr+getHWoffset(), showPC()));
	switch(addr) {
		case 0x3a: STORE_B_ff8a3a(value); break;
		case 0x3b: STORE_B_ff8a3b(value); break;
		case 0x3c: STORE_B_ff8a3c(value); break;
		case 0x3d: STORE_B_ff8a3d(value); break;
		default:
			panicbug("Blitter tried to write byte $%02x to register %x at %06x", value, addr+getHWoffset(), showPC());
	}

	if (blit) {
		Do_Blit();
		blit = false;
	}
}

void BLITTER::handleWriteW(uaecptr addr, uint16 value) {
	addr -= getHWoffset();

	D(bug("Blitter write word $%04x to register %x at %06x", value, addr+getHWoffset(), showPC()));
	if (addr < 0x20) {
		halftone_ram[addr / 2] = value;
		return;
	}

	switch(addr) {
		case 0x20: source_x_inc = value; break;
		case 0x22: source_y_inc = value; break;
		case 0x24: source_addr = (source_addr & 0x0000ffff) | (value << 16); source_addr_backup = source_addr; break;
		case 0x26: source_addr = (source_addr & 0xffff0000) | (value & 0xfffe); source_addr_backup = source_addr; break;
		case 0x28: end_mask_1 = value; break;
		case 0x2a: end_mask_2 = value; break;
		case 0x2c: end_mask_3 = value; break;
		case 0x2e: dest_x_inc = value; break;
		case 0x30: dest_y_inc = value; break;
		case 0x32: dest_addr = (dest_addr & 0x0000ffff) | (value << 16); dest_addr_backup = dest_addr; break;
		case 0x34: dest_addr = (dest_addr & 0xffff0000) | (value & 0xfffe); dest_addr_backup = dest_addr; break;
		case 0x36: x_count = value; break;
		case 0x38: y_count = value; break;
		case 0x3a: STORE_B_ff8a3a(value >> 8); STORE_B_ff8a3b(value); break;
		case 0x3c: STORE_B_ff8a3c(value >> 8); STORE_B_ff8a3d(value); break;
		default:
			panicbug("Blitter tried to write word $%04x to register %x at %06x", value, addr+getHWoffset(), showPC());
	}

	if (blit) {
		Do_Blit();
		blit = false;
	}
}

void BLITTER::handleWriteL(uaecptr addr, uint32 value) {
	addr -= getHWoffset();
	D(bug("Blitter write long $%08x to register %x at %06x", value, addr+getHWoffset(), showPC()));
	switch(addr) {
		case 0x24:
			D(bug("Blitter sets source to $%08lx at $%08lx", value, showPC()));
			source_addr = value & 0xfffffffe;
			source_addr_backup = source_addr;
			break;
		case 0x32:
			D(bug("Blitter sets dest to $%08lx at $%08lx", value, showPC()));
			dest_addr = value & 0xfffffffe;
			dest_addr_backup = dest_addr;
			break;
		default: BASE_IO::handleWriteL(addr+getHWoffset(), value); break;
	}

}

B BLITTER::LOAD_B_ff8a3c(void)
{
	return line_num & 0x3f;
}

void BLITTER::STORE_B_ff8a3a(B v)
{	hop = v & 3;	/* h/ware reg masks out the top 6 bits! */
}

void BLITTER::STORE_B_ff8a3b(B v)
{	op = v & 15;	/* h/ware reg masks out the top 4 bits! */	
}

void BLITTER::STORE_B_ff8a3c(B v)
{	// 765_3210  Line-Number
	line_num   = (UB) v & 0x3f;
	if ((y_count !=0) && (v & 0x80)) /* Busy bit set and lines to blit? */
		blit = true;
}

void BLITTER::STORE_B_ff8a3d(B v)
{	// 76__3210  Skew
	NFSR = (v & 0x40) != 0;					
	FXSR = (v & 0x80) != 0;					
	skewreg = (unsigned char) v & 0xcf;	/* h/ware reg mask %11001111 !*/	
}


void BLITTER::Do_Blit(void)
{ 	
	D(bug("Blitter started at %06x", showPC()));
#if DEBUG
	SHOWPARAMS;
#endif

	if (source_addr != source_addr_backup) {
		D(bug("Blitter starts with obsolete source addr $%08lx, should be $%08lx", source_addr, source_addr_backup));
		source_addr = source_addr_backup;
	}
	if (dest_addr != dest_addr_backup) {
		D(bug("Blitter starts with obsolete dest addr $%08lx, should be $%08lx", dest_addr, dest_addr_backup));
		dest_addr = dest_addr_backup;
	}

	/*
	 * do not check source address if is not used in operation
	 */
	if (hop != 0 && /* source is all ones */
		hop != 1 && /* source is halftone only */
		op != 0  && /* result is all zeroes */
		op != 5  && /* result is destination */
		op != 10 && /* result is ~destination */
		op != 15 && /* result is all ones */
		(source_addr <= 0x800 || (source_addr >= 0x0e80000 && source_addr < 0x1000000))) {
		panicbug("Blitter Source address out of range: $%08x", source_addr);
		return;
	}
	if (dest_addr <= 0x800 || (dest_addr >= 0x0e00000 && dest_addr < 0x1000000)) {
		panicbug("Blitter Destination address out of range: $%08x", dest_addr);
		return;
	}

	if (source_x_inc < 0) {
		do_hop_op_N[hop][op]( *this );
	}
	else {
		do_hop_op_P[hop][op]( *this );
	}
}
