/*
 * yamaha.h - Atari YM2149 emulation code - declaration
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#ifndef _YAMAHA_H
#define _YAMAHA_H

#include "icio.h"

class YAMAHA : public BASE_IO {
private:
	int active_reg;
	int yamaha_regs[16];

public:
	YAMAHA(memptr, uint32);
	~YAMAHA();
	void reset();
	virtual uint8 handleRead(memptr addr);
	virtual void handleWrite(memptr addr, uint8 value);
	int getFloppyStat();

	Parallel *parallel;
};

#endif /* _YAMAHA_H */
