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

#if defined(OS_cygwin) || defined(OS_mingw)
#ifndef WIN32
#define WIN32 1
#endif
#include "SDL_compat.h"
#include <SDL_syswm.h>
#include <windows.h>
#ifdef __CYGWIN__
#undef WIN32
#endif
#include "clipbrd.h"
#include "host.h"

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

	SDL_SysWMinfo pInfo;
	HWND hwnd;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GetWindowWMInfo(host->video->Window(), &pInfo);
	hwnd = pInfo.info.win.window;
#else
	SDL_GetWMInfo(&pInfo);
	hwnd = pInfo.window;
#endif

	if (OpenClipboard(hwnd)) {
		EmptyClipboard();
		SetClipboardData(CF_TEXT, clipdata);
		CloseClipboard();
	} else
		GlobalFree(clipdata);
}

char * read_aclip( size_t *len)
{
	SDL_SysWMinfo pInfo;
	HWND hwnd;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GetWindowWMInfo(host->video->Window(), &pInfo);
	hwnd = pInfo.info.win.window;
#else
	SDL_GetWMInfo(&pInfo);
	hwnd = pInfo.window;
#endif

	if (OpenClipboard(hwnd)) {
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

