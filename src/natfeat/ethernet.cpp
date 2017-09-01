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


// Ethernet runs at interrupt level 3 by default but can be reconfigured
#if 1
# define INTLEVEL	3
# define TRIGGER_INTERRUPT	TriggerInt3()
#else
# define INTLEVEL	5
# define TRIGGER_INTERRUPT	TriggerInt5()
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
			ret = INTLEVEL;
			break;

		case XIF_GET_MAC:	// what is the MAC address?
			/* store MAC address to provided buffer */
			{
				int ethX = getParameter(0);
				Handler *handler = getHandler(ethX);
				if (handler == NULL) {
					ret = 0; // return FALSE if ethX not defined
					break;
				}

				memptr buf_ptr = getParameter(1);	// destination buffer
				uint32 buf_size = getParameter(2);	// buffer size
				D(bug("Ethernet: getMAC(%d, %x, %d)", ethX, buf_ptr, (int)buf_size));

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
					panicbug("MAC Address of [ETH%d] in incorrect format", ethX);

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

					Handler *handler = getHandler(ethX);
					if (handler == NULL) {
						panicbug("Ethernet: handler for %d not found", ethX);
						return 0;
					}
					D(bug("Ethernet: ETH%d IRQ acknowledged", ethX));
					// Acknowledge interrupt to reception thread
					SDL_SemPost(handler->intAck);
					ret = 0;
				}
			}
			break;

		case XIF_START:
			if (startThread( getParameter(0) /* ethX */) == false)
				ret = TOS_EUNDEV;
			break;
		case XIF_STOP:
			stopThread( getParameter(0) /* ethX */);
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
			D(bug("XIF_GET_IPHOST"));
			ret = get_params(HOST_IP);
			break;
		case XIF_GET_IPATARI:
			D(bug("XIF_GET_IPATARI"));
			ret = get_params(ATARI_IP);
			break;
		case XIF_GET_NETMASK:
			D(bug("XIF_GET_NETMASK"));
			ret = get_params(NETMASK);
			break;
		default:
			D(bug("XIF: unsupported function %d", fncode));
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

	D(bug("Ethernet: getPAR(%d) for eth%d to buffer at %x of size %d",
			which, ethX, name_ptr, name_maxlen));

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
	Handler *handler = getHandler(ethX);
	if (handler == NULL) {
		panicbug("Ethernet: handler for %d not found", ethX);
		return 0;
	}
	return handler->packet_length;
}

/*
 *  ETHERNETDriver ReadPacket routine
 */

void ETHERNETDriver::readPacket(int ethX, memptr buffer, uint32 len)
{
	Handler *handler = getHandler(ethX);
	if (handler == NULL) {
		panicbug("Ethernet: handler for %d not found", ethX);
		return;
	}
	D(bug("Ethernet: ReadPacket dest %08x, len %x", buffer, len));
	Host2Atari_memcpy(buffer, handler->packet, MIN(len, MAX_PACKET_SIZE));
	if (len > MAX_PACKET_SIZE) {
		panicbug("ETHERNETDriver::readPacket() - length %d > %d", len, MAX_PACKET_SIZE);
	}
}


/*
 *  ETHERNETDriver writePacket routine
 */

void ETHERNETDriver::sendPacket(int ethX, memptr buffer, uint32 len)
{
	Handler *handler = getHandler(ethX);
	if (handler == NULL) {
		panicbug("Ethernet: handler for %d not found", ethX);
		return;
	}
	uint8 packetToWrite[MAX_PACKET_SIZE+2];

	D(bug("Ethernet: SendPacket src %08x, len %x", buffer, len));

	len = MIN(len, MAX_PACKET_SIZE);
	Atari2Host_memcpy( packetToWrite, buffer, len );

	// Transmit packet
	if (handler->send(packetToWrite, len) < 0) {
		D(bug("WARNING: Couldn't transmit packet"));
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
			stopThread(i);
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
			stopThread(i);
		}
	}
}

ETHERNETDriver::Handler *ETHERNETDriver::getHandler(int ethX)
{
	if (ethX >= 0 && ethX < MAX_ETH) {
		Handler *h = handlers[ethX];
		if (h != NULL) {
			assert(h->ethX == ethX);
			return h;
		}
	}

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
	Handler *handler = getHandler(ethX);
	if (handler == NULL) {
		panicbug("Ethernet: handler for %d not found", ethX);
		return false;
	}
	if (handler->handlingThread == NULL) {
		D(bug("Ethernet: Start thread"));

		if ((handler->intAck = SDL_CreateSemaphore(0)) == NULL) {
			D(bug("WARNING: Cannot init semaphore"));
			return false;
		}

		handler->handlingThread = SDL_CreateNamedThread( receiveFunc, "Ethernet", handler );
		if (handler->handlingThread == NULL) {
			D(bug("WARNING: Cannot start ETHERNETDriver thread"));
			return false;
		}
	}
	return true;
}


/*
 *  Stop packet reception thread
 */
void ETHERNETDriver::stopThread(int ethX)
{
	Handler *handler = getHandler(ethX);
	if (handler == NULL) {
		panicbug("Ethernet: handler for %d not found", ethX);
		return;
	}
	if (handler->handlingThread) {
		D(bug("Ethernet: Stop thread"));

#ifdef FIXME
		// pthread_cancel(handlingThread); // FIXME: set the cancel flag.
		SDL_WaitThread(handler->handlingThread, NULL);
		SDL_DestroySemaphore(handler->intAck);
#endif
		handler->handlingThread = NULL;
	}
}


/*
 *  Packet reception thread
 */
int ETHERNETDriver::receiveFunc(void *arg)
{
	Handler *handler = (Handler*)arg;

	// Call protocol handler for received packets
	// ssize_t length;
	for (;;) {
		// Read packet device
		handler->packet_length = handler->recv(handler->packet, MAX_PACKET_SIZE);

		// Trigger ETHERNETDriver interrupt (call the m68k side)
		D(bug(" packet received (len %d), triggering ETHERNETDriver interrupt", (int)handler->packet_length));

		pending_interrupts |= (1 << handler->ethX);
		TRIGGER_INTERRUPT;

		// Wait for interrupt acknowledge (m68k network driver read interrupt to finish)
		D(bug(" waiting for int acknowledge with pending irq mask %02x", pending_interrupts));
		SDL_SemWait(handler->intAck);
		pending_interrupts &= ~(1 << handler->ethX);
		D(bug(" int acknowledged, pending irq mask now %02x", pending_interrupts));
	}

	return 0;
}
