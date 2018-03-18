/*
 * nf_base.h - NatFeat common base
 *
 * Copyright (c) 2002-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#ifndef _NF_BASE_H
#define _NF_BASE_H

#include "natfeats.h"	/* nf_getparameter is defined there */

#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? (d - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? (d - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? (d - '1' + 26) : \
	 -1)
#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')

class NF_Base
{
public:
	NF_Base() {};
	virtual ~NF_Base() {};
	virtual void reset() {};
	virtual const char *name() = 0;
	virtual bool isSuperOnly() = 0;
	virtual int32 dispatch(uint32 fncode) = 0;
	uint32 getParameter(int i) { return nf_getparameter(i); }
	uint32 errnoHost2Mint( int unixerrno,int defaulttoserrno ) const;
};

#endif /* _NF_BASE_H */
