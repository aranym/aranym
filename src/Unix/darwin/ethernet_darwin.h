/*
 * ethernet_darwin.h - Darwin Ethernet support (via TUN/TAP driver)
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

#ifndef _ETHERNET_DARWIN_H
#define _ETHERNET_DARWIN_H

#include "ethernet.h"

class TunTapEthernetHandler : public ETHERNETDriver::Handler {
	int fd;

	// the /dev/net/tun driver (TAP)
	int tapOpenOld(char *dev);
	int tapOpen(char *dev);

public:
	TunTapEthernetHandler(int eth_idx);
	virtual ~TunTapEthernetHandler();

	virtual bool open();
	virtual void close();
	virtual int recv(uint8 *buf, int len);
	virtual int send(const uint8 *buf, int len);
};

#define ETHERNET_HANDLER_CLASSNAME TunTapEthernetHandler

#endif // _ETHERNET_DARWIN_H

