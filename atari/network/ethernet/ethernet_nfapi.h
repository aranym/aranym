/*
 * ARAnyM ethernet driver - header file.
 *
 * Copyright (c) 2002-2004 Standa and Petr of ARAnyM dev team (see AUTHORS)
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
	XIF_GET_NETMASK		/* (ethX, buffer, size), return IP netmask */
};

#define ETH(a)	(nfEtherID + a)

#endif /* _ARAETHER_NFAPI_H */
