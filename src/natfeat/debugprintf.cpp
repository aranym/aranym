/*
 * Copyright 2002-2005 Petr Stehlik of the Aranym dev team
 *
 * printf routines Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "cpu_emulation.h"
#include "debugprintf.h"
#include "nf_basicset.h"

#define DEBUG 0
#include "debug.h"

int32 DebugPrintf::dispatch(uint32 fncode)
{
	FILE *output = stderr;

	DUNUSED(fncode);

	memptr str_ptr = getParameter(0);
	D(bug("DebugPrintf(%d, $%08x)", fncode, str_ptr));

	int ret = debugprintf(output, str_ptr, 1);
	fflush(output);

	return ret;
}

# define TIMESTEN(x)	((((x) << 2) + (x)) << 1)

uint32 DebugPrintf::debugprintf(FILE *f, memptr fmt, uint32 param)
{
	char c;
	char fill_char;

	uint32 len = 0;
	
	int width;
	int long_flag;
	
	int   i_arg;
	long  l_arg;

	while ((c = ReadNFInt8(fmt++)) != 0)
	{
		if (c != '%')
		{
			len += PUTC (f, c, 1);
			continue;
		}
		
		c = ReadNFInt8(fmt++);
		width = 0;
		long_flag = 0;
		fill_char = ' ';
		
		if (c == '0') {
			fill_char = '0';
			c = ReadNFInt8(fmt++);
		}
		
		while (c >= '0' && c <= '9')
		{
			width = TIMESTEN (width) + (c - '0');
			c = ReadNFInt8(fmt++);
		}
		
		if (c == 'l' || c == 'L')
		{
			long_flag = 1;
			c = ReadNFInt8(fmt++);
		}
		
		if (!c) break;
		
		switch (c)
		{
			case '%':
			{
				len += PUTC (f, c, width);
				break;
			}
			case 'c':
			{
				i_arg = (int)getParameter(param++);
				len += PUTC (f, i_arg, width);
				break;
			}
			case 's':
			{
				memptr buf = getParameter(param++);
				len += PUTS (f, buf, width);
				break;
			}
			case 'i':
			case 'd':
			{
				if (long_flag)
					l_arg = (long)getParameter(param++);
				else
					l_arg = (int)getParameter(param++);
				
				if (l_arg < 0)
				{
					len += PUTC (f, '-', 1);
					width--;
					l_arg = -l_arg;
				}
				
				len += PUTL (f, l_arg, 10, width, fill_char);
				break;
			}
			case 'o':
			{
				if (long_flag)
					l_arg = (long)getParameter(param++);
				else
					l_arg = (unsigned int)getParameter(param++);
				
				len += PUTL (f, l_arg, 8, width, fill_char);
				break;
			}
			case 'x':
			{
				if (long_flag)
					l_arg = (long)getParameter(param++);
				else
					l_arg = (unsigned int)getParameter(param++);
				
				len += PUTL (f, l_arg, 16, width, fill_char);
				break;
			}
			case 'b':
			{
				if (long_flag)
					l_arg = (long)getParameter(param++);
				else
					l_arg = (unsigned int)getParameter(param++);
				
				len += PUTL (f, l_arg, 2, width, fill_char);
				break;
			}
			case 'u':
			{
				if (long_flag)
					l_arg = (long)getParameter(param++);
				else
					l_arg = (unsigned int)getParameter(param++);
				
				len += PUTL (f, l_arg, 10, width, fill_char);
				break;
			}
		}
	}
	
	return len;
}

uint32 DebugPrintf::PUTC(FILE *f, int c, int width)
{
	uint32 put;
	
	put = NF_StdErr::host_putc(f, c);
	while (--width > 0)
	{
		put += NF_StdErr::host_putc(f, ' ');
	}
	
	return put;
}

uint32 DebugPrintf::PUTS(FILE *f, memptr s, int width)
{
	return NF_StdErr::host_puts(f, s, width);
}

uint32 DebugPrintf::PUTL(FILE *f, uint32 u, int base, int width, int fill_char)
{
	char obuf[32];
	char *t = obuf;
	uint32 put = 0;
	
	do {
		*t++ = "0123456789ABCDEF"[u % base];
		u /= base;
		width--;
	}
	while (u > 0);
	
	while (width-- > 0)
	{
		put += NF_StdErr::host_putc(f, fill_char);
	}
	
	while (t != obuf)
	{
		put += NF_StdErr::host_putc(f, *--t);
	}
	
	return put;
}

/*
vim:ts=4:sw=4:
*/
