/*
 * nf_base.cpp - NatFeat base
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

#include "cpu_emulation.h"
#include "nf_base.h"

void NF_Base::a2fmemcpy( char *dest, memptr source, size_t count )
{
	while ( count-- )
		*dest++ = (char)ReadInt8( (uint32)source++ );
}

void NF_Base::f2amemcpy( memptr dest, char *source, size_t count )
{
	while ( count-- )
		WriteInt8( dest++, (uint8)*source++ );
}

void NF_Base::a2fstrcpy( char *dest, memptr source )
{
	while ( (*dest++ = (char)ReadInt8( (uint32)source++ )) != 0 );
}

void NF_Base::f2astrcpy( memptr dest, char *source )
{
	while ( *source )
		WriteInt8( dest++, (uint8)*source++ );
	WriteInt8( dest, 0 );
}

void NF_Base::atari2HostSafeStrncpy( char *dest, memptr source, size_t count )
{
	while ( count > 1 && (*dest = (char)ReadInt8( (uint32)source++ )) != 0 ) {
		count--;
		dest++;
	}
	if (count > 0)
		*dest = '\0';
}

void NF_Base::host2AtariSafeStrncpy( memptr dest, char *source, size_t count )
{
	while ( count > 1 && *source ) {
		WriteInt8( dest++, (uint8)*source++ );
		count--;
	}
	if (count > 0)
		WriteInt8( dest, 0 );
}


