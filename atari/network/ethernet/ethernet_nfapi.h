/*
 * joy 2003
 *
 * trying to clean up the NFAPI mess
 *
 * GPL
 */

#ifndef _ARAETHER_NFAPI_H
#define _ARAETHER_NFAPI_H

/* if you change anything in the enum {} below you have to increase 
   this ARAETHER_NFAPI_VERSION!
*/
#define ARAETHER_NFAPI_VERSION	0x00000004

enum {
	GET_VERSION = 0,	/* no parameters, return NFAPI_VERSION in d0 */
	XIF_INTLEVEL,		/* no parameters, return Interrupt Level in d0 */
	XIF_IRQ,			/* (ethX), acknowledge interrupt from host */
	XIF_START,			/* (ethX), called on 'ifup', start receiver thread */
	XIF_STOP,			/* (ethX), called on 'ifdown', stop the thread */
	XIF_READLENGTH,		/* (ethX), return size of network data block to read */
	XIF_READBLOCK,		/* (ethX, buffer, size), read block of network data */
	XIF_WRITEBLOCK,		/* (ethX, buffer, size), write block of network data */
	XIF_GET_MAC,		/* (ethX, buffer, size), return MAC HW addr in buffer */
	XIF_GET_IPHOST,		/* (ethX, buffer, size), return IP address of host */
	XIF_GET_IPATARI,	/* (ethX, buffer, size), return IP address of atari */
	XIF_GET_NETMASK,	/* (ethX, buffer, size), return IP netmask */
	XIF_GET_MTU			/* (ethX), return MTU size in bytes in d0 */
};

#define ETH(a)	(nfEtherFsId + a)

#endif /* _ARAETHER_NFAPI_H */
