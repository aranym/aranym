/*
    clipbrd_darwin.cpp - Darwin/Mac OS X clipbrd interaction.
	Copied and adapted to Aranym needs from SDL_Clipboard prototype.

    Copyright (C) 2009 Philipp Donze of ARAnyM Team

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

#if defined(OS_darwin)
#include "sysdeps.h"
#include <Cocoa/Cocoa.h>

#include "SDL_compat.h"
#include "clipbrd.h"
#include "maptab.h"

#define DEBUG 0
#include "debug.h"

int init_aclip() { return 0; }

int filter_aclip(const SDL_Event *event) { return 1; }

char *read_aclip(size_t *len)
{
	char* clip_buf = NULL;
	int clip_len = 0;

	D(bug("read_aclip: getting string data"));
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
	NSString* type_available = [paste_board availableTypeFromArray: [NSArray arrayWithObject:NSStringPboardType]];
	NSString* pasteboard_string = [paste_board stringForType: NSStringPboardType];
	clip_len = pasteboard_string == nil ? 0 : [pasteboard_string length];
	if(clip_len > 0)
	{
		// Load unicode characters into buffer
		unichar *utf16_str = new unichar[clip_len];
		[pasteboard_string getCharacters:utf16_str];

		// Convert unicode string to standard Atari character map
		clip_buf = new char[clip_len + 1];
		char *dst = clip_buf;		
		for (int pos = 0; pos < clip_len; pos++) {
			unsigned short ch = utf16_str[pos];
			unsigned short c = utf16_to_atari[ch];
			if (c >= 0x100)
			{
				charset_conv_error(ch);
				*dst++ = '?';
			} else
			{
				*dst++ = c;
			}
		}
		*dst = '\0';
		
		clip_len = dst - clip_buf;
		delete [] utf16_str;		
	}
	[pool release];  // should be [pool drain] for >=10.4

	*len = clip_len;
	D(bug("read_aclip: clip_len %lu", (unsigned long)clip_len));
	return clip_buf;
}


void write_aclip(char* clip_buf, size_t clip_len)
{
	D(bug("write_aclip: writing raw data %p %d", clip_buf, clip_len));

	// Convert Atari string to unicode
	unichar *utf16_str = new unichar[clip_len];
	for(int pos = 0; pos < clip_len; pos++) {
		unsigned char ch = *clip_buf++;
		utf16_str[pos] = atari_to_utf16[ch];
	}
	
	// Write string to Mac OS X clipboard
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSPasteboard* paste_board = [NSPasteboard generalPasteboard];
	[paste_board declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
	[paste_board setString:[[NSString alloc] initWithCharacters:utf16_str length:clip_len] forType:NSStringPboardType];
	[pool release];  // should be [pool drain] for >=10.4

	delete [] utf16_str;
}

#endif
