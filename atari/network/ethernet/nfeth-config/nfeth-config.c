/*
 * Get IP addresses of the networking tunnel
 *
 * Written by Petr Stehlik (c) 2003
 *
 * GPL
 */
#include <stdio.h>				/* for printf */
#if 0
#include <tos.h>				/* for gemdos and Super */
#else
#include <mintbind.h>
#endif
#include <string.h>				/* for strcmp */
#include "../ethernet_nfapi.h"

/* compiler and library compatibility defines */
#if __PUREC__
#define CDECL cdecl
#else
#define CDECL
#endif

#ifndef TRUE
#define TRUE	(1==1)
#define FALSE	(!TRUE)
#endif

/* TOS Cookie Jar */
typedef struct {
	long cookie;
	long value;
} COOKIE;

int getcookie(long target, long *p_value)
{
	if (Ssystem(-1, 0L, 0L) == 0) {
		/* Ssystem is available */
		long result = Ssystem(8, target, NULL);
		if (result != -1L) {
			*p_value = result;
			return TRUE;
		}
		return FALSE;
	}
	else {
		char *oldssp;
		COOKIE *cookie_ptr;

		oldssp = (char *) Super(0L);

		cookie_ptr = *(COOKIE **) 0x5A0;

		if (oldssp)
			Super(oldssp);

		if (cookie_ptr != NULL) {
			do {
				if (cookie_ptr->cookie == target) {
					if (p_value != NULL)
						*p_value = cookie_ptr->value;

					return TRUE;
				}
			} while ((cookie_ptr++)->cookie != 0L);
		}
	}

	return FALSE;
}

/* NatFeat code */
typedef struct {
	long magic;
	unsigned long CDECL(*nfGetID) (const char *);
	long CDECL(*nfCall) (unsigned long ID, ...);
} NatFeatCookie;

/* return NatFeat pointer if NatFeat initialization was successful,
 * otherwise NULL
 */
NatFeatCookie *initNatFeats(void)
{
	NatFeatCookie *nf_ptr = NULL;

	if (getcookie(0x5f5f4e46L, (long *)&nf_ptr) == 0) {	/* "__NF" */
		puts("NatFeat cookie not found");
		return NULL;
	}

	if (nf_ptr->magic != 0x20021021L) {		/* NatFeat magic constant */
		puts("NatFeat cookie magic value does not match");
		return NULL;
	}

	return nf_ptr;
}

unsigned long nfEthernetID(const NatFeatCookie *nf_ptr)
{
	static unsigned long nf_ethernet_id = 0;
	if (nf_ethernet_id == 0 && nf_ptr != NULL)
		nf_ethernet_id = nf_ptr->nfGetID("ETHERNET");
	return nf_ethernet_id;
}

void printEthX(const NatFeatCookie *nf_ptr, int ethX, unsigned long ID)
{
	char *oldssp;
	static char buf[80];
	buf[0] = '\0';
	
	oldssp = (char *)Super((void *)0L);
	nf_ptr->nfCall(ID, (long)ethX, buf, (long)sizeof(buf));
	Super(oldssp);
	
	puts(buf);
}

int main(int argc, char **argv)
{
	NatFeatCookie *nf_ptr = initNatFeats();
	unsigned long nfEtherFsId = nfEthernetID(nf_ptr);
	int usage = FALSE;
	enum { NONE, HOST, ATARI, NETMASK } which;
	int ethX = 0;
	unsigned long xifID = 0;

	if (nfEtherFsId == 0) {
		fprintf(stderr, "ERROR: NatFeat `ETHERNET' not found!\n");
		return 1;
	}

	if (argc <= 1) {
		usage = TRUE;
	}
	else {
		int i;
		which = NONE;
		for(i=1; i<argc && !usage; i++) {
			char *p = argv[i];
			#define PETH "eth"
			if (strcmp(p, "--get-host-ip") == 0) {
				if (which == NONE)
					which = HOST;
				else
					usage = TRUE;
			}
			else if (strcmp(p, "--get-atari-ip") == 0) {
				if (which == NONE)
					which = ATARI;
				else
					usage = TRUE;
			}
			else if (strcmp(p, "--get-netmask") == 0) {
				if (which == NONE)
					which = NETMASK;
				else
					usage = TRUE;
			}
			else if (strncmp(p, PETH, strlen(PETH)) == 0) {
				ethX = atoi(p+strlen(PETH));
			}
			else {
				usage = TRUE;
			}
		}
	}

	if (usage || which == NONE) {
		fprintf(stderr, "Usage: %s --get-host-ip | --get-atari-ip | --get-netmask] [eth0]\n", argv[0]);
		return 0;
	}

	switch(which) {
		case HOST:	xifID = ETH(XIF_GET_IPHOST); break;
		case ATARI: xifID = ETH(XIF_GET_IPATARI); break;
		case NETMASK: xifID = ETH(XIF_GET_NETMASK); break;
	}
	printEthX(nf_ptr, ethX, xifID);

	return 0;
}
