/**
 * Ethernet Emulation using tuntap driver in Linux
 *
 * Standa and Joy of ARAnyM team (c) 2004 
 *
 * GPL
 */

#include "cpu_emulation.h"
#include "main.h"
#include "ethernet_linux.h"

#define DEBUG 1
#include "debug.h"

#include <sys/poll.h>
#include <sys/wait.h>	// waitpid()
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined(HAVE_LINUX_IF_H) && defined(HAVE_LINUX_IF_TUN_H)
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <linux/if.h>
#include <linux/if_tun.h>
#endif
#if defined(HAVE_NET_IF_H) && defined(HAVE_NET_IF_TUN_H)
#include <net/if.h>
#include <net/if_tun.h>
#endif

#if 0
#   if defined(__FreeBSD__) || defined(sgi)
#       include <net/if.h>
#   endif
#endif

#include <errno.h>

/****************************
 * Configuration zone begins
 */

#define TAP_INIT	"aratapif"
#define TAP_DEVICE	"tap0"
#define TAP_MTU		"1500"

/*
 * Configuration zone ends
 **************************/


bool TunTapEthernetHandler::open( const char* mode ) {
	// int nonblock = 1;
	char devName[128]=TAP_DEVICE;

	// get the tunnel nif name if provided
	if (strlen(bx_options.ethernet.tunnel))
		strcpy(devName, bx_options.ethernet.tunnel);
	
	D(bug("TunTap: init"));

	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("TunTap: NO_NET_DRIVER_WARN '%s': %s", devName, strerror(errno));
		return false;
	}

	// if not a PPP then ok
	if ( strcmp(mode,"ppp") )
		return true;

	int pid = fork();
	if (pid < 0) {
		panicbug("TunTap: ERROR: fork() failed. Ethernet disabled!");
		::close(fd);
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
		result = execvp( TAP_INIT, args );
		_exit(result);
	}

	D(bug("TunTap: waiting for "TAP_INIT" at pid %d", pid));
	int status;
	waitpid(pid, &status, 0);
	bool failed = true;
	if (WIFEXITED(status)) {
		int err = WEXITSTATUS(status);
		if (err == 255) {
			panicbug("TunTap: ERROR: "TAP_INIT" not found. Ethernet disabled!");
		}
		else if (err != 0) {
			panicbug("TunTap: ERROR: "TAP_INIT" failed (code %d). Ethernet disabled!", err);
		}
		else {
			failed = false;
			D(bug("TunTap: "TAP_INIT" initialized OK"));
		}
	} else {
		panicbug("TunTap: ERROR: "TAP_INIT" could not be started. Ethernet disabled!");
	}

	// Close /dev/net/tun device if exec failed
	if (failed) {
		::close(fd);
		return false;
	}

	// Set nonblocking I/O
	//ioctl(fd, FIONBIO, &nonblock);

	return true;
}

bool TunTapEthernetHandler::close() {
	// Close /dev/net/tun device
	::close(fd);
	D(bug("TunTap: close"));

	return true;
}

int TunTapEthernetHandler::recv(uint8 *buf, int len) {
#if 0
	// Wait for packets to arrive
	int res;
        do {
		struct pollfd pf = {fd, POLLIN, 0};
		res = poll(&pf, 1, -1);
	} while (res <= 0)

	// Read the packets
#endif
	return  read(fd, buf, len);
}

int TunTapEthernetHandler::send(const uint8 *buf, int len) {
	int res = write(fd, buf, len);
	if (res < 0) D(bug("TunTap: WARNING: Couldn't transmit packet"));
	return res;
}


/*
 * Allocate ETHERNETDriver TAP device, returns opened fd.
 * Stores dev name in the first arg(must be large enough).
 */
int TunTapEthernetHandler::tapOpenOld(char *dev)
{
    char tapname[14];
    int i, fd;

    if( *dev ) {
		sprintf(tapname, "/dev/%s", dev);
		D(bug("TunTap: tapOpenOld %s", tapname));
		return ::open(tapname, O_RDWR);
    }

    for(i=0; i < 255; i++) {
		sprintf(tapname, "/dev/tap%d", i);
		/* Open device */
		if( (fd=::open(tapname, O_RDWR)) > 0 ) {
			sprintf(dev, "tap%d",i);
			D(bug("TunTap: tapOpenOld %s", dev));
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

int TunTapEthernetHandler::tapOpen(char *dev)
{
    struct ifreq ifr;

    if( (fd = ::open("/dev/net/tun", O_RDWR)) < 0 ) {
	    panicbug("TunTap: Error opening /dev/net/tun. Check if module is loaded and privileges are OK");
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

    D(bug("TunTap: if opened %s", dev));
    return fd;

  failed:
    ::close(fd);
    return -1;
}

#else

int TunTapEthernetHandler::tapOpen(char *dev)
{
    return tapOpenOld(dev);
}

#endif /* New driver support */
