/*
 * ifconfig   This file contains an implementation of the command
 *              that either displays or sets the characteristics of
 *              one or more of the system's networking interfaces.
 *
 * Version:     $Id$
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

#define DFLT_AF "inet"

#include "config.h"

#include <features.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

/* Ugh.  But libc5 doesn't provide POSIX types.  */
#include <asm/types.h>


#ifdef HAVE_HWSLIP
#include <linux/if_slip.h>
#endif

#if HAVE_AFINET6

#ifndef _LINUX_IN6_H
/*
 *    This is in linux/include/net/ipv6.h.
 */

struct in6_ifreq {
    struct in6_addr ifr6_addr;
    __u32 ifr6_prefixlen;
    unsigned int ifr6_ifindex;
};

#endif

#endif				/* HAVE_AFINET6 */

#if HAVE_AFIPX
#if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1)
#include <netipx/ipx.h>
#else
#include "ipx.h"
#endif
#endif
#if 0
#include "pathnames.h"
#include "version.h"
#include "../intl.h"
#include "sockets.h"
#include "util.h"

char *Release = RELEASE, *Version = "ifconfig 1.42 (2001-04-13)";
#endif
#include "net-support.h"
#include "interface.h"

int opt_a = 0;			/* show all interfaces          */
int opt_i = 0;			/* show the statistics          */
int opt_v = 0;			/* debugging output flag        */

#define _(String) String
int skfd;

int addr_family = 0;		/* currently selected AF        */

/* Like strncpy but make sure the resulting string is always 0 terminated. */
char *safe_strncpy(char *dst, const char *src, size_t size)
{
    dst[size-1] = '\0';
    return strncpy(dst,src,size-1);
}

struct aftype *get_aftype(const char *name)
{
}

int get_socket_for_af(int af)
{
    return skfd;
}
		
/* Set a certain interface flag. */
static int set_flag(char *ifname, short flag)
{
    struct ifreq ifr;

    safe_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0) {
	fprintf(stderr, _("%s: unknown interface: %s\n"), 
		ifname,	strerror(errno));
	return (-1);
    }
    safe_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_flags |= flag;
    if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0) {
	perror("SIOCSIFFLAGS");
	return -1;
    }
    return (0);
}

/* Clear a certain interface flag. */
static int clr_flag(char *ifname, short flag)
{
    struct ifreq ifr;
    int fd;

    if (strchr(ifname, ':')) {
        /* This is a v4 alias interface.  Downing it via a socket for
	   another AF may have bad consequences. */
        fd = get_socket_for_af(AF_INET);
	if (fd < 0) {
	    fprintf(stderr, _("No support for INET on this system.\n"));
	    return -1;
	}
    } else
        fd = skfd;

    safe_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
	fprintf(stderr, _("%s: unknown interface: %s\n"), 
		ifname, strerror(errno));
	return -1;
    }
    safe_strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_flags &= ~flag;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
	perror("SIOCSIFFLAGS");
	return -1;
    }
    return (0);
}

