/**
 * Ethernet
 *
 * GPL
 */

#include "cpu_emulation.h"
#include "main.h"
#include "ethernet.h"

#include "host.h"

#define DEBUG 0
#include "debug.h"

#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>

#if defined(__FreeBSD__) || defined(sgi)
#include <net/if.h>
#endif

#include <errno.h>

#include <SDL.h>
#include <SDL_thread.h>

/****************************
 * Configuration zone begins
 */

#if !defined(XIF_HOST_IP) && !defined(XIF_ATARI_IP) && !defined(XIF_NETMASK)
# define XIF_HOST_IP	"192.168.0.1"
# define XIF_ATARI_IP	"192.168.0.2"
# define XIF_NETMASK	"255.255.255.0"
#endif

// the ETHERNET NatFeat API version number (change it if you change NF subIDs)
#define XIF_VERSION	0x00000001

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
		case 0:	// what version?
			ret = XIF_VERSION;
			break;

		case 1:	// what interrupt level is used?
			ret = INTLEVEL;
			break;

		case 2:	// what is the MAC address?
			{
				/* store MAC address to provided buffer */
				char *text = MAC_ADDRESS;				// source
				memptr name_ptr = getParameter(0);		// destination
				uint32 name_maxlen = getParameter(1);	// max length

				if (! ValidAddr(name_ptr, true, name_maxlen))
					BUS_ERROR(name_ptr);

				char *name = (char *)Atari2HostAddr(name_ptr);	// use A2Hstrcpy
				strncpy(name, text, name_maxlen-1);
				name[name_maxlen-1] = '\0';
				return strlen(text);
			}
			break;

		case 3: // interrupt raised by native side thread polling tap0 interface
			if ( !getParameter(0) ) {
				D(bug("Ethernet: /IRQ"));
				finishInterupt();
			} else
				D(bug("Ethernet: IRQ"));

			break;
		case 4:
			startThread();
			break;
		case 5:
			stopThread();
			break;
		case 6:
			ret = readPacketLength( 0/* getParameter(0) *//* nif */ );
			break;
		case 7:
			readPacket( getParameter(0) /* buff */, getParameter(1) /* len */ );
			break;
		case 8:
			sendPacket( getParameter(0) /* buff */, getParameter(1) /* len */ );
			break;
	}
	return ret;
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

int initTap (int argc, char **argv);

/*
 *  Initialization
 */
