/* This file is part of STonX, the Atari ST Emulator for Unix/X
 * ============================================================
 * STonX is free software and comes with NO WARRANTY - read the file
 * COPYING for details
 *
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

/*
#define BLITTER_MEMMOVE		// this replaces complicated logic of word-by-word copying by simple and fast memmove()
#define BLITTER_SDLBLIT		// this accelerates screen to screen blits with host VGA hardware accelerated routines (using SDL_BlitSurface)
*/

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "blitter.h"

#define DEBUG 0
#include "debug.h"

#include <SDL.h>

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

typedef uae_u16	UW;
typedef uae_u8	UB;
typedef char	B;

#define ADDR(a)		(a)

BLITTER::BLITTER(memptr addr, uint32 size) : BASE_IO(addr, size) {}

UW BLITTER::LM_UW(uaecptr addr) {
	return ReadAtariInt16(addr);	//??
}

void BLITTER::SM_UW(uaecptr addr, UW value) {
#if DEBUG
	if (addr <= 0x800 || addr >= 0xe00000) {
		D(bug("Blitter Error! Tries to write to %06lx", addr));
		exit(-1);
	}
#endif
	WriteAtariInt16(addr, value);	//??
}


#define HOP_OPS(_fn_name,_op,_do_source_shift,_get_source_data,_shifted_hopd_data, _do_halftone_inc) \
void _fn_name ( BLITTER& b ) \
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
	} while (--b.y_count > 0);					\
};


