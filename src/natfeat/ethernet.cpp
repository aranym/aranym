/**
 * Ethernet Emulation using tuntap driver in Linux
 *
 * Standa and Joy of ARAnyM team (c) 2003 
 *
 * GPL
 */

#include "cpu_emulation.h"
#include "main.h"
#include "ethernet.h"

#define DEBUG 1
#include "debug.h"

#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/wait.h>	// waitpid()
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>

#if defined(__FreeBSD__) || defined(sgi)
#include <net/if.h>
#endif

#include <errno.h>

#include <SDL.h>
#include <SDL_thread.h>

#include "../../atari/network/araeth/araether_nfapi.h"

/****************************
 * Configuration zone begins
 */

#define TAP_INIT	"aratapif"
#define TAP_DEVICE	"tap0"
#define TAP_MTU		"1500"

#if !defined(XIF_HOST_IP) && !defined(XIF_ATARI_IP) && !defined(XIF_NETMASK)
# define XIF_HOST_IP	"192.168.0.1"
# define XIF_ATARI_IP	"192.168.0.2"
# define XIF_NETMASK	"255.255.255.0"
#endif

// the emulated network card HW address
#define MAC_ADDRESS "\001\002\003\004\005\006"

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
static int fd = -1;							// fd of /dev/net/tun device
static SDL_Thread *handlingThread;			// Packet reception thread
static SDL_sem *intAck;					// Interrupt acknowledge semaphore

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

		case XIF_GETMAC:	// what is the MAC address?
			D(bug("Ethernet: getHWddr"));
			/* store MAC address to provided buffer */
			{
				memptr buf_ptr = getParameter(0);	// destination buffer
				uint32 buf_size = getParameter(1);	// buffer size

				if (! ValidAddr(buf_ptr, true, buf_size))
					BUS_ERROR(buf_ptr);

				if (strlen(MAC_ADDRESS) == buf_size)
					memcpy(Atari2HostAddr(buf_ptr), MAC_ADDRESS, buf_size);	// use H2Amemcpy
				else {
					panicbug("Ethernet: getHWddr() with illegal buffer size");
				}
			}
			break;

		case XIF_IRQ: // interrupt raised by native side thread polling tap0 interface
			if ( !getParameter(0) ) {
				D(bug("Ethernet: /IRQ"));
				finishInterupt();
			} else
				D(bug("Ethernet: IRQ"));

			break;
		case XIF_START:
			startThread();
			break;
		case XIF_STOP:
			stopThread();
			break;
		case XIF_READLENGTH:
			ret = readPacketLength( 0/* getParameter(0) *//* nif */ );
			break;
		case XIF_READBLOCK:
			readPacket( getParameter(0) /* buff */, getParameter(1) /* len */ );
			break;
		case XIF_WRITEBLOCK:
			sendPacket( getParameter(0) /* buff */, getParameter(1) /* len */ );
			break;

		case XIF_GET_IPHOST:
			D(bug("XIF_GET_IPHOST\n"));
			ret = get_params(XIF_HOST_IP);
			break;
		case XIF_GET_IPATARI:
			D(bug("XIF_GET_IPHOST\n"));
			ret = get_params(XIF_ATARI_IP);
			break;
		case XIF_GET_NETMASK:
			D(bug("XIF_GET_IPHOST\n"));
			ret = get_params(XIF_NETMASK);
			break;
	}
	return ret;
}


int ETHERNETDriver::get_params(const char *text)
{
	memptr name_ptr = getParameter(0);
	uint32 name_maxlen = getParameter(1);

	if (! ValidAddr(name_ptr, true, name_maxlen))
		BUS_ERROR(name_ptr);

	char *name = (char *)Atari2HostAddr(name_ptr);	// use A2Hstrcpy
	strncpy(name, text, name_maxlen-1);
	name[name_maxlen-1] = '\0';
	return strlen(text);
}


int32 ETHERNETDriver::readPacketLength(memptr nif)
{
	return packet_length;
}

/*
 *  ETHERNETDriver ReadPacket routine
 */

void ETHERNETDriver::readPacket(memptr buffer, uint32 len)
{
	D(bug("Ethernet: ReadPacket dest %08lx, len %08lx", buffer, len));
	Host2Atari_memcpy(buffer, packet, packet_length > 1514 ? 1514 : packet_length );
}


/*
 *  ETHERNETDriver writePacket routine
 */

void ETHERNETDriver::sendPacket(memptr buffer, uint32 len)
{
	uint8 packetToWrite[1516];

	len = len > 1514 ? 1514 : len;
	Atari2Host_memcpy(packetToWrite, buffer, len );

	// Transmit packet
	if (write(fd, packetToWrite, len) < 0) {
		D(bug("WARNING: Couldn't transmit packet"));
	}
}


void ETHERNETDriver::finishInterupt()
{
	// Acknowledge interrupt to reception thread
	D(bug(" Ethernet IRQ done"));
	SDL_SemPost(intAck);
}


/*
 *  Initialization
 */
