/*
 * nfclipbrd.cpp - NatFeat Clipboard
 *
 * Copyright (c) 2006-2009 Standa Opichal of ARAnyM dev team (see AUTHORS)
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
#include "nfclipbrd.h"
#include "nfclipbrd_nfapi.h"
#include "clipbrd.h"


#define DEBUG 0
#include "debug.h"

int32 ClipbrdNatFeat::dispatch(uint32 fncode)
{
	switch(fncode) {
    	case GET_VERSION:
			D(bug("CLIPBRD: version %d", CLIPBRD_NFAPI_VERSION));
    		return CLIPBRD_NFAPI_VERSION;

		case CLIP_OPEN:
			return open( getParameter(0), getParameter(1) );
		case CLIP_CLOSE:
			return close( getParameter(0) );
		case CLIP_READ:
			return read( getParameter(0), getParameter(1), getParameter(2), getParameter(3) );
		case CLIP_WRITE:
			return write( getParameter(0), getParameter(1), getParameter(2), getParameter(3) );

		default:
			return -1;
	}
}

void ClipbrdNatFeat::reset()
{
	D(bug("clipbrd: reset"));
	if (init_aclip() < 0) {
		; // TODO disable clipboard
	}
}

int32 ClipbrdNatFeat::open(uint32 id, uint32 mode)
{
	DUNUSED(id);
	D(bug("clipbrd: open id=%d mode=%d", id, mode));
	is_read = (mode == 1);
	clip_len = 0;
	if (clip_buf) {
		delete [] clip_buf;
		clip_buf = NULL;
	}
	if (is_read) {
		clip_buf = read_aclip(&clip_len);
		if (!clip_buf) {
			clip_len = 0;
		}
	}
	return 0;
}

int32 ClipbrdNatFeat::close(uint32 id)
{
	DUNUSED(id);
	D(bug("clipbrd: close id=%d", id));
	if (clip_buf) {
		if (!is_read && clip_len > 0) {
			write_aclip(clip_buf, clip_len);
		}
		delete [] clip_buf;
		clip_buf = NULL;
	}
	return 0;
}

int32 ClipbrdNatFeat::read(uint32 id, memptr buff, uint32 size, uint32 pos)
{
	UNUSED(id);
	int len = (clip_len - pos > size) ? size : clip_len - pos;
	D(bug("clipbrd: read pos=%d, len=%d", pos, len));


	if (clip_buf) {
		if (len < 0) len = 0; 
		else if (len) Host2Atari_memcpy(buff, clip_buf + pos, len);
	}
	return len;
}

int32 ClipbrdNatFeat::write(uint32 id, memptr buff, uint32 len, uint32 pos)
{
	UNUSED(id);
	D(bug("clipbrd: write pos=%d, len=%d", pos, len));

	size_t newlen = pos + len;
	char *newbuf = new char[newlen];

	if (clip_buf) {
		memcpy(newbuf, clip_buf, (clip_len > newlen) ? pos : clip_len);
		delete [] clip_buf;
	}
	clip_buf = newbuf;
	clip_len = newlen;

	Atari2Host_memcpy(clip_buf + pos, buff, len);

	/* send to host os
	 *
	 * shortens the clip to pos+len (expects the writes to come sequentially
	 * and therefore the last write sets the complete clipboard contents)
	 */
	write_aclip(clip_buf, clip_len);
	
	return len;
}

// don't remove this modeline with intended formatting for vim:ts=4:sw=4:
