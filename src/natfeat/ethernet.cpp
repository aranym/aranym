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
#include "tools.h"

#define DEBUG 0
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

#include "../../atari/network/ethernet/ethernet_nfapi.h"

/****************************
 * Configuration zone begins
 */

#define TAP_INIT	"aratapif"
#define TAP_DEVICE	"tap0"
#define TAP_MTU		"1500"

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
				finishInterupt();
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
	// int ethX = getParameter(0);
	memptr name_ptr = getParameter(1);
	uint32 name_maxlen = getParameter(2);
	char *text = NULL;

	D(bug("Ethernet: getPAR(%d) to (%p, %d)", which, name_ptr, name_maxlen));

	if (! ValidAddr(name_ptr, true, name_maxlen))
		BUS_ERROR(name_ptr);

	switch(which) {
		case HOST_IP: text = bx_options.ethernet.ip_host; break;
		case ATARI_IP:text = bx_options.ethernet.ip_atari; break;
		case NETMASK: text = bx_options.ethernet.netmask; break;
		default: text = "";
	}

	char *name = (char *)Atari2HostAddr(name_ptr);	// use A2Hstrcpy
	strncpy(name, text, name_maxlen-1);
	name[name_maxlen-1] = '\0';
	return strlen(text);
}


int32 ETHERNETDriver::readPacketLength(int ethX)
{
	return packet_length; /* packet_length[ethX] */
}

/*
 *  ETHERNETDriver ReadPacket routine
 */

void ETHERNETDriver::readPacket(int ethX, memptr buffer, uint32 len)
{
	D(bug("Ethernet: ReadPacket dest %08lx, len %lx", buffer, len));
	Host2Atari_memcpy(buffer, packet, packet_length > 1514 ? 1514 : packet_length );
}


/*
 *  ETHERNETDriver writePacket routine
 */

void ETHERNETDriver::sendPacket(int ethX, memptr buffer, uint32 len)
{
	uint8 packetToWrite[1516];

	D(bug("Ethernet: SendPacket src %08lx, len %lx", buffer, len));

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
	char devName[128]=TAP_DEVICE;

	// get the tunnel nif name if provided
	if (strlen(bx_options.ethernet.tunnel))
		strcpy(devName, bx_options.ethernet.tunnel);
	
	D(bug("Ethernet: init"));

	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("NO_NET_DRIVER_WARN '%s': %s", devName, strerror(errno));
		return false;
	}

	// if not a PPP then ok
	strapply(bx_options.ethernet.type, tolower);
	if ( strcmp(bx_options.ethernet.type,"ppp") )
		return true;

	int pid = fork();
	if (pid < 0) {
		panicbug("Ethernet ERROR: fork() failed. Ethernet disabled!");
		close(fd);
		fd = -1;
		return false;
	}

	if (pid == 0) {
		// the arguments _need_ to be placed into the child process
		// memory (otherwise this does not work here)
		char *args[] = {
			TAP_INIT, TAP_DEVICE,
			bx_options.ethernet.ip_host,
			bx_options.ethernet.ip_atari,
			bx_options.ethernet.netmask,
			TAP_MTU, NULL
		};
		int result;
		result = execvp( "./" TAP_INIT, args );
		_exit(result);
	}

	D(bug("waiting for "TAP_INIT" at pid %d", pid));
	int status;
	waitpid(pid, &status, 0);
	bool failed = true;
	if (WIFEXITED(status)) {
		int err = WEXITSTATUS(status);
		if (err == 255) {
			panicbug("Ethernet ERROR: "TAP_INIT" not found. Ethernet disabled!");
		}
		else if (err != 0) {
			panicbug("Ethernet ERROR: "TAP_INIT" failed (code %d). Ethernet disabled!", err);
		}
		else {
			failed = false;
			D(bug("Ethernet: "TAP_INIT" initialized OK"));
		}
	} else {
		panicbug("Ethernet ERROR: "TAP_INIT" could not be started. Ethernet disabled!");
	}

	// Close /dev/net/tun device if exec failed
	if (failed) {
		close(fd);
		fd = -1;
		return false;
	}

	// Set nonblocking I/O
	//ioctl(fd, FIONBIO, &nonblock);

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

	// Close /dev/net/tun device
	if (fd > 0) {
		close(fd);
		fd = -1;
	}
}

// ctor
ETHERNETDriver::ETHERNETDriver()
{
	init();
}

// reset, called upon OS reboot
void ETHERNETDriver::reset()
{
	// TODO needs something smarter than exit&init
	exit();
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
	if (fd > 0 && !handlingThread) {
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
void ETHERNETDriver::stopThread(int ethX)
{
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
    if( *dev )
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
	    if (errno != EBADFD)
		    goto failed;

	    /* Try old ioctl */
	    if (ioctl(fd, OTUNSETIFF, (void *) &ifr) < 0)
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
