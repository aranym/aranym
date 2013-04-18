/*
 * ethernet_macosx.cpp - Mac OS X Ethernet support (via Berkley Packet Filter device)
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


#if defined(OS_darwin)

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/socket.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <CoreFoundation/CoreFoundation.h>

#include "cpu_emulation.h"
#include "main.h"
#include "ethernet_macosx.h"

#define DEBUG 0
#include "debug.h"

#include "fd_trans.h"

#define ETH_HELPER "bpf_helper"

/* BPF filter program:
 -allow all ARP messages
 -allow only IP messages for specific IP address
 */
#define ETHERTYPE_IP	0x800
#define ETHERTYPE_ARP	0x806
#define IP_ADDR_PLH		0xAAAAAAAA

struct bpf_insn ip_filter[] = {
	BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 12),                 //      ld  P[12:2]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETHERTYPE_IP, 0, 5),//      jeq 0x800, IP, NOK
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 26),                 // IP:  ld  P[26:4]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IP_ADDR_PLH, 2, 0), //      jeq IP_ADDR, OK, 0
	BPF_STMT(BPF_LD+BPF_W+BPF_ABS, 30),                 //      ld  P[30:4]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, IP_ADDR_PLH, 0, 1), //      jeq IP_ADDR, OK, NOK
	BPF_STMT(BPF_RET+BPF_K, (u_int)-1),                 // OK:  ret -1
	BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 12),                 // NOK: ld  P[12:2]
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETHERTYPE_ARP, 0, 1),//     jeq 0x806, 0, NOK
	BPF_STMT(BPF_RET+BPF_K, (u_int)-1),                 //      ret -1
	BPF_STMT(BPF_RET+BPF_K, 0),                         // NOK: ret 0
};

struct bpf_program filter = {sizeof(ip_filter)/sizeof(struct bpf_insn), &ip_filter[0]};

// Ethernet frame
struct ethernet_frame
{
	unsigned char dst_addr[ 6 ];
	unsigned char src_addr[ 6 ];
	unsigned char type[2];
	unsigned char payload;
};

// ARP packet structure
struct arp_packet
{
	unsigned char hardware_type[2];
	unsigned char proto_type[2];
	unsigned char hardware_addr_length;
	unsigned char proto_addr_length;
	unsigned char opcode[2];
	unsigned char src_mac[6];
	unsigned char src_ip[4];
	unsigned char dst_mac[6];
	unsigned char dst_ip[4];
};

// IP packet structure
struct ip_packet
{
	unsigned char version_hdr_len;
	unsigned char services;
	unsigned char tot_len[2];
	unsigned char ident[2];
	unsigned char flags_frag_offset[2];
	unsigned char ttl;
	unsigned char proto_type;
	unsigned char hdr_chksum[2];
	unsigned char src_ip[4];
	unsigned char dst_ip[4];
	unsigned char data;
};

void dump_frame(const char *prefix, struct ethernet_frame *frame)
{
	unsigned short int frame_type = (frame->type[0]<<8)|frame->type[1];
	bug("  %s: %02x:%02x:%02x:%02x:%02x:%02x > %02x:%02x:%02x:%02x:%02x:%02x  (Type %04x)", prefix,
		frame->src_addr[0], frame->src_addr[1], frame->src_addr[2], frame->src_addr[3], frame->src_addr[4], frame->src_addr[5],
		frame->dst_addr[0], frame->dst_addr[1], frame->dst_addr[2], frame->dst_addr[3], frame->dst_addr[4], frame->dst_addr[5],
		frame_type);
	if (frame_type == 0x0806) {
		struct arp_packet* arp = (struct arp_packet*) &frame->payload;
		bug("  %s: ARP packet %d.%d.%d.%d > %d.%d.%d.%d  (Type %04x)", prefix,
			arp->src_ip[0], arp->src_ip[1], arp->src_ip[2], arp->src_ip[3],
			arp->dst_ip[0], arp->dst_ip[1], arp->dst_ip[2], arp->dst_ip[3],
			(arp->proto_type[0]<<8)|arp->proto_type[1]);
	}
	else if (frame_type == 0x0800) {
		struct ip_packet* ip = (struct ip_packet*) &frame->payload;
		int tot_len = (ip->tot_len[0]<<8) + ip->tot_len[1];
		bug("  %s: IP packet %d.%d.%d.%d > %d.%d.%d.%d  (Type %04x Total %02x%02x = %d, Payload=%d)", prefix,
			ip->src_ip[0], ip->src_ip[1], ip->src_ip[2], ip->src_ip[3],
			ip->dst_ip[0], ip->dst_ip[1], ip->dst_ip[2], ip->dst_ip[3],
			ip->proto_type, ip->tot_len[0], ip->tot_len[1], tot_len, tot_len - sizeof(ip_packet)+1);
		
		for (unsigned char *ptr=&ip->data; ptr<(unsigned char*)&ip->data+tot_len; ptr++)
			bug(">%*s\n",  tot_len-(sizeof(ip_packet)-1), &ip->data);
	}
}

