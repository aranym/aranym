/*
 * nf_basicset.cpp - NatFeat Basic Set
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

#include "cpu_emulation.h"
#include "nf_basicset.h"
#include "maptab.h"
#include "version.h"
#include "main.h"

#define DEBUG 0
#include "debug.h"


int32 NF_Name::dispatch(uint32 fncode)
{
	char buf[strlen(version_string) + 80];
	memptr name_ptr = getParameter(0);
	uint32 name_maxlen = getParameter(1);
	D(bug("NF_Name($%08x, %d)", name_ptr, name_maxlen));

	const char *text;
	switch(fncode) {
		case 0:/* get_pure_name(char *name, uint32 max_len) */
			text = name_string;
			break;

		case 1:	/* get_complete_name(char *name, uint32 max_len) */
			strcpy(buf, version_string);
			strcat(buf, " (Host: " OS_TYPE "/" CPU_TYPE ")");
			text = buf;
			break;

		default:
			text = "Unimplemented NF_NAME sub-id";
			break;
	}
	Host2AtariSafeStrncpy(name_ptr, text, name_maxlen-1);
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
	switch (fncode)
	{
	case 0: /* shutdown(HALT) */
		exit_val = 0;
		Quit680x0();
		break;
	case 1: /* shutdown(BOOT) */
		RestartAll(false);
		break;
	case 2: /* shutdown(COLDBOOT) */
		RestartAll(true);
		break;
	case 3: /* shutdown(POWEROFF) */
		exit_val = 0;
		Quit680x0();
		break;
	}
	return 0;
}

// shut down the application
int32 NF_Exit::dispatch(uint32 fncode)
{
	switch (fncode)
	{
	case 0:
		exit_val = getParameter(0);
		Quit680x0();
		break;
	}
	return 0;
}

// print text on standard error stream
// internally limited to 2048 characters for now
int32 NF_StdErr::dispatch(uint32 fncode)
{
	FILE *output = stderr;
	int32 put;
	
	DUNUSED(fncode);

	memptr str_ptr = getParameter(0);
	D(bug("NF_StdErr(%d, $%08x)", fncode, str_ptr));

	put = host_puts(output, str_ptr, 0);
	fflush(output); // ensure immediate data output
	return put;
}

uint32 NF_StdErr::host_puts(FILE *f, memptr s, int width)
{
	uint32 put = 0;
	
	if (!s)
	{
		fputs("(null)", f);
		put += 6;
		width -= 6;
	} else
	{
		unsigned char c;
		while ((c = ReadNFInt8(s++)) != 0)
		{
			put += host_putc(f, c);
			width--;
		}
	}
		
	while (width-- > 0)
	{
		put += host_putc(f, ' ');
	}
	
	return put;
}

uint32 NF_StdErr::host_putc(FILE *f, unsigned char c)
{
	uint32 put;
	unsigned short ch;
	
	if (c == 0x0d)
	{
		/* ignore CRs */
		return 0;
	}
	if (c == 0x0a)
	{
		fputc('\n', f);
		return 1;
	}
	ch = atari_to_utf16[c];
	if (ch < 0x80)
	{
		fputc(ch, f);
		put = 1;
	} else if (ch < 0x800)
	{
		fputc(((ch >> 6) & 0x3f) | 0xc0, f);
		fputc((ch & 0x3f) | 0x80, f);
		put = 2;
	} else 
	{
		fputc(((ch >> 12) & 0x0f) | 0xe0, f);
		fputc(((ch >> 6) & 0x3f) | 0x80, f);
		fputc((ch & 0x3f) | 0x80, f);
		put = 3;
	}
	return put;
}

/*
vim:ts=4:sw=4:
*/