static void usage(void)
{
    fprintf(stderr, _("Usage:\n  ifconfig [-a] [-i] [-v] [-s] <interface> [[<AF>] <address>]\n"));
#if HAVE_AFINET
    fprintf(stderr, _("  [add <address>[/<prefixlen>]]\n"));
    fprintf(stderr, _("  [del <address>[/<prefixlen>]]\n"));
    fprintf(stderr, _("  [[-]broadcast [<address>]]  [[-]pointopoint [<address>]]\n"));
    fprintf(stderr, _("  [netmask <address>]  [dstaddr <address>]  [tunnel <address>]\n"));
#endif
#ifdef SIOCSKEEPALIVE
    fprintf(stderr, _("  [outfill <NN>] [keepalive <NN>]\n"));
#endif
    fprintf(stderr, _("  [hw <HW> <address>]  [metric <NN>]  [mtu <NN>]\n"));
    fprintf(stderr, _("  [[-]trailers]  [[-]arp]  [[-]allmulti]\n"));
    fprintf(stderr, _("  [multicast]  [[-]promisc]\n"));
    fprintf(stderr, _("  [mem_start <NN>]  [io_addr <NN>]  [irq <NN>]  [media <type>]\n"));
#ifdef HAVE_TXQUEUELEN
    fprintf(stderr, _("  [txqueuelen <NN>]\n"));
#endif
#ifdef HAVE_DYNAMIC
    fprintf(stderr, _("  [[-]dynamic]\n"));
#endif
    fprintf(stderr, _("  [up|down] ...\n\n"));

    fprintf(stderr, _("  <HW>=Hardware Type.\n"));
    fprintf(stderr, _("  List of possible hardware types:\n"));
    /* print_hwlist(0); */ /* 1 = ARPable */
    fprintf(stderr, _("  <AF>=Address family. Default: %s\n"), DFLT_AF);
    fprintf(stderr, _("  List of possible address families:\n"));
    /* print_aflist(0); */ /* 1 = routeable */
    exit(-1);
}

static int set_netmask(int skfd, struct ifreq *ifr, struct sockaddr *sa)
{
    int err = 0;

    memcpy((char *) &ifr->ifr_netmask, (char *) sa,
	   sizeof(struct sockaddr));
    if (ioctl(skfd, SIOCSIFNETMASK, ifr) < 0) {
	fprintf(stderr, "SIOCSIFNETMASK: %s\n",
		strerror(errno));
	err = 1;
    }
    return 0;
}

int ifconfig(int argc, char **argv)
{
    struct sockaddr sa;
    struct sockaddr_in sin;
    char host[128];
    struct aftype *ap;
    struct ifreq ifr;
    int goterr = 0, didnetmask = 0;
    char **spp;
    int fd;
#if HAVE_AFINET6
    extern struct aftype inet6_aftype;
    struct sockaddr_in6 sa6;
    struct in6_ifreq ifr6;
    unsigned long prefix_len;
    char *cp;
#endif
#if HAVE_AFINET
    extern struct aftype inet_aftype;
#endif

    /* Create a channel to the NET kernel. */
    if ((skfd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("socket");
	exit(1);
    }

    /* No. Fetch the interface name. */
    spp = argv;
    safe_strncpy(ifr.ifr_name, *spp++, IFNAMSIZ);
    if (*spp == (char *) NULL) {
	fprintf(stderr, _("Bad interface name: %s\n"), ifr.ifr_name);
      	(void) close(skfd);
        exit(1);
    }
		

#if 0
    /* The next argument is either an address family name, or an option. */
    if ((ap = get_aftype(*spp)) != NULL)
	spp++; /* it was a AF name */
    else 
#endif
	ap = get_aftype(DFLT_AF);
	
    if (ap) {
	addr_family = ap->af;
	skfd = ap->fd;
    }

    /* Process the remaining arguments. */
    while (*spp != (char *) NULL) {
	if (!strcmp(*spp, "arp")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_NOARP);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "-arp")) {
	    goterr |= set_flag(ifr.ifr_name, IFF_NOARP);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "trailers")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_NOTRAILERS);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "-trailers")) {
	    goterr |= set_flag(ifr.ifr_name, IFF_NOTRAILERS);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "promisc")) {
	    goterr |= set_flag(ifr.ifr_name, IFF_PROMISC);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "-promisc")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_PROMISC);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "multicast")) {
	    goterr |= set_flag(ifr.ifr_name, IFF_MULTICAST);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "-multicast")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_MULTICAST);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "allmulti")) {
	    goterr |= set_flag(ifr.ifr_name, IFF_ALLMULTI);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "-allmulti")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_ALLMULTI);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "up")) {
	    goterr |= set_flag(ifr.ifr_name, (IFF_UP | IFF_RUNNING));
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "down")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_UP);
	    spp++;
	    continue;
	}