void dump_bpf_buf(const char *prefix, bpf_hdr* bpf_buf)
{
	if (bpf_buf->bh_datalen > 0) {
		bug("  %s: %d bytes in buf:", prefix, bpf_buf->bh_datalen);
		
		dump_frame(prefix, (struct ethernet_frame*)((char*)bpf_buf + bpf_buf->bh_hdrlen));
	}
}

bool BPFEthernetHandler::open()
{
	// int nonblock = 1;
	char *type = bx_options.ethernet[ethX].type;
	char *dev_name = bx_options.ethernet[ethX].tunnel;
	
	if (strcmp(type, "none") == 0 || strlen(type) == 0)
	{
		return false;
	}
	
	D(bug("BPF(%d): open() type=%s", ethX, type));
	if (strstr(type, "bridge") == NULL )
	{
		panicbug("BPF(%d): unsupported type '%s", ethX, type);
		return false;
	}
	
	debug = (strstr(type, "debug") != NULL);
	D(bug("BPF(%d): debug mode=%d", ethX, debug));
	
	/******************************************************
	 Fork child process to get an open BPF file descriptor
	 ******************************************************/
	char exe_path[2048];
	CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	CFURLGetFileSystemRepresentation(url, true, (UInt8 *)exe_path, sizeof(exe_path));
	CFRelease(url);
	strcat(exe_path, "/Contents/MacOS/"ETH_HELPER);
	D(bug("BPF(%d): starting helper from <%s>", ethX, exe_path));
	
	pid_t pid = fork();
	if (pid < 0)
	{
		panicbug("BPF(%d): ERROR: fork() failed. Ethernet disabled!", ethX);
		return false;
	}
	else if (pid == 0)
	{
		D(bug("BPF(%d): "ETH_HELPER" child running", ethX));
		sleep(2);
		int result = execl(exe_path, exe_path, NULL);
		_exit(result);
	}
	
	D(bug("BPF(%d): waiting for "ETH_HELPER" (PID %d) to send file descriptor", ethX, pid));
	fd = receive_fd();
	if (fd < 0)
	{
		panicbug("BPF(%d): failed receiving file descriptor from "ETH_HELPER".", ethX, dev_name);
		return false;
	}
	
	
	/******************************************************
	 Configure BPF device
	 ******************************************************/
	// associate with specified interface
	struct ifreq ifr;
	safe_strncpy(ifr.ifr_name, dev_name, IFNAMSIZ);
	D(bug("BPF(%d): connecting with device %s", ethX, dev_name));
	if(ioctl(fd, BIOCSETIF, &ifr) > 0)
	{
		panicbug("BPF(%d): Failed associating to %s: %s", ethX, dev_name, strerror(errno));
		::close(fd);
		return false;
	}
	
	// activate immediate mode
	int immediate = 1;
	D(bug("BPF(%d): enabling immediate mode", ethX));
	if(ioctl(fd, BIOCIMMEDIATE, &immediate) == -1)
	{
		panicbug("BPF(%d): Unable to set immediate mode: %s", ethX, strerror(errno));
		::close(fd);
		return false;
	}
	
	// request buffer length
	if(ioctl(fd, BIOCGBLEN, &buf_len) == -1)
	{
		panicbug("BPF(%d): Unable to get buffer length: %s", ethX, strerror(errno));
		::close(fd);
		return false;
	}
	D(bug("BPF(%d): buf_len=%d\n", ethX, buf_len));
	bpf_buf = (struct bpf_hdr*) malloc(buf_len);
	
	// activate promiscious mode
	D(bug("BPF(%d): enabling promiscious mode", ethX));
	if(ioctl(fd, BIOCPROMISC, NULL) == -1)
	{
		panicbug("BPF(%d): Unable to set promiscious mode: %s", ethX, strerror(errno));
		::close(fd);
		return false;
	}
	
	// activate "header complete" mode
	D(bug("BPF(%d): enabling header complete mode", ethX));
	int complete = 1;
	if(ioctl(fd, BIOCGHDRCMPLT, &complete) == -1)
	{
		panicbug("BPF(%d): Unable to set header complete mode: %s", ethX, strerror(errno));
		::close(fd);
		return false;
	}
	
	// disable "see sent" mode
	D(bug("BPF(%d): disabling see sent mode", ethX));
	int seesent = 1;
	if(ioctl(fd, BIOCSSEESENT, &seesent) == -1)
	{
		panicbug("BPF(%d): Unable to disable see sent mode: %s", ethX, strerror(errno));
		::close(fd);
		return false;
	}
	
	if (strstr(type, "nofilter") == NULL )
	{
		// convert and validate specified IP address
		in_addr_t ip_addr = inet_addr(bx_options.ethernet[ethX].ip_atari);
		if (ip_addr == INADDR_NONE) {
			panicbug("BPF(%d): Invalid IP address specified: %s", ethX, bx_options.ethernet[ethX].ip_atari);
			::close(fd);
			return false;
		}
		
		// modify filter program to use specified IP address
		D(bug("BPF(%d): setting filter program for IP address %s", ethX, bx_options.ethernet[ethX].ip_atari));
		
		ip_filter[3].k = ip_addr;
		ip_filter[5].k = ip_addr;
		
		// Enable filter program
		if(ioctl(fd, BIOCSETF, &filter) == -1)
		{
			panicbug("BPF(%d): Unable to load filter program: %s", ethX, strerror(errno));
			::close(fd);
			return false;
		}
		bug("BPF(%d): IP filter program loaded", ethX);
	}
	else {
		bug("BPF(%d): IP filter program skipped", ethX);
	}
	
	return true;
}

