/*
 * nf_basicset.cpp - NatFeat Basic Set
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
#include "nf_basicset.h"

#define DEBUG 0
#include "debug.h"

int32 NF_Name::dispatch(uint32 fncode)
{
	memptr name_ptr = getParameter(0);
	uint32 name_maxlen = getParameter(1);
	D(bug("NF_Name(%p, %d)", name_ptr, name_maxlen));

	// maybe the ValidAddr check is no longer necessary
	// hopefully the host2AtariSafeStrncpy would throw the bus error?
	if (! ValidAddr(name_ptr, true, name_maxlen))
		BUS_ERROR(name_ptr);

	char *text;
	switch(fncode) {
		case 0:/* get_pure_name(char *name, uint32 max_len) */
			text = NAME_STRING;
			break;

		case 1:	/* get_complete_name(char *name, uint32 max_len) */
			text = VERSION_STRING " (Host: " OS_TYPE "/" CPU_TYPE ")";
			break;

		default:
			text = "Unimplemented NF_NAME sub-id";
			break;
	}
	host2AtariSafeStrncpy(name_ptr, text, name_maxlen-1);
	return strlen(text);
}

// return version of the NF interface in the form HI.LO (currently 1.0)
int32 NF_Version::dispatch(uint32 /*fncode*/)
{
	return 0x00010000UL;
}

// shut down the application
int32 NF_Shutdown::dispatch(uint32 fncode)
{
	if (fncode == 0) {
		Quit680x0();
	}
	return 0;
}

// print text on standard error stream
// internally limited to 2048 characters for now
int32 NF_StdErr::dispatch(uint32 fncode)
{
	char buffer[2048];
	FILE *output = stderr;

	DUNUSED(fncode);

	memptr str_ptr = getParameter(0);
	D(bug("NF_StdErr(%d, %p)", fncode, str_ptr));

	// maybe the ValidAddr check is no longer necessary
	// hopefully the atari2HostSafeStrncpy would throw the bus error?
	if (! ValidAddr(str_ptr, false, 1))
		BUS_ERROR(str_ptr);

	atari2HostSafeStrncpy(buffer, str_ptr, sizeof(buffer));
	fputs(buffer, output);
	fflush(output); // ensure immediate data output
	return strlen(buffer);
}
