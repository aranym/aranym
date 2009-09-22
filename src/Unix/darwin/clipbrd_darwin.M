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
#define DEBUG 1
#include "debug.h"
#include <Cocoa/Cocoa.h>

int init_aclip() { return 0; }

char *read_aclip(size_t *len)
{
	char* clip_buf;
	int clip_len;
	NSPasteboard* paste_board;
	NSString* type_available;
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	paste_board = [NSPasteboard generalPasteboard];
	type_available = [paste_board availableTypeFromArray: [NSArray arrayWithObject:NSStringPboardType]];
	if([type_available isEqualToString:NSStringPboardType])
	{
		D(bug("read_from_clipboard: getting string data", read));
		/* Get the text string from the pasteboard */
		NSString* pasteboard_string;
		pasteboard_string = [paste_board stringForType: NSStringPboardType];
		clip_len = pasteboard_string == nil ? 0 : [pasteboard_string length];
		if(clip_len > 0)
		{
			/* Convert the NSString to a C string */
			NSData *converted_data = [pasteboard_string dataUsingEncoding: NSISOLatin1StringEncoding allowLossyConversion: YES];
			clip_len = [converted_data length];
			clip_buf = new char[clip_len+1];
			memcpy(clip_buf, [converted_data bytes], clip_len);
		}
	}
	else if(type_available != nil)
	{
		D(bug("read_from_clipboard: getting raw data", read));
		/* Get the raw data from the pasteboard */
		NSData* pasteboard_data;
		pasteboard_data = [paste_board dataForType:NSStringPboardType];
		clip_len = pasteboard_data == nil ? 0 : [pasteboard_data length];
		if(clip_len > 0)
		{
			clip_buf = new char[clip_len];
			memcpy(clip_buf, [pasteboard_data bytes], clip_len);
		}
	}
	[pool release];  // should be [pool drain] for >=10.4

	*len = clip_len;
	return clip_buf;
}


void write_aclip(char* clip_buf, size_t clip_len)
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	NSPasteboard* paste_board;
	paste_board = [NSPasteboard generalPasteboard];

	if(strlen(clip_buf) == clip_len)
	{
		/* null terminated C string here. src_len is ignored. */
		[paste_board declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
		[paste_board setString:[NSString stringWithUTF8String:clip_buf] forType:NSStringPboardType];
	}
	else
	{
		/* Any data */
		[paste_board declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
		[paste_board setData:[NSData dataWithBytes:clip_buf length:clip_len] forType:NSStringPboardType];
	}
	[pool release];  // should be [pool drain] for >=10.4
}

#endif
