/*
 * Get IP addresses of the networking tunnel
 *
 * Written by Petr Stehlik (c) 2003,2004
 *
 * GPL
 */
#include <stdio.h>				/* for printf */
#include <stdlib.h>				/* for atoi */
#include <string.h>				/* for strcmp */
#include <sys/ioctl.h>			/* for ioctl */
#include <net/if.h>				/* for ifreq */

/* the following constants are defined in freemint/sys/mint/sockio.h */
# define SIOCGIFADDRFH	(('S' << 8) | 53)	/* get iface address */
# define SIOCGIFDSTADDRFH (('S' << 8) | 54)	/* get iface remote address */
# define SIOCGIFNETMASKFH (('S' << 8) | 55)	/* get iface network mask */

int main(int argc, char **argv)
{
	int usage = 0;
	enum {
		NONE = 0,
		ADDR = SIOCGIFADDRFH,
		DSTADDR = SIOCGIFDSTADDRFH,
		NETMASK = SIOCGIFNETMASKFH
	} which = NONE;
	int ethX = 0;

	if (argc <= 1) {
		usage = 1;
	}
	else {
		int i;
		for(i=1; i<argc && !usage; i++) {
			char *p = argv[i];
			#define PETH "eth"
			if (strcmp(p, "--get-host-ip") == 0) {
				if (which == NONE)
					which = DSTADDR;
				else
					usage = 1;
			}
			else if (strcmp(p, "--get-atari-ip") == 0) {
				if (which == NONE)
					which = ADDR;
				else
					usage = 1;
			}
			else if (strcmp(p, "--get-netmask") == 0) {
				if (which == NONE)
					which = NETMASK;
				else
					usage = 1;
			}
			else if (strncmp(p, PETH, strlen(PETH)) == 0) {
				ethX = atoi(p+strlen(PETH));
			}
			else {
				usage = 1;
			}
		}
	}

	if (usage || which == NONE) {
		fprintf(stderr, "Usage: %s --get-host-ip | --get-atari-ip | --get-netmask] [eth0]\n", argv[0]);
		return 0;
	}

	{
		int sockfd;
		struct ifreq ifreq_ioctl;
		long addr = 0;
		int ret;

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockfd < 0) {
			perror("socket could not be open");
			return 1;
		}
		strcpy(ifreq_ioctl.ifr_name, "eth0");
		ret = ioctl(sockfd, which, &addr);
		if (ret != 0) {
			printf("ioctl returns %d\n", ret);
			return 1;
		}
		printf("%ld.%ld.%ld.%ld\n", (addr >> 24) & 0xff, (addr >> 16) & 0xff,
							  (addr >> 8) & 0xff, addr & 0xff);
	}

	return 0;
}
