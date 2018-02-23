/*
    clipbrd_beos.cpp - BeOS clipbrd interaction.

    Copyright (C) 2011 François Revol <revol@free.fr>

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
#if defined(OS_beos)
#include <Clipboard.h>
#include <String.h>
#include <string.h>
#include <stdio.h>

#include "SDL_compat.h"
#include "clipbrd.h"

#define DEBUG 0
#include "debug.h"

int init_aclip() { return 0; }

int filter_aclip(const SDL_Event *event) { UNUSED(event); return 1; }

void write_aclip(char *data, size_t len)
{
	BMessage *clip = NULL;
	fprintf(stderr, "%s()\n", __FUNCTION__);
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		clip = be_clipboard->Data();
		if (clip) {
			BString text(data, len);
			text.RemoveAll("\r");
			clip->AddData("text/plain", B_MIME_TYPE, text.String(), text.Length());
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
}

char * read_aclip( size_t *len)
{
	const char *text;
	ssize_t textLen;
	BMessage *clip = NULL;
	char *data = NULL;
	fprintf(stderr, "%s()\n", __FUNCTION__);
	
	if (be_clipboard->Lock()) {
		clip = be_clipboard->Data();
		if (clip && clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &textLen) == B_OK) {
			BString t(text, textLen);
			t.ReplaceAll("\n", "\r\n");
			*len = t.Length();
	   		data = new char[*len];
			memcpy( data, t.String(), *len);
		}
		be_clipboard->Unlock();
	}
	return data;
}

#endif

