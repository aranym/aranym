/**
 * Ethernet Emulation using tuntap driver in Linux
 *
 * Standa and Joy of ARAnyM team (c) 2004-2005
 *
 * GPL
 */

#if defined(OS_darwin)
 
#include "cpu_emulation.h"
#include "main.h"
#include "ethernet_darwin.h"

#define DEBUG 0
#include "debug.h"

#include <sys/poll.h>
#include <sys/wait.h>	// waitpid()
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>

/****************************
 * Configuration zone begins
 */

#define TAP_INIT	"aratapif"
#define TAP_MTU		"1500"

/*
 * Configuration zone ends
 **************************/


bool TunTapEthernetHandler::open() {
	// int nonblock = 1;
	char *devName = bx_options.ethernet[ethX].tunnel;

	// get the tunnel nif name if provided
	if (strlen(devName) == 0) {
		D(bug("TunTap(%d): tunnel name undefined", ethX));
		return false;
	}

	D(bug("TunTap(%d): open('%s')", ethX, devName));

	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("TunTap(%d): NO_NET_DRIVER_WARN '%s': %s", ethX, devName, strerror(errno));
		return false;
	}

	// if 'bridge' mode then we are done
	if ( strcmp(bx_options.ethernet[ethX].type, "bridge") == 0 )
		return true;

	int pid = fork();
	if (pid < 0) {
		panicbug("TunTap(%d): ERROR: fork() failed. Ethernet disabled!", ethX);
		::close(fd);
		return false;
	}

	if (pid == 0) {
		// the arguments _need_ to be placed into the child process
		// memory (otherwise this does not work here)
		char *args[] = {
			TAP_INIT,
			bx_options.ethernet[ethX].tunnel,
			bx_options.ethernet[ethX].ip_host,
			bx_options.ethernet[ethX].ip_atari,
			bx_options.ethernet[ethX].netmask,
			TAP_MTU, NULL
		};
		int result;
		result = execvp( TAP_INIT, args );
		_exit(result);
	}

	D(bug("TunTap(%d): waiting for "TAP_INIT" at pid %d", ethX, pid));
	int status;
	waitpid(pid, &status, 0);
	bool failed = true;
	if (WIFEXITED(status)) {
		int err = WEXITSTATUS(status);
		if (err == 255) {
			panicbug("TunTap(%d): ERROR: "TAP_INIT" not found. Ethernet disabled!", ethX);
		}
		else if (err != 0) {
			panicbug("TunTap(%d): ERROR: "TAP_INIT" failed (code %d). Ethernet disabled!", ethX, err);
		}
		else {
			failed = false;
			D(bug("TunTap(%d): "TAP_INIT" initialized OK", ethX));
		}
	} else {
		panicbug("TunTap(%d): ERROR: "TAP_INIT" could not be started. Ethernet disabled!", ethX);
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
	D(bug("TunTap(%d): close", ethX));

	return true;
}

int TunTapEthernetHandler::recv(uint8 *buf, int len) {
	return  read(fd, buf, len);
}

int TunTapEthernetHandler::send(const uint8 *buf, int len) {
	int res = write(fd, buf, len);
	if (res < 0) D(bug("TunTap(%d): WARNING: Couldn't transmit packet", ethX));
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
		snprintf(tapname, sizeof(tapname), "/dev/%s", dev);
		tapname[sizeof(tapname)-1] = '\0';
		D(bug("TunTap(%d): tapOpenOld %s", ethX, tapname));
		return ::open(tapname, O_RDWR);
    }

    for(i=0; i < 255; i++) {
		sprintf(tapname, "/dev/tap%d", i);
		/* Open device */
		if( (fd=::open(tapname, O_RDWR)) > 0 ) {
			sprintf(dev, "tap%d",i);
			D(bug("TunTap(%d): tapOpenOld %s", ethX, dev));
			return fd;
		}
    }
    return -1;
}

int TunTapEthernetHandler::tapOpen(char *dev)
{
    return tapOpenOld(dev);
}

#endif /* OS_darwin? */