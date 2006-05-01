/*
 * nfclipbrd.cpp - NatFeat Clipboard
 *
 * Copyright (c) 2006 Standa Opichal of ARAnyM dev team (see AUTHORS)
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

#ifdef OS_cygwin

/* from Unix/cygwin/clipbrd_cygwin.cpp
 * FIXME: generalize for all OSes -> make C++ Clipboarc class? */
void write_aclip(char *data, int len);
char * read_aclip( int *len);


#define DEBUG 0
#include "debug.h"

// load a TOS executable to the designated address
int32 ClipbrdNatFeat::dispatch(uint32 fncode)
{
	switch(fncode) {
    	case GET_VERSION:
			D(bug("CLIPBRD: version %ld", CLIPBRD_NFAPI_VERSION));
    		return CLIPBRD_NFAPI_VERSION;

		case CLIP_OPEN:
			return open( getParameter(0) );
		case CLIP_CLOSE:
			return close( getParameter(0) );
		case CLIP_READ:
			return read( getParameter(0), getParameter(1), getParameter(2), getParameter(3) );
		case CLIP_WRITE:
			return write( getParameter(0), getParameter(1), getParameter(2), getParameter(3) );

		default:;
				return -1;
	}
}

int32 ClipbrdNatFeat::open(uint32 id)
{
	D(bug("clipbrd: open id=%ld", id));
	return 0;
}

int32 ClipbrdNatFeat::close(uint32 id)
{
	D(bug("clipbrd: close id=%ld", id));
	return 0;
}

int32 ClipbrdNatFeat::read(uint32 id, memptr buff, uint32 size, uint32 pos)
{
	int len;
	char *data = read_aclip( &len);

	D(bug("clipbrd: read pos=%ld, len=%ld", pos, len));

	len = len-pos>size ? size : len-pos;
	if ( data ) {
		if ( len < 0 ) len = 0; 
		if ( len ) Host2Atari_memcpy(buff, data + pos, len);
		delete []data;
	}
	return len;
}

int32 ClipbrdNatFeat::write(uint32 id, memptr buff, uint32 len, uint32 pos)
{
	int clen;
	char *cdata = read_aclip( &clen);
	char *data = new char[pos+len];

	D(bug("clipbrd: write pos=%ld, len=%d", pos, len));

	if ( cdata ) {
		memcpy( data, cdata, clen>pos+len ? pos : clen);
		delete []cdata;
	}

	Atari2Host_memcpy(data + pos, buff, len);

	/* send to host os */
	write_aclip( data, pos+len);

	delete []data;
	return len;
}

#endif

/*
vim:ts=4:sw=4:
*/
