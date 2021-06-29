/*
 * ethernet.cpp - Ethernet Card Emulation
 *
 * Copyright (c) 2002-2005 Standa Opichal, Petr Stehlik of ARAnyM team
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

#include <cassert>
#include "cpu_emulation.h"
#include "main.h"
#include "ethernet.h"
#include "tools.h"
#include "toserror.h"

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"
#include <SDL_thread.h>

#include "../../atari/network/ethernet/ethernet_nfapi.h"

/****************************
 * Configuration zone begins
 */

#ifdef OS_cygwin
#include "cygwin/ethernet_cygwin.h"
#else
#  ifdef OS_darwin
#    ifdef ENABLE_BPF
#      include "../Unix/MacOSX/ethernet_macosx.h"
#    else
#      include "../Unix/darwin/ethernet_darwin.h"
#    endif
#  else
#  include "linux/ethernet_linux.h"
#  endif
#endif


/*
 * Configuration zone ends
 **************************/

int ETHERNETDriver::pending_interrupts;

int32 ETHERNETDriver::dispatch(uint32 fncode)
{
	D(bug("Ethernet: Dispatch %d", fncode));

	int32 ret = 0;
	switch(fncode) {
		case GET_VERSION:
			D(bug("Ethernet: getVersion"));
			ret = ARAETHER_NFAPI_VERSION;
			break;

		case XIF_INTLEVEL:	// what interrupt level is used?
			D(bug("Ethernet: getINTlevel"));
			ret = bx_options.ethernet[0].intLevel;
			break;

		case XIF_GET_MAC:	// what is the MAC address?
			/* store MAC address to provided buffer */
			{
				int ethX = getParameter(0);
				Handler *handler = getHandler(ethX, false);
				if (handler == NULL) {
					ret = 0; // return FALSE if ethX not defined
					break;
				}

				memptr buf_ptr = getParameter(1);	// destination buffer
				uint32 buf_size = getParameter(2);	// buffer size
				D(bug("ETH%d: getMAC(%x, %d)", ethX, buf_ptr, (int)buf_size));

				// default MAC Address is just made up
				uint8 mac_addr[6] = {'\0','A','E','T','H', uint8('0'+ethX) };

				// convert user-defined MAC Address from string to 6 bytes array
				char *ms = bx_options.ethernet[ethX].mac_addr;
				bool format_OK = false;
				if (strlen(ms) == 2*6+5 && (ms[2] == ':' || ms[2] == '-')) {
					ms[2] = ms[5] = ms[8] = ms[11] = ms[14] = ':';
					int md[6] = {0, 0, 0, 0, 0, 0};
					int matched = sscanf(ms, "%02x:%02x:%02x:%02x:%02x:%02x",
						&md[0], &md[1], &md[2], &md[3], &md[4], &md[5]);
					if (matched == 6) {
						for(int i=0; i<6; i++)
							mac_addr[i] = md[i];
						format_OK = true;
					}
				}
				if (!format_OK) {
					panicbug("ETH%d: MAC Address of in incorrect format", ethX);

				}
				Host2Atari_memcpy(buf_ptr, mac_addr, MIN(buf_size, sizeof(mac_addr)));

				ret = 1; // TRUE
			}
			break;

		case XIF_IRQ: // interrupt raised by native side thread polling tap0 interface
			{
				int dev_bit = getParameter(0);
				if (dev_bit == 0) {
					// dev_bit = 0 means "tell me what devices want me to serve their interrupts"
					ret = pending_interrupts;
				}
				else {
					// otherwise the set bit means "I'm acknowledging this device's interrupt"
					int ethX = -1;
					switch(dev_bit) {
						case 0x01: ethX = 0; break;
						case 0x02: ethX = 1; break;
						case 0x04: ethX = 2; break;
						case 0x08: ethX = 3; break;
						case 0x10: ethX = 4; break;
						case 0x20: ethX = 5; break;
						case 0x40: ethX = 6; break;
						case 0x80: ethX = 7; break;
						default: panicbug("Ethernet: wrong XIF_IRQ(%d)", dev_bit); break;
					}

					Handler *handler = getHandler(ethX, true);
					if (handler == NULL) {
						return 0;
					}
					D(bug("ETH%d: IRQ acknowledged", ethX));
					// Acknowledge interrupt to reception thread
					pthread_cond_signal(&handler->intAck);
					ret = 0;
				}
			}
			break;

		case XIF_START:
			if (startThread(getParameter(0) /* ethX */) == false)
				ret = TOS_EUNDEV;
			break;
		case XIF_STOP:
			stopThread(getHandler(getParameter(0) /* ethX */, false));
			break;
		case XIF_READLENGTH:
			ret = readPacketLength( getParameter(0) /* ethX */);
			break;
		case XIF_READBLOCK:
			readPacket( getParameter(0) /* ethX */,
						getParameter(1) /* buff */,
						getParameter(2) /* len */ );
			break;
		case XIF_WRITEBLOCK:
			sendPacket( getParameter(0) /* ethX */,
						getParameter(1) /* buff */,
						getParameter(2) /* len */ );
			break;

		case XIF_GET_IPHOST:
			ret = get_params(HOST_IP);
			break;
		case XIF_GET_IPATARI:
			ret = get_params(ATARI_IP);
			break;
		case XIF_GET_NETMASK:
			ret = get_params(NETMASK);
			break;
		default:
			D(bug("Ethernet: unsupported function %d", fncode));
			ret = TOS_ENOSYS;
			break;
	}
	return ret;
}


