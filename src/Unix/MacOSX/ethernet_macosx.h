/*
 * ethernet_macosx.h - Mac OS X Ethernet support (via Berkley Packet Filter device)
 *
 * Copyright (c) 2007 ARAnyM dev team (see AUTHORS)
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
 */

#ifndef _ETHERNET_MACOSX_H
#define _ETHERNET_MACOSX_H

#include <net/bpf.h>

#include "ethernet.h"

class BPFEthernetHandler : public ETHERNETDriver::Handler {
	bool debug;
	int fd;
	int buf_len;
	struct bpf_hdr* bpf_buf;

	// variables used for looping over multiple packets
	int read_len;
	struct bpf_hdr* bpf_packet;
	
	void reset_read_pos();
	
public:
	BPFEthernetHandler(int eth_idx);
	virtual ~BPFEthernetHandler();

	virtual bool open();
	virtual void close();
	virtual int recv(uint8 *buf, int len);
	virtual int send(const uint8 *buf, int len);
};

#define ETHERNET_HANDLER_CLASSNAME BPFEthernetHandler

#endif // _ETHERNET_MACOSX_H