HOP_OPS(_HOP_0_OP_00_N,(0), source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_01_N,(opd_data & dst_data) ,source_buffer >>=16,; , 0xffff, ;)
HOP_OPS(_HOP_0_OP_02_N,(opd_data & ~dst_data) ,source_buffer >>=16,; , 0xffff,;)	 
HOP_OPS(_HOP_0_OP_03_N,(opd_data) ,source_buffer >>=16,; , 0xffff,;)
HOP_OPS(_HOP_0_OP_04_N,(~opd_data & dst_data) ,source_buffer >>=16,;, 0xffff,;)
HOP_OPS(_HOP_0_OP_05_N,(dst_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_06_N,(opd_data ^ dst_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_07_N,(opd_data | dst_data) ,source_buffer >>=16,; , 0xffff, ;)
HOP_OPS(_HOP_0_OP_08_N,(~opd_data & ~dst_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_09_N,(~opd_data ^ dst_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_10_N,(~dst_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_11_N,(opd_data | ~dst_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_12_N,(~opd_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_13_N,(~opd_data | dst_data) ,source_buffer >>=16,;, 0xffff, ;)	 
HOP_OPS(_HOP_0_OP_14_N,(~opd_data | ~dst_data) ,source_buffer >>=16,;, 0xffff, ;)
HOP_OPS(_HOP_0_OP_15_N,(0xffff) ,source_buffer >>=16,;, 0xffff, ;)

HOP_OPS(_HOP_1_OP_00_N,(0) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_01_N,(opd_data & dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_02_N,(opd_data & ~dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_03_N,(opd_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_04_N,(~opd_data & dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_05_N,(dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_06_N,(opd_data ^ dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_07_N,(opd_data | dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_08_N,(~opd_data & ~dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_09_N,(~opd_data ^ dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_10_N,(~dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_11_N,(opd_data | ~dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_12_N,(~opd_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_13_N,(~opd_data | dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_14_N,(~opd_data | ~dst_data) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_15_N,(0xffff) ,source_buffer >>=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )

HOP_OPS(_HOP_2_OP_00_N,(0) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_01_N,(opd_data & dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_02_N,(opd_data & ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_03_N,(opd_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_04_N,(~opd_data & dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_05_N,(dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_06_N,(opd_data ^ dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_07_N,(opd_data | dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_08_N,(~opd_data & ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_09_N,(~opd_data ^ dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_10_N,(~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_11_N,(opd_data | ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_12_N,(~opd_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_13_N,(~opd_data | dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_14_N,(~opd_data | ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_15_N,(0xffff) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew),;)

HOP_OPS(_HOP_3_OP_00_N,(0) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_01_N,(opd_data & dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_02_N,(opd_data & ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_03_N,(opd_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_04_N,(~opd_data & dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_05_N,(dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_06_N,(opd_data ^ dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_07_N,(opd_data | dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_08_N,(~opd_data & ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_09_N,(~opd_data ^ dst_data) , source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_10_N,(~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_11_N,(opd_data | ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_12_N,(~opd_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_13_N,(~opd_data | dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)
HOP_OPS(_HOP_3_OP_14_N,(~opd_data | ~dst_data) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15) 
HOP_OPS(_HOP_3_OP_15_N,(0xffff) ,source_buffer >>=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) << 16) ,(source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15)


HOP_OPS(_HOP_0_OP_00_P,(0) ,source_buffer <<=16,;, 0xffff,;)
HOP_OPS(_HOP_0_OP_01_P,(opd_data & dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_02_P,(opd_data & ~dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_03_P,(opd_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_04_P,(~opd_data & dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_05_P,(dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_06_P,(opd_data ^ dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_07_P,(opd_data | dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_08_P,(~opd_data & ~dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_09_P,(~opd_data ^ dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_10_P,(~dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_11_P,(opd_data | ~dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_12_P,(~opd_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_13_P,(~opd_data | dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_14_P,(~opd_data | ~dst_data) ,source_buffer <<=16,;, 0xffff,;)	
HOP_OPS(_HOP_0_OP_15_P,(0xffff) ,source_buffer <<=16,;, 0xffff,;)	

HOP_OPS(_HOP_1_OP_00_P,(0) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_01_P,(opd_data & dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_02_P,(opd_data & ~dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_03_P,(opd_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_04_P,(~opd_data & dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_05_P,(dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_06_P,(opd_data ^ dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_07_P,(opd_data | dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_08_P,(~opd_data & ~dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )\
HOP_OPS(_HOP_1_OP_09_P,(~opd_data ^ dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_10_P,(~dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_11_P,(opd_data | ~dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_12_P,(~opd_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_13_P,(~opd_data | dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_14_P,(~opd_data | ~dst_data) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )
HOP_OPS(_HOP_1_OP_15_P,(0xffff) ,source_buffer <<=16,;,b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 )

HOP_OPS(_HOP_2_OP_00_P,(0) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_01_P,(opd_data & dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_02_P,(opd_data & ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_03_P,(opd_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_04_P,(~opd_data & dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_05_P,(dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_06_P,(opd_data ^ dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_07_P,(opd_data | dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_08_P,(~opd_data & ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_09_P,(~opd_data ^ dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_10_P,(~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_11_P,(opd_data | ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)

HOP_OPS(_HOP_2_OP_12_P,(~opd_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_13_P,(~opd_data | dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_14_P,(~opd_data | ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)
HOP_OPS(_HOP_2_OP_15_P,(0xffff) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ) , (source_buffer >> skew),;)

HOP_OPS(_HOP_3_OP_00_P,(0) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_01_P,(opd_data & dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_02_P,(opd_data & ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_03_P,(opd_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_04_P,(~opd_data & dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_05_P,(dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_06_P,(opd_data ^ dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_07_P,(opd_data | dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_08_P,(~opd_data & ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_09_P,(~opd_data ^ dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_10_P,(~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_11_P,(opd_data | ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_12_P,(~opd_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_13_P,(~opd_data | dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_14_P,(~opd_data | ~dst_data) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 
HOP_OPS(_HOP_3_OP_15_P,(0xffff) ,source_buffer <<=16,source_buffer |= ((unsigned int) b.LM_UW(ADDR(b.source_addr)) ), (source_buffer >> skew) & b.halftone_ram[b.halftone_curroffset],b.halftone_curroffset=(b.halftone_curroffset+b.halftone_direction) & 15 ) 



void hop2op3p( BLITTER& b )
{
#if BLITTER_MEMMOVE
	if (getVIDEL ()->getScreenBpp() == 16) {
#if BLITTER_SDLBLIT
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

void hop2op3n( BLITTER& b )
{
#if BLITTER_MEMMOVE
	if (getVIDEL ()->getScreenBpp() == 16) {
		b.source_addr += ((b.x_count-1)*b.source_x_inc);
		b.dest_addr += ((b.x_count-1)*b.dest_x_inc);
#if BLITTER_SDLBLIT
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



static void (*do_hop_op_N[4][16])( BLITTER& ) =
{{ 	_HOP_0_OP_00_N, _HOP_0_OP_01_N, _HOP_0_OP_02_N, _HOP_0_OP_03_N, _HOP_0_OP_04_N, _HOP_0_OP_05_N, _HOP_0_OP_06_N, _HOP_0_OP_07_N, _HOP_0_OP_08_N, _HOP_0_OP_09_N, _HOP_0_OP_10_N, _HOP_0_OP_11_N, _HOP_0_OP_12_N, _HOP_0_OP_13_N, _HOP_0_OP_14_N, _HOP_0_OP_15_N,},{
	_HOP_1_OP_00_N, _HOP_1_OP_01_N, _HOP_1_OP_02_N, _HOP_1_OP_03_N, _HOP_1_OP_04_N, _HOP_1_OP_05_N, _HOP_1_OP_06_N, _HOP_1_OP_07_N, _HOP_1_OP_08_N, _HOP_1_OP_09_N, _HOP_1_OP_10_N, _HOP_1_OP_11_N, _HOP_1_OP_12_N, _HOP_1_OP_13_N, _HOP_1_OP_14_N, _HOP_1_OP_15_N,},{
	_HOP_2_OP_00_N, _HOP_2_OP_01_N, _HOP_2_OP_02_N, hop2op3n /*_HOP_2_OP_03_N*/, _HOP_2_OP_04_N, _HOP_2_OP_05_N, _HOP_2_OP_06_N, _HOP_2_OP_07_N, _HOP_2_OP_08_N, _HOP_2_OP_09_N, _HOP_2_OP_10_N, _HOP_2_OP_11_N, _HOP_2_OP_12_N, _HOP_2_OP_13_N, _HOP_2_OP_14_N, _HOP_2_OP_15_N,},{
	_HOP_3_OP_00_N, _HOP_3_OP_01_N, _HOP_3_OP_02_N, _HOP_3_OP_03_N, _HOP_3_OP_04_N, _HOP_3_OP_05_N, _HOP_3_OP_06_N, _HOP_3_OP_07_N, _HOP_3_OP_08_N, _HOP_3_OP_09_N, _HOP_3_OP_10_N, _HOP_3_OP_11_N, _HOP_3_OP_12_N, _HOP_3_OP_13_N, _HOP_3_OP_14_N, _HOP_3_OP_15_N,}
};

static void (*do_hop_op_P[4][16])( BLITTER& ) =
{{ 	_HOP_0_OP_00_P, _HOP_0_OP_01_P, _HOP_0_OP_02_P, _HOP_0_OP_03_P, _HOP_0_OP_04_P, _HOP_0_OP_05_P, _HOP_0_OP_06_P, _HOP_0_OP_07_P, _HOP_0_OP_08_P, _HOP_0_OP_09_P, _HOP_0_OP_10_P, _HOP_0_OP_11_P, _HOP_0_OP_12_P, _HOP_0_OP_13_P, _HOP_0_OP_14_P, _HOP_0_OP_15_P,},{
	_HOP_1_OP_00_P, _HOP_1_OP_01_P, _HOP_1_OP_02_P, _HOP_1_OP_03_P, _HOP_1_OP_04_P, _HOP_1_OP_05_P, _HOP_1_OP_06_P, _HOP_1_OP_07_P, _HOP_1_OP_08_P, _HOP_1_OP_09_P, _HOP_1_OP_10_P, _HOP_1_OP_11_P, _HOP_1_OP_12_P, _HOP_1_OP_13_P, _HOP_1_OP_14_P, _HOP_1_OP_15_P,},{
		_HOP_2_OP_00_P, _HOP_2_OP_01_P, _HOP_2_OP_02_P, hop2op3p /*_HOP_2_OP_03_P*/, _HOP_2_OP_04_P, _HOP_2_OP_05_P, _HOP_2_OP_06_P, _HOP_2_OP_07_P, _HOP_2_OP_08_P, _HOP_2_OP_09_P, _HOP_2_OP_10_P, _HOP_2_OP_11_P, _HOP_2_OP_12_P, _HOP_2_OP_13_P, _HOP_2_OP_14_P, _HOP_2_OP_15_P,},{
	_HOP_3_OP_00_P, _HOP_3_OP_01_P, _HOP_3_OP_02_P, _HOP_3_OP_03_P, _HOP_3_OP_04_P, _HOP_3_OP_05_P, _HOP_3_OP_06_P, _HOP_3_OP_07_P, _HOP_3_OP_08_P, _HOP_3_OP_09_P, _HOP_3_OP_10_P, _HOP_3_OP_11_P, _HOP_3_OP_12_P, _HOP_3_OP_13_P, _HOP_3_OP_14_P, _HOP_3_OP_15_P,},
};


static const int HW = 0xff8a00;

uae_u8 BLITTER::handleRead(uaecptr addr) {
	addr -= HW;
	if (addr > 0x3d)
		return 0;

/*
	if (blit) {
		Do_Blit();
		blit = false;
	}
*/

	if (addr < 0x20) {
		uae_u16 hfr = halftone_ram[addr / 2];
		if (addr % 1)
			return hfr & 0x00ff;
		else
			return hfr >> 8;
	}

	D(bug("Blitter reads %x at %06x", addr+HW, showPC()));

	switch(addr) {
		case 0x20: return source_x_inc >> 8;
		case 0x21: return source_x_inc;
		case 0x22: return source_y_inc >> 8;
		case 0x23: return source_y_inc;
		case 0x24: return source_addr >> 24;
		case 0x25: return source_addr >> 16;
		case 0x26: return source_addr >> 8;
		case 0x27: return source_addr;
		case 0x28: return LOAD_B_ff8a28();
		case 0x29: return LOAD_B_ff8a29();
		case 0x2a: return LOAD_B_ff8a2a();
		case 0x2b: return LOAD_B_ff8a2b();
		case 0x2c: return LOAD_B_ff8a2c();
		case 0x2d: return LOAD_B_ff8a2d();
		case 0x2e: return dest_x_inc >> 8;
		case 0x2f: return dest_x_inc;
		case 0x30: return dest_y_inc >> 8;
		case 0x31: return dest_y_inc;
		case 0x32: return LOAD_B_ff8a32();
		case 0x33: return LOAD_B_ff8a33();
		case 0x34: return LOAD_B_ff8a34();
		case 0x35: return LOAD_B_ff8a35();
		case 0x36: return LOAD_B_ff8a36();
		case 0x37: return LOAD_B_ff8a37();
		case 0x38: return LOAD_B_ff8a38();
		case 0x39: return LOAD_B_ff8a39();
		case 0x3a: return LOAD_B_ff8a3a();
		case 0x3b: return LOAD_B_ff8a3b();
		case 0x3c: return LOAD_B_ff8a3c();
		case 0x3d: return LOAD_B_ff8a3d();
	}

	return 0;
}

void BLITTER::handleWrite(uaecptr addr, uae_u8 value) {
	addr -= HW;
	if (addr > 0x3d)
		return;

	if (addr < 0x20) {
		uae_u16 hfr = halftone_ram[addr / 2];
		if (addr & 1)
			hfr = (hfr & 0xff00) | value;
		else
			hfr = (hfr & 0x00ff) | (value << 8);
		halftone_ram[addr / 2] = hfr;
		return;
	}

	D(bug("Blitter writes: %x = %d ($%x) at %06x", addr+HW, value, value, showPC()));

	switch(addr) {
		case 0x20: source_x_inc = (source_x_inc & 0x00ff) | (value << 8); break;
		case 0x21: source_x_inc = (source_x_inc & 0xff00) | (value & 0xfe); break;
		case 0x22: source_y_inc = (source_y_inc & 0x00ff) | (value << 8); break;
		case 0x23: source_y_inc = (source_y_inc & 0xff00) | (value & 0xfe); break;
		case 0x24: source_addr = (source_addr & 0x00ffffff) | (value << 24); break;
		case 0x25: source_addr = (source_addr & 0xff00ffff) | (value << 16); break;
		case 0x26: source_addr = (source_addr & 0xffff00ff) | (value << 8); break;
		case 0x27: source_addr = (source_addr & 0xffffff00) | (value & 0xfe); break;	// ignore LSB
		case 0x28: STORE_B_ff8a28(value); break;
		case 0x29: STORE_B_ff8a29(value); break;
		case 0x2a: STORE_B_ff8a2a(value); break;
		case 0x2b: STORE_B_ff8a2b(value); break;
		case 0x2c: STORE_B_ff8a2c(value); break;
		case 0x2d: STORE_B_ff8a2d(value); break;
		case 0x2e: dest_x_inc = (dest_x_inc & 0x00ff) | (value << 8); break;
		case 0x2f: dest_x_inc = (dest_x_inc & 0xff00) | (value & 0xfe); break;
		case 0x30: dest_y_inc = (dest_y_inc & 0x00ff) | (value << 8); break;
		case 0x31: dest_y_inc = (dest_y_inc & 0xff00) | (value & 0xfe); break;
		case 0x32: STORE_B_ff8a32(value); break;
		case 0x33: STORE_B_ff8a33(value); break;
		case 0x34: STORE_B_ff8a34(value); break;
		case 0x35: STORE_B_ff8a35(value); break;
		case 0x36: STORE_B_ff8a36(value); break;
		case 0x37: STORE_B_ff8a37(value); break;
		case 0x38: STORE_B_ff8a38(value); break;
		case 0x39: STORE_B_ff8a39(value); break;
		case 0x3a: STORE_B_ff8a3a(value); break;
		case 0x3b: STORE_B_ff8a3b(value); break;
		case 0x3c: STORE_B_ff8a3c(value); break;
		case 0x3d: STORE_B_ff8a3d(value); break;
	}

}

B BLITTER::LOAD_B_ff8a28(void)
{	return (end_mask_1 >> 8) & 0xff;
}

B BLITTER::LOAD_B_ff8a29(void)
{	return end_mask_1 & 0xff;
}

B BLITTER::LOAD_B_ff8a2a(void)
{	return (end_mask_2 >> 8) & 0xff;
}

B BLITTER::LOAD_B_ff8a2b(void)
{	return end_mask_2 & 0xff;
}

B BLITTER::LOAD_B_ff8a2c(void)
{	return (end_mask_3 >> 8) & 0xff;
}

B BLITTER::LOAD_B_ff8a2d(void)
{	return end_mask_3 & 0xff;
}

B BLITTER::LOAD_B_ff8a32(void)
{	return (dest_addr >> 24);
}

B BLITTER::LOAD_B_ff8a33(void)
{	return (dest_addr >> 16) & 0xff;
}

B BLITTER::LOAD_B_ff8a34(void)
{	return (dest_addr >> 8) & 0xff;
}

B BLITTER::LOAD_B_ff8a35(void)
{	return (dest_addr) & 0xff;
}

B BLITTER::LOAD_B_ff8a36(void)
{	return (x_count >> 8) & 0xff;
}

B BLITTER::LOAD_B_ff8a37(void)
{	return (x_count) & 0xff;
}

B BLITTER::LOAD_B_ff8a38(void)
{	return (y_count >> 8) & 0xff;
}

B BLITTER::LOAD_B_ff8a39(void)
{	return (y_count) & 0xff;
}

B BLITTER::LOAD_B_ff8a3a(void)
{	return (B) hop;
}

B BLITTER::LOAD_B_ff8a3b(void)
{	return (B) op;
}

B BLITTER::LOAD_B_ff8a3c(void)
{
	if (blit) {
		Do_Blit();
		blit = false;
	}
	return (B) line_num & 0x3f;
}

B BLITTER::LOAD_B_ff8a3d(void)
{	
	return (B) skewreg;
}

void BLITTER::STORE_B_ff8a32(B v)
{	dest_addr &= 0x00ffffff;
	dest_addr |= (v&0xff) << 24;
}

void BLITTER::STORE_B_ff8a33(B v)
{	dest_addr &= 0xff00ffff;
	dest_addr |= (v&0xff) << 16;
}

void BLITTER::STORE_B_ff8a34(B v)
{	dest_addr &= 0xffff00ff;
	dest_addr |= (v&0xff) << 8;
}

void BLITTER::STORE_B_ff8a35(B v)
{	dest_addr &= 0xffffff00;
	dest_addr |= (v & 0xfe);	// ignore LSB
}

void BLITTER::STORE_B_ff8a28(B v)
{	end_mask_1 &= 0x00ff;
	end_mask_1 |= (v&0xff) << 8;
}

void BLITTER::STORE_B_ff8a29(B v)
{	end_mask_1 &= 0xff00;
	end_mask_1 |= (v&0xff);
}

void BLITTER::STORE_B_ff8a2a(B v)
{	end_mask_2 &= 0x00ff;
	end_mask_2 |= (v&0xff) << 8;
}

void BLITTER::STORE_B_ff8a2b(B v)
{	end_mask_2 &= 0xff00;
	end_mask_2 |= (v&0xff);
}

void BLITTER::STORE_B_ff8a2c(B v)
{	end_mask_3 &= 0x00ff;
	end_mask_3 |= (v&0xff) << 8;
}

void BLITTER::STORE_B_ff8a2d(B v)
{	end_mask_3 &= 0xff00;
	end_mask_3 |= (v&0xff);
}

void BLITTER::STORE_B_ff8a36(B v)
{	x_count &= 0x00ff;
	x_count |= (v&0xff) << 8;
}

void BLITTER::STORE_B_ff8a37(B v)
{	x_count &= 0xff00;
	x_count |= (v&0xff);
}

void BLITTER::STORE_B_ff8a38(B v)
{	y_count &= 0x00ff;
	y_count |= (v&0xff) << 8;
}

void BLITTER::STORE_B_ff8a39(B v)
{	y_count &= 0xff00;
	y_count |= (v&0xff);
}

void BLITTER::STORE_B_ff8a3a(B v)
{	hop = v & 3;	/* h/ware reg masks out the top 6 bits! */
}

void BLITTER::STORE_B_ff8a3b(B v)
{	op = v & 15;	/* h/ware reg masks out the top 4 bits! */	
}

void BLITTER::STORE_B_ff8a3c(B v)
{	
	line_num   = (UB) v & 0x3f;
	if ((y_count !=0) && (v & 0x80)) /* Busy bit set and lines to blit? */
		blit = true;
}

void BLITTER::STORE_B_ff8a3d(B v)
{	
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

	if (source_x_inc < 0) {
		do_hop_op_N[hop][op]( *this );
	}
	else {
		do_hop_op_P[hop][op]( *this );
	}
}
