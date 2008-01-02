/*
 * nfbootstrap.h - NatFeat Bootstrap
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

#ifndef _NF_BOOTSTRAP_H
#define _NF_BOOTSTRAP_H

#include "nf_base.h"

class BootstrapNatFeat : public NF_Base
{
public:
	const char *name() { return "BOOTSTRAP"; }
	bool isSuperOnly() { return false; }
	int32 dispatch(uint32 fncode);
};

#endif // _NF_BOOTSTRAP_H