int ETHERNETDriver::get_params(GET_PAR which)
{
	int ethX = getParameter(0);
	memptr name_ptr = getParameter(1);
	uint32 name_maxlen = getParameter(2);
	const char *text = NULL;

	D(bug("ETH%d: getPAR(%d) to buffer at %x of size %d",
			ethX, which, name_ptr, name_maxlen));

	if (ethX < 0 || ethX >= MAX_ETH)
	{
		panicbug("Ethernet: handler for %d not found", ethX);
		return 0;
	}
	if (! ValidAddr(name_ptr, true, name_maxlen))
		BUS_ERROR(name_ptr);

	switch(which) {
		case HOST_IP: text = bx_options.ethernet[ethX].ip_host; break;
		case ATARI_IP:text = bx_options.ethernet[ethX].ip_atari; break;
		case NETMASK: text = bx_options.ethernet[ethX].netmask; break;
		default: text = "";
	}

	Host2AtariSafeStrncpy(name_ptr, text, name_maxlen);
	return strlen(text);
}


int32 ETHERNETDriver::readPacketLength(int ethX)
{
	Handler *handler = getHandler(ethX, true);
	if (handler == NULL)
		return 0;
	return handler->packet_length;
}

/*
 *  ETHERNETDriver ReadPacket routine
 */

void ETHERNETDriver::readPacket(int ethX, memptr buffer, uint32 len)
{
	Handler *handler = getHandler(ethX, true);
	if (handler == NULL)
		return;
	D(bug("ETH%d: ReadPacket dest %08x, len %x", ethX, buffer, len));
	Host2Atari_memcpy(buffer, handler->packet, MIN(len, MAX_PACKET_SIZE));
	if (len > MAX_PACKET_SIZE) {
		panicbug("ETH%d: readPacket() - length %d > %d", ethX, len, MAX_PACKET_SIZE);
	}
}


/*
 *  Debug output
 */
