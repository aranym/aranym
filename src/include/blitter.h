/*
 * blitter.h - Atari Blitter emulation code - declaration
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

#ifndef _BLITTER_H
#define _BLITTER_H

#include "icio.h"

class BLITTER : public BASE_IO {
  public:
	uint16 halftone_ram[16];
	uint16 end_mask_1,end_mask_2,end_mask_3;
	uint8 NFSR,FXSR; 
	uint16 x_count,y_count;
	uint8 hop,op,line_num,skewreg;
	short int halftone_curroffset,halftone_direction;
	short int source_x_inc, source_y_inc, dest_x_inc, dest_y_inc;
	uint32 source_addr, source_addr_backup;
	uint32 dest_addr, dest_addr_backup;
	bool blit;

public:
	BLITTER(memptr, uint32);
	void reset();
	virtual uint8 handleRead(memptr);
	virtual void handleWrite(memptr, uint8);
	virtual uae_u16 handleReadW(uaecptr addr);
	virtual void handleWriteW(uaecptr addr, uae_u16 value);
	virtual uae_u32 handleReadL(uaecptr addr);
	virtual void handleWriteL(uaecptr addr, uae_u32 value);

	uint16 LM_UW(memptr);
	void SM_UW(memptr, uint16);

private:
	void Do_Blit(void);
	char LOAD_B_ff8a3c();
	void STORE_B_ff8a3a(char);
	void STORE_B_ff8a3b(char);
	void STORE_B_ff8a3c(char);
	void STORE_B_ff8a3d(char);
};

#endif /* _BLITTER_H */
