/*
 * ide.h - Falcon IDE emulation code - class definition
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


#ifndef _IDE_H
#define _IDE_H

#include "icio.h"

class IDE : public BASE_IO {
public:
	IDE(memptr, uint32);
	void reset();
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
	virtual uae_u16 handleReadW(uaecptr addr);
	virtual void handleWriteW(uaecptr addr, uae_u16 value);
	virtual uae_u32 handleReadL(uaecptr addr);
	virtual void handleWriteL(uaecptr addr, uae_u32 value);
};

#endif /* _IDE_H */
