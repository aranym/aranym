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
#include <sockios.h>			/* for SIOCGLNKSTATS */

int main(int argc, char **argv)
{
	int usage = 0;
	enum { NONE = -1, ADDR, DSTADDR, NETMASK } which = NONE;
	char *nif = "eth0";
	int sockfd;
	struct ifreq ifr;
	long addr[10]; /* indexed by the enum 'which' */


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
				nif = p;
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

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket failed");
		return 1;
	}
	strcpy(ifr.ifr_name, nif);
	ifr.ifr_ifru.ifru_data = (caddr_t)addr;
	if (ioctl(sockfd, SIOCGLNKSTATS, &ifr) != 0) {
		perror("ioctl failed");
		return 1;
	}
	printf("%ld.%ld.%ld.%ld\n", (addr[which] >> 24) & 0xff,
								(addr[which] >> 16) & 0xff,
								(addr[which] >> 8) & 0xff,
								addr[which] & 0xff);

	return 0;
}