bool ETHERNETDriver::init(void)
{
	// int nonblock = 1;
	char devName[128];

	D(bug("Ethernet: init"));

/* argv[0]      Name of this program                                 */
/* argv[1]      Name of the TUN network device (tun0)                */
/* argv[2]      The maximum transmission unit size                   */
/* argv[3]      The IP address of the Atari side of the link         */
/* argv[4]      The IP address of the host system side of the link   */
/* argv[5]      The netmask                                          */
	char *argv[] = {
		"aranym",
		"tap0",
		"1500",
		XIF_ATARI_IP,
		XIF_HOST_IP,
		XIF_NETMASK
	};

	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("NO_NET_DRIVER_WARN '%s': %s", devName, strerror(errno));
		return false;
	}

	if ( initTap( 6, argv ) ) {
		panicbug("initTap failure");
		return false;
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

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
		return tapOpenOld(dev);

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




/* HERCIFC.C    (c) Copyright Roger Bowler, 2000-2001                */
/*              Hercules interface configuration program             */

/*-------------------------------------------------------------------*/
/* This module configures the TUN interface for Hercules.            */
/* It is invoked as a setuid root program by ctcadpt.c               */
/* The command line arguments are:                                   */
/* argv[0]      Name of this program                                 */
/* argv[1]      Name of the TUN network device (tun0)                */
/* argv[2]      The maximum transmission unit size                   */
/* argv[3]      The IP address of the Hercules side of the link      */
/* argv[4]      The IP address of the driving system side of the link*/
/* argv[5]      The netmask                                          */
/* Error messages are written to stderr, which is redirected to      */
/* the Hercules message log by ctcadpt.c                             */
/* The exit status is zero if successful, non-zero if error.         */
/*-------------------------------------------------------------------*/

//#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Subroutine to configure the MTU size into the interface           */
/*-------------------------------------------------------------------*/
static int
set_mtu (int sockfd, char *ifname, char *mtusize)
{
int             rc;                     /* Return code               */
struct ifreq    ifreq;                  /* Structure for ioctl       */

    /* Initialize the structure for ioctl */
    memset (&ifreq, 0, sizeof(ifreq));
    strcpy (ifreq.ifr_name, ifname);
    ifreq.ifr_mtu = atoi(mtusize);

    /* Set the MTU size */
    rc = ioctl (sockfd, SIOCSIFMTU, &ifreq);
    if (rc < 0)
    {
        fprintf (stderr,
            "HHC894I Error setting MTU for %s: %s\n",
            ifname, strerror(errno));
        return 1;
    }

    return 0;
} /* end function set_mtu */

/*-------------------------------------------------------------------*/
/* Subroutine to configure an IP address into the interface          */
/*-------------------------------------------------------------------*/
static int
set_ipaddr (int sockfd, char *ifname, int oper,
            char *opname, char *ipaddr)
{
int             rc;                     /* Return code               */
struct ifreq    ifreq;                  /* Structure for ioctl       */
struct sockaddr_in *sin;                /* -> sockaddr within ifreq  */

    /* Initialize the structure for ioctl */
    memset (&ifreq, 0, sizeof(ifreq));
    strcpy (ifreq.ifr_name, ifname);

    /* Determine where in the structure to store the address */
    sin = (struct sockaddr_in*)
            (oper == SIOCSIFADDR ? &ifreq.ifr_addr
            :oper == SIOCSIFDSTADDR ? &ifreq.ifr_dstaddr
            :oper == SIOCSIFNETMASK ? &ifreq.ifr_netmask
            :NULL);

    /* Store the IP address into the structure */
    sin->sin_family = AF_INET;
    rc = inet_aton (ipaddr, &sin->sin_addr);
    if (rc == 0)
    {
        fprintf (stderr,
            "HHC896I Invalid %s for %s: %s\n",
            opname, ifname, ipaddr);
        return 1;
    }

    /* Configure the interface with the specified IP address */
    rc = ioctl (sockfd, oper, &ifreq);
    if (rc < 0)
    {
        fprintf (stderr,
            "HHC897I Error setting %s for %s: %s\n",
            opname, ifname, strerror(errno));
        return 1;
    }

    return 0;
} /* end function set_ipaddr */

/*-------------------------------------------------------------------*/
/* Subroutine to set the interface flags                             */
/*-------------------------------------------------------------------*/
static int
set_flags (int sockfd, char *ifname, int flags)
{
int             rc;                     /* Return code               */
struct ifreq    ifreq;                  /* Structure for ioctl       */

    /* Initialize the structure for ioctl */
    memset (&ifreq, 0, sizeof(ifreq));
    strcpy (ifreq.ifr_name, ifname);

    /* Get the current flags */
    rc = ioctl (sockfd, SIOCGIFFLAGS, &ifreq);
    if (rc < 0)
    {
        fprintf (stderr,
            "HHC898I Error getting flags for %s: %s\n",
            ifname, strerror(errno));
        return 1;
    }

    /* Set the flags */
    ifreq.ifr_flags |= flags;
    rc = ioctl (sockfd, SIOCSIFFLAGS, &ifreq);
    if (rc < 0)
    {
        fprintf (stderr,
            "HHC899I Error setting flags for %s: %s\n",
            ifname, strerror(errno));
        return 1;
    }

    return 0;
} /* end function set_flags */

/*-------------------------------------------------------------------*/
/* HERCIFC program entry point                                       */
/*-------------------------------------------------------------------*/
int initTap (int argc, char **argv)
{
char           *progname;               /* Name of this program      */
char           *tundevn;                /* Name of TUN device        */
char           *mtusize;                /* MTU size (characters)     */
char           *hercaddr;               /* Hercules IP address       */
char           *drivaddr;               /* Driving system IP address */
char           *netmask;                /* Network mask              */
int             sockfd;                 /* Socket descriptor         */
int             errflag = 0;            /* 1=error(s) occurred       */
char            ifname[IFNAMSIZ];       /* Interface name: tun[0-9]  */

    /* Check for correct number of arguments */
    if (argc != 6)
    {
        fprintf (stderr,
            "HHC891I Incorrect number of arguments passed to %s\n",
            argv[0]);
        exit(1);
    }

    /* Copy the argument pointers */
    progname = argv[0];
    tundevn = argv[1];
    mtusize = argv[2];
    hercaddr = argv[3];
    drivaddr = argv[4];
    netmask = argv[5];

    /* Only check for length, since network device-names can actually
     * be anything!!
     */
    if (strlen(tundevn) > (IFNAMSIZ - 1))
    {
        fprintf (stderr,
            "HHC892I Invalid device name %s passed to %s\n",
            tundevn, progname);
        exit(2);
    }
    strcpy(ifname, tundevn);

    /* Obtain a socket for ioctl operations */
    sockfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        fprintf (stderr,
            "HHC893I %s cannot obtain socket: %s\n",
            progname, strerror(errno));
        exit(3);
    }

    /* Set the MTU size */
    if (set_mtu (sockfd, ifname, mtusize))
        errflag = 1;

    /* Set the driving system's IP address */
    if (set_ipaddr (sockfd, ifname, SIOCSIFADDR,
        "driving system IP addr", drivaddr))
        errflag = 1;

    /* Set the Hercules IP address */
    if (set_ipaddr (sockfd, ifname, SIOCSIFDSTADDR,
        "Hercules IP addr", hercaddr))
        errflag = 1;

    /* Set the netmask */
    if (set_ipaddr (sockfd, ifname, SIOCSIFNETMASK,
        "netmask", netmask))
        errflag = 1;

    /* Set the flags */
    if (set_flags (sockfd, ifname,
                IFF_UP | IFF_RUNNING | IFF_POINTOPOINT ))
        errflag = 1;

    /* Exit with status code 4 if any errors occurred */
    if (errflag) return 4;

    /* Exit with status code 0 if no errors occurred */
    return 0;

} /* end function main */
