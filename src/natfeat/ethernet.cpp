/**
 * Ethernet Card Emulation
 *
 * Standa and Joy of ARAnyM team (c) 2004 
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


static ssize_t packet_length;
static uint8 packet[1516];

// Global variables
static SDL_Thread *handlingThread;			// Packet reception thread
static SDL_sem *intAck;					// Interrupt acknowledge semaphore

int32 ETHERNETDriver::dispatch(uint32 fncode)
{
	D(bug("Ethernet: Dispatch %d", fncode));

	// If disabled then do nothing (the initialization didn't went through)
	if ( !handler ) return 0;

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
				memptr buf_ptr = getParameter(1);	// destination buffer
				uint32 buf_size = getParameter(2);	// buffer size
				D(bug("Ethernet: getMAC(%d, %p, %d", ethX, buf_ptr, buf_size));

				if (! ValidAddr(buf_ptr, true, buf_size))
					BUS_ERROR(buf_ptr);

				// generate the MAC as '\0AETH0' for eth0
				// CAUTION: the 'ARETH0' wasn't a good choice
				// (Linux bridging didn't work with that)
				uint8 mac_addr[6] = {'\0','A','E','T','H', '0'+ethX };
				memcpy(Atari2HostAddr(buf_ptr), mac_addr, buf_size);	// use H2Amemcpy
			}
			break;

		case XIF_IRQ: // interrupt raised by native side thread polling tap0 interface
			if ( !getParameter(0) ) {
				D(bug("Ethernet: /IRQ"));
				// Acknowledge interrupt to reception thread
				SDL_SemPost(intAck);
			} else {
				D(bug("Ethernet: IRQ"));
				ret = 0;	/* ethX requested the interrupt */
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
	DUNUSED(which);
	int ethX = getParameter(0);
	DUNUSED(ethX);
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

	host2AtariSafeStrncpy(name_ptr, text, name_maxlen);
	return strlen(text);
}


int32 ETHERNETDriver::readPacketLength(int ethX)
{
	DUNUSED(ethX);
	return packet_length; /* packet_length[ethX] */
}

/*
 *  ETHERNETDriver ReadPacket routine
 */

void ETHERNETDriver::readPacket(int ethX, memptr buffer, uint32 len)
{
	DUNUSED(ethX);
	D(bug("Ethernet: ReadPacket dest %08lx, len %lx", buffer, len));
	Host2Atari_memcpy(buffer, packet, len > 1514 ? 1514 : len );
	if (len > 1514) {
		panicbug("ETHERNETDriver::readPacket() - length %d > 1514", len);
	}
}


/*
 *  ETHERNETDriver writePacket routine
 */

void ETHERNETDriver::sendPacket(int ethX, memptr buffer, uint32 len)
{
	DUNUSED(ethX);
	uint8 packetToWrite[1516];

	D(bug("Ethernet: SendPacket src %08lx, len %lx", buffer, len));

	len = len > 1514 ? 1514 : len;
	Atari2Host_memcpy(packetToWrite, buffer, len );

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
	handlingThread = NULL;
	handler = new ETHERNET_HANDLER_CLASSNAME;

	strapply(bx_options.ethernet.type, tolower);
	bool opened = handler->open( bx_options.ethernet.type );
	if ( !opened ) {
		delete handler;
		handler = NULL;
	}
	return true;
}


/*
 *  Deinitialization
 */
void ETHERNETDriver::exit()
{
	D(bug("Ethernet: exit"));

	// Stop reception thread
	stopThread(0 /* ethX */);

	if ( !handler ) return;
	handler->close();
	delete handler;
	handler = NULL;
}

// reset, called upon OS reboot
void ETHERNETDriver::reset()
{
	D(bug("Ethernet: reset"));

	// TODO needs something smarter than exit&init
	exit();
	init();
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
	DUNUSED(ethX);
	if (!handlingThread) {
		D(bug("Ethernet: Start thread"));

		if ((intAck = SDL_CreateSemaphore(0)) == NULL) {
			D(bug("WARNING: Cannot init semaphore"));
			return false;
		}

		handlingThread = SDL_CreateThread( receiveFunc, handler );
		if (!handlingThread) {
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
	DUNUSED(ethX);
	if (handlingThread) {
		D(bug("Ethernet: Stop thread"));

#if FIXME
		// pthread_cancel(handlingThread); // FIXME: set the cancel flag.
		SDL_WaitThread(handlingThread, NULL);
		SDL_DestroySemaphore(intAck);
#endif
		handlingThread = NULL;
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
		packet_length = handler->recv(packet, 1514);

		// Trigger ETHERNETDriver interrupt (call the m68k side)
		D(bug(" packet received (len %d), triggering ETHERNETDriver interrupt", packet_length));

		TRIGGER_INTERRUPT;

		// Wait for interrupt acknowledge (m68k network driver read interrupt to finish)
		D(bug(" waiting for int acknowledge"));
		SDL_SemWait(intAck);
		D(bug(" int acknowledged"));
	}

	return 0;
}