#ifdef HAVE_DYNAMIC
	if (!strcmp(*spp, "dynamic")) {
	    goterr |= set_flag(ifr.ifr_name, IFF_DYNAMIC);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "-dynamic")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_DYNAMIC);
	    spp++;
	    continue;
	}
#endif

	if (!strcmp(*spp, "metric")) {
	    if (*++spp == NULL)
		usage();
	    ifr.ifr_metric = atoi(*spp);
	    if (ioctl(skfd, SIOCSIFMETRIC, &ifr) < 0) {
		fprintf(stderr, "SIOCSIFMETRIC: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "mtu")) {
	    if (*++spp == NULL)
		usage();
	    ifr.ifr_mtu = atoi(*spp);
	    if (ioctl(skfd, SIOCSIFMTU, &ifr) < 0) {
		fprintf(stderr, "SIOCSIFMTU: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
#ifdef SIOCSKEEPALIVE
	if (!strcmp(*spp, "keepalive")) {
	    if (*++spp == NULL)
		usage();
	    ifr.ifr_data = (caddr_t) atoi(*spp);
	    if (ioctl(skfd, SIOCSKEEPALIVE, &ifr) < 0) {
		fprintf(stderr, "SIOCSKEEPALIVE: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
#endif

#ifdef SIOCSOUTFILL
	if (!strcmp(*spp, "outfill")) {
	    if (*++spp == NULL)
		usage();
	    ifr.ifr_data = (caddr_t) atoi(*spp);
	    if (ioctl(skfd, SIOCSOUTFILL, &ifr) < 0) {
		fprintf(stderr, "SIOCSOUTFILL: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
#endif

	if (!strcmp(*spp, "-broadcast")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_BROADCAST);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "broadcast")) {
	    if (*++spp != NULL) {
		safe_strncpy(host, *spp, (sizeof host));
		if (ap->input(0, host, &sa) < 0) {
		    ap->herror(host);
		    goterr = 1;
		    spp++;
		    continue;
		}
		memcpy((char *) &ifr.ifr_broadaddr, (char *) &sa,
		       sizeof(struct sockaddr));
		if (ioctl(ap->fd, SIOCSIFBRDADDR, &ifr) < 0) {
		    fprintf(stderr, "SIOCSIFBRDADDR: %s\n",
			    strerror(errno));
		    goterr = 1;
		}
		spp++;
	    }
	    goterr |= set_flag(ifr.ifr_name, IFF_BROADCAST);
	    continue;
	}
	if (!strcmp(*spp, "dstaddr")) {
	    if (*++spp == NULL)
		usage();
	    safe_strncpy(host, *spp, (sizeof host));
	    if (ap->input(0, host, &sa) < 0) {
		ap->herror(host);
		goterr = 1;
		spp++;
		continue;
	    }
	    memcpy((char *) &ifr.ifr_dstaddr, (char *) &sa,
		   sizeof(struct sockaddr));
	    if (ioctl(ap->fd, SIOCSIFDSTADDR, &ifr) < 0) {
		fprintf(stderr, "SIOCSIFDSTADDR: %s\n",
			strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "netmask")) {
	    if (*++spp == NULL || didnetmask)
		usage();
	    safe_strncpy(host, *spp, (sizeof host));
	    if (ap->input(0, host, &sa) < 0) {
		ap->herror(host);
		goterr = 1;
		spp++;
		continue;
	    }
	    didnetmask++;
	    goterr = set_netmask(ap->fd, &ifr, &sa);
	    spp++;
	    continue;
	}
#ifdef HAVE_TXQUEUELEN
	if (!strcmp(*spp, "txqueuelen")) {
	    if (*++spp == NULL)
		usage();
	    ifr.ifr_qlen = strtoul(*spp, NULL, 0);
	    if (ioctl(skfd, SIOCSIFTXQLEN, &ifr) < 0) {
		fprintf(stderr, "SIOCSIFTXQLEN: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
#endif

	if (!strcmp(*spp, "mem_start")) {
	    if (*++spp == NULL)
		usage();
	    if (ioctl(skfd, SIOCGIFMAP, &ifr) < 0) {
		fprintf(stderr, "mem_start: SIOCGIFMAP: %s\n", strerror(errno)); 
		spp++; 
		goterr = 1;
		continue;
	    }
	    ifr.ifr_map.mem_start = strtoul(*spp, NULL, 0);
	    if (ioctl(skfd, SIOCSIFMAP, &ifr) < 0) {
		fprintf(stderr, "mem_start: SIOCSIFMAP: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "io_addr")) {
	    if (*++spp == NULL)
		usage();
	    if (ioctl(skfd, SIOCGIFMAP, &ifr) < 0) {
		fprintf(stderr, "io_addr: SIOCGIFMAP: %s\n", strerror(errno)); 
		spp++; 
		goterr = 1;
		continue;
	    }
	    ifr.ifr_map.base_addr = strtol(*spp, NULL, 0);
	    if (ioctl(skfd, SIOCSIFMAP, &ifr) < 0) {
		fprintf(stderr, "io_addr: SIOCSIFMAP: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "irq")) {
	    if (*++spp == NULL)
		usage();
	    if (ioctl(skfd, SIOCGIFMAP, &ifr) < 0) {
		fprintf(stderr, "irq: SIOCGIFMAP: %s\n", strerror(errno)); 
		goterr = 1;
		spp++; 
		continue;
	    }
	    ifr.ifr_map.irq = atoi(*spp);
	    if (ioctl(skfd, SIOCSIFMAP, &ifr) < 0) {
		fprintf(stderr, "irq: SIOCSIFMAP: %s\n", strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "-pointopoint")) {
	    goterr |= clr_flag(ifr.ifr_name, IFF_POINTOPOINT);
	    spp++;
	    continue;
	}
	if (!strcmp(*spp, "pointopoint")) {
	    if (*(spp + 1) != NULL) {
		spp++;
		safe_strncpy(host, *spp, (sizeof host));
		if (ap->input(0, host, &sa)) {
		    ap->herror(host);
		    goterr = 1;
		    spp++;
		    continue;
		}
		memcpy((char *) &ifr.ifr_dstaddr, (char *) &sa,
		       sizeof(struct sockaddr));
		if (ioctl(ap->fd, SIOCSIFDSTADDR, &ifr) < 0) {
		    fprintf(stderr, "SIOCSIFDSTADDR: %s\n",
			    strerror(errno));
		    goterr = 1;
		}
	    }
	    goterr |= set_flag(ifr.ifr_name, IFF_POINTOPOINT);
	    spp++;
	    continue;
	};

#if HAVE_AFINET6
	if (!strcmp(*spp, "tunnel")) {
	    if (*++spp == NULL)
		usage();
	    if ((cp = strchr(*spp, '/'))) {
		prefix_len = atol(cp + 1);
		if ((prefix_len < 0) || (prefix_len > 128))
		    usage();
		*cp = 0;
	    } else {
		prefix_len = 0;
	    }
	    safe_strncpy(host, *spp, (sizeof host));
	    if (inet6_aftype.input(1, host, (struct sockaddr *) &sa6) < 0) {
		inet6_aftype.herror(host);
		goterr = 1;
		spp++;
		continue;
	    }
	    memcpy((char *) &ifr6.ifr6_addr, (char *) &sa6.sin6_addr,
		   sizeof(struct in6_addr));

	    fd = get_socket_for_af(AF_INET6);
	    if (fd < 0) {
		fprintf(stderr, _("No support for INET6 on this system.\n"));
		goterr = 1;
		spp++;
		continue;
	    }
	    if (ioctl(fd, SIOGIFINDEX, &ifr) < 0) {
		perror("SIOGIFINDEX");
		goterr = 1;
		spp++;
		continue;
	    }
	    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
	    ifr6.ifr6_prefixlen = prefix_len;

	    if (ioctl(fd, SIOCSIFDSTADDR, &ifr6) < 0) {
		fprintf(stderr, "SIOCSIFDSTADDR: %s\n",
			strerror(errno));
		goterr = 1;
	    }
	    spp++;
	    continue;
	}
#endif

	/* If the next argument is a valid hostname, assume OK. */
	safe_strncpy(host, *spp, (sizeof host));

	/* FIXME: sa is too small for INET6 addresses, inet6 should use that too, 
	   broadcast is unexpected */
	if (ap->getmask) {
	    switch (ap->getmask(host, &sa, NULL)) {
	    case -1:
		usage();
		break;
	    case 1:
		if (didnetmask)
		    usage();

		goterr = set_netmask(skfd, &ifr, &sa);
		didnetmask++;
		break;
	    }
	}
	if (ap->input == NULL) {
	   fprintf(stderr, _("ifconfig: Cannot set address for this protocol family.\n"));
	   exit(1);
	}
	if (ap->input(0, host, &sa) < 0) {
	    ap->herror(host);
	    fprintf(stderr, _("ifconfig: `--help' gives usage information.\n"));
	    exit(1);
	}
	memcpy((char *) &ifr.ifr_addr, (char *) &sa, sizeof(struct sockaddr));
	{
	    int r = 0;		/* to shut gcc up */
	    switch (ap->af) {
#if HAVE_AFINET
	    case AF_INET:
		fd = get_socket_for_af(AF_INET);
		if (fd < 0) {
		    fprintf(stderr, _("No support for INET on this system.\n"));
		    exit(1);
		}
		r = ioctl(fd, SIOCSIFADDR, &ifr);
		break;
#endif
#if HAVE_AFECONET
	    case AF_ECONET:
		fd = get_socket_for_af(AF_ECONET);
		if (fd < 0) {
		    fprintf(stderr, _("No support for ECONET on this system.\n"));
		    exit(1);
		}
		r = ioctl(fd, SIOCSIFADDR, &ifr);
		break;
#endif
	    default:
		fprintf(stderr,
		_("Don't know how to set addresses for family %d.\n"), ap->af);
		exit(1);
	    }
	    if (r < 0) {
		perror("SIOCSIFADDR");
		goterr = 1;
	    }
	}

       /*
        * Don't do the set_flag() if the address is an alias with a - at the
        * end, since it's deleted already! - Roman
        *
        * Should really use regex.h here, not sure though how well it'll go
        * with the cross-platform support etc. 
        */
        {
            char *ptr;
            short int found_colon = 0;
            for (ptr = ifr.ifr_name; *ptr; ptr++ )
                if (*ptr == ':') found_colon++;
                
            if (!(found_colon && *(ptr - 1) == '-'))
                goterr |= set_flag(ifr.ifr_name, (IFF_UP | IFF_RUNNING));
        }

	spp++;
    }

    return (goterr);
}

struct ifcmd {
    int flag;
    unsigned long addr;
    char *base;
    int baselen;
};

static unsigned char searcher[256];

static int set_ip_using(const char *name, int c, unsigned long ip)
{
    struct ifreq ifr;
    struct sockaddr_in sin;

    safe_strncpy(ifr.ifr_name, name, IFNAMSIZ);
    memset(&sin, 0, sizeof(struct sockaddr));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ip;
    memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    if (ioctl(skfd, c, &ifr) < 0)
	return -1;
    return 0;
}

int main(int argc, char **argv) {
//	printf(stderr, argv[0]);
	char *args[] = {
		"tap0",
		argv[0],
		"pointopoint",
		argv[1],
		"mtu 1500",
		"netmask 255.255.255.0",
		"up"
	};

	fprintf(stderr, argv[0]);

	ifconfig( sizeof(args)/sizeof(*args), args );			
}
