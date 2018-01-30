/*
    clipbrd_cygwin.cpp - Windows clipbrd interaction.

    Copyright (C) 2006 Standa Opichal of ARAnyM Team

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "sysdeps.h"
#if defined(OS_cygwin) || defined(OS_mingw)
#include "SDL_compat.h"
#include "win32_supp.h"
#include "clipbrd.h"
#include "host.h"
#include "maptab.h"

#define DEBUG 0
#include "debug.h"

int init_aclip() { return 0; }

int filter_aclip(const SDL_Event *event) { (void) event; return 1; }

void write_aclip(char *src, size_t len)
{
	HGLOBAL clipdata;
	unsigned short *dst;
	unsigned short ch;

	clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(unsigned short) * (len + 1));
	if (!clipdata)
		return;
	dst = (unsigned short *)GlobalLock(clipdata);
	if (!dst)
		return;
	size_t count = len;
	while ( count > 0)
	{
		ch = (unsigned char)*src++;
		if (ch == 0)
			break;
		ch = atari_to_utf16[ch];
		*dst++ = ch;
		count--;
	}
	*dst = 0;
	GlobalUnlock(clipdata);

	if (OpenClipboard(NULL)) {
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, clipdata);
		CloseClipboard();
	} else
	{
		GlobalFree(clipdata);
		D(bug("OpenClipboard failed: %s", win32_errstring(GetLastError())));
	}
}

char * read_aclip( size_t *dstlen)
{
	if (OpenClipboard(NULL)) {
		HANDLE hData = GetClipboardData( CF_UNICODETEXT );
		if (hData)
		{
			unsigned short *src = (unsigned short *)GlobalLock( hData );
			size_t len = GlobalSize(hData) / sizeof(unsigned short);
		   	char *data = new char[len + 1];
		   	char *dst = data;
		   	while (len)
		   	{
		   		unsigned short ch = *src++;
		   		if (ch == 0)
		   			break;
				unsigned short c = utf16_to_atari[ch];
				if (c >= 0x100)
				{
					charset_conv_error(ch);
					*dst++ = '?';
				} else
				{
					*dst++ = c;
				}
				len--;
		   	}
		   	*dst = '\0';
			*dstlen = dst - data;
			GlobalUnlock( hData );
			CloseClipboard();
	
			return data;
		}
	} else
	{
		D(bug("OpenClipboard failed: %s", win32_errstring(GetLastError())));
	}
	return NULL;
}

#endif