void ETHERNETDriver::dump_ether_packet(const uint8_t *buf, int len)
{
	int i;
	int type = 0;
	const char *proto;

	/* ethernet header */	
	if (len >= 14)
	{
		/* dest mac */
		fprintf(stderr, "eth: dst %02x:%02x:%02x:%02x:%02x:%02x ",
			buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		/* source mac */
		fprintf(stderr, "src %02x:%02x:%02x:%02x:%02x:%02x ",
			buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
		/* ether type */
		type = (buf[12] << 8) | buf[13];
		fprintf(stderr, "type %04x  ", type);
		len -= 14;
		buf += 14;
	}

	/* IP header */
	if (len >= 20 && type == 0x800 && (buf[0] & 0xf0) == 0x40) /* IPv4 */
	{
		fprintf(stderr, "ip: ");
		/* version/hd_len/tos */
		fprintf(stderr, "hl %02x tos %02x ", buf[0], buf[1]);
		/* length */
		fprintf(stderr, "length %5d ", (buf[2] << 8) | buf[3]);
		/* ident */
		fprintf(stderr, "ident %5d ", (buf[4] << 8) | buf[5]);
		/* fragment */
		fprintf(stderr, "frag %04x ", (buf[6] << 8) | buf[7]);
		/* ttl/protocol */
		switch (buf[9])
		{
			case 1: proto = "ICMP"; break;
			case 2: proto = "IGMP"; break;
			case 3: proto = "GGP"; break;
			case 4: proto = "IPinIP"; break;
			case 5: proto = "ST"; break;
			case 6: proto = "TCP"; break;
			case 7: proto = "CBT"; break;
			case 8: proto = "EGP"; break;
			case 9: proto = "IGP"; break;
			case 10: proto = "BBN"; break;
			case 11: proto = "NVP"; break;
			case 12: proto = "PUP"; break;
			case 13: proto = "ARGUS"; break;
			case 14: proto = "EMCON"; break;
			case 15: proto = "XNET"; break;
			case 16: proto = "CHAOS"; break;
			case 17: proto = "UDP"; break;
			case 18: proto = "MUX"; break;
			case 21: proto = "PRM"; break;
			case 31: proto = "DCCP"; break;
			case 40: proto = "IL"; break;
			case 41: proto = "IPv6"; break;
			case 42: proto = "SDRP"; break;
			case 43: proto = "IPv6-Route"; break;
			case 44: proto = "IPv6-Frag"; break;
			case 56: proto = "TLSP"; break;
			case 92: proto = "MTP"; break;
			case 97: proto = "ETHERIP"; break;
			case 98: proto = "ENCAP"; break;
			case 121: proto = "SMP"; break;
			case 122: proto = "SM"; break;
			default: proto = "unknown"; break;
		}
		fprintf(stderr, "ttl %02x proto %02x(%s) ", buf[8], buf[9], proto);
		/* header checksum */
		fprintf(stderr, "csum %04x ", (buf[10] << 8) | buf[11]);
		/* source ip */
		fprintf(stderr, "src %d.%d.%d.%d ", buf[12], buf[13], buf[14], buf[15]);
		/* dest ip */
		fprintf(stderr, "dst %d.%d.%d.%d  ", buf[16], buf[17], buf[18], buf[19]);
		len -= 20;
		buf += 20;
	} else if (len >= 28 && (type == 0x0806 || type == 0x8035)) /* ARP/RARP */
	{
		if (type == 0x0806)
			fprintf(stderr, "arp: ");
		else
			fprintf(stderr, "rarp: ");
		/* hardware space */
		fprintf(stderr, "hws %04x ", (buf[0] << 8) | buf[1]);
		/* protocol space */
		fprintf(stderr, "prs %04x ", (buf[2] << 8) | buf[3]);
		/* hardware_len/protcol_len */
		fprintf(stderr, "hwl %02x prl %02x ", buf[4], buf[5]);
		/* opcode */
		fprintf(stderr, "opcode %04x ", (buf[6] << 8) | buf[7]);
		/* source mac */
		fprintf(stderr, "src %02x:%02x:%02x:%02x:%02x:%02x ",
			buf[8], buf[9], buf[10], buf[11], buf[12], buf[13]);
		/* source ip */
		fprintf(stderr, "%d.%d.%d.%d ", buf[14], buf[15], buf[16], buf[17]);
		/* dest mac */
		fprintf(stderr, "dst %02x:%02x:%02x:%02x:%02x:%02x ",
			buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);
		/* dest ip */
		fprintf(stderr, "%d.%d.%d.%d ", buf[24], buf[25], buf[26], buf[27]);
		len -= 28;
		buf += 28;
	}

	fprintf(stderr, " data:");
	/* payload data */
	for (i = 0; i < len; i++)
		fprintf(stderr, " %02x", buf[i]);
	fprintf(stderr, "\n");
	fflush(stderr);
}


/*
 *  ETHERNETDriver writePacket routine
 */
void ETHERNETDriver::sendPacket(int ethX, memptr buffer, uint32 len)
{
	Handler *handler = getHandler(ethX, true);
	if (handler == NULL)
		return;
	uint8 packetToWrite[MAX_PACKET_SIZE+2];

	D(bug("ETH%d: SendPacket src %08x, len %x", ethX, buffer, len));

	len = MIN(len, MAX_PACKET_SIZE);
	Atari2Host_memcpy( packetToWrite, buffer, len );

	if (handler->debug)
	{
		fprintf(stderr, "ETH%d: send %4d:", ethX, len);
		dump_ether_packet(packetToWrite, len);
	}

	// Transmit packet
	if (handler->send(packetToWrite, len) < 0) {
		D(bug("ETH%d: WARNING: Couldn't transmit packet", ethX));
	}
}


/*
 *  Initialization
 */
bool ETHERNETDriver::init(void)
{
	pending_interrupts = 0;

	for(int i=0; i<MAX_ETH; i++) {
		Handler *handler = new ETHERNET_HANDLER_CLASSNAME(i);
		if ( handler->open() ) {
			handlers[i] = handler;
		}
		else {
			delete handler;
			handlers[i] = NULL;
		}
	}
	return true; // kinda unnecessary
}


/*
 *  Deinitialization
 */
void ETHERNETDriver::exit()
{
	D(bug("Ethernet: exit"));

	for(int i=0; i<MAX_ETH; i++) {
		// Stop reception thread
		Handler *handler = handlers[i];
		if ( handler ) {
			stopThread(handler);
			handler->close();
			delete handler;
			handlers[i]= NULL;
		}
	}
}

// reset, called upon OS reboot
void ETHERNETDriver::reset()
{
	D(bug("Ethernet: reset"));

	for(int i=0; i<MAX_ETH; i++) {
		// Stop reception thread
		Handler *handler = handlers[i];
		if ( handler ) {
			stopThread(handler);
		}
	}
}

ETHERNETDriver::Handler *ETHERNETDriver::getHandler(int ethX, bool msg)
{
	if (ethX >= 0 && ethX < MAX_ETH) {
		Handler *h = handlers[ethX];
		if (h != NULL) {
			assert(h->ethX == ethX);
			return h;
		}
	}

	if (msg)
		panicbug("Ethernet: handler for %d not found", ethX);

	return NULL;
}

// ctor
ETHERNETDriver::ETHERNETDriver()
{
	init();
}

// destructor, called on exit automatically
ETHERNETDriver::~ETHERNETDriver()
{
	exit();
}

/*
 *  Start packet reception thread
 */
bool ETHERNETDriver::startThread(int ethX)
{
    pthread_attr_t attr;

	Handler *handler = getHandler(ethX, true);
	if (handler == NULL)
		return false;
	if (handler->handlingThread == 0) {
		D(bug("ETH%d: Start thread", handler->ethX));

		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		if (pthread_create(&handler->handlingThread, &attr, receiveFunc, handler) < 0)
		{
			D(bug("ETH%d: WARNING: Cannot start ETHERNETDriver thread", ethX));
			return false;
		}
	}
	return true;
}


/*
 *  Stop packet reception thread
 */
void ETHERNETDriver::stopThread(Handler *handler)
{
	if (handler == NULL)
		return;
	if (handler->handlingThread) {
		D(bug("ETH%d: Stop thread", handler->ethX));

		pthread_cancel(handler->handlingThread);
		pthread_join(handler->handlingThread, NULL);
		handler->handlingThread = 0;
	}
}


/*
 *  Packet reception thread
 */
void *ETHERNETDriver::receiveFunc(void *arg)
{
	Handler *handler = (Handler*)arg;

	// Call protocol handler for received packets
	// ssize_t length;
	for (;;) {
		// Read packet device
		handler->packet_length = handler->recv(handler->packet, MAX_PACKET_SIZE);

		// Trigger ETHERNETDriver interrupt (call the m68k side)
		D(bug("ETH%d: packet received (len %d), triggering ETHERNETDriver interrupt", handler->ethX, (int)handler->packet_length));
		if (handler->debug)
		{
			fprintf(stderr, "ETH%d: recv %4d:", handler->ethX, (int)handler->packet_length);
			dump_ether_packet(handler->packet, handler->packet_length);
		}

		/*
		 * The atari driver does not like to see negative values here
		 */
		if (handler->packet_length < 0)
			handler->packet_length = 0;
		
		/* but needs to be triggered, anyway */
		pthread_mutex_lock(&handler->intLock);
		pending_interrupts |= (1 << handler->ethX);
		// Ethernet runs at interrupt level 3 by default but can be reconfigured
		switch (bx_options.ethernet[0].intLevel)
		{
		default:
		case 3:
			TriggerInt3();
			break;
		case 4:
			TriggerVBL();
			break;
		case 5:
			TriggerInt5();
			break;
		}
		// Wait for interrupt acknowledge (m68k network driver read interrupt to finish)
		D(bug("ETH%d: waiting for int acknowledge with pending irq mask %02x", handler->ethX, pending_interrupts));
		pthread_cond_wait(&handler->intAck, &handler->intLock);
		pending_interrupts &= ~(1 << handler->ethX);
		pthread_mutex_unlock(&handler->intLock);
		D(bug("ETH%d: int acknowledged, pending irq mask now %02x", handler->ethX, pending_interrupts));
	}

	return 0;
}
