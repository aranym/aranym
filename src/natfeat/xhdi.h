/*
 * xhdi.h - XHDI like disk driver interface - declaration
 *
 * Copyright (c) 2002-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#ifndef _XHDI_H
#define _XHDI_H
#include "nf_base.h"
#include "parameters.h"

typedef memptr wmemptr;
typedef memptr lmemptr;

class XHDIDriver : public NF_Base
{
private:
	bx_atadevice_options_t ide0, ide1;

private:
	bx_atadevice_options_t *dev2disk(uint16 major, uint16 minor);
	void byteSwapBuf(uint8 *buf, int size);

protected:
	int32 XHDrvMap();
	int32 XHInqDriver(uint16 bios_device, memptr name, memptr version,
				memptr company, wmemptr ahdi_version, wmemptr maxIPL);
	int32 XHReadWrite(uint16 major, uint16 minor, uint16 rwflag,
				uint32 recno, uint16 count, memptr buf);
	int32 XHInqTarget2(uint16 major, uint16 minor, lmemptr blocksize,
				lmemptr device_flags, memptr product_name, uint16 stringlen);
	int32 XHInqDev2(uint16 bios_device, wmemptr major, wmemptr minor,
				lmemptr start_sector, memptr bpb, lmemptr blocks,
				memptr partid);
	int32 XHGetCapacity(uint16 major, uint16 minor,
				lmemptr blocks, lmemptr blocksize);

public:
	void reset();
	char *name() { return "XHDI"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif /* _XHDI_H */
