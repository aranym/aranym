/*
 *  ether_unix.cpp - Ethernet device driver, Unix specific stuff (Linux and FreeBSD)
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

#include "sysdeps.h"

#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef HAVE_NEW_HEADERS
# include <cerrno>
# include <cstdio>
# include <map>
#else
# include <errno.h>
# include <stdio.h>
# include <map.h>
#endif

#if defined(__FreeBSD__) || defined(sgi)
#include <net/if.h>
#endif

#include "cpu_emulation.h"
#include "emul_op.h"
#include "main.h"
#include "ether.h"
#include "ether_defs.h"

#ifndef NO_STD_NAMESPACE
using std::map;
#endif

#define DEBUG 0
#include "debug.h"

#include <SDL.h>
#include <SDL_thread.h>


#define MONITOR 0


// Global variables
static int fd = -1;							// fd of sheep_net device
static SDL_Thread *ether_thread;			// Packet reception thread
static SDL_sem *int_ack;					// Interrupt acknowledge semaphore
static bool is_ethertap;					// Flag: Ethernet device is ethertap
static bool udp_tunnel;						// Flag: UDP tunnelling active, fd is the socket descriptor

// Attached network protocols, maps protocol type to MacOS handler address
static map<uint16, uint32> net_protocols;

// Prototypes
static int receive_func(void *arg);


/*
 *  Start packet reception thread
 */

static bool start_thread(void)
{
	if ((int_ack = SDL_CreateSemaphore(0)) == NULL) {
		printf("WARNING: Cannot init semaphore");
		return false;
	}

	ether_thread = SDL_CreateThread( receive_func, NULL );
	if (!ether_thread) {
		printf("WARNING: Cannot start Ethernet thread");
		return false;
	}

	return true;
}


/*
 *  Stop packet reception thread
 */

static void stop_thread(void)
{
	if (ether_thread) {
		// pthread_cancel(ether_thread); // FIXME: set the cancel flag.
		SDL_WaitThread(ether_thread, NULL);
		SDL_DestroySemaphore(int_ack);
		ether_thread = NULL;
	}
}


/*
 *  Initialization
 */

bool ether_init(void)
{
	int nonblock = 1;

	// Do nothing if no Ethernet device specified
	const char *name = "tap0"; // FIXME: configuration? PrefsFindString("ether");
	if (name == NULL)
		return false;

	// Is it Ethertap?
	is_ethertap = (strncmp(name, "tap", 3) == 0);

	// Open sheep_net or ethertap device
	char dev_name[16];
	if (is_ethertap)
		sprintf(dev_name, "/dev/%s", name);
	else
		strcpy(dev_name, "/dev/sheep_net");
	fd = open(dev_name, O_RDWR);
	if (fd < 0) {
		panicbug("STR_NO_SHEEP_NET_DRIVER_WARN '%s': %s", dev_name, strerror(errno));
		goto open_error;
	}

#if defined(__linux__)
	// Attach sheep_net to selected Ethernet card
	if (!is_ethertap && ioctl(fd, SIOCSIFLINK, name) < 0) {
		panicbug("STR_SHEEP_NET_ATTACH_WARN: %s", strerror(errno));
		goto open_error;
	}
#endif

	// Set nonblocking I/O
	ioctl(fd, FIONBIO, &nonblock);

	// Get Ethernet address
	if (is_ethertap) {
		pid_t p = getpid();	// If configured for multicast, ethertap requires that the lower 32 bit of the Ethernet address are our PID
		ether_addr[0] = 0xfe;
		ether_addr[1] = 0xfd;
		ether_addr[2] = p >> 24;
		ether_addr[3] = p >> 16;
		ether_addr[4] = p >> 8;
		ether_addr[5] = p;
	} else
		ioctl(fd, SIOCGIFADDR, ether_addr);
	D(bug("Ethernet address %02x %02x %02x %02x %02x %02x\n", ether_addr[0], ether_addr[1], ether_addr[2], ether_addr[3], ether_addr[4], ether_addr[5]));

	// Start packet reception thread
	if (!start_thread())
		goto open_error;

	// Everything OK
	return true;

open_error:
	stop_thread();

	if (fd > 0) {
		close(fd);
		fd = -1;
	}
	return false;
}


