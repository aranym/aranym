/*
 * ARAnyM ethernet networking: ifconfig
 *
 * made by extreme stripping of unnecessary code in standard ifconfig
 *
 * Standa and Joy of ARAnyM team in 03/2003 (C) 2003-2007
 * 
 */

/*
 * ifconfig   This file contains an implementation of the command
 *              that either displays or sets the characteristics of
 *              one or more of the system's networking interfaces.
 *
 * Author:      Fred N. van Kempen, <waltje@uwalt.nl.mugnet.org>
 *              and others.  Copyright 1993 MicroWalt Corporation
 *
 *              This program is free software; you can redistribute it
 *              and/or  modify it under  the terms of  the GNU General
 *              Public  License as  published  by  the  Free  Software
 *              Foundation;  either  version 2 of the License, or  (at
 *              your option) any later version.
 *
 * Patched to support 'add' and 'del' keywords for INET(4) addresses
 * by Mrs. Brisby <mrs.brisby@nimh.org>
 *
 * {1.34} - 19980630 - Arnaldo Carvalho de Melo <acme@conectiva.com.br>
 *                     - gettext instead of catgets for i18n
 *          10/1998  - Andi Kleen. Use interface list primitives.       
 *	    20001008 - Bernd Eckenfels, Patch from RH for setting mtu 
 *			(default AF was wrong)
 *          20010404 - Arnaldo Carvalho de Melo, use setlocale
 */

#ifdef __linux__
#include "linux/libcwrap.h"
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

static int skfd;

/* Like strncpy but make sure the resulting string is always 0 terminated. */
static char *safe_strncpy(char *dst, const char *src, size_t size)
{
    dst[size-1] = '\0';
    return strncpy(dst,src,size-1);
}

static int set_ip_using(const char *name, unsigned long c, const char *ip, const char *text)
{
    struct ifreq ifr;
    struct sockaddr_in sin;
    char host[128];

    memset(&ifr, 0, sizeof(ifr));
    safe_strncpy(ifr.ifr_name, name, IFNAMSIZ);
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
	safe_strncpy(host, ip, (sizeof host));
    if (! inet_aton(host, &sin.sin_addr)) {
		fprintf(stderr, "%s '%s' invalid\n", text, ip);
    	return 1;
    }

    if (c == SIOCSIFADDR)
    	memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    else if (c == SIOCSIFDSTADDR)
    	memcpy(&ifr.ifr_dstaddr, &sin, sizeof(struct sockaddr));
    else if (c == SIOCSIFNETMASK)
#ifdef OS_darwin		
    	memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
#else	
		memcpy(&ifr.ifr_netmask, &sin, sizeof(struct sockaddr));
#endif
    else
    	return 2;

    if (ioctl(skfd, c, &ifr) < 0) {
    	perror(text);
		return -1;
	}
    return 0;
}

int set_mtu(const char *name, int mtu_size)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    safe_strncpy(ifr.ifr_name, name, IFNAMSIZ);
	ifr.ifr_mtu = mtu_size;
	if (ioctl(skfd, SIOCSIFMTU, &ifr) < 0) {
		perror("SIOCSIFMTU");
		return -1;
	}
	return 0;
}

/* Set a certain interface flag. */
static int set_flag(const char *name, short flag)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    safe_strncpy(ifr.ifr_name, name, IFNAMSIZ);
    if (ioctl (skfd, SIOCGIFFLAGS, &ifr) < 0) {
		perror("SIOCGIFFLAGS");
		return -1;
    }
    ifr.ifr_flags |= flag;
    if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0) {
		perror("SIOCSIFFLAGS");
		return -2;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *device;

    if (argc != 6) {
    	fprintf(stderr, "Usage: %s tap_device host_IP atari_IP netmask mtu_size\n", argv[0]);
    	return -1;
    }

    device = argv[1];

    /* Make sure this is a tap device - don't allow messing with other
     * interfaces. Poor security is better than none. */
    if (strncmp(device, "tap", 3)) {
    	fprintf(stderr, "%s designed for tap devices only\n", argv[0]);
    }

    /* Create a channel to the NET kernel. */
    if ((skfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return 1;
    }

	/* Set Host IP */
	if (set_ip_using(device, SIOCSIFADDR, argv[2], "host_IP")) {
		return 2;
   	}

    /* Set Atari IP */
	if (set_ip_using(device, SIOCSIFDSTADDR, argv[3], "atari_IP")) {
		return 3;
   	}

    /* Set Netmask */
    if (set_ip_using(device, SIOCSIFNETMASK, argv[4], "netmask")) {
		return 4;
    }

	/* Set MTU */
	if (set_mtu(device, atoi(argv[5]))) {
		return 5;
	}

	/* Set Point-to-point flag and put it up */
    if (set_flag(device, IFF_UP | IFF_RUNNING | IFF_POINTOPOINT)) {
    	return 6;
    }

	return 0;
}
