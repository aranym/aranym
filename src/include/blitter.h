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
	uint32 source_addr;
	uint32 dest_addr;
	bool blit;

public:
	BLITTER(memptr, uint32);
	void reset();
	virtual uint8 handleRead(memptr);
	virtual void handleWrite(memptr, uint8);

	uint16 LM_UW(memptr);
	void SM_UW(memptr, uint16);

private:
	void Do_Blit(void);

	char LOAD_B_ff8a28();
	char LOAD_B_ff8a29();
	char LOAD_B_ff8a2a();
	char LOAD_B_ff8a2b();
	char LOAD_B_ff8a2c();
	char LOAD_B_ff8a2d();
	char LOAD_B_ff8a32();
	char LOAD_B_ff8a33();
	char LOAD_B_ff8a34();
	char LOAD_B_ff8a35();
	char LOAD_B_ff8a36();
	char LOAD_B_ff8a37();
	char LOAD_B_ff8a38();
	char LOAD_B_ff8a39();
	char LOAD_B_ff8a3a();
	char LOAD_B_ff8a3b();
	char LOAD_B_ff8a3c();
	char LOAD_B_ff8a3d();

	void STORE_B_ff8a28(char);
	void STORE_B_ff8a29(char);
	void STORE_B_ff8a2a(char);
	void STORE_B_ff8a2b(char);
	void STORE_B_ff8a2c(char);
	void STORE_B_ff8a2d(char);
	void STORE_B_ff8a32(char);
	void STORE_B_ff8a33(char);
	void STORE_B_ff8a34(char);
	void STORE_B_ff8a35(char);
	void STORE_B_ff8a36(char);
	void STORE_B_ff8a37(char);
	void STORE_B_ff8a38(char);
	void STORE_B_ff8a39(char);
	void STORE_B_ff8a3a(char);
	void STORE_B_ff8a3b(char);
	void STORE_B_ff8a3c(char);
	void STORE_B_ff8a3d(char);
};

#endif /* _BLITTER_H */