/*
 *  Deinitialization
 */

void ether_exit(void)
{
	// Stop reception thread
	if (ether_thread) {
		// pthread_cancel(ether_thread);
		SDL_WaitThread(ether_thread, NULL);
		SDL_DestroySemaphore(int_ack);
		ether_thread = NULL;
	}

	// Close sheep_net device
	if (fd > 0)
		close(fd);
}


/*
 *  Reset
 */

void ether_reset(void)
{
}


/*
 *  Add multicast address
 */

int16 ether_add_multicast(uint32 pb)
{
	if (ioctl(fd, SIOCADDMULTI, Atari2HostAddr(pb + eMultiAddr)) < 0) {
		D(bug("WARNING: Couldn't enable multicast address\n"));
		if (is_ethertap)
			return noErr;
		else
			return eMultiErr;
	} else
		return noErr;
}


/*
 *  Delete multicast address
 */

int16 ether_del_multicast(uint32 pb)
{
	if (ioctl(fd, SIOCDELMULTI, Atari2HostAddr(pb + eMultiAddr)) < 0) {
		D(bug("WARNING: Couldn't disable multicast address\n"));
		return eMultiErr;
	} else
		return noErr;
}

/*
 *  Transmit raw ethernet packet
 */

int16 ether_write(uint32 wds)
{
	// Copy packet to buffer
	uint8 packet[1516], *p = packet;
	int len = 0;
#if defined(__linux__)
	if (is_ethertap) {
		*p++ = 0;	// Linux ethertap discards the first 2 bytes
		*p++ = 0;
		len += 2;
	}
#endif
	len += ether_wds_to_buffer(wds, p);

#if MONITOR
	bug("Sending Ethernet packet:\n");
	for (int i=0; i<len; i++) {
		bug("%02x ", packet[i]);
	}
	bug("\n");
#endif

	// Transmit packet
	if (write(fd, packet, len) < 0) {
		D(bug("WARNING: Couldn't transmit packet\n"));
		return excessCollsns;
	} else
		return noErr;
}

/*
 *  Packet reception thread
 */

static int receive_func(void *arg)
{
	for (;;) {

		// Wait for packets to arrive
		struct pollfd pf = {fd, POLLIN, 0};
		int res = poll(&pf, 1, -1);
		if (res <= 0)
			break;

		// Trigger Ethernet interrupt
		D(bug(" packet received, triggering Ethernet interrupt\n"));
		SetInterruptFlag(INTFLAG_ETHER);
		TriggerInterrupt();

		// Wait for interrupt acknowledge by EtherInterrupt()
		SDL_SemWait(int_ack);
	}
	return 0;
}



/*
 *  Read packet from UDP socket
 */

void ether_udp_read(uint8 *packet, int length, struct sockaddr_in *from)
{
        // Drop packets sent by us
        if (memcmp(packet + 6, ether_addr, 6) == 0)
                return;

#if MONITOR
        bug("Receiving Ethernet packet:\n");
        for (int i=0; i<length; i++) {
                bug("%02x ", packet[i]);
        }
        bug("\n");
#endif

        // Get packet type
        uint16 type = (packet[12] << 8) | packet[13];

#if 0
        // Look for protocol
        uint16 search_type = (type <= 1500 ? 0 : type);
        if (udp_protocols.find(search_type) == udp_protocols.end())
                return;
        uint32 handler = udp_protocols[search_type];
        if (handler == 0)
                return;
#endif

        // Copy header to RHA
        Host2Atari_memcpy(ether_data + ed_RHA, packet, 14);
        D(bug(" header %08x%04x %08x%04x %04x\n", ReadInt32(ether_data + ed_RHA), ReadInt16(ether_data + ed_RHA + 4), ReadInt32(ether_data + ed_RHA + 6), ReadInt16(ether_data + ed_RHA + 10), ReadInt16(ether_data + ed_RHA + 12)));

        // Call protocol handler
        M68kRegisters r;
        r.d[0] = type;                                                                  // Packet type
        r.d[1] = length - 14;                                                   // Remaining packet length (without header, for ReadPacket)
        r.a[0] = (uint32)packet + 14;                                   // Pointer to packet (host address, for ReadPacket)
        r.a[3] = ether_data + ed_RHA + 14;                              // Pointer behind header in RHA
        r.a[4] = ether_data + ed_ReadPacket;                    // Pointer to ReadPacket/ReadRest routines
        D(bug(" calling protocol handler %08x, type %08x, length %08x, data %08x, rha %08x, read_packet %08x\n", handler, r.d[0], r.d[1], r.a[0], r.a[3], r.a[4]));
        EmulOp(M68K_EMUL_OP_ETHER_READ_PACKET, &r);
}


