/*
 * rtc.h - Atari NVRAM emulation code - declaration
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

#ifndef _RTC_H
#define _RTC_H

#include "icio.h"

class RTC : public BASE_IO {
private:
	uint8 index;
	char nvram_filename[512];

public:
	RTC(memptr, uint32);
	virtual ~RTC();
	void reset();
	void init(void);
	bool load(void);
	bool save(void);
	virtual uint8 handleRead(memptr);
	virtual void handleWrite(memptr, uint8);

private:
	void setAddr(uint8 value);
	uint8 getData();
	void setData(uint8);
	void setChecksum();
};

#endif // _RTC_H
