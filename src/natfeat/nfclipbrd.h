/*
 * nfclipbrd.cpp - NatFeat Clipboard
 *
 * Copyright (c) 2006 Standa Opichal of ARAnyM dev team (see AUTHORS)
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

#ifndef _NF_CLIPBRD_H
#define _NF_CLIPBRD_H

#include "nf_base.h"

class ClipbrdNatFeat : public NF_Base
{
private:
	bool is_read;
	char *clip_buf;
	size_t clip_len;
	
public:
	ClipbrdNatFeat() 
	{
		is_read = false;
		clip_buf = NULL;
		clip_len = 0;
	}
	
	const char *name() { return "CLIPBRD"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
	void reset();

	int32 open(uint32 id, uint32 mode);
	int32 close(uint32 id);
	int32 read(uint32 id, memptr buff, uint32 size, uint32 pos);
	int32 write(uint32 id, memptr buff, uint32 len, uint32 pos);
};

#endif

/*
vim:ts=4:sw=4:
*/
