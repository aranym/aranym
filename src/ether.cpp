/*
 *  ether.cpp - Ethernet device driver
 *
 *  Basilisk II (C) 1997-2002 Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  SEE ALSO
 *    Inside Macintosh: Devices, chapter 1 "Device Manager"
 *    Inside Macintosh: Networking, chapter 11 "Ethernet, Token Ring, and FDDI"
 *    Inside AppleTalk, chapter 3 "Ethernet and TokenTalk Link Access Protocols"
 */

#include "sysdeps.h"

#include "cpu_emulation.h"
#include "main.h"
#include "emul_op.h"
#include "ether.h"
#include "ether_defs.h"

#ifndef NO_STD_NAMESPACE
using std::map;
#endif

#define DEBUG 0
#include "debug.h"

#define MONITOR 0

// Global variables
uint8 ether_addr[6];			// Ethernet address (set by ether_init())
static bool net_open = false;	// Flag: initialization succeeded, network device open (set by EtherInit())

// Atari address of driver data in Atari RAM
memptr ether_data;


/*
 *  Initialization
 */
void EtherInit(void)
{
	net_open = false;
	if (ether_init())
		net_open = true;
}


/*
 *  Deinitialization
 */
void EtherExit(void)
{
	if (net_open) {
		ether_exit();
		net_open = false;
	}
}

/*
 *  Reset
 */
void EtherReset(void)
{
	ether_reset();
}

/*
 *  Driver Open() routine
 */
int16 EtherOpen(uint32 pb, uint32 dce)
{
	D(bug("EtherOpen"));

	// DODO - nahodit zarizeni (up)
	return 0;
}

/*
 *  Driver Close() routine
 */
int16 EtherClose(uint32 pb, uint32 dce)
{
	D(bug("EtherClose"));

	// DODO - shodit zarizeni (down)
	return 0;
}

/*
 *  Driver Control() routine
 */
int16 EtherControl(uint32 pb, uint32 dce)
{
	// Dodelat Config a IOctl z dummy.xif
	uint16 code = ReadInt16(pb + csCode);
	D(bug("EtherControl %d", code));
	switch (code) {
		case 1:					// KillIO
			return -1;
#if 0
		case kENetAddMulti:		// Add multicast address
			D(bug(" AddMulti %08x%04x", ReadInt32(pb + eMultiAddr), ReadInt16(pb + eMultiAddr + 4)));
			if (net_open && !udp_tunnel)
				return ether_add_multicast(pb);
			return noErr;

		case kENetDelMulti:		// Delete multicast address
			D(bug(" DelMulti %08x%04x\n", ReadInt32(pb + eMultiAddr), ReadInt16(pb + eMultiAddr + 4)));
			if (net_open && !udp_tunnel)
				return ether_del_multicast(pb);
			return noErr;
#endif
		case kENetWrite: {		// Transmit raw Ethernet packet
			uint32 wds = ReadInt32(pb + ePointer);
			D(bug(" EtherWrite "));
			if (ReadInt16(wds) < 14)
				return eLenErr;	// Header incomplete

			// Set source address
			uint32 hdr = ReadInt32(wds + 2);
			Host2Atari_memcpy(hdr + 6, ether_addr, 6);
			D(bug("to %08x%04x, type %04x", ReadInt32(hdr), ReadInt16(hdr + 4), ReadInt16(hdr + 12)));

			if (net_open) {
				return ether_write(wds);
			}
			return noErr;
		}

		case kENetGetInfo: {	// Get device information/statistics
			D(bug(" GetInfo buf %08x, size %d", ReadInt32(pb + ePointer), ReadInt16(pb + eBuffSize)));

			// Collect info (only ethernet address)
			uint8 buf[18];
			memset(buf, 0, 18);
			memcpy(buf, ether_addr, 6);

			// Transfer info to supplied buffer
			int16 size = ReadInt16(pb + eBuffSize);
			if (size > 18)
				size = 18;
			WriteInt16(pb + eDataSize, size);	// Number of bytes actually written
			Host2Atari_memcpy(ReadInt32(pb + ePointer), buf, size);
			return noErr;
		}
#if 0
		case kENetSetGeneral:	// Set general mode (always in general mode)
			D(bug(" SetGeneral"));
			return noErr;
#endif
		default:
			printf("WARNING: Unknown EtherControl(%d)\n", code);
			return controlErr;
	}
}

/*
 *  Ethernet ReadPacket routine
 */
void EtherReadPacket(uint8 **src, uint32 &dest, uint32 &len, uint32 &remaining)
{
	D(bug("EtherReadPacket src %p, dest %08x, len %08x, remaining %08x", *src, dest, len, remaining));
	uint32 todo = len > remaining ? remaining : len;
	Host2Atari_memcpy(dest, *src, todo);
	*src += todo;
	dest += todo;
	len -= todo;
	remaining -= todo;
}