/*
 *  Ethernet interrupt - activate deferred tasks to call IODone or protocol handlers
 */

void EtherInterrupt(void)
{
	D(bug("EtherIRQ\n"));

	// Call protocol handler for received packets
	uint8 packet[1516];
	ssize_t length;
	for (;;) {

		if (udp_tunnel) {

			// Read packet from socket
			struct sockaddr_in from;
			socklen_t from_len = sizeof(from);
			length = recvfrom(fd, packet, 1514, 0, (struct sockaddr *)&from, &from_len);
			if (length < 14)
				break;
			ether_udp_read(packet, length, &from);

		} else {

			// Read packet from sheep_net device
#if defined(__linux__)
			length = read(fd, packet, is_ethertap ? 1516 : 1514);
#else
			length = read(fd, packet, 1514);
#endif
			if (length < 14)
				break;

#if MONITOR
			bug("Receiving Ethernet packet:\n");
			for (int i=0; i<length; i++) {
				bug("%02x ", packet[i]);
			}
			bug("\n");
#endif

			// Pointer to packet data (Ethernet header)
			uint8 *p = packet;
#if defined(__linux__)
			if (is_ethertap) {
				p += 2;			// Linux ethertap has two random bytes before the packet
				length -= 2;
			}
#endif

			// Get packet type
			uint16 type = (p[12] << 8) | p[13];

#if 0
			// Look for protocol
			uint16 search_type = (type <= 1500 ? 0 : type);
			if (net_protocols.find(search_type) == net_protocols.end())
				continue;
			uint32 handler = net_protocols[search_type];

			// No default handler
			if (handler == 0)
				continue;
#endif
			// Copy header to RHA
			Host2Atari_memcpy(ether_data + ed_RHA, p, 14);
			D(bug(" header %08x%04x %08x%04x %04x\n", ReadInt32(ether_data + ed_RHA), ReadInt16(ether_data + ed_RHA + 4), ReadInt32(ether_data + ed_RHA + 6), ReadInt16(ether_data + ed_RHA + 10), ReadInt16(ether_data + ed_RHA + 12)));

			// Call protocol handler
			M68kRegisters r;
			r.d[0] = type;									// Packet type
			r.d[1] = length - 14;							// Remaining packet length (without header, for ReadPacket)
			r.a[0] = (uint32)p + 14;						// Pointer to packet (host address, for ReadPacket)
			r.a[3] = ether_data + ed_RHA + 14;				// Pointer behind header in RHA
			r.a[4] = ether_data + ed_ReadPacket;			// Pointer to ReadPacket/ReadRest routines
			D(bug(" calling protocol handler %08x, type %08x, length %08x, data %08x, rha %08x, read_packet %08x\n", handler, r.d[0], r.d[1], r.a[0], r.a[3], r.a[4]));
			EmulOp(M68K_EMUL_OP_ETHER_READ_PACKET, &r);
		}
	}

	// Acknowledge interrupt to reception thread
	D(bug(" EtherIRQ done\n"));
	SDL_SemPost(int_ack);
}
