/*
 * ethernet_darwin.cpp - Darwin Ethernet support (via TUN/TAP driver)
 *
 * Copyright (c) 2007 ARAnyM dev team (see AUTHORS)
 * 
 * Inspired by Bernie Meyer's UAE-JIT and Gwenole Beauchesne's Basilisk II-JIT
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
 *
 * Last modified: 2007-08-09 Jens Heitmann
 *
 */


#if defined(OS_darwin)
 
#include "cpu_emulation.h"
#include "main.h"
#include "ethernet_darwin.h"

#define DEBUG 0
#include "debug.h"

#include <sys/wait.h>	// waitpid()
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>

/****************************
 * Configuration zone begins
 */

#define SUDO		"sudo"
#define TAP_INIT	"/usr/local/bin/aratapif.sh"
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
	
	// if 'bridge' mode then we are done
	if ( strcmp(bx_options.ethernet[ethX].type, "bridge") == 0 ) {
		panicbug("TunTap(%d): Bridge mode currently not supported '%s': %s", ethX, devName);
		return false;	
	}
	
	D(bug("TunTap(%d): open('%s')", ethX, devName));
	
	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("TunTap(%d): NO_NET_DRIVER_WARN '%s': %s", ethX, devName, strerror(errno));
		return false;
	}


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
			SUDO,
			TAP_INIT,
			bx_options.ethernet[ethX].tunnel,
			bx_options.ethernet[ethX].ip_host,
			bx_options.ethernet[ethX].ip_atari,
			bx_options.ethernet[ethX].netmask,
			TAP_MTU, NULL
		};
		int result;
		result = execvp( SUDO, args );
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