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
 * Last modified: 2007-08-22 Jens Heitmann
 *
 */


#if defined(OS_darwin)
 
#include "cpu_emulation.h"
#include "main.h"
#include "host.h"			// for the HostScreen
#include "ethernet_darwin.h"
#include "host.h"

#define DEBUG 1
#include "debug.h"

#include <sys/wait.h>	// waitpid()
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <net/if.h>
#include <errno.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>

/****************************
 * Configuration zone begins
 */

#define TAP_SHELL	"/bin/sh"
#define TAP_INIT	"aratapif.sh"
#define TAP_MTU		"1500"

/*
 * Configuration zone ends
 **************************/

static OSStatus authStatus;
static AuthorizationRef authorizationRef;
static bool fullscreen;

static int openAuthorizationContext()
{
	if (authorizationRef == NULL) {

		AuthorizationFlags authFlags = kAuthorizationFlagDefaults;// 1

		authStatus = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment,// 3
					authFlags, &authorizationRef);// 4
		if (authStatus != errAuthorizationSuccess)
			return authStatus;

		fullscreen = bx_options.video.fullscreen;
		if ( fullscreen )
			host->video->toggleFullScreen();			

		AuthorizationItem authItems = {kAuthorizationRightExecute, 0,// 5
						NULL, 0};// 6
		AuthorizationRights authRights = {1, &authItems};// 7
		authFlags = kAuthorizationFlagDefaults |// 8
						kAuthorizationFlagInteractionAllowed |// 9
						kAuthorizationFlagPreAuthorize |// 10
						kAuthorizationFlagExtendRights;// 11
		authStatus = AuthorizationCopyRights (authorizationRef, &authRights, NULL, authFlags, NULL );// 12
		return authStatus;
		}
	else
		return authStatus;
}

static void closeAuthorizationContext()
{
	if (authorizationRef) {
		AuthorizationFree (authorizationRef, kAuthorizationFlagDefaults);// 17
		authorizationRef = NULL;

		if ( fullscreen ) {
			host->video->toggleFullScreen();
		}
	}
}

static int executeScriptAsRoot(char *application, char *args[]) {
    OSStatus execStatus;
	char app[MAXPATHLEN];
		/*
	CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFURLGetFileSystemRepresentation(url, true, (UInt8 *)app, MAXPATHLEN);
	printf("TunTap: Binary directory '%s'\n", app);
	CFRelease(url);*/
	//Host::getDataFolder(app, sizeof(app));
	Host::getConfFolder(app, sizeof(app));
	strcat(app, DIRSEPARATOR);
	strcat(app, application);
	args[0] = app;
	
	FILE *authCommunicationsPipe = NULL;
	char execReadBuffer[128];

	AuthorizationFlags authFlags = kAuthorizationFlagDefaults;// 13
	execStatus = AuthorizationExecuteWithPrivileges// 14
                    (authorizationRef, TAP_SHELL, authFlags, args,// 15
                    &authCommunicationsPipe);// 16
					
	if (execStatus == errAuthorizationSuccess)
		for(;;) {
			int bytesRead = read (fileno (authCommunicationsPipe),
				execReadBuffer, sizeof (execReadBuffer));
			if (bytesRead < 1) break;
			write (fileno (stdout), execReadBuffer, bytesRead);
		}
				
    return execStatus;
}

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

	if ( strlen(type) == 0 || strcmp(bx_options.ethernet[ethX].type, "none") == 0 ) {
		if (ethX == MAX_ETH - 1) closeAuthorizationContext();
		return false;	
	}

	// if 'bridge' mode then we are done
	if ( strstr(bx_options.ethernet[ethX].type, "bridge") != NULL ) {
		panicbug("TunTap(%d): Bridge mode currently not supported '%s'", ethX, devName);
		if (ethX == MAX_ETH - 1) closeAuthorizationContext();
		return false;	
	}
	
	// get the tunnel nif name if provided
	if (strlen(devName) == 0) {
		D(bug("TunTap(%d): tunnel name undefined", ethX));
		if (ethX == MAX_ETH - 1) closeAuthorizationContext();
		return false;
	}
	
	D(bug("TunTap(%d): open('%s')", ethX, devName));
	
	fd = tapOpen( devName );
	if (fd < 0) {
		panicbug("TunTap(%d): NO_NET_DRIVER_WARN '%s': %s", ethX, devName, strerror(errno));
		if (ethX == MAX_ETH - 1) closeAuthorizationContext();
		return false;
	}
	int auth = openAuthorizationContext();
	if (auth) {
		close();
		panicbug("TunTap(%d): Authorization failed'%s'", ethX, devName);
		return false;	
	}
	
	bool failed = true;
	{
		// the arguments _need_ to be placed into the child process
		// memory (otherwise this does not work here)
		char *args[] = {
			NULL, // replaced with application support shell script
			bx_options.ethernet[ethX].tunnel,
			bx_options.ethernet[ethX].ip_host,
			bx_options.ethernet[ethX].ip_atari,
			bx_options.ethernet[ethX].netmask,
			(char *)TAP_MTU, NULL
		};

		int result = executeScriptAsRoot( (char *)TAP_INIT, args );
		if (result != 0) {
			panicbug("TunTap(%d): ERROR: "TAP_INIT" failed (code %d). Ethernet disabled!", ethX, result);
		}
		else {
			failed = false;
			D(bug("TunTap(%d): "TAP_INIT" initialized OK", ethX));
		}
	}
	
	// Close /dev/net/tun device if exec failed
	if (failed) {
		close();
		if (ethX == MAX_ETH - 1) closeAuthorizationContext();
		return false;
	}

	// Set nonblocking I/O
	//ioctl(fd, FIONBIO, &nonblock);

	if (ethX == MAX_ETH - 1) closeAuthorizationContext();
	return true;
}

void TunTapEthernetHandler::close() {
	D(bug("TunTap(%d): close", ethX));
	// Close /dev/net/tun device
	if (fd > 0)
	{
		::close(fd);
		fd = -1;
	}
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
    char tapname[sizeof(bx_options.ethernet[0].tunnel) + 5];
    int i, fd;

    if( *dev ) {
		snprintf(tapname, sizeof(tapname), "/dev/%s", dev);
		tapname[sizeof(tapname)-1] = '\0';
		D(bug("TunTap(%d): tapOpenOld %s", ethX, tapname));
		fd=::open(tapname, O_RDWR);
		if (fd > 0)	return fd;
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
