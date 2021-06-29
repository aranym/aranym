/**
 * Ethernet Emulation using tuntap driver in Linux
 *
 * Standa and Joy of ARAnyM team (c) 2004-2008
 *
 * GPL
 */

#include "cpu_emulation.h"
#include "main.h"
#include "ethernet_linux.h"

#define DEBUG 0
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

static char tap_init[] = "aratapif";
static char tap_mtu[] = "1500";

/*
 * Configuration zone ends
 **************************/

TunTapEthernetHandler::TunTapEthernetHandler(int eth_idx)
	: Handler(eth_idx),
	fd(-1)
{
}

TunTapEthernetHandler::~TunTapEthernetHandler()
{
	close();
}

bool TunTapEthernetHandler::open() {
	// int nonblock = 1;
	char *type = bx_options.ethernet[ethX].type;
	char *devName = bx_options.ethernet[ethX].tunnel;

	close();

	if (strcmp(type, "none") == 0 || strlen(type) == 0)
	{
		return false;
	}
	debug = strstr(type, "debug") != NULL;

	// get the tunnel nif name if provided
	if (strlen(devName) == 0) {
		D(bug("ETH%d: tunnel name undefined", ethX));
		return false;
	}

	D(bug("ETH%d: open('%s')", ethX, devName));

	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("ETH%d: NO_NET_DRIVER_WARN '%s': %s", ethX, devName, strerror(errno));
		return false;
	}

	// if 'bridge' mode then we are done
	if ( strstr(bx_options.ethernet[ethX].type, "bridge") != NULL )
		return true;

	int pid = fork();
	if (pid < 0) {
		panicbug("ETH%d: ERROR: fork() failed. Ethernet disabled!", ethX);
		close();
		return false;
	}

	if (pid == 0) {
		// the arguments _need_ to be placed into the child process
		// memory (otherwise this does not work here)
		char *args[] = {
			tap_init,
			bx_options.ethernet[ethX].tunnel,
			bx_options.ethernet[ethX].ip_host,
			bx_options.ethernet[ethX].ip_atari,
			bx_options.ethernet[ethX].netmask,
			tap_mtu, NULL
		};
		int result;
		result = execvp(tap_init, args);
		_exit(result);
	}

	D(bug("ETH%d: waiting for %s at pid %d", ethX, tap_init, pid));
	int status;
	waitpid(pid, &status, 0);
	bool failed = true;
	if (WIFEXITED(status)) {
		int err = WEXITSTATUS(status);
		if (err == 255) {
			panicbug("ETH%d: ERROR: %s not found. Ethernet disabled!", ethX, tap_init);
		}
		else if (err != 0) {
			panicbug("ETH%d: ERROR: %s failed (code %d). Ethernet disabled!", ethX, tap_init, err);
		}
		else {
			failed = false;
			D(bug("ETH%d: %s initialized OK", ethX, tap_init));
		}
	} else {
		panicbug("ETH%d: ERROR: %s could not be started. Ethernet disabled!", ethX, tap_init);
	}

	// Close /dev/net/tun device if exec failed
	if (failed) {
		close();
		return false;
	}

	// Set nonblocking I/O
	//ioctl(fd, FIONBIO, &nonblock);

	return true;
}

void TunTapEthernetHandler::close() {
	D(bug("ETH%d: close", ethX));

	// Close /dev/net/tun device
	if (fd > 0)
	{
		::close(fd);
		fd = -1;
	}
}

int TunTapEthernetHandler::recv(uint8 *buf, int len) {
	/*
	 * this is called from a thread,
	 * so we don't need to poll() here
	 */
	return ::read(fd, buf, len);
}

int TunTapEthernetHandler::send(const uint8 *buf, int len) {
	int res = write(fd, buf, len);

	if (res < 0) { D(bug("ETH%d: WARNING: Couldn't transmit packet", ethX)); }
	return res;
}


/*
 * Allocate ETHERNETDriver TAP device, returns opened fd.
 * Stores dev name in the first arg(must be large enough).
 */
int TunTapEthernetHandler::tapOpenOld(char *dev)
{
    char tapname[sizeof(bx_options.ethernet[0].tunnel) + 5];
    int i, fd;

    if( *dev ) {
		snprintf(tapname, sizeof(tapname), "/dev/%s", dev);
		tapname[sizeof(tapname)-1] = '\0';
		D(bug("ETH%d: tapOpenOld %s", ethX, tapname));
		return ::open(tapname, O_RDWR);
    }

    for(i=0; i < 255; i++) {
		sprintf(tapname, "/dev/tap%d", i);
		/* Open device */
		if( (fd=::open(tapname, O_RDWR)) > 0 ) {
			sprintf(dev, "tap%d",i);
			D(bug("ETH%d: tapOpenOld %s", ethX, dev));
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
	    panicbug("ETH%d: Error opening /dev/net/tun. Check if module is loaded and privileges are OK", ethX);
	    return tapOpenOld(dev);
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if( *dev )
	memcpy(ifr.ifr_name, dev, MIN(strlen(dev), IFNAMSIZ));

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0) {
	    if (errno != EBADFD)
		    goto failed;

	    /* Try old ioctl */
	    if (ioctl(fd, OTUNSETIFF, (void *) &ifr) < 0)
		    goto failed;
    }

    strcpy(dev, ifr.ifr_name);

    D(bug("ETH%d: if opened %s", ethX, dev));
    return fd;

  failed:
    close();
    return -1;
}

#else

int TunTapEthernetHandler::tapOpen(char *dev)
{
    return tapOpenOld(dev);
}

#endif /* New driver support */
