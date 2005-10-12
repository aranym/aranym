/**
 * Ethernet Card Emulation
 *
 * Standa and Joy of ARAnyM team (c) 2002-2005
 *
 * GPL
 */

#include "cpu_emulation.h"
#include "main.h"
#include "ethernet.h"
#include "tools.h"

#define DEBUG 0
#include "debug.h"


#include <SDL.h>
#include <SDL_thread.h>

#include "../../atari/network/ethernet/ethernet_nfapi.h"

/****************************
 * Configuration zone begins
 */

#ifdef OS_cygwin
#include "cygwin/ethernet_cygwin.h"
#else
#include "linux/ethernet_linux.h"
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

int32 ETHERNETDriver::dispatch(uint32 fncode)
{
	D(bug("Ethernet: Dispatch %d", fncode));

	// If disabled then do nothing (the initialization didn't went through)
	//// FIXME if ( !handler ) return 0;

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
				// TODO: make sure ethX is defined in ARAnyM configuration
				if (ethX != 0 && ethX != 1) { // TODO, currently hacked to allow ETH0/1 only
					ret = 0; // return FALSE if ethX not defined
					break;
				}

				memptr buf_ptr = getParameter(1);	// destination buffer
				uint32 buf_size = getParameter(2);	// buffer size
				D(bug("Ethernet: getMAC(%d, %p, %d", ethX, buf_ptr, buf_size));

				if (! ValidAddr(buf_ptr, true, buf_size))
					BUS_ERROR(buf_ptr);

				// generate the MAC as '\0AETH0' for eth0
				// CAUTION: the 'ARETH0' wasn't a good choice
				// (Linux bridging didn't work with that)
				uint8 mac_addr[6] = {'\0','A','E','T','H', '0'+ethX };
				Host2Atari_memcpy(buf_ptr, mac_addr, MIN(buf_size, sizeof(mac_addr)));

				ret = 1; // TRUE
			}
			break;

		case XIF_IRQ: // interrupt raised by native side thread polling tap0 interface
			{
			int dev_bit = getParameter(0);
			if (dev_bit == 0) {
				// dev_bit = 0 means "tell me what devices want me to serve their interrupts"
				ret = 1;	/* eth0 requested the interrupt */
//				ret = 2;	/* eth1 requested the interrupt */
//				ret = 4;	/* eth2 requested the interrupt */
//				ret = 8;	/* eth3 requested the interrupt */
			}
			else {
				// otherwise the set bit means "I'm acknowledging this device's interrupt"
				int ethX = -1;
				switch(dev_bit) {
					case 0x01: ethX = 0; break;
					case 0x02: ethX = 1; break;
					case 0x04: ethX = 2; break;
					case 0x08: ethX = 3; break;
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
			startThread( getParameter(0) /* ethX */);
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
			D(bug("XIF_GET_IPHOST\n"));
			ret = get_params(HOST_IP);
			break;
		case XIF_GET_IPATARI:
			D(bug("XIF_GET_IPATARI\n"));
			ret = get_params(ATARI_IP);
			break;
		case XIF_GET_NETMASK:
			D(bug("XIF_GET_NETMASK\n"));
			ret = get_params(NETMASK);
			break;
	}
	return ret;
}


int ETHERNETDriver::get_params(GET_PAR which)
{
	int ethX = getParameter(0);
	DUNUSED(ethX); // FIXME
	memptr name_ptr = getParameter(1);
	uint32 name_maxlen = getParameter(2);
	char *text = NULL;

	D(bug("Ethernet: getPAR(%d) for eth%d to buffer at %p of size %d",
			which, ethX, name_ptr, name_maxlen));

	if (! ValidAddr(name_ptr, true, name_maxlen))
		BUS_ERROR(name_ptr);

	switch(which) {
		case HOST_IP: text = bx_options.ethernet.ip_host; break;
		case ATARI_IP:text = bx_options.ethernet.ip_atari; break;
		case NETMASK: text = bx_options.ethernet.netmask; break;
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
	D(bug("Ethernet: ReadPacket dest %08lx, len %lx", buffer, len));
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

	D(bug("Ethernet: SendPacket src %08lx, len %lx", buffer, len));

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
	for(int i=0; i<MAX_ETH; i++) {
		Handler *handler = new ETHERNET_HANDLER_CLASSNAME;
		strapply(bx_options.ethernet.type, tolower);
		if ( handler->open( bx_options.ethernet.type ) ) {
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

	// TODO needs something smarter than exit&init
	exit();
	init();
}

ETHERNETDriver::Handler *ETHERNETDriver::getHandler(int ethX)
{
	if (ethX >= 0 && ethX < MAX_ETH)
		return handlers[ethX];
	else
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

		handler->handlingThread = SDL_CreateThread( receiveFunc, handler );
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

#if FIXME
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
		D(bug(" packet received (len %d), triggering ETHERNETDriver interrupt", packet_length));

		TRIGGER_INTERRUPT;

		// Wait for interrupt acknowledge (m68k network driver read interrupt to finish)
		D(bug(" waiting for int acknowledge"));
		SDL_SemWait(handler->intAck);
		D(bug(" int acknowledged"));
	}

	return 0;
}

/*
vim:ts=4:sw=4:
*/
