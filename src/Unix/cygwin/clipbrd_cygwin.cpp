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

#if defined(OS_cygwin)
#define WIN32 1
#include <SDL.h>
#include <SDL_syswm.h>
#include <windows.h>
#undef WIN32
#include "clipbrd.h"

int init_aclip() { return 0; }

int filter_aclip(const SDL_Event *event) { (void) event; return 1; }

void write_aclip(char *data, size_t len)
{
	HGLOBAL clipdata;
	void *lock;

	clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len + 1);
	if (!clipdata)
		return;
	lock = GlobalLock(clipdata);
	if (!lock)
		return;
	memcpy(lock, data, len);
	((unsigned char *) lock)[len] = 0;
	GlobalUnlock(clipdata);

	static SDL_SysWMinfo pInfo;
	SDL_GetWMInfo(&pInfo);

	if (OpenClipboard(pInfo.window)) {
		EmptyClipboard();
		SetClipboardData(CF_TEXT, clipdata);
		CloseClipboard();
	} else
		GlobalFree(clipdata);
}

char * read_aclip( size_t *len)
{
	static SDL_SysWMinfo pInfo;
	SDL_GetWMInfo(&pInfo);

	if (OpenClipboard(pInfo.window)) {
		HANDLE hData = GetClipboardData( CF_TEXT );
		char* buffer = (char*)GlobalLock( hData );
		char *data;
		*len = strlen(buffer);
	   	data = new char[*len];
		memcpy( data, buffer, *len);
		GlobalUnlock( hData );
		CloseClipboard();

		return data;
	}
	return NULL;
}

#endif

