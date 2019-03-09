/*
 * nf_basicset.h - NatFeat Basic Set - declaration
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

#ifndef _NF_BASICSET_H
#define _NF_BASICSET_H

#include "nf_base.h"

class NF_Name : public NF_Base
{
public:
	const char *name() { return "NF_NAME"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

class NF_Version : public NF_Base
{
public:
	const char *name() { return "NF_VERSION"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

class NF_Shutdown : public NF_Base
{
public:
	const char *name() { return "NF_SHUTDOWN"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

class NF_Exit : public NF_Base
{
public:
	const char *name() { return "NF_EXIT"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

class NF_StdErr : public NF_Base
{
public:
	const char *name() { return "NF_STDERR"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
	static uint32 host_puts(FILE *f, memptr s, int width);
	static uint32 host_putc(FILE *f, unsigned char c);
};

#endif // _NF_BASICSET_H
