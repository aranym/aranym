/*
 * nfbootstrap.cpp - NatFeat Bootstrap
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
#include "nfbootstrap.h"

#define DEBUG 0
#include "debug.h"

// load a TOS executable to the designated address
int32 BootstrapNatFeat::dispatch(uint32 fncode)
{
	switch(fncode) {
		case 0: {/* load_executable(char *addr, uint32 max_len) */
					memptr addr = getParameter(0);
					long size = getParameter(1);
					bug("NF BOOTSTRAP(%s -> $%08x, %ld)", bx_options.bootstrap_path, addr, size);

					long length = 0;
					FILE *fh = fopen( bx_options.bootstrap_path, "rb" );
					if ( fh ) {
						char buffer[2048];
						while ( size ) {
							long read = fread(buffer, 1, sizeof(buffer), fh);
							if ( read == 0 )
								break;

							/* the file size is bigger than the space we have? */
							if ( read > size )
								return 0;
							Host2Atari_memcpy(addr + length, buffer, read);

							length += read;
							size -= read;
						}
						fclose(fh);
					}
					bug("NF BOOTSTRAP($%08x, %ld) -> %ld", addr, size, length);
					return length;
				}

		case 1:	/* get_bootdrive() */
				return tolower(bx_options.bootdrive)-'a';

		case 2:	/* get_bootargs() */
				{
					memptr addr = getParameter(0);
					long size = getParameter(1);
					bug("NF BOOTSTRAP_ARGS(%s)", bx_options.bootstrap_args);
					Host2AtariSafeStrncpy(addr, bx_options.bootstrap_args, size);
				}
				return 0;

		default:;
				return -1;
	}
}

/*
vim:ts=4:sw=4:
*/