bool ETHERNETDriver::init(void)
{
	// int nonblock = 1;
	char devName[128]="/dev/"TAP_DEVICE;

	D(bug("Ethernet: init"));

	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("NO_NET_DRIVER_WARN '%s': %s", devName, strerror(errno));
		return false;
	}

	int pid = fork();
	if (pid < 0) {
		panicbug("Ethernet: fork() failed! closing device");
		close(fd);
		return false;
	}

	if (pid == 0) {
		// the arguments _need_ to be placed into the child process
		// memory (otherwise this does not work here)
		char *args[] = {
			TAP_INIT, TAP_DEVICE,
			XIF_HOST_IP, XIF_ATARI_IP, XIF_NETMASK, TAP_MTU,
			NULL
		};
		int result;
		result = execvp( TAP_INIT, args );
		::exit(result);
	}

	D(bug("waiting for "TAP_INIT));
	int status;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status)) {
			panicbug(TAP_INIT" failed (code %d).", WEXITSTATUS(status));
			// Close /dev/net/tun device
			if (fd > 0)
				close(fd);
			return false;
		}
		else {
			D(bug(TAP_INIT" initialized OK"));
		}
	} else {
		D(bug(TAP_INIT"'s fork/exec/waitpid failed badly"));
	}

	// Set nonblocking I/O
	//ioctl(fd, FIONBIO, &nonblock);

	return true;
}


/*
 *  Deinitialization
 */
void ETHERNETDriver::exit(void)
{
	D(bug("Ethernet: exit"));

	// Stop reception thread
	stopThread();

	// Close /dev/net/tun device
	if (fd > 0)
		close(fd);
}



/*
 *  Start packet reception thread
 */
bool ETHERNETDriver::startThread(void)
{
	if (!handlingThread) {
		D(bug("Ethernet: Start thread"));

		if ((intAck = SDL_CreateSemaphore(0)) == NULL) {
			D(bug("WARNING: Cannot init semaphore"));
			return false;
		}

		handlingThread = SDL_CreateThread( receiveFunc, NULL );
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
void ETHERNETDriver::stopThread(void)
{
	if (handlingThread) {
		D(bug("Ethernet: Stop thread"));

#if FIXME
		// pthread_cancel(handlingThread); // FIXME: set the cancel flag.
		SDL_WaitThread(handlingThread, NULL);
		SDL_DestroySemaphore(intAck);
		handlingThread = NULL;
#endif
	}
}


/*
 *  Packet reception thread
 */
int ETHERNETDriver::receiveFunc(void *arg)
{
	for (;;) {
		D(bug("Ethernet: receive function"));

		// Wait for packets to arrive
		struct pollfd pf = {fd, POLLIN, 0};
		int res = poll(&pf, 1, -1);
		if (res <= 0)
			break;

		D(bug("Ethernet: going to read?"));

		// Call protocol handler for received packets
		// ssize_t length;
		for (;;) {
			// Read packet device
			packet_length = read(fd, packet, 1514);
			if (packet_length < 14)
				break;

			// Trigger ETHERNETDriver interrupt
			D(bug(" packet received, triggering ETHERNETDriver interrupt"));

			TRIGGER_INTERRUPT;

			D(bug(" waiting for int acknowledge"));
			// Wait for interrupt acknowledge by ETHERNETDriver::handleInteruptIf()
			SDL_SemWait(intAck);
			D(bug(" int acknowledged"));
		}
	}
	return 0;
}



/*
 * Allocate ETHERNETDriver TAP device, returns opened fd.
 * Stores dev name in the first arg(must be large enough).
 */
int ETHERNETDriver::tapOpenOld(char *dev)
{
    char tapname[14];
    int i, fd;

    if( *dev ) {
		sprintf(tapname, "/dev/%s", dev);
		D(bug(" tapOpenOld %s", tapname));
		return open(tapname, O_RDWR);
    }

    for(i=0; i < 255; i++) {
		sprintf(tapname, "/dev/tap%d", i);
		/* Open device */
		if( (fd=open(tapname, O_RDWR)) > 0 ) {
			sprintf(dev, "tap%d",i);
			D(bug(" tapOpenOld %s", dev));
			return fd;
		}
    }
    return -1;
}

#ifdef HAVE_LINUX_IF_TUN_H /* New driver support */
#include <linux/if_tun.h>

/* pre 2.4.6 compatibility */
#define OTUNSETNOCSUM  (('T'<< 8) | 200)
#define OTUNSETDEBUG   (('T'<< 8) | 201)
#define OTUNSETIFF     (('T'<< 8) | 202)
#define OTUNSETPERSIST (('T'<< 8) | 203)
#define OTUNSETOWNER   (('T'<< 8) | 204)

int ETHERNETDriver::tapOpen(char *dev)
{
    struct ifreq ifr;

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
    	panicbug("Error opening /dev/net/tun. Check if module is loaded and privileges are OK");
		return tapOpenOld(dev);
	}

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
		if (errno == EBADFD) {
			/* Try old ioctl */
			if (ioctl(fd, OTUNSETIFF, (void *) &ifr) < 0)
				goto failed;
		} else
			goto failed;
    }

    strcpy(dev, ifr.ifr_name);
	D(bug("Ethernet: if opened %s", dev));
    return fd;

  failed:
    close(fd);
    return -1;
}

#else

int ETHERNETDriver::tapOpen(char *dev)
{
    return tapOpenOld(dev);
}

#endif /* New driver support */