bool BPFEthernetHandler::close()
{
	D(bug("BPF(%d): close", ethX));
	
	::close(fd);
	
	free(bpf_buf);
	
	return true;
}

int BPFEthernetHandler::recv(uint8 *buf, int len)
{
	D(bug("BPF(%d): recv(len=%d)", ethX, len));
	// Read a packet from the ethernet device, timeout after 2s
	int res = 0;
	
	// Initialize the file descriptor set.
	fd_set set;
	struct timeval timeout;
	FD_ZERO (&set);
	FD_SET (fd, &set);
	
	// Initialize the timeout data structure.
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	
	// select returns 0 if timeout, 1 if input available, -1 if error.
	if (select(FD_SETSIZE, &set, NULL, NULL, &timeout) == 1) {
		
		res = read(fd, bpf_buf, buf_len);
		if (res > 0) {
			D(bug("BPF(%d): recv %d bytes => %d data bytes", ethX, res, bpf_buf->bh_datalen));
			if (debug) {
				bug("BPF(%d): recv %d bytes => %d data bytes", ethX, res, bpf_buf->bh_datalen);
				dump_bpf_buf("recv", bpf_buf);
			}
			int copy_len = MIN(len,bpf_buf->bh_datalen);
			memcpy(buf, ((char*)bpf_buf + bpf_buf->bh_hdrlen), copy_len);
			if (copy_len != bpf_buf->bh_datalen) {
				bug("BPF(%d): received %d bytes on Host but %d bytes expected by Atari. Packet truncated to %d!", ethX, bpf_buf->bh_datalen, len, copy_len);
			}
			return copy_len;
		}
	}
	return res;
}

int BPFEthernetHandler::send(const uint8 *buf, int len)
{
	D(bug("BPF(%d): send(len=%d)", ethX, len));
	int res = -1;
	if (len > 0)
	{
		if (debug) {
			bug("BPF(%d): send(len=%d)", ethX, len);
			dump_frame("send", (struct ethernet_frame*) buf);
		}
		res = write(fd, buf, len);
		if (res < 0) D(bug("BPF(%d): WARNING: Couldn't transmit packet", ethX));
	}
	return res;
}


#endif /* OS_darwin? */
