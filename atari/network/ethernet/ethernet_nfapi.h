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
#define ARAETHER_NFAPI_VERSION	0x00000003

enum {
	GET_VERSION = 0,	/* subID = 0 */
	XIF_INTLEVEL, XIF_GETMAC, XIF_IRQ, XIF_START, XIF_STOP, XIF_READLENGTH,
	XIF_READBLOCK, XIF_WRITEBLOCK,
	XIF_GET_IPHOST, XIF_GET_IPATARI, XIF_GET_NETMASK
};

#define ETH(a)	(nfEtherFsId + a)

#endif /* _ARAETHER_NFAPI_H */
